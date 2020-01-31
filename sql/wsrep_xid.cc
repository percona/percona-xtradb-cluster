/* Copyright 2015 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//! @file some utility functions and classes not directly related to replication

#include "wsrep_xid.h"
#include "mysql/plugin.h"  // MYSQL_STORAGE_ENGINE_PLUGIN
#include "sql_class.h"
#include "sql_plugin.h"
#include "wsrep_mysqld.h"  // for logging macros
#include "mysql/components/services/log_builtins.h"
#include "sql/log.h"
#include "service_wsrep.h"

extern bool srv_sys_tablespaces_open;

void wsrep_xid_init(XID *xid, const wsrep::gtid &wsgtid) {
  xid->reset();
  xid->set_format_id(1);
  xid->set_gtrid_length(WSREP_XID_GTRID_LEN);
  xid->set_bqual_length(0);
  char data[XIDDATASIZE];

  memset(data, 0, XIDDATASIZE);
  memcpy(data, WSREP_XID_PREFIX, WSREP_XID_PREFIX_LEN);
  memcpy(data + WSREP_XID_UUID_OFFSET, wsgtid.id().data(), sizeof(wsrep::id));
  int8store(data + WSREP_XID_SEQNO_OFFSET, wsgtid.seqno().get());

  xid->set_data(data, XIDDATASIZE);
}

int wsrep_is_wsrep_xid(const void *xid_ptr) {
  const XID *xid = reinterpret_cast<const XID *>(xid_ptr);
  return (xid->get_format_id() == 1 &&
          xid->get_gtrid_length() == WSREP_XID_GTRID_LEN &&
          xid->get_bqual_length() == 0 &&
          !memcmp(xid->get_data(), WSREP_XID_PREFIX, WSREP_XID_PREFIX_LEN));
}

const unsigned char *wsrep_xid_uuid(const xid_t *xid) {
  DBUG_ASSERT(xid);
  static wsrep::id const undefined;
  if (wsrep_is_wsrep_xid(xid))
    return reinterpret_cast<const unsigned char *>(xid->get_data() +
                                                   WSREP_XID_UUID_OFFSET);
  else
    return static_cast<const unsigned char *>(wsrep::id::undefined().data());
}

const wsrep::id& wsrep_xid_uuid(const XID &xid) {
  DBUG_ASSERT(sizeof(wsrep::id) == sizeof(wsrep_uuid_t));
  return *reinterpret_cast<const wsrep::id *>(wsrep_xid_uuid(&xid));
}

long long wsrep_xid_seqno(const XID* xid) {
  long long ret = wsrep::seqno::undefined().get();
  if (wsrep_is_wsrep_xid(xid)) {
    memcpy(&ret, xid->get_data() + WSREP_XID_SEQNO_OFFSET, sizeof(ret));
  }
  return ret;
}

wsrep::seqno wsrep_xid_seqno(const XID &xid) {
  return wsrep::seqno(wsrep_xid_seqno(&xid));
}

static bool set_SE_checkpoint(THD* , plugin_ref plugin, void *arg) {
  XID *xid = static_cast<XID *>(arg);
  handlerton *hton = plugin_data<handlerton *>(plugin);

  if (hton->db_type == DB_TYPE_INNODB) {
    const unsigned char* uuid = wsrep_xid_uuid(xid);
    char uuid_str[40] = {
        0,
    };
    wsrep_uuid_print((const wsrep_uuid_t *)uuid, uuid_str, sizeof(uuid_str));
    WSREP_DEBUG("Setting WSREPXid (InnoDB): %s:%lld", uuid_str,
                (long long)wsrep_xid_seqno(xid));
    hton->wsrep_set_checkpoint(hton, xid);
  }

  return false;
}

bool wsrep_set_SE_checkpoint(XID &xid) {
  return plugin_foreach(NULL, set_SE_checkpoint, MYSQL_STORAGE_ENGINE_PLUGIN,
                        &xid);
}

bool wsrep_set_SE_checkpoint(const wsrep::gtid& wsgtid) {
  if (!WSREP_ON || wsrep_unireg_abort) return true;

  if (!srv_sys_tablespaces_open) {
    /* If system/default innodb tablespace (ibdata1) is encrypted and
    user starts node with wrong/insufficient configuration then server will
    initiate srv_init_abort sequence shutting down galera.
    On shutdown galera will try to update wsrep co-ordinates to sys_header
    that is located in innodb tablespace (ibdata1) and will fail. */

    WSREP_ERROR("Failed to execute wsrep_set_SE_checkpoint."
                " System tablespace not open");
    return true;
  }

  XID xid;
  wsrep_xid_init(&xid, wsgtid);
  return wsrep_set_SE_checkpoint(xid);
}

static bool get_SE_checkpoint(THD *, plugin_ref plugin, void *arg) {
  XID *xid = reinterpret_cast<XID *>(arg);
  handlerton *hton = plugin_data<handlerton *>(plugin);

  if (hton->db_type == DB_TYPE_INNODB) {
    hton->wsrep_get_checkpoint(hton, xid);
    wsrep_uuid_t uuid;
    memcpy(&uuid, wsrep_xid_uuid(xid), sizeof(uuid));
    char uuid_str[40] = {
        0,
    };
    wsrep_uuid_print(&uuid, uuid_str, sizeof(uuid_str));
    WSREP_DEBUG("Read WSREPXid (InnoDB): %s:%lld", uuid_str,
                (long long)wsrep_xid_seqno(xid));
  }

  return false;
}

bool wsrep_get_SE_checkpoint(XID &xid) {
  return plugin_foreach(NULL, get_SE_checkpoint, MYSQL_STORAGE_ENGINE_PLUGIN,
                        &xid);
}

wsrep::gtid wsrep_get_SE_checkpoint() {
  if (!WSREP_ON || wsrep_unireg_abort) return wsrep::gtid();

  if (!srv_sys_tablespaces_open) {
    /* If system/default innodb tablespace (ibdata1) is encrypted and
    user starts node with wrong/insufficient configuration then server will
    initiate srv_init_abort sequence shutting down galera.
    On shutdown galera will try to update wsrep co-ordinates to sys_header
    that is located in innodb tablespace (ibdata1) and will fail. */

    WSREP_ERROR(
        "Failed to execute wsrep_get_SE_checkpoint."
        " System tablespace not open");
    return wsrep::gtid();
  }

  XID xid;
  memset(static_cast<void*>(&xid), 0, sizeof(xid));
  xid.set_format_id(-1);

  if (wsrep_get_SE_checkpoint(xid)) {
    return wsrep::gtid();
  }

  if (xid.is_null()) {
    return wsrep::gtid();
  }

  if (!wsrep_is_wsrep_xid(&xid)) {
    WSREP_WARN("Read non-wsrep XID from storage engines.");
    return wsrep::gtid();
}

  return wsrep::gtid(wsrep_xid_uuid(xid), wsrep_xid_seqno(xid));
}
