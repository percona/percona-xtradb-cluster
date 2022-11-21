/* Copyright (c) 2013, 2022, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "sql/xa.h"

#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>

#include "m_ctype.h"
#include "m_string.h"
#include "map_helpers.h"
#include "my_dbug.h"
#include "my_loglevel.h"
#include "my_macros.h"
#include "my_psi_config.h"
#include "my_sys.h"
#include "mysql/components/services/bits/mysql_mutex_bits.h"
#include "mysql/components/services/bits/psi_bits.h"
#include "mysql/components/services/bits/psi_mutex_bits.h"
#include "mysql/components/services/log_builtins.h"
#include "mysql/plugin.h"  // MYSQL_XIDDATASIZE
#include "mysql/psi/mysql_mutex.h"
#include "mysql/psi/mysql_transaction.h"
#include "mysql/service_mysql_alloc.h"
#include "mysql_com.h"
#include "mysqld_error.h"
#include "scope_guard.h"  // Scope_guard
#include "sql/auth/sql_security_ctx.h"
#include "sql/binlog.h"  // is_transaction_empty
#include "sql/clone_handler.h"
#include "sql/debug_sync.h"  // DEBUG_SYNC
#include "sql/handler.h"     // handlerton
#include "sql/item.h"
#include "sql/log.h"
#include "sql/mdl.h"
#include "sql/mdl_context_backup.h"  // MDL_context_backup_manager
#include "sql/mysqld.h"              // server_id
#include "sql/protocol.h"
#include "sql/psi_memory_key.h"  // key_memory_xa_transaction_contexts
#include "sql/query_options.h"
#include "sql/rpl_context.h"
#include "sql/rpl_gtid.h"
#include "sql/rpl_handler.h"                       // RUN_HOOK
#include "sql/rpl_replica_commit_order_manager.h"  // Commit_order_manager
#include "sql/sql_class.h"                         // THD
#include "sql/sql_const.h"
#include "sql/sql_error.h"
#include "sql/sql_lex.h"  // struct LEX
#include "sql/sql_list.h"
#include "sql/sql_plugin.h"  // plugin_foreach
#include "sql/sql_table.h"   // filename_to_tablename
#include "sql/system_variables.h"
#include "sql/tc_log.h"       // tc_log
#include "sql/transaction.h"  // trans_begin, trans_rollback
#include "sql/transaction_info.h"
#include "sql/xa/recovery.h"           // xa::recovery
#include "sql/xa/sql_xa_commit.h"      // Sql_cmd_xa_commit
#include "sql/xa/transaction_cache.h"  // xa::Transaction_cache
#include "sql_string.h"
#include "template_utils.h"
#include "thr_mutex.h"

const char *XID_STATE::xa_state_names[] = {"NON-EXISTING", "ACTIVE", "IDLE",
                                           "PREPARED", "ROLLBACK ONLY"};

#ifdef WITH_WSREP
#include "wsrep_xid.h"
#endif /* WITH_WSREP */

/* for recover() handlerton call */
static const int MIN_XID_LIST_SIZE = 128;
static const int MAX_XID_LIST_SIZE = 1024 * 128;

static const uint MYSQL_XID_PREFIX_LEN = 8;  // must be a multiple of 8
static const uint MYSQL_XID_OFFSET = MYSQL_XID_PREFIX_LEN + sizeof(server_id);
static const uint MYSQL_XID_GTRID_LEN = MYSQL_XID_OFFSET + sizeof(my_xid);

static void attach_native_trx(THD *thd);

my_xid xid_t::get_my_xid() const {
  static_assert(XIDDATASIZE == MYSQL_XIDDATASIZE,
                "Our #define needs to match the one in plugin.h.");

  if (gtrid_length == static_cast<long>(MYSQL_XID_GTRID_LEN) &&
      bqual_length == 0 &&
      !memcmp(data, MYSQL_XID_PREFIX, MYSQL_XID_PREFIX_LEN)) {
    my_xid tmp;
    memcpy(&tmp, data + MYSQL_XID_OFFSET, sizeof(tmp));
    return tmp;
  }
  return 0;
}

void xid_t::set(my_xid xid) {
  formatID = 1;
  memcpy(data, MYSQL_XID_PREFIX, MYSQL_XID_PREFIX_LEN);
  memcpy(data + MYSQL_XID_PREFIX_LEN, &server_id, sizeof(server_id));
  memcpy(data + MYSQL_XID_OFFSET, &xid, sizeof(xid));
  gtrid_length = MYSQL_XID_GTRID_LEN;
  bqual_length = 0;
}

xid_t &xid_t::operator=(binary_log::XA_prepare_event::MY_XID const &rhs) {
  this->set_format_id(rhs.formatID);
  this->set_gtrid_length(rhs.gtrid_length);
  this->set_bqual_length(rhs.bqual_length);
  this->set_data(rhs.data, rhs.gtrid_length + rhs.bqual_length);
  return (*this);
}

bool xid_t::operator==(struct xid_t const &rhs) const { return this->eq(&rhs); }

bool xid_t::operator!=(struct xid_t const &rhs) const {
  return !((*this) == rhs);
}

bool xid_t::operator<(const xid_t &rhs) const {
  if (this->get_format_id() < rhs.get_format_id()) return true;
  if (this->get_gtrid_length() < rhs.get_gtrid_length()) {
    return true;
  }
  if (this->get_gtrid_length() > rhs.get_gtrid_length()) {
    return false;
  }
  if (this->get_bqual_length() < rhs.get_bqual_length()) {
    return true;
  }
  if (this->get_bqual_length() > rhs.get_bqual_length()) {
    return false;
  }
  if (std::strncmp(this->get_data(), rhs.get_data(),
                   this->get_gtrid_length() + this->get_bqual_length()) < 0)
    return true;
  return false;
}

std::ostream &operator<<(std::ostream &out, struct xid_t const &in) {
  char buf[XID::ser_buf_size] = {0};
  out << const_cast<const char *>(in.serialize(buf)) << std::flush;
  return out;
}

Recovered_xa_transactions *Recovered_xa_transactions::m_instance = nullptr;

Recovered_xa_transactions::Recovered_xa_transactions()
    : m_prepared_xa_trans(Malloc_allocator<XA_recover_txn *>(
          key_memory_xa_recovered_transactions)),
      m_mem_root_inited(false) {}

Recovered_xa_transactions &Recovered_xa_transactions::instance() {
  return *m_instance;
}

bool Recovered_xa_transactions::init() {
  m_instance = new (std::nothrow) Recovered_xa_transactions();
  return m_instance == nullptr;
}

void Recovered_xa_transactions::destroy() {
  delete m_instance;
  m_instance = nullptr;
}

bool Recovered_xa_transactions::add_prepared_xa_transaction(
    XA_recover_txn const *prepared_xa_trn_arg) {
  XA_recover_txn *prepared_xa_trn = new (&m_mem_root) XA_recover_txn();

  if (prepared_xa_trn == nullptr) {
    LogErr(ERROR_LEVEL, ER_SERVER_OUTOFMEMORY,
           static_cast<int>(sizeof(XA_recover_txn)));
    return true;
  }

  prepared_xa_trn->id = prepared_xa_trn_arg->id;
  prepared_xa_trn->mod_tables = prepared_xa_trn_arg->mod_tables;

  m_prepared_xa_trans.push_back(prepared_xa_trn);

  return false;
}

MEM_ROOT *Recovered_xa_transactions::get_allocated_memroot() {
  if (!m_mem_root_inited) {
    init_sql_alloc(key_memory_xa_transaction_contexts, &m_mem_root,
                   TABLE_ALLOC_BLOCK_SIZE);
    m_mem_root_inited = true;
  }
  return &m_mem_root;
}

static bool xarecover_create_mdl_backup(XA_recover_txn &txn,
                                        MEM_ROOT *mem_root) {
  MDL_request_list mdl_requests;
  List_iterator<st_handler_tablename> table_list_it(*txn.mod_tables);
  st_handler_tablename *tbl_name;

  while ((tbl_name = table_list_it++)) {
    MDL_request *table_mdl_request = new (mem_root) MDL_request;
    if (table_mdl_request == nullptr) {
      /* Out of memory: Abort() */
      return true;
    }

    char db_buff[NAME_CHAR_LEN * FILENAME_CHARSET_MBMAXLEN + 1];
    int len = filename_to_tablename(tbl_name->db, db_buff, sizeof(db_buff));
    db_buff[len] = '\0';

    char name_buff[NAME_CHAR_LEN * FILENAME_CHARSET_MBMAXLEN + 1];
    len = filename_to_tablename(tbl_name->tablename, name_buff,
                                sizeof(name_buff));
    name_buff[len] = '\0';

    /*
      We do not have information about the actual lock taken
      during the transaction. Hence we are going with a strong
      lock to be safe.
    */
    MDL_REQUEST_INIT(table_mdl_request, MDL_key::TABLE, db_buff, name_buff,
                     MDL_SHARED_WRITE, MDL_TRANSACTION);
    mdl_requests.push_front(table_mdl_request);
  }

  return MDL_context_backup_manager::instance().create_backup(
      &mdl_requests, txn.id.key(), txn.id.key_length());
}

