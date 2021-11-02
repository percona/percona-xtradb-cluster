/* Copyright (c) 2012, 2021, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   Without limiting anything contained in the foregoing, this file,
   which is part of C Driver for MySQL (Connector/C), is also subject to the
   Universal FOSS Exception, version 1.0, a copy of which can be found at
   http://oss.oracle.com/licenses/universal-foss-exception.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */


/**
  @file

  @brief
  Wrapper functions for OpenSSL.
*/

#include <my_global.h>
#include <my_md5.h>

#if defined(HAVE_OPENSSL)
#include <openssl/md5.h>

static void my_md5_hash(unsigned char* digest, unsigned const char *buf, int len)
{
  MD5_CTX ctx;
  MD5_Init (&ctx);
  MD5_Update (&ctx, buf, len);
  MD5_Final (digest, &ctx);
}

#endif /* HAVE_OPENSSL */

/**
    Wrapper function to compute MD5 message digest.

    @param digest [out]  Computed MD5 digest
    @param buf    [in]   Message to be computed
    @param len    [in]   Length of the message

    @return              void
*/
void compute_md5_hash(char *digest, const char *buf, int len)
{
#if defined(HAVE_OPENSSL)
  my_md5_hash((unsigned char*)digest, (unsigned const char*)buf, len);
#endif /* HAVE_OPENSSL */
}

#ifdef WITH_WSREP

/* For certification we need to identify each row uniquely.
Generally this is done using PK but if table is created w/o PK
then a md5-hash (16 bytes) string is generated using the complete record.
Following functions act as helper function in generation of this md5-hash. */

void *wsrep_md5_init()
{
#if defined(HAVE_YASSL)
  TaoCrypt::MD5 *hasher= new TaoCrypt::MD5;
  return (void*)hasher;
#elif defined(HAVE_OPENSSL)
  MD5_CTX *ctx = new MD5_CTX();
  MD5_Init (ctx);
  return (void *)ctx;
#endif /* HAVE_YASSL */
}

void wsrep_md5_update(void *ctx, char* buf, int len)
{
#if defined(HAVE_YASSL)
  ((TaoCrypt::MD5 *)ctx)->Update((TaoCrypt::byte *) buf, len);
#elif defined(HAVE_OPENSSL)
  MD5_Update((MD5_CTX*)(ctx), buf, len);
#endif /* HAVE_YASSL */
}

void wsrep_compute_md5_hash(char *digest, void *ctx)
{
#if defined(HAVE_YASSL)
  ((TaoCrypt::MD5*)ctx)->Final((TaoCrypt::byte *) digest);
  delete (TaoCrypt::MD5*)ctx;
#elif defined(HAVE_OPENSSL)
  MD5_Final ((unsigned char*)digest, (MD5_CTX*)ctx);
  delete (MD5_CTX*)ctx;
#endif /* HAVE_YASSL */
}

#endif /* WITH_WSREP */
