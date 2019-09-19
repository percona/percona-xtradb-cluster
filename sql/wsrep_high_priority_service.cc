/* Copyright 2018 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "wsrep_high_priority_service.h"
#include "mysql/components/services/log_builtins.h"
#include "wsrep_applier.h"
#include "wsrep_binlog.h"
#include "wsrep_schema.h"
#include "wsrep_trans_observer.h"
#include "wsrep_xid.h"
#include "sql/rpl_info_factory.h"

#include "debug_sync.h"
#include "sql_class.h" /* THD */
#include "transaction.h"
#include "item_func.h"

#include "sql_base.h"     // close_temporary_table()

extern handlerton *binlog_hton;

/* RLI */
#include "rpl_rli.h"
#define NUMBER_OF_FIELDS_TO_IDENTIFY_COORDINATOR 1
#define NUMBER_OF_FIELDS_TO_IDENTIFY_WORKER 2
#include "rpl_mi.h"

namespace {
/*
  Scoped mode for applying non-transactional write sets (TOI)
 */
class Wsrep_non_trans_mode {
 public:
  Wsrep_non_trans_mode(THD *thd, const wsrep::ws_meta &ws_meta)
      : m_thd(thd),
        m_option_bits(thd->variables.option_bits),
        m_server_status(thd->server_status) {
    m_thd->variables.option_bits &= ~OPTION_BEGIN;
    m_thd->server_status &= ~SERVER_STATUS_IN_TRANS;
    m_thd->wsrep_cs().enter_toi(ws_meta);
  }
  ~Wsrep_non_trans_mode() {
    m_thd->variables.option_bits = m_option_bits;
    m_thd->server_status = m_server_status;
    m_thd->wsrep_cs().leave_toi();
  }

 private:
  Wsrep_non_trans_mode(const Wsrep_non_trans_mode &);
  Wsrep_non_trans_mode &operator=(const Wsrep_non_trans_mode &);
  THD *m_thd;
  ulonglong m_option_bits;
  uint m_server_status;
};
}  // namespace

static Relay_log_info *wsrep_relay_log_init(const char *) {
  uint rli_option = INFO_REPOSITORY_DUMMY;
  Relay_log_info *rli = NULL;
  rli =
      Rpl_info_factory::create_rli(rli_option, false, WSREP_CHANNEL_NAME, true);
  if (!rli) {
    WSREP_ERROR("Failed to create Relay-Log for wsrep thread, aborting");
    unireg_abort(MYSQLD_ABORT_EXIT);
  }
  rli->set_rli_description_event(new Format_description_log_event());

  rli->current_mts_submode = new Mts_submode_wsrep();
  return (rli);
}

static void wsrep_setup_uk_and_fk_checks(THD *thd) {
  /* Tune FK and UK checking policy. These are reset back to original
     in Wsrep_high_priority_service destructor. */
  if (wsrep_slave_UK_checks == false)
    thd->variables.option_bits |= OPTION_RELAXED_UNIQUE_CHECKS;
  else
    thd->variables.option_bits &= ~OPTION_RELAXED_UNIQUE_CHECKS;

  if (wsrep_slave_FK_checks == false)
    thd->variables.option_bits |= OPTION_NO_FOREIGN_KEY_CHECKS;
  else
    thd->variables.option_bits &= ~OPTION_NO_FOREIGN_KEY_CHECKS;
}

/****************************************************************************
                         High priority service
*****************************************************************************/

