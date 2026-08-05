/* Copyright (c) 2026 Percona LLC and/or its affiliates.

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

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "my_config.h"

#include <gtest/gtest.h>

#include "my_byteorder.h"
#include "sql/wsrep_xid.h"
#include "sql/xa.h"

namespace wsrep_xid_unittest {

constexpr char kCodershipPrefix[] = "WSREPXi";
constexpr size_t kCodershipPrefixLen = 7;
constexpr size_t kCodershipVersionOffset = kCodershipPrefixLen;
constexpr size_t kUuidOffset = 8;
constexpr size_t kSeqnoOffset = kUuidOffset + sizeof(wsrep_uuid_t);
constexpr long kGtridLenV1V2 = kSeqnoOffset + sizeof(wsrep_seqno_t);
constexpr long kGtridLenV3V4V5 = 64;
constexpr long kBqualLenV5 = 4;

constexpr unsigned char kUuid[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                                     0xcd, 0xef, 0x10, 0x32, 0x54, 0x76,
                                     0x98, 0xba, 0xdc, 0xfe};
constexpr long long kSeqno = 1234567890123LL;

XID make_codership_xid(char version, long gtrid_len, long bqual_len) {
  XID xid;
  xid.reset();
  xid.set_format_id(1);
  xid.set_gtrid_length(gtrid_len);
  xid.set_bqual_length(bqual_len);

  char data[XIDDATASIZE] = {};
  memcpy(data, kCodershipPrefix, kCodershipPrefixLen);
  data[kCodershipVersionOffset] = version;
  memcpy(data + kUuidOffset, kUuid, sizeof(kUuid));
  int8store(data + kSeqnoOffset, kSeqno);
  xid.set_data(data, sizeof(data));
  return xid;
}

void expect_wsrep_gtid(const XID &xid) {
  EXPECT_TRUE(wsrep_is_wsrep_xid(&xid));
  EXPECT_EQ(wsrep::id(kUuid, sizeof(kUuid)), wsrep_xid_uuid(xid));
  EXPECT_EQ(wsrep::seqno(kSeqno), wsrep_xid_seqno(xid));
}

TEST(WsrepXidTest, ReadsPxcXid) {
  wsrep::gtid const gtid(wsrep::id(kUuid, sizeof(kUuid)), wsrep::seqno(kSeqno));
  XID xid;
  wsrep_xid_init(&xid, gtid);

  expect_wsrep_gtid(xid);
  EXPECT_EQ(WSREP_XID_GTRID_LEN, xid.get_gtrid_length());
  EXPECT_EQ(0, xid.get_bqual_length());
}

TEST(WsrepXidTest, ReadsCodershipVersionedXids) {
  expect_wsrep_gtid(make_codership_xid('d', kGtridLenV1V2, 0));
  expect_wsrep_gtid(make_codership_xid('e', kGtridLenV1V2, 0));
  expect_wsrep_gtid(make_codership_xid('f', kGtridLenV3V4V5, 0));
  expect_wsrep_gtid(make_codership_xid('g', kGtridLenV3V4V5, 0));
  expect_wsrep_gtid(make_codership_xid('h', kGtridLenV3V4V5, kBqualLenV5));
}

TEST(WsrepXidTest, RejectsInvalidCodershipVersionedXids) {
  XID invalid_bqual = make_codership_xid('h', kGtridLenV3V4V5, 0);
  EXPECT_FALSE(wsrep_is_wsrep_xid(&invalid_bqual));

  XID invalid_version = make_codership_xid('x', kGtridLenV1V2, 0);
  EXPECT_FALSE(wsrep_is_wsrep_xid(&invalid_version));
}

}  // namespace wsrep_xid_unittest