bool Recovered_xa_transactions::recover_prepared_xa_transactions() {
  bool ret = false;

  if (m_mem_root_inited) {
    while (!m_prepared_xa_trans.empty()) {
      auto prepared_xa_trn = m_prepared_xa_trans.front();
      xa::Transaction_cache::insert(&prepared_xa_trn->id);

      if (xarecover_create_mdl_backup(*prepared_xa_trn, &m_mem_root)) {
        ret = true;
        break;
      }

      m_prepared_xa_trans.pop_front();
    }
    m_mem_root.Clear();
    m_mem_root_inited = false;
  }

  return ret;
}

<<<<<<< HEAD
static bool xarecover_handlerton(THD *, plugin_ref plugin, void *arg) {
  handlerton *hton = plugin_data<handlerton *>(plugin);
  xarecover_st *info = (struct xarecover_st *)arg;
  int got;

  if (hton->state == SHOW_OPTION_YES && hton->recover) {
    while (
        (got = hton->recover(
             hton, info->list, info->len,
             Recovered_xa_transactions::instance().get_allocated_memroot())) >
        0) {
      LogErr(INFORMATION_LEVEL, ER_XA_RECOVER_FOUND_TRX_IN_SE, got,
             ha_resolve_storage_engine_name(hton));
      for (int i = 0; i < got; i++) {
#ifdef WITH_WSREP
        my_xid x = (wsrep_is_wsrep_xid(&info->list[i].id)
                        ? wsrep_xid_seqno(info->list[i].id).get()
                        : info->list[i].id.get_my_xid());
#else
        my_xid x = info->list[i].id.get_my_xid();
#endif /* WITH_WSREP */

        if (!x)  // not "mine" - that is generated by external TM
        {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_IGNORING_XID, xid->xid_to_str(buf));
#endif

          if (Recovered_xa_transactions::instance().add_prepared_xa_transaction(
                  &info->list[i])) {
            return true;
          }
          info->found_foreign_xids++;
          continue;
        }
        if (info->dry_run) {
          info->found_my_xids++;
          continue;
        }
        // recovery mode
        if (info->commit_list
                ? info->commit_list->count(x) != 0
                : tc_heuristic_recover == TC_HEURISTIC_RECOVER_COMMIT) {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_COMMITTING_XID, xid->xid_to_str(buf));
#endif
          hton->commit_by_xid(hton, &info->list[i].id);
        } else {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_ROLLING_BACK_XID,
                 xid->xid_to_str(buf));
#endif
          hton->rollback_by_xid(hton, &info->list[i].id);
        }
      }
      if (got < info->len) break;
    }
  }
  return false;
}

int ha_recover(const mem_root_unordered_set<my_xid> *commit_list) {
||||||| merged common ancestors
static bool xarecover_handlerton(THD *, plugin_ref plugin, void *arg) {
  handlerton *hton = plugin_data<handlerton *>(plugin);
  xarecover_st *info = (struct xarecover_st *)arg;
  int got;

  if (hton->state == SHOW_OPTION_YES && hton->recover) {
    while (
        (got = hton->recover(
             hton, info->list, info->len,
             Recovered_xa_transactions::instance().get_allocated_memroot())) >
        0) {
      LogErr(INFORMATION_LEVEL, ER_XA_RECOVER_FOUND_TRX_IN_SE, got,
             ha_resolve_storage_engine_name(hton));
      for (int i = 0; i < got; i++) {
        my_xid x = info->list[i].id.get_my_xid();
        if (!x)  // not "mine" - that is generated by external TM
        {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_IGNORING_XID, xid->xid_to_str(buf));
#endif

          if (Recovered_xa_transactions::instance().add_prepared_xa_transaction(
                  &info->list[i])) {
            return true;
          }
          info->found_foreign_xids++;
          continue;
        }
        if (info->dry_run) {
          info->found_my_xids++;
          continue;
        }
        // recovery mode
        if (info->commit_list
                ? info->commit_list->count(x) != 0
                : tc_heuristic_recover == TC_HEURISTIC_RECOVER_COMMIT) {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_COMMITTING_XID, xid->xid_to_str(buf));
#endif
          hton->commit_by_xid(hton, &info->list[i].id);
        } else {
#ifndef NDEBUG
          char buf[XIDDATASIZE * 4 + 6];  // see xid_to_str
          XID *xid = &info->list[i].id;
          LogErr(INFORMATION_LEVEL, ER_XA_ROLLING_BACK_XID,
                 xid->xid_to_str(buf));
#endif
          hton->rollback_by_xid(hton, &info->list[i].id);
        }
      }
      if (got < info->len) break;
    }
  }
  return false;
}