Wsrep_high_priority_service::Wsrep_high_priority_service(THD *thd)
    : wsrep::high_priority_service(Wsrep_server_state::instance()),
      wsrep::high_priority_context(thd->wsrep_cs()),
      m_thd(thd),
      m_rli() {
  LEX_CSTRING db_str = {NULL, 0};
  m_shadow.option_bits = thd->variables.option_bits;
  m_shadow.server_status = thd->server_status;
  m_shadow.vio = thd->active_vio;
  m_shadow.tx_isolation = thd->variables.transaction_isolation;
  m_shadow.db = (char *)thd->db().str;
  m_shadow.db_length = thd->db().length;
  m_shadow.user_time = thd->user_time;
  m_shadow.row_count_func = thd->get_row_count_func();
  m_shadow.wsrep_applier = thd->wsrep_applier;

  /* Disable general logging on applier threads */
  thd->variables.option_bits |= OPTION_LOG_OFF;
  /* Enable binlogging if opt_log_slave_updates is set */
  if (opt_log_slave_updates)
    thd->variables.option_bits |= OPTION_BIN_LOG;
  else
    thd->variables.option_bits &= ~(OPTION_BIN_LOG);

  thd->set_active_vio(0);
  thd->reset_db(db_str);
  thd->clear_error();
  thd->variables.transaction_isolation = ISO_READ_COMMITTED;
  thd->tx_isolation = ISO_READ_COMMITTED;

  /* From trans_begin() */
  thd->variables.option_bits |= OPTION_BEGIN;
  thd->server_status |= SERVER_STATUS_IN_TRANS;

  /* Make THD wsrep_applier so that it cannot be killed */
  thd->wsrep_applier = true;

  if (!thd->wsrep_rli) {
    thd->wsrep_rli = wsrep_relay_log_init("wsrep_relay");
    assert(!thd->rli_slave);
    thd->rli_slave = thd->wsrep_rli;
    thd->wsrep_rli->info_thd = thd;

    thd->init_query_mem_roots();

    if ((thd->wsrep_rli->deferred_events_collecting =
             thd->wsrep_rli->rpl_filter->is_on()))
      thd->wsrep_rli->deferred_events = new Deferred_log_events();

    DBUG_ASSERT(thd->rli_slave->info_thd == thd);

    // thd->init_for_queries(thd->wsrep_rli);
  }
  thd->wsrep_rli->info_thd = thd;

  m_rli = thd->wsrep_rli;
  thd_proc_info(thd, "wsrep applier idle");
}

Wsrep_high_priority_service::~Wsrep_high_priority_service() {
  THD *thd = m_thd;
  thd->variables.option_bits = m_shadow.option_bits;
  thd->server_status = m_shadow.server_status;
  thd->set_active_vio(m_shadow.vio);
  thd->variables.transaction_isolation = m_shadow.tx_isolation;
  LEX_CSTRING db_str = {m_shadow.db, m_shadow.db_length};
  thd->reset_db(db_str);
  thd->user_time = m_shadow.user_time;

  if (thd->wsrep_rli) {
    delete thd->wsrep_rli->current_mts_submode;
    thd->wsrep_rli->current_mts_submode = 0;

    if (thd->wsrep_rli->deferred_events != NULL) {
      delete thd->wsrep_rli->deferred_events;
    }
    thd->wsrep_rli->deferred_events = 0;

    delete thd->wsrep_rli;
    thd->wsrep_rli = 0;

    /* rli_slave MySQL counter part which is initialized to wsrep_rli. */
    thd->rli_slave = NULL;
  }

  thd->set_row_count_func(m_shadow.row_count_func);
  thd->wsrep_applier = m_shadow.wsrep_applier;
}

int Wsrep_high_priority_service::start_transaction(
    const wsrep::ws_handle &ws_handle, const wsrep::ws_meta &ws_meta) {
  DBUG_ENTER(" Wsrep_high_priority_service::start_transaction");
  /* trans_begin doesn't make sense here as transaction is started
  through a BEGIN event that is part of the write-set replication.
  This explicit trans_begin is added by upstream for MDB compatibility
  that doesn't carry BEGIN event like MySQL. */
#if 0
  DBUG_RETURN(m_thd->wsrep_cs().start_transaction(ws_handle, ws_meta) ||
              trans_begin(m_thd));
#endif
  DBUG_RETURN(m_thd->wsrep_cs().start_transaction(ws_handle, ws_meta));
}

const wsrep::transaction &Wsrep_high_priority_service::transaction() const {
  DBUG_ENTER(" Wsrep_high_priority_service::transaction");
  DBUG_RETURN(m_thd->wsrep_trx());
}

int Wsrep_high_priority_service::adopt_transaction(
    const wsrep::transaction &transaction) {
  DBUG_ENTER(" Wsrep_high_priority_service::adopt_transaction");
  /* Adopt transaction first to set up transaction meta data for
     trans begin. If trans_begin() fails for some reason, roll back
     the wsrep transaction before return. */
  m_thd->wsrep_cs().adopt_transaction(transaction);
  int ret = trans_begin(m_thd);
  if (ret) {
    m_thd->wsrep_cs().before_rollback();
    m_thd->wsrep_cs().after_rollback();
  }
  DBUG_RETURN(ret);
}

