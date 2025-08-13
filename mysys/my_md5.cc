/* Copyright (c) 2012, 2025, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is designed to work with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have either included with
   the program or referenced in the documentation.

   Without limiting anything contained in the foregoing, this file,
   which is part of C Driver for MySQL (Connector/C), is also subject to the
   Universal FOSS Exception, version 1.0, a copy of which can be found at
   http://oss.oracle.com/licenses/universal-foss-exception.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file mysys/my_md5.cc
  Wrapper functions for OpenSSL.
*/

#include "my_md5.h"
#include "my_compiler.h"
#include "my_ssl_algo_cache.h"
#include "template_utils.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#ifdef WITH_WSREP
#include "md5.h"
#endif

// returns 1 for success and 0 for failure
[[nodiscard]] int my_md5_hash(unsigned char *digest, unsigned const char *buf,
                              size_t len) {
  // OpenSSL3.x EVP API is 1.5x-4x slower than this (deprecated) API
  // (depending if caching algorithm pointer etc.)
  // Issue reported at: https://github.com/openssl/openssl/issues/25858
  MY_COMPILER_DIAGNOSTIC_PUSH()
  MY_COMPILER_CLANG_DIAGNOSTIC_IGNORE("-Wdeprecated-declarations")
  MY_COMPILER_GCC_DIAGNOSTIC_IGNORE("-Wdeprecated-declarations")
  MY_COMPILER_MSVC_DIAGNOSTIC_IGNORE(4996)

  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, buf, len);
  return MD5_Final(digest, &ctx);

  // restore clang/gcc checks for -Wdeprecated-declarations
  MY_COMPILER_DIAGNOSTIC_POP()
}

/**
    Wrapper function to compute MD5 message digest.

    @param [out] digest Computed MD5 digest
    @param [in] buf     Message to be computed
    @param [in] len     Length of the message
    @return             0 when MD5 hash function called successfully
                        1 when MD5 hash function doesn't called because of fips
   mode (ON/STRICT)
*/
int compute_md5_hash(char *digest, const char *buf, size_t len) {
  /* If fips mode is ON/STRICT restricted method calls will result into abort,
   * skipping call. */
  const bool is_fips = (my_get_fips_mode() != 0);
  const int retval =
      is_fips ||
      (0 == my_md5_hash(pointer_cast<unsigned char *>(digest),
                        pointer_cast<unsigned const char *>(buf), len));
  if (!is_fips && retval) {
    ERR_clear_error();
  }

  return retval;
}

#ifdef WITH_WSREP

/* For certification we need to identify each row uniquely.
Generally this is done using PK but if table is created w/o PK
then a md5-hash (16 bytes) string is generated using the complete record.
Following functions act as helper function in generation of this md5-hash. */

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
static void *wsrep_md5_init_openssl() {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex2(ctx, EVP_md5(), nullptr);
  return (void *)ctx;
}

static void wsrep_md5_update_openssl(void *ctx, char *buf, int len) {
  EVP_DigestUpdate((EVP_MD_CTX *)ctx, buf, len);
}

static void wsrep_compute_md5_hash_openssl(unsigned char *digest, void *ctx) {
  unsigned int md_len;
  EVP_DigestFinal_ex((EVP_MD_CTX *)ctx, digest, &md_len);
  EVP_MD_CTX_free((EVP_MD_CTX *)ctx);
}

#else  /* OPENSSL_VERSION_NUMBER >= 0x30000000L */
static void *wsrep_md5_init_openssl() {
  MD5_CTX *ctx = new MD5_CTX();
  MD5_Init(ctx);
  return (void *)ctx;
}

static void wsrep_md5_update_openssl(void *ctx, char *buf, int len) {
  MD5_Update((MD5_CTX *)(ctx), buf, len);
}

static void wsrep_compute_md5_hash_openssl(unsigned char *digest, void *ctx) {
  MD5_Final(digest, (MD5_CTX *)ctx);
  delete (MD5_CTX *)ctx;
}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L */

/* Non-OpenSSL implementation start */
using namespace websocketpp::md5;

static void *wsrep_md5_init_custom() {
  md5_state_t *ctx = new md5_state_t();
  md5_init(ctx);
  return ctx;
}

static void wsrep_md5_update_custom(void *ctx, char *buf, int len) {
  md5_append((md5_state_t *)ctx, (md5_byte_t *)buf, len);
}

static void wsrep_compute_md5_hash_custom(unsigned char *digest, void *ctx) {
  md5_finish((md5_state_t *)ctx, digest);
  delete (md5_state_t *)ctx;
}
/* Non-OpenSSL implementation end */

static void *(*wsrep_md5_init_fn)() = &wsrep_md5_init_openssl;
static void (*wsrep_md5_update_fn)(void *, char *,
                                   int) = &wsrep_md5_update_openssl;
static void (*wsrep_compute_md5_hash_fn)(unsigned char *, void *) =
    &wsrep_compute_md5_hash_openssl;

void *wsrep_md5_init() { return wsrep_md5_init_fn(); }

void wsrep_md5_update(void *ctx, char *buf, int len) {
  wsrep_md5_update_fn(ctx, buf, len);
}

void wsrep_compute_md5_hash(unsigned char *digest, void *ctx) {
  wsrep_compute_md5_hash_fn(digest, ctx);
}

void wsrep_enable_fips_mode() {
  wsrep_md5_init_fn = &wsrep_md5_init_custom;
  wsrep_md5_update_fn = &wsrep_md5_update_custom;
  wsrep_compute_md5_hash_fn = &wsrep_compute_md5_hash_custom;
}

/* The following function should be used only by unit tests.
   It switches the backend implementation back to OpenSSL */
void wsrep_disable_fips_mode() {
  wsrep_md5_init_fn = &wsrep_md5_init_openssl;
  wsrep_md5_update_fn = &wsrep_md5_update_openssl;
  wsrep_compute_md5_hash_fn = &wsrep_compute_md5_hash_openssl;
}

#endif /* WITH_WSREP */