int ha_recover(const mem_root_unordered_set<my_xid> *commit_list) {
=======
int ha_recover(Xid_commit_list *commit_list, Xa_state_list *xa_list) {
>>>>>>> tag/Percona-Server-8.0.30-22
  xarecover_st info;
  DBUG_TRACE;
  info.found_foreign_xids = info.found_my_xids = 0;
  info.commit_list = commit_list;
  info.dry_run = (info.commit_list == nullptr &&
                  tc_heuristic_recover == TC_HEURISTIC_NOT_USED);
  info.list = nullptr;

  std::unique_ptr<MEM_ROOT> mem_root{nullptr};
  std::unique_ptr<Xa_state_list::allocator> map_alloc{nullptr};
  std::unique_ptr<Xa_state_list::list> xid_map{nullptr};
  std::unique_ptr<Xa_state_list> external_xids{nullptr};
  if (xa_list == nullptr) {
    std::tie(mem_root, map_alloc, xid_map, external_xids) =
        Xa_state_list::new_instance();
    xa_list = external_xids.get();
  }
  info.xa_list = xa_list;

  /* commit_list and tc_heuristic_recover cannot be set both */
  assert(info.commit_list == nullptr ||
         tc_heuristic_recover == TC_HEURISTIC_NOT_USED);
  /* if either is set, total_ha_2pc must be set too */
  assert(info.dry_run || total_ha_2pc > (ulong)opt_bin_log);

  if (total_ha_2pc <= (ulong)opt_bin_log) return 0;

  if (info.commit_list) LogErr(SYSTEM_LEVEL, ER_XA_STARTING_RECOVERY);

#ifdef WITH_WSREP
  // WSREP is a built-in plugin and is always counted (even if not ON)
  // So add an extra 1 to account for wsrep (the other 1 is for innodb)
  if (total_ha_2pc > (ulong)opt_bin_log + 1 + 1 /* WSREP */) {
#else
  if (total_ha_2pc > (ulong)opt_bin_log + 1) {
#endif /* WITH_WSREP */
    if (tc_heuristic_recover == TC_HEURISTIC_RECOVER_ROLLBACK) {
      LogErr(ERROR_LEVEL, ER_XA_NO_MULTI_2PC_HEURISTIC_RECOVER);
      return 1;
    }
  } else {
    /*
      If there is only one 2pc capable storage engine it is always safe
      to rollback. This setting will be ignored if we are in automatic
      recovery mode.
    */
    tc_heuristic_recover = TC_HEURISTIC_RECOVER_ROLLBACK;  // forcing ROLLBACK
    info.dry_run = false;
  }

  for (info.len = MAX_XID_LIST_SIZE;
       info.list == nullptr && info.len > MIN_XID_LIST_SIZE; info.len /= 2) {
    info.list = new (std::nothrow) XA_recover_txn[info.len];
  }
  if (!info.list) {
    LogErr(ERROR_LEVEL, ER_SERVER_OUTOFMEMORY,
           static_cast<int>(info.len * sizeof(XID)));
    return 1;
  }
  auto clean_up_guard = create_scope_guard([&] { delete[] info.list; });

  if (plugin_foreach(nullptr, xa::recovery::recover_prepared_in_tc_one_ht,
                     MYSQL_STORAGE_ENGINE_PLUGIN, &info)) {
    return 1;
  }
  if (plugin_foreach(nullptr, xa::recovery::recover_one_ht,
                     MYSQL_STORAGE_ENGINE_PLUGIN, &info)) {
    return 1;
  }

  if (info.found_foreign_xids)
    LogErr(WARNING_LEVEL, ER_XA_RECOVER_FOUND_XA_TRX, info.found_foreign_xids);
  if (info.dry_run && info.found_my_xids) {
    LogErr(ERROR_LEVEL, ER_XA_RECOVER_EXPLANATION, info.found_my_xids,
           opt_tc_log_file);
    return 1;
  }
  if (info.commit_list) LogErr(SYSTEM_LEVEL, ER_XA_RECOVERY_DONE);
  return 0;
}

bool xa_trans_force_rollback(THD *thd) {
  /*
    We must reset rm_error before calling ha_rollback(),
    so thd->transaction.xid structure gets reset
    by ha_rollback()/THD::transaction::cleanup().
  */
  thd->get_transaction()->xid_state()->reset_error();
  if (ha_rollback_trans(thd, true)) {
    my_error(ER_XAER_RMERR, MYF(0));
    return true;
  }
  return false;
}

void cleanup_trans_state(THD *thd) {
  thd->variables.option_bits &= ~OPTION_BEGIN;
  thd->server_status &=
      ~(SERVER_STATUS_IN_TRANS | SERVER_STATUS_IN_TRANS_READONLY);
  thd->get_transaction()->reset_unsafe_rollback_flags(Transaction_ctx::SESSION);
  DBUG_PRINT("info", ("clearing SERVER_STATUS_IN_TRANS"));
  xa::Transaction_cache::remove(thd->get_transaction());
}

std::shared_ptr<Transaction_ctx> find_trn_for_recover_and_check_its_state(
    THD *thd, xid_t *xid_for_trn_in_recover, XID_STATE *xid_state) {
  if (!xid_state->has_state(XID_STATE::XA_NOTR) ||
      thd->in_active_multi_stmt_transaction()) {
    DBUG_PRINT("xa", ("Failed to look up tran because it is NOT in XA_NOTR"));
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return nullptr;
  }

  auto foundit = xa::Transaction_cache::find(
      xid_for_trn_in_recover,
      [&](std::shared_ptr<Transaction_ctx> const &item) -> bool {
        DEBUG_SYNC(thd, "before_accessing_xid_state");
        return item->xid_state()
            ->is_detached();  // Safe from race condition with `~THD()` after
                              // we verify that no THD owns the transaction
                              // context (is_detached() == true).
      });

  if (foundit == nullptr) {
    my_error(ER_XAER_NOTA, MYF(0));
    return nullptr;
  }

  return foundit;
}

bool acquire_mandatory_metadata_locks(THD *thd, xid_t *detached_xid) {
  /*
    Acquire metadata lock which will ensure that XA ROLLBACK is blocked
    by active FLUSH TABLES WITH READ LOCK (and vice versa ROLLBACK in
    progress blocks FTWRL). This is to avoid binlog and redo entries
    while a backup is in progress.
  */
  MDL_request mdl_request;
  MDL_REQUEST_INIT(&mdl_request, MDL_key::COMMIT, "", "",
                   MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);
  if (thd->mdl_context.acquire_lock(&mdl_request,
                                    thd->variables.lock_wait_timeout)) {
    return true;
  }

  /*
    Like in the commit case a failure to store gtid is regarded
    as the resource manager issue.
  */

  if (MDL_context_backup_manager::instance().restore_backup(
<<<<<<< HEAD
          &thd->mdl_context, external_xid->key(), external_xid->key_length())) {
    return true;
  }

  return false;
}

/**
  Handle the statement XA COMMIT for the case when xid corresponds to
  an external XA transaction, that it a transaction generated outside
  current connection context. When xa_detach_on_prepare is ON (default),
  this applies to all prepared XA transactions. That is, an XA
  transaction pepared earlier on this connection, on a another active
  connection, on a terminated connection, or on a connction of an
  earlier server instance.

  @param thd           Thread context
  @param external_xid  XID value specified by XA COMMIT that corresponds to
                       a XA transaction generated outside current connection
                       context.
  @param thd_xid_state State of XA transaction corresponding to the current
                       connection that expected to have the value
                       XID_STATE::XA_NOTR. Non-const because it is (ab)used
                       to pass binlogged flag down to xa_binlog_commit().

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_commit::process_external_xa_commit(THD *thd,
                                                   xid_t *external_xid,
                                                   XID_STATE *thd_xid_state) {
  std::shared_ptr<Transaction_ctx> transaction =
      find_trn_for_recover_and_check_its_state(thd, external_xid,
                                               thd_xid_state);

  if (!transaction) return true;

  XID_STATE *xs = transaction->xid_state();

  assert(xs->get_xid()->eq(external_xid));

  /*
    Resumed transaction XA-commit.
    The case deals with the "external" XA-commit by either a slave applier
    or a different than XA-prepared transaction session.
  */
  bool res = xs->xa_trans_rolled_back();

  /*
    Metadata locks taken during XA COMMIT should be released when
    there is error in commit order execution, so we take a savepoint
    and rollback to it in case of error. The error during commit order
    execution can be temporary like commit order deadlock (check header
    comment for check_and_report_deadlock()) and can be recovered after
    retrying unlike other commit errors. And to do so we need to restore
    status of metadata locks i.e. rollback to savepoint, before the retry
    attempt to ensure order for applier threads.
  */
  MDL_savepoint mdl_savepoint = thd->mdl_context.mdl_savepoint();

  DEBUG_SYNC(thd, "external_xa_commit_before_acquire_xa_lock");
  /*
    Acquire XID_STATE::m_xa_lock to prevent concurrent running of two
    XA COMMIT/XA ROLLBACK statements. Without acquiring this lock an attempt
    to run two XA COMMIT/XA ROLLBACK statement for the same xid value may lead
    to writing two events for the same xid into the binlog (e.g. twice
    XA COMMIT event, that is an event for XA COMMIT some_xid_value
    followed by an another event XA COMMIT with the same xid value).
    As a consequences, presence of two XA COMMIT/XA ROLLACK statements for
    the same xid value in binlog would break replication.
  */
  std::lock_guard<std::mutex> lk(xs->get_xa_lock());
  /*
    Double check that the XA transaction still does exist since the transaction
    could be removed from the cache by another XA COMMIT/XA ROLLBACK statement
    being executed concurrently from parallel session with the same xid value.
  */
  if (!find_trn_for_recover_and_check_its_state(thd, external_xid,
                                                thd_xid_state))
    return true;

  if (acquire_mandatory_metadata_locks(thd, external_xid)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    my_error(ER_XA_RETRY, MYF(0));
    return true;
  }

  DEBUG_SYNC(thd, "external_xa_commit_after_acquire_commit_lock");

  /* Do not execute gtid wrapper whenever 'res' is true (rm error) */
  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  if (gtid_error) my_error(ER_XA_RBROLLBACK, MYF(0));
  res = res || gtid_error;

  /*
    xs' is_binlogged() is passed through xid_state's member to low-level
    logging routines for deciding how to log.  The same applies to
    Rollback case.
    Note that thd_xid_state is not at all related to the XA transaction
    we are committing externally. We just use it to pass the binlogged status
    down to do_binlog_xa_commit_rollback() which uses current_thd to retrieve
    the thd_xid_state and read the binlogged flag.
  */
  assert(thd_xid_state->is_binlogged() == false);
  if (xs->is_binlogged())
    thd_xid_state->set_binlogged();
  else
    thd_xid_state->unset_binlogged();

  /*
    Ensure externalization order for applier threads.

    Note: the calls to Commit_order_manager::wait/wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  if (Commit_order_manager::wait(thd)) {
    /*
      In case of error, wait_and_finish() checks if transaction can be
      retried and if it can be retried then only it remove itself from
      waiting (Commit Order Queue) and allow next applier thread to be
      ordered.
    */
    Commit_order_manager::wait_and_finish(thd, true);

    need_clear_owned_gtid = true;
    gtid_error = true;
    gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
    thd->mdl_context.rollback_to_savepoint(mdl_savepoint);
    thd_xid_state->unset_binlogged();
    return true;
  }

  res = ha_commit_or_rollback_by_xid(external_xid, !res) || res;

  /*
    After ensuring externalization order for applier thread, remove it
    from waiting (Commit Order Queue) and allow next applier thread to
    be ordered.

    Note: the calls to Commit_order_manager::wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  Commit_order_manager::wait_and_finish(thd, res);

  // Restoring the binlogged status of the thd_xid_state after borrowing it
  // to pass the binlogged flag to binlog_xa_commit().
  thd_xid_state->unset_binlogged();

  MDL_context_backup_manager::instance().delete_backup(
      external_xid->key(), external_xid->key_length());

  transaction_cache_delete(transaction.get());
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  return res;
}

/**
  Handle the statement XA COMMIT for the case when xid corresponds to
  an internal XA transaction, that is a transaction generated by
  current session context.

  @param thd           Thread context
  @param xid_state     State of XA transaction corresponding to the current
                       session.

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_commit::process_internal_xa_commit(THD *thd,
                                                   XID_STATE *xid_state) {
  DBUG_TRACE;
  bool res = false;
  bool gtid_error = false, need_clear_owned_gtid = false;

  if (xid_state->xa_trans_rolled_back()) {
    xa_trans_force_rollback(thd);
    res = thd->is_error();
  } else if (xid_state->has_state(XID_STATE::XA_IDLE) &&
             m_xa_opt == XA_ONE_PHASE) {
    int r = ha_commit_trans(thd, true);
    if ((res = r)) my_error(r == 1 ? ER_XA_RBROLLBACK : ER_XAER_RMERR, MYF(0));
  } else if (xid_state->has_state(XID_STATE::XA_PREPARED) &&
             m_xa_opt == XA_NONE) {
    MDL_request mdl_request;

    /*
      Acquire metadata lock which will ensure that COMMIT is blocked
      by active FLUSH TABLES WITH READ LOCK (and vice versa COMMIT in
      progress blocks FTWRL).

      We allow FLUSHer to COMMIT; we assume FLUSHer knows what it does.
    */
    MDL_REQUEST_INIT(&mdl_request, MDL_key::COMMIT, "", "",
                     MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);
    if (thd->mdl_context.acquire_lock(&mdl_request,
                                      thd->variables.lock_wait_timeout)) {
      /*
        We can't rollback an XA transaction on lock failure due to
        Innodb redo log and bin log update are involved in rollback.
        Return error to user for a retry.
      */
      my_error(ER_XA_RETRY, MYF(0));
      return true;
    }

    std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
    if (gtid_error) {
      res = true;
      /*
        Failure to store gtid is regarded as a unilateral one of the
        resource manager therefore the transaction is to be rolled back.
        The specified error is the same as @c xa_trans_force_rollback.
        The prepared XA will be rolled back along and so will do Gtid state,
        see ha_rollback_trans().

        Todo/fixme: fix binlogging, "XA rollback" event could be missed out.
        Todo/fixme: as to XAER_RMERR, should not it be XA_RBROLLBACK?
                    Rationale: there's no consistency concern after rollback,
                    unlike what XAER_RMERR suggests.
      */
      ha_rollback_trans(thd, true);
      my_error(ER_XAER_RMERR, MYF(0));
    } else {
      DBUG_EXECUTE_IF("simulate_crash_on_commit_xa_trx", DBUG_SUICIDE(););
      DEBUG_SYNC(thd, "trans_xa_commit_after_acquire_commit_lock");

      if (tc_log)
        res = tc_log->commit(thd, /* all */ true);
      else
        res = ha_commit_low(thd, /* all */ true);

      DBUG_EXECUTE_IF("simulate_xa_commit_log_failure", { res = true; });

      if (res)
        my_error(ER_XAER_RMERR, MYF(0));  // todo/fixme: consider to rollback it
#ifdef HAVE_PSI_TRANSACTION_INTERFACE
      else {
        /*
          Since we don't call ha_commit_trans() for prepared transactions,
          we need to explicitly mark the transaction as committed.
        */
        MYSQL_COMMIT_TRANSACTION(thd->m_transaction_psi);
      }

      thd->m_transaction_psi = nullptr;
#endif
    }
  } else {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  cleanup_trans_state(thd);

  xid_state->set_state(XID_STATE::XA_NOTR);
  xid_state->unset_binlogged();
  trans_track_end_trx(thd);
  /* The transaction should be marked as complete in P_S. */
  assert(thd->m_transaction_psi == nullptr || res);
  return res;
}