int Wsrep_high_priority_service::append_fragment_and_commit(
    const wsrep::ws_handle &ws_handle, const wsrep::ws_meta &ws_meta,
    const wsrep::const_buffer &data) {
  DBUG_ENTER("Wsrep_high_priority_service::append_fragment_and_commit");
  int ret = start_transaction(ws_handle, ws_meta);
  /*
    Start transaction explicitly to avoid early commit via
    trans_commit_stmt() in append_fragment()
  */
  ret = ret || trans_begin(m_thd);
  ret = ret || wsrep_schema->append_fragment(
                   m_thd, ws_meta.server_id(), ws_meta.transaction_id(),
                   ws_meta.seqno(), ws_meta.flags(), data);

#if 0
  /*
    Note: The commit code below seems to be identical to
    Wsrep_storage_service::commit(). Consider implementing
    common utility function to deal with commit.
   */
  const bool do_binlog_commit = (opt_log_slave_updates && wsrep_gtid_mode &&
                                 m_thd->variables.gtid_seq_no);
  // TODO: G-4
  assert(0);
  const bool do_binlog_commit = false;
  
  /*
   Write skip event into binlog if gtid_mode is on. This is to
   maintain gtid continuity.
 */
  if (do_binlog_commit) {
    ret = wsrep_write_skip_event(m_thd);
  }
#endif

  if (!ret) {
    ret = m_thd->wsrep_cs().prepare_for_ordering(ws_handle, ws_meta, true);
  }

  ret = ret || trans_commit(m_thd);

  m_thd->wsrep_cs().after_applying();
  m_thd->mdl_context.release_transactional_locks();

  thd_proc_info(m_thd, "wsrep applier committed");

  DBUG_RETURN(ret);
}

int Wsrep_high_priority_service::remove_fragments(
    const wsrep::ws_meta &ws_meta) {
  DBUG_ENTER("Wsrep_high_priority_service::remove_fragments");
  int ret = wsrep_schema->remove_fragments(m_thd, ws_meta.server_id(),
                                           ws_meta.transaction_id(),
                                           m_thd->wsrep_sr().fragments());
  DBUG_RETURN(ret);
}

int Wsrep_high_priority_service::commit(const wsrep::ws_handle &ws_handle,
                                        const wsrep::ws_meta &ws_meta) {
  DBUG_ENTER("Wsrep_high_priority_service::commit");
  THD *thd = m_thd;
  DBUG_ASSERT(thd->wsrep_trx().active());
  thd->wsrep_cs().prepare_for_ordering(ws_handle, ws_meta, true);

  // thd_proc_info(thd, "committing");
  THD_STAGE_INFO(thd, stage_wsrep_committing);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: committing write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  const bool is_ordered = !ws_meta.seqno().is_undefined();
  int ret = trans_commit(thd);

  if (ret == 0) {
    m_rli->cleanup_context(m_thd, 0);
    m_thd->variables.gtid_next.set_automatic();
  }

  m_thd->mdl_context.release_transactional_locks();

  // thd_proc_info(thd, "wsrep applier committed");
  THD_STAGE_INFO(thd, stage_wsrep_committed);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: %s write set (%lld)",
           !ret ? "committed" : "failed to commit",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);


  if (!is_ordered) {
    m_thd->wsrep_cs().before_rollback();
    m_thd->wsrep_cs().after_rollback();
  } else if (m_thd->wsrep_trx().state() == wsrep::transaction::s_executing) {
    /*
      Wsrep commit was ordered but it did not go through commit time
      hooks and remains active. Cycle through commit hooks to release
      commit order and to make cleanup happen in after_applying() call.

      This is a workaround for CTAS with empty result set.
    */
    WSREP_DEBUG("Commit not finished for applier %u", thd->thread_id());
    ret = ret || m_thd->wsrep_cs().before_commit() ||
          m_thd->wsrep_cs().ordered_commit() ||
          m_thd->wsrep_cs().after_commit();
  }

  thd->lex->sql_command = SQLCOM_END;

  must_exit_ = check_exit_status();
  DBUG_RETURN(ret);
}

