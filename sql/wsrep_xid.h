/* Copyright (C) 2015 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef WSREP_XID_H
#define WSREP_XID_H

#include "handler.h"  // XID typedef
#include "wsrep-lib/include/wsrep/gtid.hpp"
#include "wsrep_api.h"  // wsrep_uuid_t, wsrep_seqno_t

#define WSREP_XID_PREFIX "WSREPXi"
#define WSREP_XID_PREFIX_LEN 7
#define WSREP_XID_VERSION_OFFSET WSREP_XID_PREFIX_LEN
/*
  WSREP XID version history (Codership mysql-wsrep / MariaDB Galera):

  V1 ('d'): original format, seqno in host-native byte order (memcpy).
  V2 ('e'): portable seqno encoding via int8store/sint8korr.
  V3 ('f'): gtrid extended to 64 bytes; extra 32 bytes store a
            wsrep_local_gtid_t (uuid + server_id + seqno) as raw
            memcpy -- used by MariaDB for its native GTID tracking.
  V4 ('g'): same 64-byte gtrid, but the local GTID layout is
            restructured: removes redundant UUID (reuses the Galera
            UUID), stores seqno/server_id with portable encoding,
            and adds the MySQL server_uuid.
  V5 ('h'): adds a 4-byte bqual field containing flags (currently
            WSREP_XID_FLAG_SKIP_LOCAL_GTID) used during crash
            recovery to control local GTID generation.

  PXC writes V2. The extra fields in V3-V5 serve Codership's
  Wsrep_local_gtid_manager and wsrep_sync_server_uuid features,
  which PXC does not implement - PXC uses MySQL's native GTID
  infrastructure directly. The read path accepts all five
  versions for compatibility with XIDs that may be persisted
  from other Galera-family implementations.
*/
#define WSREP_XID_VERSION_1 'd'
#define WSREP_XID_VERSION_2 'e'
#define WSREP_XID_VERSION_3 'f'
#define WSREP_XID_VERSION_4 'g'
#define WSREP_XID_VERSION_5 'h'
#define WSREP_XID_UUID_OFFSET 8
#define WSREP_XID_SEQNO_OFFSET (WSREP_XID_UUID_OFFSET + sizeof(wsrep_uuid_t))
#define WSREP_XID_GTRID_LEN (WSREP_XID_SEQNO_OFFSET + sizeof(wsrep_seqno_t))
#define WSREP_XID_GTRID_LEN_V_1_2 WSREP_XID_GTRID_LEN
#define WSREP_XID_GTRID_LEN_V_3_4_5 64
#define WSREP_XID_BQUAL_LEN_V_5 4

void wsrep_xid_init(xid_t *, const wsrep::gtid &);
int wsrep_is_wsrep_xid(const void *xid);
const wsrep::id &wsrep_xid_uuid(const XID &);
wsrep::seqno wsrep_xid_seqno(const XID &);

wsrep::gtid wsrep_get_SE_checkpoint();
bool wsrep_set_SE_checkpoint(const wsrep::gtid &gtid);

#endif /* WSREP_UTILS_H */