bool Sql_cmd_xa_commit::execute(THD *thd) {
  bool st = trans_xa_commit(thd);

  if (!st) {
    thd->mdl_context.release_transactional_locks();
#ifdef WITH_WSREP
    WSREP_DEBUG("XA commit failed, MDL released: %u", thd->thread_id());
#endif /* WITH_WSREP */
    /*
        We've just done a commit, reset transaction
        isolation level and access mode to the session default.
    */
    trans_reset_one_shot_chistics(thd);

    my_ok(thd);
  }
  return st;
}

/**
  Roll back and terminate a XA transaction.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_rollback::trans_xa_rollback(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  bool res = true;

  /* Inform clone handler of XA operation. */
  Clone_handler::XA_Operation xa_guard(thd);
  if (!xid_state->has_same_xid(m_xid)) {
    res = process_external_xa_rollback(thd, m_xid, xid_state);
  } else {
    res = process_internal_xa_rollback(thd, xid_state);
  }
  return (res);
}

/**
  Handle the statement XA ROLLBACK for the case when xid corresponds to
  an external XA transaction, that it a transaction generated outside
  current session context.

  @param thd           Thread context
  @param external_xid  XID value specified by XA ROLLBACK that corresponds to
                       a XA transaction generated outside current session
                       context. In fact, it means that XA ROLLBACK is run
                       against a XA transaction recovered after server restart.
  @param xid_state     State of XA transaction corresponding to the current
                       session that expected to have the value
                       XID_STATE::XA_NOTR

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_rollback::process_external_xa_rollback(THD *thd,
                                                       xid_t *external_xid,
                                                       XID_STATE *xid_state) {
  DBUG_TRACE;

  std::shared_ptr<Transaction_ctx> transaction =
      find_trn_for_recover_and_check_its_state(thd, external_xid, xid_state);

  if (!transaction) return true;

  XID_STATE *xs = transaction->xid_state();

  assert(xs->get_xid()->eq(external_xid));

  /*
    Metadata locks taken during XA ROLLBACK should be released when
    there is error in commit order execution, so we take a savepoint
    and rollback to it in case of error.
  */
  MDL_savepoint mdl_savepoint = thd->mdl_context.mdl_savepoint();

  /*
    Acquire XID_STATE::m_xa_lock to prevent concurrent running of two
    XA COMMIT/XA ROLLBACK statements. Without acquiring this lock an attempt
    to run two XA COMMIT/XA ROLLBACK statement for the same xid value may lead
    to writing two events for the same xid into the binlog (e.g. twice
    XA ROLLBACK event, that is an event for XA ROLLBACK some_xid_value
    followed by an another event XA ROLLBACK with the same xid value).
    As a consequences, presence of two XA COMMIT/XA ROLLACK statements for
    the same xid value in binlog would break replication.
  */
  std::lock_guard<std::mutex> lk(xs->get_xa_lock());
  /*
    Double check that the XA transaction still does exist since the transaction
    could be removed from the cache by another XA COMMIT/XA ROLLBACK statement
    being executed concurrently from parallel session with the same xid value.
  */
  if (!find_trn_for_recover_and_check_its_state(thd, external_xid, xid_state))
    return true;

  if (acquire_mandatory_metadata_locks(thd, external_xid)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    DBUG_PRINT("xa", ("Failed to acquire MDLs for XA ROLLBACK on"));
    my_error(ER_XAER_RMERR, MYF(0));
    return true;
  }

  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  if (gtid_error) my_error(ER_XA_RBROLLBACK, MYF(0));
  bool res = xs->xa_trans_rolled_back();

  if (xs->is_binlogged())
    xid_state->set_binlogged();
  else
    xid_state->unset_binlogged();

  /*
    Ensure externalization order for applier threads.

    Note: the calls to Commit_order_manager::wait/wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  if (Commit_order_manager::wait(thd)) {
    /*
      In case of error, wait_and_finish() checks if transaction can be
      retried and if it can be retried then only it remove itself from
      waiting (Commit Order Queue) and allow next applier thread to be
      ordered.
    */
    Commit_order_manager::wait_and_finish(thd, true);

    need_clear_owned_gtid = true;
    gtid_error = true;
    gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
    thd->mdl_context.rollback_to_savepoint(mdl_savepoint);
    return true;
  }

  res = ha_commit_or_rollback_by_xid(external_xid, false) || res;

  /*
    After ensuring externalization order for applier thread, remove it
    from waiting (Commit Order Queue) and allow next applier thread to
    be ordered.

    Note: the calls to Commit_order_manager::wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  Commit_order_manager::wait_and_finish(thd, res);

  // This is normally done in ha_rollback_trans, but since we do not call this
  // for an external rollback we need to do it explicitly here.
  transaction->push_unsafe_rollback_warnings(thd);
  xid_state->unset_binlogged();

  MDL_context_backup_manager::instance().delete_backup(
      external_xid->key(), external_xid->key_length());
  transaction_cache_delete(transaction.get());
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  return res || gtid_error;
}

/**
  Handle the statement XA ROLLBACK for the case when xid corresponds to
  an internal XA transaction, that is a transaction generated by
  current session context.

  @param thd           Thread context
  @param xid_state     State of XA transaction corresponding to the current
                       session.

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_rollback::process_internal_xa_rollback(THD *thd,
                                                       XID_STATE *xid_state) {
  DBUG_TRACE;

  if (xid_state->has_state(XID_STATE::XA_NOTR) ||
      xid_state->has_state(XID_STATE::XA_ACTIVE)) {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  /*
    Acquire metadata lock which will ensure that XA ROLLBACK is blocked
    by active FLUSH TABLES WITH READ LOCK (and vice versa ROLLBACK in
    progress blocks FTWRL). This is to avoid binlog and redo entries
    while a backup is in progress.
  */
  MDL_request mdl_request;
  MDL_REQUEST_INIT(&mdl_request, MDL_key::COMMIT, "", "",
                   MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);
  if (thd->mdl_context.acquire_lock(&mdl_request,
                                    thd->variables.lock_wait_timeout)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    my_error(ER_XAER_RMERR, MYF(0));
    return true;
  }

  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  bool res = xa_trans_force_rollback(thd) || gtid_error;
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  // todo: report a bug in that the raised rm_error in this branch
  //       is masked unlike the "external" rollback branch above.
  DBUG_EXECUTE_IF("simulate_xa_rm_error", {
    my_error(ER_XA_RBROLLBACK, MYF(0));
    res = true;
  });

  cleanup_trans_state(thd);

  xid_state->set_state(XID_STATE::XA_NOTR);
  xid_state->unset_binlogged();
  trans_track_end_trx(thd);
  /* The transaction should be marked as complete in P_S. */
  assert(thd->m_transaction_psi == nullptr);
  return res;
}