int Wsrep_high_priority_service::rollback(const wsrep::ws_handle &ws_handle,
                                          const wsrep::ws_meta &ws_meta) {
  DBUG_ENTER("Wsrep_high_priority_service::rollback");
  m_thd->wsrep_cs().prepare_for_ordering(ws_handle, ws_meta, false);

  THD_STAGE_INFO(m_thd, stage_wsrep_rolling_back);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: rolling back write set (%lld)",
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(m_thd, m_thd->wsrep_info);

#if 0
  int ret = (trans_rollback_stmt(m_thd) || trans_rollback(m_thd));
  // Following call to trans_rollback_stmt has been made conditional
  // based on presence of statement level transaction to comply
  // with PXC-5.7 behavior. trans_rollback_stmt functionality as defined
  // by upstream is to call rollback (MYSQL_BIN_LOG::rollback) even if
  // there is no statement level transaction to commit.
#endif
  int ret = (!m_thd->get_transaction()->is_empty(Transaction_ctx::STMT))
                ? (trans_rollback_stmt(m_thd) || trans_rollback(m_thd))
                : (trans_rollback(m_thd));

  if (ret == 0) {
    m_rli->cleanup_context(m_thd, 0);
    m_thd->variables.gtid_next.set_automatic();
  }

  THD_STAGE_INFO(m_thd, stage_wsrep_rolled_back);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: %s write set (%lld)",
           !ret ? "rolled back" : "failed to rollback",
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(m_thd, m_thd->wsrep_info);

  m_thd->mdl_context.release_transactional_locks();
  mysql_ull_cleanup(m_thd);
  m_thd->mdl_context.release_explicit_locks();
  DBUG_RETURN(ret);
}

int Wsrep_high_priority_service::apply_toi(const wsrep::ws_meta &ws_meta,
                                           const wsrep::const_buffer &data) {
  DBUG_ENTER("Wsrep_high_priority_service::apply_toi");
  THD *thd = m_thd;
  Wsrep_non_trans_mode non_trans_mode(thd, ws_meta);

  wsrep::client_state &client_state(thd->wsrep_cs());
  DBUG_ASSERT(client_state.in_toi());

  // thd_proc_info(thd, "wsrep applier toi");
  THD_STAGE_INFO(m_thd, stage_wsrep_applying_toi_writeset);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: applying TOI write-set (%lld)",
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(thd, m_thd->wsrep_info);

  WSREP_DEBUG("Wsrep_high_priority_service::apply_toi: %lld",
              client_state.toi_meta().seqno().get());

  /* DDL are atomic so flow (in wsrep_apply_events) will assign XID.
  Avoid over-writting of this XID by MySQL XID */
  thd->get_transaction()->xid_state()->get_xid()->set_keep_wsrep_xid(true);

  int ret = wsrep_apply_events(thd, m_rli, data.data(), data.size());
  if (ret != 0 || thd->wsrep_has_ignored_error) {
    wsrep_dump_rbr_buf(thd, data.data(), data.size());
    thd->wsrep_has_ignored_error = false;
    /* todo: error voting */
  }

  THD_STAGE_INFO(m_thd, stage_wsrep_applied_writeset);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: %s TOI write set (%lld)",
           !ret ? "applied" : "failed to apply",
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(thd, m_thd->wsrep_info);

  THD_STAGE_INFO(m_thd, stage_wsrep_toi_committing);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: committing TOI write set (%lld)",
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(thd, m_thd->wsrep_info);

  ret = trans_commit(thd);

  THD_STAGE_INFO(m_thd, stage_wsrep_committed);
  snprintf(m_thd->wsrep_info, sizeof(m_thd->wsrep_info),
           "wsrep: %s TOI write set (%lld)",
           (!ret ? "committed" : "failed to commit"),
           (long long)wsrep_thd_trx_seqno(m_thd));
  WSREP_DEBUG("%s", m_thd->wsrep_info);
  thd_proc_info(thd, m_thd->wsrep_info);

  if (ret == 0) {
    m_rli->cleanup_context(m_thd, 0);
    m_thd->variables.gtid_next.set_automatic();
  }

  TABLE *tmp;
  while ((tmp = thd->temporary_tables)) {
    WSREP_DEBUG("Applier %u, has temporary tables: %s.%s", thd->thread_id(),
                (tmp->s) ? tmp->s->db.str : "void",
                (tmp->s) ? tmp->s->table_name.str : "void");
    close_temporary_table(thd, tmp, true, true);
  }

  thd->lex->sql_command = SQLCOM_END;

  wsrep_set_SE_checkpoint(client_state.toi_meta().gtid());

  thd->get_transaction()->xid_state()->get_xid()->set_keep_wsrep_xid(false);
  /* Reset the xid once the transaction has been committed.
  This being TOI transaction it will not pass through wsrep_xxx hooks.
  Resetting is important to ensure that the applier xid state is restored
  so if node rejoins and applier thread is re-use for logging view or local
  activity like rolling back of SR transaction then stale state is not used. */
  thd->get_transaction()->xid_state()->get_xid()->reset();

  must_exit_ = check_exit_status();

  DBUG_RETURN(ret);
}

