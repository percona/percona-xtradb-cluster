#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef WITH_WSREP

#include "my_md5.h"

extern void wsrep_disable_fips_mode();

namespace md5_unittest {

static constexpr int MD5_HASH_SIZE = 16;

class md5Test : public ::testing::Test {
 protected:
  virtual void SetUp() {
    memset(digest_openssl_, 0xAA, MD5_HASH_SIZE);
    memset(digest_sw_, 0xBB, MD5_HASH_SIZE);
  }
  virtual void TearDown() { wsrep_disable_fips_mode(); }

  void initialize_buffer(char *buf, size_t buf_size, char seed = 0) {
    for (size_t i = 0; i < buf_size; ++i) {
      buf[i] = (seed + i) & 0xFF;
    }
  }

  void check_results() {
    EXPECT_TRUE(0 == memcmp(digest_openssl_, digest_sw_, MD5_HASH_SIZE));
  }

  unsigned char digest_openssl_[MD5_HASH_SIZE];
  unsigned char digest_sw_[MD5_HASH_SIZE];
};

TEST_F(md5Test, Sanity_test) { EXPECT_EQ(1, 1); }

TEST_F(md5Test, Single_mid_size_buffer) {
  constexpr int BUF_SIZE = 10;
  char buf[BUF_SIZE];
  initialize_buffer(buf, BUF_SIZE, 13);

  void *ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_openssl_, ctx);

  wsrep_enable_fips_mode();

  ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_sw_, ctx);

  check_results();
}

TEST_F(md5Test, Single_zero_size_buffer) {
  constexpr int BUF_SIZE = 0;
  char buf[BUF_SIZE] = {};

  void *ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_openssl_, ctx);

  wsrep_enable_fips_mode();

  ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_sw_, ctx);

  check_results();
}

TEST_F(md5Test, Single_short_buffer) {
  constexpr int BUF_SIZE = 1;
  char buf[BUF_SIZE];
  initialize_buffer(buf, BUF_SIZE, 0xE3);

  void *ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_openssl_, ctx);

  wsrep_enable_fips_mode();

  ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_sw_, ctx);

  check_results();
}

TEST_F(md5Test, Single_long_buffer) {
  constexpr int BUF_SIZE = 9817;
  char buf[BUF_SIZE];
  initialize_buffer(buf, BUF_SIZE, 33);

  void *ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_openssl_, ctx);

  wsrep_enable_fips_mode();

  ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf, BUF_SIZE);
  wsrep_compute_md5_hash(digest_sw_, ctx);

  check_results();
}

TEST_F(md5Test, Multipe_buffers) {
  constexpr int BUF1_SIZE = 9817;
  char buf1[BUF1_SIZE];
  initialize_buffer(buf1, BUF1_SIZE, 0);

  constexpr int BUF2_SIZE = 16;
  char buf2[BUF2_SIZE];
  initialize_buffer(buf2, BUF2_SIZE, 1);

  constexpr int BUF3_SIZE = 0;
  char buf3[BUF3_SIZE];

  constexpr int BUF4_SIZE = 3;
  char buf4[BUF4_SIZE];
  initialize_buffer(buf4, BUF4_SIZE, 0xFE);

  void *ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf1, BUF1_SIZE);
  wsrep_md5_update(ctx, buf2, BUF2_SIZE);
  wsrep_md5_update(ctx, buf3, BUF3_SIZE);
  wsrep_md5_update(ctx, buf4, BUF4_SIZE);
  wsrep_compute_md5_hash(digest_openssl_, ctx);

  wsrep_enable_fips_mode();

  ctx = wsrep_md5_init();
  wsrep_md5_update(ctx, buf1, BUF1_SIZE);
  wsrep_md5_update(ctx, buf2, BUF2_SIZE);
  wsrep_md5_update(ctx, buf3, BUF3_SIZE);
  wsrep_md5_update(ctx, buf4, BUF4_SIZE);
  wsrep_compute_md5_hash(digest_sw_, ctx);

  check_results();
}

}  // namespace md5_unittest

#endif /* WITH_WSREP */