bool Sql_cmd_xa_rollback::execute(THD *thd) {
  bool st = trans_xa_rollback(thd);

  if (!st) {
    thd->mdl_context.release_transactional_locks();
#ifdef WITH_WSREP
    WSREP_DEBUG("XA rollback failed, MDL released: %u", thd->thread_id());
#endif /* WITH_WSREP */
    /*
      We've just done a rollback, reset transaction
      isolation level and access mode to the session default.
    */
    trans_reset_one_shot_chistics(thd);
    my_ok(thd);
  }

  DBUG_EXECUTE_IF("crash_after_xa_rollback", DBUG_SUICIDE(););

  return st;
}

/**
  Start a XA transaction with the given xid value.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_start::trans_xa_start(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  DBUG_TRACE;

  if (xid_state->has_state(XID_STATE::XA_IDLE) && m_xa_opt == XA_RESUME) {
    bool not_equal = !xid_state->has_same_xid(m_xid);
    if (not_equal)
      my_error(ER_XAER_NOTA, MYF(0));
    else {
      xid_state->set_state(XID_STATE::XA_ACTIVE);
      MYSQL_SET_TRANSACTION_XA_STATE(
          thd->m_transaction_psi,
          (int)thd->get_transaction()->xid_state()->get_state());
    }
    return not_equal;
  }

  /* TODO: JOIN is not supported yet. */
  if (m_xa_opt != XA_NONE)
    my_error(ER_XAER_INVAL, MYF(0));
  else if (!xid_state->has_state(XID_STATE::XA_NOTR))
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
  else if (thd->locked_tables_mode || thd->in_active_multi_stmt_transaction())
    my_error(ER_XAER_OUTSIDE, MYF(0));
  else if (!trans_begin(thd)) {
    xid_state->start_normal_xa(m_xid);
    MYSQL_SET_TRANSACTION_XID(thd->m_transaction_psi,
                              (const void *)xid_state->get_xid(),
                              (int)xid_state->get_state());
    if (transaction_cache_insert(m_xid, thd->get_transaction())) {
      xid_state->reset();
      trans_rollback(thd);
    }
  }

  return thd->is_error() || !xid_state->has_state(XID_STATE::XA_ACTIVE);
}

bool Sql_cmd_xa_start::execute(THD *thd) {
  bool st = trans_xa_start(thd);

  if (!st) {
    thd->rpl_detach_engine_ha_data();
    my_ok(thd);
  }

  return st;
}

/**
  Put a XA transaction in the IDLE state.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_end::trans_xa_end(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  DBUG_TRACE;

  /* TODO: SUSPEND and FOR MIGRATE are not supported yet. */
  if (m_xa_opt != XA_NONE)
    my_error(ER_XAER_INVAL, MYF(0));
  else if (!xid_state->has_state(XID_STATE::XA_ACTIVE) &&
           !xid_state->has_state(XID_STATE::XA_ROLLBACK_ONLY))
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
  else if (!xid_state->has_same_xid(m_xid))
    my_error(ER_XAER_NOTA, MYF(0));
  else if (!xid_state->xa_trans_rolled_back()) {
    xid_state->set_state(XID_STATE::XA_IDLE);
    MYSQL_SET_TRANSACTION_XA_STATE(thd->m_transaction_psi,
                                   (int)xid_state->get_state());
  } else {
    MYSQL_SET_TRANSACTION_XA_STATE(thd->m_transaction_psi,
                                   (int)xid_state->get_state());
  }

  return thd->is_error() || !xid_state->has_state(XID_STATE::XA_IDLE);
}

bool Sql_cmd_xa_end::execute(THD *thd) {
  bool st = trans_xa_end(thd);

  if (!st) my_ok(thd);

  return st;
}