void Wsrep_high_priority_service::store_globals() {
  DBUG_ENTER("Wsrep_high_priority_service::store_globals");
  /* In addition to calling THD::store_globals(), call
     wsrep::client_state::store_globals() to gain ownership of
     the client state */
  m_thd->store_globals();
  m_thd->wsrep_cs().store_globals();
  DBUG_VOID_RETURN;
}

void Wsrep_high_priority_service::reset_globals() {
  DBUG_ENTER("Wsrep_high_priority_service::reset_globals");
  m_thd->restore_globals();
  DBUG_VOID_RETURN;
}

void Wsrep_high_priority_service::switch_execution_context(
    wsrep::high_priority_service &orig_high_priority_service) {
  DBUG_ENTER("Wsrep_high_priority_service::switch_execution_context");
  Wsrep_high_priority_service &orig_hps =
      static_cast<Wsrep_high_priority_service &>(orig_high_priority_service);
  m_thd->thread_stack = orig_hps.m_thd->thread_stack;
  DBUG_VOID_RETURN;
}

int Wsrep_high_priority_service::log_dummy_write_set(
    const wsrep::ws_handle &ws_handle, const wsrep::ws_meta &ws_meta) {
  DBUG_ENTER("Wsrep_high_priority_service::log_dummy_write_set");
  int ret = 0;
  DBUG_PRINT("info",
             ("Wsrep_high_priority_service::log_dummy_write_set: seqno=%lld",
              ws_meta.seqno().get()));
  m_thd->wsrep_cs().start_transaction(ws_handle, ws_meta);
  WSREP_DEBUG("Log dummy write set %lld", ws_meta.seqno().get());

  m_thd->wsrep_cs().before_rollback();
  m_thd->wsrep_cs().after_rollback();

#if 0
  // TODO: G-4
  if (!(opt_log_slave_updates && wsrep_gtid_mode &&
        m_thd->variables.gtid_seq_no)) {
    m_thd->wsrep_cs().before_rollback();
    m_thd->wsrep_cs().after_rollback();
  }
#endif
  m_thd->wsrep_cs().after_applying();
  DBUG_RETURN(ret);
}

void Wsrep_high_priority_service::debug_crash(const char *crash_point) {
  DBUG_ASSERT(m_thd == current_thd);
  DBUG_EXECUTE_IF(crash_point, DBUG_SUICIDE(););
}

/****************************************************************************
                           Applier service
*****************************************************************************/

Wsrep_applier_service::Wsrep_applier_service(THD *thd)
    : Wsrep_high_priority_service(thd) {
  thd->wsrep_applier_service = this;
  thd->wsrep_cs().open(wsrep::client_id(thd->thread_id()));
  thd->wsrep_cs().before_command();
  thd->wsrep_cs().debug_log_level(wsrep_debug);
}

Wsrep_applier_service::~Wsrep_applier_service() {
  m_thd->wsrep_cs().after_command_before_result();
  m_thd->wsrep_cs().after_command_after_result();
  m_thd->wsrep_cs().close();
  m_thd->wsrep_cs().cleanup();
  m_thd->wsrep_applier_service = NULL;
}

