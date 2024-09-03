/* Copyright (c) 2022, 2024, Oracle and/or its affiliates.

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
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "sql/binlog/recovery.h"

#include "sql/binlog/decompressing_event_object_istream.h"  // binlog::Decompressing_event_object_istream
#include "sql/psi_memory_key.h"
#include "sql/psi_memory_resource.h"
#include "sql/raii/sentry.h"     // raii::Sentry<>
#include "sql/xa/xid_extract.h"  // xa::XID_extractor

#ifdef WITH_WSREP
#include "sql/wsrep_mysqld.h"  // WSREP_DEBUG
#endif

binlog::Binlog_recovery::Binlog_recovery(Binlog_file_reader &binlog_file_reader)
    : m_reader{binlog_file_reader} {
  this->m_validation_started = true;
}

bool binlog::Binlog_recovery::has_failures() const {
  return this->is_log_malformed() || this->m_no_engine_recovery;
}

bool binlog::Binlog_recovery::is_binlog_malformed() const {
  return this->is_log_malformed();
}

bool binlog::Binlog_recovery::has_engine_recovery_failed() const {
  return this->m_no_engine_recovery;
}

std::string const &binlog::Binlog_recovery::get_failure_message() const {
  return this->m_failure_message;
}

binlog::Binlog_recovery &binlog::Binlog_recovery::recover() {
#ifdef WITH_WSREP
  /*
    If binlog is enabled then SE will persist redo at following stages:
    - during prepare
    - during binlog write
    - during innodb-commit
    3rd stage (fsync) can be skipped as transaction can be recovered
    from binlog.

    During 3rd stage, commit co-ordinates are also recorded in sysheader
    under wsrep placeholder. These co-ordinates are then used to stop
    binlog scan (in addition to get recovery position in case of node-crash).

    Since fsync of 3rd stage is delayed (as transaction can be recovered
    using binlog) it is possible that even though transaction is reported
    as success to end-user said co-ordinates are not persisted to disk.

    On restart, a prepare state transaction could be recovered and committed
    but since wsrep co-ordinates are not persisted a successfully committed
    transaction recovery is skipped causing data inconsistency from end-user
    application perspective.

    This behavior is now changed to let recovery proceed independent of
    wsrep co-ordinates and wsrep co-ordinates are updated to reflect
    the recovery committed transaction.
  */
  if (WSREP_ON) {
    wsrep::gtid gtid = wsrep_get_SE_checkpoint();
    std::ostringstream oss;
    oss << gtid;
    WSREP_INFO("Before binlog recovery (wsrep position: %s)",
               oss.str().c_str());
  }

  /* Pre-notes:
  Logic below will scan for xid events and will add them to the xid vector/set.
  Note: being a set it can hold only unique event.

  Ideally xid should be unique but in case of PXC if there is node-local
  transaction then it would get logged to binlog with MySQL XID and not with
  WSREP XID. MySQL has different logic to generate xid and wsrep has different
  logic. This would end up in a situation where-in 2 different event with be
  logged to binlog with different GTID (if enabled) but same XID.

  MySQL logic below doesn't expect this situation but till MySQL-5.7 use of HASH
  ignored unique insert so duplicate XID were getting inserted.
  Though this also means, if 2 transactions (one with WSREP XID and other with
  MySQL XID) with same XID (but different GTID) are logged and MySQL recover
  both of these transactions as part of recovery, then both of them will
  under-go a commit_by_xid flow ir-respective of which transaction wrote
  XID event to binlog as xid is common for both of them.

  This effectively boils down to:
  - Avoid node local changes in PXC.
  - If there are node-local changes then these changes should not be binlogged.
  - If there is non-replicating-atomic-ddl then it should not be binlogged too.

  There-by ensuring that there is no MySQL XID transactions.

  This also means 2 important things, node local transaction or non-replicating
  ddl transaction will not be replicated even through async replication
  as they are not being binlogged.

  Note: operation to temporary tables (that are session specific) are never
  binlogged */
#endif /* WITH_WSREP */

  process_logs(m_reader);

#ifdef WITH_WSREP
  if (WSREP_ON) {
    wsrep::gtid gtid = wsrep_get_SE_checkpoint();
    std::ostringstream oss;
    oss << gtid;
    WSREP_INFO("After binlog recovery (wsrep position: %s)", oss.str().c_str());
  }
#endif /* WITH_WSREP */

  if (!this->is_log_malformed() && total_ha_2pc > 1) {
    Xa_state_list xa_list{this->m_external_xids};
    this->m_no_engine_recovery = ha_recover(&this->m_internal_xids, &xa_list);
    if (this->m_no_engine_recovery) {
      this->m_failure_message.assign("Recovery failed in storage engines");
    }
  }
  return (*this);
}