/**
  Put a XA transaction in the PREPARED state.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_prepare::trans_xa_prepare(THD *thd) {
  DBUG_TRACE;

  Transaction_ctx *tran = thd->get_transaction();
  XID_STATE *xid_state = tran->xid_state();

  DBUG_PRINT("xa", ("trans_xa_prepare: formatID:%ld",
                    xid_state->get_xid()->get_format_id()));

  if (!xid_state->has_state(XID_STATE::XA_IDLE)) {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  if (!xid_state->has_same_xid(m_xid)) {
    my_error(ER_XAER_NOTA, MYF(0));
||||||| merged common ancestors
          &thd->mdl_context, external_xid->key(), external_xid->key_length())) {
    return true;
  }

  return false;
}

/**
  Handle the statement XA COMMIT for the case when xid corresponds to
  an external XA transaction, that it a transaction generated outside
  current connection context. When xa_detach_on_prepare is ON (default),
  this applies to all prepared XA transactions. That is, an XA
  transaction pepared earlier on this connection, on a another active
  connection, on a terminated connection, or on a connction of an
  earlier server instance.

  @param thd           Thread context
  @param external_xid  XID value specified by XA COMMIT that corresponds to
                       a XA transaction generated outside current connection
                       context.
  @param thd_xid_state State of XA transaction corresponding to the current
                       connection that expected to have the value
                       XID_STATE::XA_NOTR. Non-const because it is (ab)used
                       to pass binlogged flag down to xa_binlog_commit().

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_commit::process_external_xa_commit(THD *thd,
                                                   xid_t *external_xid,
                                                   XID_STATE *thd_xid_state) {
  std::shared_ptr<Transaction_ctx> transaction =
      find_trn_for_recover_and_check_its_state(thd, external_xid,
                                               thd_xid_state);

  if (!transaction) return true;

  XID_STATE *xs = transaction->xid_state();

  assert(xs->get_xid()->eq(external_xid));

  /*
    Resumed transaction XA-commit.
    The case deals with the "external" XA-commit by either a slave applier
    or a different than XA-prepared transaction session.
  */
  bool res = xs->xa_trans_rolled_back();

  /*
    Metadata locks taken during XA COMMIT should be released when
    there is error in commit order execution, so we take a savepoint
    and rollback to it in case of error. The error during commit order
    execution can be temporary like commit order deadlock (check header
    comment for check_and_report_deadlock()) and can be recovered after
    retrying unlike other commit errors. And to do so we need to restore
    status of metadata locks i.e. rollback to savepoint, before the retry
    attempt to ensure order for applier threads.
  */
  MDL_savepoint mdl_savepoint = thd->mdl_context.mdl_savepoint();

  DEBUG_SYNC(thd, "external_xa_commit_before_acquire_xa_lock");
  /*
    Acquire XID_STATE::m_xa_lock to prevent concurrent running of two
    XA COMMIT/XA ROLLBACK statements. Without acquiring this lock an attempt
    to run two XA COMMIT/XA ROLLBACK statement for the same xid value may lead
    to writing two events for the same xid into the binlog (e.g. twice
    XA COMMIT event, that is an event for XA COMMIT some_xid_value
    followed by an another event XA COMMIT with the same xid value).
    As a consequences, presence of two XA COMMIT/XA ROLLACK statements for
    the same xid value in binlog would break replication.
  */
  std::lock_guard<std::mutex> lk(xs->get_xa_lock());
  /*
    Double check that the XA transaction still does exist since the transaction
    could be removed from the cache by another XA COMMIT/XA ROLLBACK statement
    being executed concurrently from parallel session with the same xid value.
  */
  if (!find_trn_for_recover_and_check_its_state(thd, external_xid,
                                                thd_xid_state))
    return true;

  if (acquire_mandatory_metadata_locks(thd, external_xid)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    my_error(ER_XA_RETRY, MYF(0));
    return true;
  }

  DEBUG_SYNC(thd, "external_xa_commit_after_acquire_commit_lock");

  /* Do not execute gtid wrapper whenever 'res' is true (rm error) */
  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  if (gtid_error) my_error(ER_XA_RBROLLBACK, MYF(0));
  res = res || gtid_error;

  /*
    xs' is_binlogged() is passed through xid_state's member to low-level
    logging routines for deciding how to log.  The same applies to
    Rollback case.
    Note that thd_xid_state is not at all related to the XA transaction
    we are committing externally. We just use it to pass the binlogged status
    down to do_binlog_xa_commit_rollback() which uses current_thd to retrieve
    the thd_xid_state and read the binlogged flag.
  */
  assert(thd_xid_state->is_binlogged() == false);
  if (xs->is_binlogged())
    thd_xid_state->set_binlogged();
  else
    thd_xid_state->unset_binlogged();

  /*
    Ensure externalization order for applier threads.

    Note: the calls to Commit_order_manager::wait/wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  if (Commit_order_manager::wait(thd)) {
    /*
      In case of error, wait_and_finish() checks if transaction can be
      retried and if it can be retried then only it remove itself from
      waiting (Commit Order Queue) and allow next applier thread to be
      ordered.
    */
    Commit_order_manager::wait_and_finish(thd, true);

    need_clear_owned_gtid = true;
    gtid_error = true;
    gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
    thd->mdl_context.rollback_to_savepoint(mdl_savepoint);
    thd_xid_state->unset_binlogged();
    return true;
  }

  res = ha_commit_or_rollback_by_xid(external_xid, !res) || res;

  /*
    After ensuring externalization order for applier thread, remove it
    from waiting (Commit Order Queue) and allow next applier thread to
    be ordered.

    Note: the calls to Commit_order_manager::wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  Commit_order_manager::wait_and_finish(thd, res);

  // Restoring the binlogged status of the thd_xid_state after borrowing it
  // to pass the binlogged flag to binlog_xa_commit().
  thd_xid_state->unset_binlogged();

  MDL_context_backup_manager::instance().delete_backup(
      external_xid->key(), external_xid->key_length());

  transaction_cache_delete(transaction.get());
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  return res;
}

/**
  Handle the statement XA COMMIT for the case when xid corresponds to
  an internal XA transaction, that is a transaction generated by
  current session context.

  @param thd           Thread context
  @param xid_state     State of XA transaction corresponding to the current
                       session.

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_commit::process_internal_xa_commit(THD *thd,
                                                   XID_STATE *xid_state) {
  DBUG_TRACE;
  bool res = false;
  bool gtid_error = false, need_clear_owned_gtid = false;

  if (xid_state->xa_trans_rolled_back()) {
    xa_trans_force_rollback(thd);
    res = thd->is_error();
  } else if (xid_state->has_state(XID_STATE::XA_IDLE) &&
             m_xa_opt == XA_ONE_PHASE) {
    int r = ha_commit_trans(thd, true);
    if ((res = r)) my_error(r == 1 ? ER_XA_RBROLLBACK : ER_XAER_RMERR, MYF(0));
  } else if (xid_state->has_state(XID_STATE::XA_PREPARED) &&
             m_xa_opt == XA_NONE) {
    MDL_request mdl_request;

    /*
      Acquire metadata lock which will ensure that COMMIT is blocked
      by active FLUSH TABLES WITH READ LOCK (and vice versa COMMIT in
      progress blocks FTWRL).

      We allow FLUSHer to COMMIT; we assume FLUSHer knows what it does.
    */
    MDL_REQUEST_INIT(&mdl_request, MDL_key::COMMIT, "", "",
                     MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);
    if (thd->mdl_context.acquire_lock(&mdl_request,
                                      thd->variables.lock_wait_timeout)) {
      /*
        We can't rollback an XA transaction on lock failure due to
        Innodb redo log and bin log update are involved in rollback.
        Return error to user for a retry.
      */
      my_error(ER_XA_RETRY, MYF(0));
      return true;
    }

    std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
    if (gtid_error) {
      res = true;
      /*
        Failure to store gtid is regarded as a unilateral one of the
        resource manager therefore the transaction is to be rolled back.
        The specified error is the same as @c xa_trans_force_rollback.
        The prepared XA will be rolled back along and so will do Gtid state,
        see ha_rollback_trans().

        Todo/fixme: fix binlogging, "XA rollback" event could be missed out.
        Todo/fixme: as to XAER_RMERR, should not it be XA_RBROLLBACK?
                    Rationale: there's no consistency concern after rollback,
                    unlike what XAER_RMERR suggests.
      */
      ha_rollback_trans(thd, true);
      my_error(ER_XAER_RMERR, MYF(0));
    } else {
      DBUG_EXECUTE_IF("simulate_crash_on_commit_xa_trx", DBUG_SUICIDE(););
      DEBUG_SYNC(thd, "trans_xa_commit_after_acquire_commit_lock");

      if (tc_log)
        res = tc_log->commit(thd, /* all */ true);
      else
        res = ha_commit_low(thd, /* all */ true);

      DBUG_EXECUTE_IF("simulate_xa_commit_log_failure", { res = true; });

      if (res)
        my_error(ER_XAER_RMERR, MYF(0));  // todo/fixme: consider to rollback it
#ifdef HAVE_PSI_TRANSACTION_INTERFACE
      else {
        /*
          Since we don't call ha_commit_trans() for prepared transactions,
          we need to explicitly mark the transaction as committed.
        */
        MYSQL_COMMIT_TRANSACTION(thd->m_transaction_psi);
      }

      thd->m_transaction_psi = nullptr;
#endif
    }
  } else {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  cleanup_trans_state(thd);

  xid_state->set_state(XID_STATE::XA_NOTR);
  xid_state->unset_binlogged();
  trans_track_end_trx(thd);
  /* The transaction should be marked as complete in P_S. */
  assert(thd->m_transaction_psi == nullptr || res);
  return res;
}

bool Sql_cmd_xa_commit::execute(THD *thd) {
  bool st = trans_xa_commit(thd);

  if (!st) {
    thd->mdl_context.release_transactional_locks();
    /*
        We've just done a commit, reset transaction
        isolation level and access mode to the session default.
    */
    trans_reset_one_shot_chistics(thd);

    my_ok(thd);
  }
  return st;
}

/**
  Roll back and terminate a XA transaction.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_rollback::trans_xa_rollback(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  bool res = true;

  /* Inform clone handler of XA operation. */
  Clone_handler::XA_Operation xa_guard(thd);
  if (!xid_state->has_same_xid(m_xid)) {
    res = process_external_xa_rollback(thd, m_xid, xid_state);
  } else {
    res = process_internal_xa_rollback(thd, xid_state);
  }
  return (res);
}

/**
  Handle the statement XA ROLLBACK for the case when xid corresponds to
  an external XA transaction, that it a transaction generated outside
  current session context.

  @param thd           Thread context
  @param external_xid  XID value specified by XA ROLLBACK that corresponds to
                       a XA transaction generated outside current session
                       context. In fact, it means that XA ROLLBACK is run
                       against a XA transaction recovered after server restart.
  @param xid_state     State of XA transaction corresponding to the current
                       session that expected to have the value
                       XID_STATE::XA_NOTR

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_rollback::process_external_xa_rollback(THD *thd,
                                                       xid_t *external_xid,
                                                       XID_STATE *xid_state) {
  DBUG_TRACE;

  std::shared_ptr<Transaction_ctx> transaction =
      find_trn_for_recover_and_check_its_state(thd, external_xid, xid_state);

  if (!transaction) return true;

  XID_STATE *xs = transaction->xid_state();

  assert(xs->get_xid()->eq(external_xid));

  /*
    Metadata locks taken during XA ROLLBACK should be released when
    there is error in commit order execution, so we take a savepoint
    and rollback to it in case of error.
  */
  MDL_savepoint mdl_savepoint = thd->mdl_context.mdl_savepoint();

  /*
    Acquire XID_STATE::m_xa_lock to prevent concurrent running of two
    XA COMMIT/XA ROLLBACK statements. Without acquiring this lock an attempt
    to run two XA COMMIT/XA ROLLBACK statement for the same xid value may lead
    to writing two events for the same xid into the binlog (e.g. twice
    XA ROLLBACK event, that is an event for XA ROLLBACK some_xid_value
    followed by an another event XA ROLLBACK with the same xid value).
    As a consequences, presence of two XA COMMIT/XA ROLLACK statements for
    the same xid value in binlog would break replication.
  */
  std::lock_guard<std::mutex> lk(xs->get_xa_lock());
  /*
    Double check that the XA transaction still does exist since the transaction
    could be removed from the cache by another XA COMMIT/XA ROLLBACK statement
    being executed concurrently from parallel session with the same xid value.
  */
  if (!find_trn_for_recover_and_check_its_state(thd, external_xid, xid_state))
    return true;

  if (acquire_mandatory_metadata_locks(thd, external_xid)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    DBUG_PRINT("xa", ("Failed to acquire MDLs for XA ROLLBACK on"));
    my_error(ER_XAER_RMERR, MYF(0));
    return true;
  }

  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  if (gtid_error) my_error(ER_XA_RBROLLBACK, MYF(0));
  bool res = xs->xa_trans_rolled_back();

  if (xs->is_binlogged())
    xid_state->set_binlogged();
  else
    xid_state->unset_binlogged();

  /*
    Ensure externalization order for applier threads.

    Note: the calls to Commit_order_manager::wait/wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  if (Commit_order_manager::wait(thd)) {
    /*
      In case of error, wait_and_finish() checks if transaction can be
      retried and if it can be retried then only it remove itself from
      waiting (Commit Order Queue) and allow next applier thread to be
      ordered.
    */
    Commit_order_manager::wait_and_finish(thd, true);

    need_clear_owned_gtid = true;
    gtid_error = true;
    gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
    thd->mdl_context.rollback_to_savepoint(mdl_savepoint);
    return true;
  }

  res = ha_commit_or_rollback_by_xid(external_xid, false) || res;

  /*
    After ensuring externalization order for applier thread, remove it
    from waiting (Commit Order Queue) and allow next applier thread to
    be ordered.

    Note: the calls to Commit_order_manager::wait_and_finish() will be
          no-op for threads other than replication applier threads.
  */
  Commit_order_manager::wait_and_finish(thd, res);

  // This is normally done in ha_rollback_trans, but since we do not call this
  // for an external rollback we need to do it explicitly here.
  transaction->push_unsafe_rollback_warnings(thd);
  xid_state->unset_binlogged();

  MDL_context_backup_manager::instance().delete_backup(
      external_xid->key(), external_xid->key_length());
  transaction_cache_delete(transaction.get());
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  return res || gtid_error;
}