int Wsrep_applier_service::apply_write_set(const wsrep::ws_meta &ws_meta,
                                           const wsrep::const_buffer &data) {
  DBUG_ENTER("Wsrep_applier_service::apply_write_set");
  THD *thd = m_thd;

  thd->variables.option_bits |= OPTION_BEGIN;
  thd->variables.option_bits |= OPTION_NOT_AUTOCOMMIT;
  DBUG_ASSERT(thd->wsrep_trx().active());
  DBUG_ASSERT(thd->wsrep_trx().state() == wsrep::transaction::s_executing);

  // thd_proc_info(thd, "applying write set");
  THD_STAGE_INFO(thd, stage_wsrep_applying_writeset);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: applying write-set (%lld)",
           ws_meta.seqno().get());
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  /* moved dbug sync point here, after possible THD switch for SR transactions
     has ben done
  */
  /* Allow tests to block the applier thread using the DBUG facilities */
  DBUG_EXECUTE_IF("sync.wsrep_apply_cb", {
    const char act[] =
        "now "
        "SIGNAL sync.wsrep_apply_cb_reached "
        "WAIT_FOR signal.wsrep_apply_cb";
    DBUG_ASSERT(!debug_sync_set_action(thd, STRING_WITH_LEN(act)));
  };);

  wsrep_setup_uk_and_fk_checks(thd);

  int ret = wsrep_apply_events(thd, m_rli, data.data(), data.size());

  if (ret || thd->wsrep_has_ignored_error) {
    wsrep_dump_rbr_buf(thd, data.data(), data.size());
  }

  TABLE *tmp;
  while ((tmp = thd->temporary_tables)) {
    WSREP_DEBUG("Applier %u, has temporary tables: %s.%s", thd->thread_id(),
                (tmp->s) ? tmp->s->db.str : "void",
                (tmp->s) ? tmp->s->table_name.str : "void");
    close_temporary_table(thd, tmp, true, true);
  }

  if (!ret && !(ws_meta.flags() & wsrep::provider::flag::commit)) {
    thd->wsrep_cs().fragment_applied(ws_meta.seqno());
  }

  // thd_proc_info(thd, "wsrep applied write set");
  THD_STAGE_INFO(thd, stage_wsrep_applied_writeset);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: %s write set (%lld)", !ret ? "applied" : "failed to apply",
           ws_meta.seqno().get());
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  DBUG_RETURN(ret);
}

void Wsrep_applier_service::after_apply() {
  DBUG_ENTER("Wsrep_applier_service::after_apply");
  wsrep_after_apply(m_thd);
  DBUG_VOID_RETURN;
}

bool Wsrep_applier_service::check_exit_status() const {
  bool ret = false;
  mysql_mutex_lock(&LOCK_wsrep_slave_threads);
  if (wsrep_slave_count_change < 0) {
    ++wsrep_slave_count_change;
    ret = true;
  }
  mysql_mutex_unlock(&LOCK_wsrep_slave_threads);
  return ret;
}

/****************************************************************************
                           Replayer service
*****************************************************************************/

Wsrep_replayer_service::Wsrep_replayer_service(THD *replayer_thd, THD *orig_thd)
    : Wsrep_high_priority_service(replayer_thd),
      m_orig_thd(orig_thd),
      m_da_shadow(),
      m_replay_status() {
  /* Response must not have been sent to client */
  DBUG_ASSERT(!orig_thd->get_stmt_da()->is_sent());
  /* PS reprepare observer should have been removed already
     open_table() will fail if we have dangling observer here */
  DBUG_ASSERT(orig_thd->get_reprepare_observer() == NULL);
  /* Replaying should happen always from after_statement() hook
     after rollback, which should guarantee that there are no
     transactional locks */
  DBUG_ASSERT(!orig_thd->mdl_context.has_transactional_locks());

  /* Make a shadow copy of diagnostics area and reset */
  m_da_shadow.status = orig_thd->get_stmt_da()->status();
  if (m_da_shadow.status == Diagnostics_area::DA_OK) {
    m_da_shadow.affected_rows = orig_thd->get_stmt_da()->affected_rows();
    m_da_shadow.last_insert_id = orig_thd->get_stmt_da()->last_insert_id();
    strmake(m_da_shadow.message, orig_thd->get_stmt_da()->message_text(),
            sizeof(m_da_shadow.message) - 1);
  }
  orig_thd->get_stmt_da()->reset_diagnostics_area();

  /* Release explicit locks */
  if (orig_thd->locked_tables_mode && orig_thd->lock) {
    WSREP_WARN("releasing table lock for replaying (%u)",
               orig_thd->thread_id());
    orig_thd->locked_tables_list.unlock_locked_tables(orig_thd);
    orig_thd->variables.option_bits &= ~(OPTION_TABLE_LOCK);
  }

  // thd_proc_info(orig_thd, "wsrep replaying trx");
  THD_STAGE_INFO(orig_thd, stage_wsrep_replaying_trx);
  snprintf(orig_thd->wsrep_info, sizeof(orig_thd->wsrep_info),
           "wsrep: replaying transaction with write set (%lld)",
           (long long)wsrep_thd_trx_seqno(orig_thd));
  WSREP_DEBUG("%s", orig_thd->wsrep_info);
  thd_proc_info(orig_thd, orig_thd->wsrep_info);

  /*
    Swith execution context to replayer_thd and prepare it for
    replay execution.
  */
  orig_thd->restore_globals();
  replayer_thd->store_globals();
  replayer_thd->wsrep_replayer = true;
  wsrep_open(replayer_thd);
  wsrep_before_command(replayer_thd);
  replayer_thd->wsrep_cs().clone_transaction_for_replay(orig_thd->wsrep_trx());
}

Wsrep_replayer_service::~Wsrep_replayer_service() {
  THD *replayer_thd = m_thd;
  THD *orig_thd = m_orig_thd;

  /* Store replay result/state to original thread wsrep client
     state and switch execution context back to original. */
  if (m_replay_status == wsrep::provider::error_certification_failed) {
    /* Replay of transaction failed at certification level.
       This will leave transaction in s_replaying mode.
       De-attach the transaction from original transaction and proceed. */
    replayer_thd->wsrep_cs().deattach_after_replay();
    wsrep_after_command_ignore_result(replayer_thd);
    wsrep_close(replayer_thd);
  } else {
    orig_thd->wsrep_cs().after_replay(replayer_thd->wsrep_trx());
    wsrep_after_apply(replayer_thd);
    wsrep_after_command_ignore_result(replayer_thd);
    wsrep_close(replayer_thd);
  }
  replayer_thd->wsrep_replayer = false;
  replayer_thd->restore_globals();
  orig_thd->store_globals();

  DBUG_ASSERT(!orig_thd->get_stmt_da()->is_sent());
  DBUG_ASSERT(!orig_thd->get_stmt_da()->is_set());

  if (m_replay_status == wsrep::provider::success) {
    DBUG_ASSERT(replayer_thd->wsrep_cs().current_error() == wsrep::e_success);
    orig_thd->killed = THD::NOT_KILLED;
    my_ok(orig_thd, m_da_shadow.affected_rows, m_da_shadow.last_insert_id);
  } else if (m_replay_status == wsrep::provider::error_certification_failed) {
    wsrep_override_error(orig_thd, ER_LOCK_DEADLOCK);
  } else {
    DBUG_ASSERT(0);
    WSREP_ERROR("trx_replay failed for: %d, schema: %s, query: %s",
                m_replay_status, orig_thd->db().str, WSREP_QUERY(orig_thd));
    unireg_abort(1);
  }
}

int Wsrep_replayer_service::apply_write_set(const wsrep::ws_meta &ws_meta,
                                            const wsrep::const_buffer &data) {
  DBUG_ENTER("Wsrep_replayer_service::apply_write_set");
  THD *thd = m_thd;

  DBUG_ASSERT(thd->wsrep_trx().active());
  DBUG_ASSERT(thd->wsrep_trx().state() == wsrep::transaction::s_replaying);

  wsrep_setup_uk_and_fk_checks(thd);

  int ret = 0;
  if (!wsrep::starts_transaction(ws_meta.flags())) {
    DBUG_ASSERT(thd->wsrep_trx().is_streaming());
    ret = wsrep_schema->replay_transaction(thd, m_rli, ws_meta,
                                           thd->wsrep_sr().fragments());
  }

  ret = ret || wsrep_apply_events(thd, m_rli, data.data(), data.size());

  if (ret || thd->wsrep_has_ignored_error) {
    wsrep_dump_rbr_buf(thd, data.data(), data.size());
  }


  TABLE *tmp;
  while ((tmp = thd->temporary_tables)) {
    WSREP_DEBUG("Applier %u, has temporary tables: %s.%s", thd->thread_id(),
                (tmp->s) ? tmp->s->db.str : "void",
                (tmp->s) ? tmp->s->table_name.str : "void");
    close_temporary_table(thd, tmp, true, true);
  }

  if (!ret && !(ws_meta.flags() & wsrep::provider::flag::commit)) {
    thd->wsrep_cs().fragment_applied(ws_meta.seqno());
  }

  // thd_proc_info(thd, "wsrep replayed write set");
  THD_STAGE_INFO(thd, stage_wsrep_replayed_write_set);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: replayed write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  DBUG_RETURN(ret);
}