/**
  Handle the statement XA ROLLBACK for the case when xid corresponds to
  an internal XA transaction, that is a transaction generated by
  current session context.

  @param thd           Thread context
  @param xid_state     State of XA transaction corresponding to the current
                       session.

  @return  operation result
    @retval false  Success
    @retval true   Failure
*/
bool Sql_cmd_xa_rollback::process_internal_xa_rollback(THD *thd,
                                                       XID_STATE *xid_state) {
  DBUG_TRACE;

  if (xid_state->has_state(XID_STATE::XA_NOTR) ||
      xid_state->has_state(XID_STATE::XA_ACTIVE)) {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  /*
    Acquire metadata lock which will ensure that XA ROLLBACK is blocked
    by active FLUSH TABLES WITH READ LOCK (and vice versa ROLLBACK in
    progress blocks FTWRL). This is to avoid binlog and redo entries
    while a backup is in progress.
  */
  MDL_request mdl_request;
  MDL_REQUEST_INIT(&mdl_request, MDL_key::COMMIT, "", "",
                   MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);
  if (thd->mdl_context.acquire_lock(&mdl_request,
                                    thd->variables.lock_wait_timeout)) {
    /*
      We can't rollback an XA transaction on lock failure due to
      Innodb redo log and bin log update is involved in rollback.
      Return error to user for a retry.
    */
    my_error(ER_XAER_RMERR, MYF(0));
    return true;
  }

  bool gtid_error = false;
  bool need_clear_owned_gtid = false;
  std::tie(gtid_error, need_clear_owned_gtid) = commit_owned_gtids(thd, true);
  bool res = xa_trans_force_rollback(thd) || gtid_error;
  gtid_state_commit_or_rollback(thd, need_clear_owned_gtid, !gtid_error);
  // todo: report a bug in that the raised rm_error in this branch
  //       is masked unlike the "external" rollback branch above.
  DBUG_EXECUTE_IF("simulate_xa_rm_error", {
    my_error(ER_XA_RBROLLBACK, MYF(0));
    res = true;
  });

  cleanup_trans_state(thd);

  xid_state->set_state(XID_STATE::XA_NOTR);
  xid_state->unset_binlogged();
  trans_track_end_trx(thd);
  /* The transaction should be marked as complete in P_S. */
  assert(thd->m_transaction_psi == nullptr);
  return res;
}

bool Sql_cmd_xa_rollback::execute(THD *thd) {
  bool st = trans_xa_rollback(thd);

  if (!st) {
    thd->mdl_context.release_transactional_locks();
    /*
      We've just done a rollback, reset transaction
      isolation level and access mode to the session default.
    */
    trans_reset_one_shot_chistics(thd);
    my_ok(thd);
  }

  DBUG_EXECUTE_IF("crash_after_xa_rollback", DBUG_SUICIDE(););

  return st;
}

/**
  Start a XA transaction with the given xid value.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_start::trans_xa_start(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  DBUG_TRACE;

  if (xid_state->has_state(XID_STATE::XA_IDLE) && m_xa_opt == XA_RESUME) {
    bool not_equal = !xid_state->has_same_xid(m_xid);
    if (not_equal)
      my_error(ER_XAER_NOTA, MYF(0));
    else {
      xid_state->set_state(XID_STATE::XA_ACTIVE);
      MYSQL_SET_TRANSACTION_XA_STATE(
          thd->m_transaction_psi,
          (int)thd->get_transaction()->xid_state()->get_state());
    }
    return not_equal;
  }

  /* TODO: JOIN is not supported yet. */
  if (m_xa_opt != XA_NONE)
    my_error(ER_XAER_INVAL, MYF(0));
  else if (!xid_state->has_state(XID_STATE::XA_NOTR))
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
  else if (thd->locked_tables_mode || thd->in_active_multi_stmt_transaction())
    my_error(ER_XAER_OUTSIDE, MYF(0));
  else if (!trans_begin(thd)) {
    xid_state->start_normal_xa(m_xid);
    MYSQL_SET_TRANSACTION_XID(thd->m_transaction_psi,
                              (const void *)xid_state->get_xid(),
                              (int)xid_state->get_state());
    if (transaction_cache_insert(m_xid, thd->get_transaction())) {
      xid_state->reset();
      trans_rollback(thd);
    }
  }

  return thd->is_error() || !xid_state->has_state(XID_STATE::XA_ACTIVE);
}

bool Sql_cmd_xa_start::execute(THD *thd) {
  bool st = trans_xa_start(thd);

  if (!st) {
    thd->rpl_detach_engine_ha_data();
    my_ok(thd);
  }

  return st;
}

/**
  Put a XA transaction in the IDLE state.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_end::trans_xa_end(THD *thd) {
  XID_STATE *xid_state = thd->get_transaction()->xid_state();
  DBUG_TRACE;

  /* TODO: SUSPEND and FOR MIGRATE are not supported yet. */
  if (m_xa_opt != XA_NONE)
    my_error(ER_XAER_INVAL, MYF(0));
  else if (!xid_state->has_state(XID_STATE::XA_ACTIVE) &&
           !xid_state->has_state(XID_STATE::XA_ROLLBACK_ONLY))
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
  else if (!xid_state->has_same_xid(m_xid))
    my_error(ER_XAER_NOTA, MYF(0));
  else if (!xid_state->xa_trans_rolled_back()) {
    xid_state->set_state(XID_STATE::XA_IDLE);
    MYSQL_SET_TRANSACTION_XA_STATE(thd->m_transaction_psi,
                                   (int)xid_state->get_state());
  } else {
    MYSQL_SET_TRANSACTION_XA_STATE(thd->m_transaction_psi,
                                   (int)xid_state->get_state());
  }

  return thd->is_error() || !xid_state->has_state(XID_STATE::XA_IDLE);
}

bool Sql_cmd_xa_end::execute(THD *thd) {
  bool st = trans_xa_end(thd);

  if (!st) my_ok(thd);

  return st;
}

/**
  Put a XA transaction in the PREPARED state.

  @param thd    Current thread

  @retval false  Success
  @retval true   Failure
*/

bool Sql_cmd_xa_prepare::trans_xa_prepare(THD *thd) {
  DBUG_TRACE;

  Transaction_ctx *tran = thd->get_transaction();
  XID_STATE *xid_state = tran->xid_state();

  DBUG_PRINT("xa", ("trans_xa_prepare: formatID:%ld",
                    xid_state->get_xid()->get_format_id()));

  if (!xid_state->has_state(XID_STATE::XA_IDLE)) {
    my_error(ER_XAER_RMFAIL, MYF(0), xid_state->state_name());
    return true;
  }

  if (!xid_state->has_same_xid(m_xid)) {
    my_error(ER_XAER_NOTA, MYF(0));
=======
          &thd->mdl_context, detached_xid->key(), detached_xid->key_length())) {
>>>>>>> tag/Percona-Server-8.0.30-22
    return true;
  }

  return false;
}

bool XID_STATE::xa_trans_rolled_back() {
  DBUG_EXECUTE_IF("simulate_xa_rm_error", rm_error = true;);
  if (rm_error) {
    switch (rm_error) {
      case ER_LOCK_WAIT_TIMEOUT:
        my_error(ER_XA_RBTIMEOUT, MYF(0));
        break;
      case ER_LOCK_DEADLOCK:
        my_error(ER_XA_RBDEADLOCK, MYF(0));
        break;
      default:
        my_error(ER_XA_RBROLLBACK, MYF(0));
    }
    xa_state = XID_STATE::XA_ROLLBACK_ONLY;
  }

  return (xa_state == XID_STATE::XA_ROLLBACK_ONLY);
}

bool XID_STATE::check_xa_idle_or_prepared(bool report_error) const {
  if (xa_state == XA_IDLE || xa_state == XA_PREPARED) {
    if (report_error)
      my_error(ER_XAER_RMFAIL, MYF(0), xa_state_names[xa_state]);

    return true;
  }

  return false;
}

bool XID_STATE::check_has_uncommitted_xa() const {
  if (xa_state == XA_IDLE || xa_state == XA_PREPARED ||
      xa_state == XA_ROLLBACK_ONLY) {
    my_error(ER_XAER_RMFAIL, MYF(0), xa_state_names[xa_state]);
    return true;
  }

  return false;
}

bool XID_STATE::check_in_xa(bool report_error) const {
  if (xa_state != XA_NOTR) {
    if (report_error)
      my_error(ER_XAER_RMFAIL, MYF(0), xa_state_names[xa_state]);
    return true;
  }

  return false;
}

void XID_STATE::set_error(THD *thd) {
  if (xa_state != XA_NOTR) rm_error = thd->get_stmt_da()->mysql_errno();
}

void XID_STATE::store_xid_info(Protocol *protocol,
                               bool print_xid_as_hex) const {
  protocol->store_longlong(static_cast<longlong>(m_xid.formatID), false);
  protocol->store_longlong(static_cast<longlong>(m_xid.gtrid_length), false);
  protocol->store_longlong(static_cast<longlong>(m_xid.bqual_length), false);

  if (print_xid_as_hex) {
    /*
      xid_buf contains enough space for 0x followed by HEX representation
      of the binary XID data and one null termination character.
    */
    char xid_buf[XIDDATASIZE * 2 + 2 + 1];

    xid_buf[0] = '0';
    xid_buf[1] = 'x';

    size_t xid_str_len =
        bin_to_hex_str(xid_buf + 2, sizeof(xid_buf) - 2, m_xid.data,
                       m_xid.gtrid_length + m_xid.bqual_length) +
        2;
    protocol->store_string(xid_buf, xid_str_len, &my_charset_bin);
  } else {
    protocol->store_string(m_xid.data, m_xid.gtrid_length + m_xid.bqual_length,
                           &my_charset_bin);
  }
}

#ifndef NDEBUG
char *XID::xid_to_str(char *buf) const {
  char *s = buf;
  *s++ = '\'';

  for (int i = 0; i < gtrid_length + bqual_length; i++) {
    /* is_next_dig is set if next character is a number */
    bool is_next_dig = false;
    if (i < XIDDATASIZE) {
      char ch = data[i + 1];
      is_next_dig = (ch >= '0' && ch <= '9');
    }
    if (i == gtrid_length) {
      *s++ = '\'';
      if (bqual_length) {
        *s++ = '.';
        *s++ = '\'';
      }
    }
    uchar c = static_cast<uchar>(data[i]);
    if (c < 32 || c > 126) {
      *s++ = '\\';
      /*
        If next character is a number, write current character with
        3 octal numbers to ensure that the next number is not seen
        as part of the octal number
      */
      if (c > 077 || is_next_dig) *s++ = _dig_vec_lower[c >> 6];
      if (c > 007 || is_next_dig) *s++ = _dig_vec_lower[(c >> 3) & 7];
      *s++ = _dig_vec_lower[c & 7];
    } else {
      if (c == '\'' || c == '\\') *s++ = '\\';
      *s++ = c;
    }
  }
  *s++ = '\'';
  *s = 0;
  return buf;
}
#endif

/**
  The function restores previously saved storage engine transaction context.

  @param     thd     Thread context
*/
static void attach_native_trx(THD *thd) {
  for (auto &ha_info :
       thd->get_transaction()->ha_trx_info(Transaction_ctx::SESSION)) {
    ha_info.reset();
  }
  thd->rpl_reattach_engine_ha_data();
}

bool applier_reset_xa_trans(THD *thd) {
  DBUG_TRACE;
  Transaction_ctx *trn_ctx = thd->get_transaction();

  if (!is_xa_tran_detached_on_prepare(thd)) {
    XID_STATE *xid_state = trn_ctx->xid_state();

    if (MDL_context_backup_manager::instance().create_backup(
            &thd->mdl_context, xid_state->get_xid()->key(),
            xid_state->get_xid()->key_length()))
      return true;

    /*
      In the following the server transaction state gets reset for
      a slave applier thread similarly to xa_commit logics
      except commit does not run.
    */
    thd->variables.option_bits &= ~OPTION_BEGIN;
    trn_ctx->reset_unsafe_rollback_flags(Transaction_ctx::STMT);
    thd->server_status &= ~SERVER_STATUS_IN_TRANS;
    /* Server transaction ctx is detached from THD */
    xa::Transaction_cache::detach(trn_ctx);
    xid_state->reset();
  }
  /*
     The current engine transactions is detached from THD, and
     previously saved is restored.
  */
  attach_native_trx(thd);
  trn_ctx->set_ha_trx_info(Transaction_ctx::SESSION, nullptr);
  trn_ctx->set_no_2pc(Transaction_ctx::SESSION, false);
  trn_ctx->cleanup();
#ifdef HAVE_PSI_TRANSACTION_INTERFACE
  thd->m_transaction_psi = nullptr;
#endif
  thd->mdl_context.release_transactional_locks();
  /*
    On client sessions a XA PREPARE will always be followed by a XA COMMIT
    or a XA ROLLBACK, and both statements will reset the tx isolation level
    and access mode when the statement is finishing a transaction.

    For replicated workload it is possible to have other transactions between
    the XA PREPARE and the XA [COMMIT|ROLLBACK].

    So, if the slave applier changed the current transaction isolation level,
    it needs to be restored to the session default value after having the
    XA transaction prepared.
  */
  trans_reset_one_shot_chistics(thd);

  return thd->is_error();
}

/**
  The function detaches existing storage engines transaction
  context from thd. Backup area to save it is provided to low level
  storage engine function.

  is invoked by plugin_foreach() after
  trans_xa_start() for each storage engine.

  @param[in,out]     thd     Thread context
  @param             plugin  Reference to handlerton

  @return    false   on success, true otherwise.
*/

bool detach_native_trx(THD *thd, plugin_ref plugin, void *) {
  DBUG_TRACE;
  handlerton *hton = plugin_data<handlerton *>(plugin);

  if (hton->replace_native_transaction_in_thd) {
    /* Ensure any active backup engine ha_data won't be overwritten */
    assert(!thd->get_ha_data(hton->slot)->ha_ptr_backup);

    hton->replace_native_transaction_in_thd(
        thd, nullptr, &thd->get_ha_data(hton->slot)->ha_ptr_backup);
  }

  return false;
}

bool reattach_native_trx(THD *thd, plugin_ref plugin, void *) {
  DBUG_TRACE;
  handlerton *hton = plugin_data<handlerton *>(plugin);

  if (hton->replace_native_transaction_in_thd) {
    /* restore the saved original engine transaction's link with thd */
    void **trx_backup = &thd->get_ha_data(hton->slot)->ha_ptr_backup;

    hton->replace_native_transaction_in_thd(thd, *trx_backup, nullptr);
    *trx_backup = nullptr;
  }
  return false;
}

/**
   Disconnect transaction in SE. This the same action which is performed
   by SE when disconnecting a connection which has a prepared XA transaction,
   when xa_detach_on_prepare is OFF. Signature matches that
   required by plugin_foreach.
*/
bool disconnect_native_trx(THD *thd, plugin_ref plugin, void *) {
  handlerton *hton = plugin_data<handlerton *>(plugin);
  assert(hton != nullptr);

  if (hton->state != SHOW_OPTION_YES) {
    assert(hton->replace_native_transaction_in_thd == nullptr);
    return false;
  }
  assert(hton->slot != HA_SLOT_UNDEF);

  if (hton->replace_native_transaction_in_thd != nullptr) {
    // Force call to trx_disconnect_prepared in Innodb when calling with
    // nullptr,nullptr
    hton->replace_native_transaction_in_thd(thd, nullptr, nullptr);
  }

  // Reset session Ha_trx_info so it is not marked as started.
  // Otherwise, we will not be able to start a new XA transaction on
  // this connection.
  thd->get_ha_data(hton->slot)
      ->ha_info[Transaction_ctx::SESSION]
      .reset();  // Mark as not started

  thd->get_ha_data(hton->slot)
      ->ha_info[Transaction_ctx::STMT]
      .reset();  // Mark as not started

  return false;
}

bool thd_holds_xa_transaction(THD *thd) {
  auto xs = thd->get_transaction()->xid_state();
  return xs->get_xid()->get_my_xid() == 0 && !xs->has_state(XID_STATE::XA_NOTR);
}

bool is_xa_prepare(THD *thd) {
  return thd->lex->sql_command == SQLCOM_XA_PREPARE;
}

bool is_xa_rollback(THD *thd) {
  return thd->lex->sql_command == SQLCOM_XA_ROLLBACK;
}
