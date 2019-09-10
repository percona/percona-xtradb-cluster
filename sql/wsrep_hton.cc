/* Copyright 2008-2015 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "mysql/plugin.h"
#include "mysqld.h"
#include "sql/sql_lex.h"
#include "sql_class.h"

#include <cstdio>
#include <cstdlib>
#include "binlog.h"
#include "debug_sync.h"
#include "rpl_filter.h"
#include "sql_base.h"
#include "wsrep_binlog.h"
#include "wsrep_mysqld.h"
#include "mysql/components/services/log_builtins.h"
#include "wsrep_xid.h"
#include "sql/log.h"

extern "C" int thd_binlog_format(const MYSQL_THD thd);

enum wsrep_trx_status wsrep_run_wsrep_commit(THD *thd, handlerton *hton,
                                             bool all);

/*
  Cleanup after local transaction commit/rollback, replay or TOI.
*/
void wsrep_cleanup_transaction(THD *thd) {
  if (!WSREP(thd)) return;

  // Enabling it can cause password to appear in log.
  // WSREP_DEBUG("wsrep_cleanup_transaction %s", thd->query().str);
  DBUG_ASSERT(thd->wsrep_conflict_state != MUST_REPLAY &&
              thd->wsrep_conflict_state != REPLAYING);

  if (thd->mdl_context.has_transactional_locks()) {
    /* If thd has not yet released the locks then this thd
    can conflict with background running applier thread or parallel
    local running TOI thread that can cause this thd to get marked
    with state = MUST_ABORT. Since existing query is already complete
    this MUST_ABORT state can have 2 side effects:
    - Can cause next query to get processed with MUST_ABORT
    - Can cause retry of existing successfully completed query.
    */
    thd->wsrep_safe_to_abort = false;
  }

  if (wsrep_emulate_bin_log) wsrep_thd_binlog_trx_reset(thd);
  thd->wsrep_ws_handle.trx_id = WSREP_UNDEFINED_TRX_ID;
  thd->wsrep_trx_meta.gtid = WSREP_GTID_UNDEFINED;
  thd->set_wsrep_next_trx_id(WSREP_UNDEFINED_TRX_ID);
  thd->wsrep_trx_meta.depends_on = WSREP_SEQNO_UNDEFINED;
  thd->wsrep_exec_mode = LOCAL_STATE;
  thd->wsrep_affected_rows = 0;
  thd->wsrep_void_applier_trx = true;
  thd->wsrep_skip_wsrep_GTID = false;
  thd->wsrep_skip_SE_checkpoint = false;

  if (thd->wsrep_non_replicating_atomic_ddl) {
    /* Restore the sql_log_bin mode back to original value
    on completion of the non-replicating atomic DDL statement. */
    if (thd->variables.wsrep_saved_binlog_state ==
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_ENABLED) {
      thd->variables.option_bits |= OPTION_BIN_LOG;
    } else {
      thd->variables.option_bits &= ~OPTION_BIN_LOG;
    }

    thd->variables.wsrep_saved_binlog_state =
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_NOTSET;
  }
  thd->wsrep_non_replicating_atomic_ddl = false;

  return;
}

/*
  wsrep hton
*/
handlerton *wsrep_hton;

/*
  Registers wsrep hton at commit time if transaction has registered htons
  for supported engine types.

  Hton should not be registered for TOTAL_ORDER operations.

  Registration is needed for both LOCAL_MODE and REPL_RECV transactions to run
  commit in 2pc so that wsrep position gets properly recorded in storage
  engines.

  Note that all hton calls should immediately return for threads that are
  in REPL_RECV mode as their states are controlled by wsrep appliers or
  replaying code. Only threads in LOCAL_MODE should run wsrep callbacks
  from hton methods.
*/
void wsrep_register_hton(THD *thd, bool all) {
  if (!WSREP(thd)) return;

  /* Skip wsrep-hton handler registration.
  This happens in case like following:
  - DDL is executed with sql_log_bin = 0 where-in ddl replication is skipped
    so the exec_mode != TOTAL_ORDER but as thumb rule hton should get registered
    for DDL/TOI statement.
  */
  if (WSREP(thd) && thd->wsrep_skip_wsrep_hton) return;

  /* only LOCAL_STATE processors may replicate.
     For filtered mysql replication we may end up here in LOCAL_COMMIT state
     this can happen after GTID event replication for filtered event

  */
  if (thd->wsrep_exec_mode == LOCAL_COMMIT) {
    return;
  }
  if (thd->wsrep_exec_mode != TOTAL_ORDER && !thd->wsrep_apply_toi) {
    if (thd->wsrep_exec_mode == LOCAL_STATE &&
        (thd_sql_command(thd) == SQLCOM_OPTIMIZE ||
         thd_sql_command(thd) == SQLCOM_ANALYZE ||
         thd_sql_command(thd) == SQLCOM_REPAIR) &&
        thd->lex->no_write_to_binlog == 1) {
      WSREP_DEBUG("Skip registering wsrep for LOCAL SQL ADMIN command: %s",
                  WSREP_QUERY(thd));
      return;
    }

    Transaction_ctx *trn_ctx = thd->get_transaction();
    Transaction_ctx::enum_trx_scope trx_scope =
        all ? Transaction_ctx::SESSION : Transaction_ctx::STMT;

    Ha_trx_info *ha_info = trn_ctx->ha_trx_info(trx_scope), *ha_info_next;
    if (ha_info) {
      for (; ha_info; ha_info = ha_info_next) {
        if (ha_info->ht()->db_type == DB_TYPE_INNODB) {
          trans_register_ha(thd, all, wsrep_hton, NULL);

          /* follow innodb read/write settting
           */
          if (ha_info->is_trx_read_write()) {
            thd->get_ha_data(wsrep_hton->slot)
                ->ha_info[all]
                .set_trx_read_write();
          }
          break;
        }
        ha_info_next = ha_info->next();
      }
    }
  }
}

/*
  Calls wsrep->interim_commit() for given transactions that have
  got seqno from provider (must commit) and don't require replaying.
 */
extern bool opt_log_slave_updates;
void wsrep_interim_commit(THD *thd) {
  if (!WSREP(thd)) return;

  /* Interim Commit Optimization can be used only if log_slave_updates is
  ON that ensures all slave thread (including pxc replication threads)
  binlogs the events there-by following group commit protocol.
  Even if one thread doesn't follow group commit protocol interim
  commit optimization will not work.
  Interim commit optimization rely on ordered commit of MySQL.
  If this feature is turned-off then skip this optimization. */
  if (!opt_log_slave_updates || !opt_binlog_order_commits) return;

  switch (thd->wsrep_exec_mode) {
    case LOCAL_COMMIT: {
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);
      if (wsrep->interim_commit(wsrep, &thd->wsrep_ws_handle)) {
        DBUG_PRINT("wsrep", ("set committed fail"));
        WSREP_WARN("set committed fail: %llu %d", (long long)thd->real_id,
                   thd->get_stmt_da()->status());
      }
      break;
    }
    case LOCAL_STATE:
      break;
    case REPL_RECV: {
      /* There are cases when replicated transaction are empty and there is no
      need to commit such transaction but given that they have a valid seqno
      assigned we need to update commit monitor by enter and leaving it.
      Empty transaction use case: a. CTAS b. pre-executed action on pxc-node
      acting as slave to an independent master. */
      if (thd->wsrep_void_applier_trx &&
          thd->wsrep_conflict_state != REPLAYING &&
          thd->wsrep_ws_handle.opaque && thd->wsrep_applier)
        wsrep->applier_pre_commit(wsrep, thd->wsrep_ws_handle.opaque);

      if (thd->wsrep_ws_handle.opaque &&
          thd->wsrep_conflict_state != REPLAYING) {
        /* Interim-commit hook will stop commit ordering. */
        wsrep->applier_interim_commit(wsrep, thd->wsrep_ws_handle.opaque);
        thd->wsrep_ws_handle.opaque = 0;
      }

      break;
    }
    default:
      break;
  }
}

/*
  Calls wsrep->post_commit() for given transactions that have
  got seqno from provider (must commit) and don't require replaying.
 */
void wsrep_post_commit(THD *thd, bool all) {
  if (!WSREP(thd)) return;

  switch (thd->wsrep_exec_mode) {
    case LOCAL_COMMIT: {
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);
      if (wsrep->post_commit(wsrep, &thd->wsrep_ws_handle)) {
        DBUG_PRINT("wsrep", ("set committed fail"));
        WSREP_WARN("Post-Commit action failed: %llu %d",
                   (long long)thd->real_id, thd->get_stmt_da()->status());
      }
      wsrep_cleanup_transaction(thd);
      break;
    }
    case LOCAL_STATE: {
      /* non-InnoDB statements may have populated events in stmt cache
         => cleanup
      */
      WSREP_DEBUG("Cleaning up wsrep-transaction for local query: %s",
                  WSREP_QUERY(thd));
      /*
        Run post-rollback hook to clean up in the case if
        some keys were populated for the transaction in provider
        but during commit time there was no write set to replicate.
        This may happen when client sets the SAVEPOINT and immediately
        rolls back to savepoint after first operation.
       */
      if (all && thd->wsrep_conflict_state != MUST_REPLAY &&
          thd->wsrep_conflict_state != REPLAYING &&
          wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle)) {
        WSREP_WARN("post_rollback fail: %llu %d", (long long)thd->thread_id(),
                   thd->get_stmt_da()->status());
      }

      if (thd->wsrep_conflict_state != MUST_REPLAY &&
          thd->wsrep_conflict_state != REPLAYING) {
        wsrep_cleanup_transaction(thd);
      }
      break;
    }
    case REPL_RECV: {
      /* There are cases when replicated transaction are empty and there is no
      need to commit such transaction but given that they have a valid seqno
      assigned we need to update commit monitor by enter and leaving it.
      Empty transaction use case: a. CTAS b. pre-executed action on pxc-node
      acting as slave to an independent master. */
      if (thd->wsrep_void_applier_trx &&
          thd->wsrep_conflict_state != REPLAYING &&
          thd->wsrep_ws_handle.opaque && thd->wsrep_applier)
        wsrep->applier_pre_commit(wsrep, thd->wsrep_ws_handle.opaque);

      if (thd->wsrep_ws_handle.opaque &&
          thd->wsrep_conflict_state != REPLAYING) {
        // WSREP_DEBUG("Committed repl-transaction with global-seqno-no: %lu",
        //            thd->wsrep_trx_meta.gtid.seqno);
        /* Post-commit hook will stop commit ordering. */
        wsrep->applier_post_commit(wsrep, thd->wsrep_ws_handle.opaque);
        thd->wsrep_ws_handle.opaque = 0;
      }

      thd->wsrep_void_applier_trx = true;
      break;
    }
    default:
      break;
  }
}

/*
  wsrep exploits binlog's caches even if binlogging itself is not
  activated. In such case connection close needs calling
  actual binlog's method.
  Todo: split binlog hton from its caches to use ones by wsrep
  without referring to binlog's stuff.
*/
static int wsrep_close_connection(handlerton *, THD *thd) {
  DBUG_ENTER("wsrep_close_connection");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    DBUG_RETURN(0);
  }
  DBUG_RETURN(wsrep_binlog_close_connection(thd));
}

/*
  prepare/wsrep_run_wsrep_commit can fail in two ways
  - certification test or an equivalent. As a result,
    the current transaction just rolls back
    Error codes:
           WSREP_TRX_CERT_FAIL, WSREP_TRX_SIZE_EXCEEDED, WSREP_TRX_ERROR
  - a post-certification failure makes this server unable to
    commit its own WS and therefore the server must abort
*/
static int wsrep_prepare(handlerton *, THD *thd, bool all) {
  DBUG_ENTER("wsrep_prepare");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    thd->wsrep_void_applier_trx = false;
    DBUG_RETURN(0);
  }

  DBUG_ASSERT(
      thd->get_ha_data(wsrep_hton->slot)->ha_info[all].is_trx_read_write());
  DBUG_ASSERT(thd->wsrep_exec_mode == LOCAL_STATE);
  DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno == WSREP_SEQNO_UNDEFINED);

  /* all could be mis-leading but just imagine it to be indicate as follows:
  all (true): explicit commit (BEGIN .. COMMIT)
  all (false): statement is part of multi-statement transaction.

  If statement is not part of multi-statement transaction (autocommit = 1)
  then second OR condition evaluate to true even though all = false as
  commit is being driven by trans_commit_stmt. */
  if (((all || !thd_test_options(thd, OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN)) &&
       (thd->variables.wsrep_on && !wsrep_trans_cache_is_empty(thd)))) {
    DBUG_RETURN(wsrep_replicate(thd));
  }
  DBUG_RETURN(0);
}

static int wsrep_savepoint_set(handlerton *, THD *thd, void *sv) {
  DBUG_ENTER("wsrep_savepoint_set");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    DBUG_RETURN(0);
  }

  if (!wsrep_emulate_bin_log) DBUG_RETURN(0);
  int rcode = wsrep_binlog_savepoint_set(thd, sv);
  DBUG_RETURN(rcode);
}

static int wsrep_savepoint_rollback(handlerton *, THD *thd, void *sv) {
  DBUG_ENTER("wsrep_savepoint_rollback");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    DBUG_RETURN(0);
  }

  if (!wsrep_emulate_bin_log) DBUG_RETURN(0);
  int rcode = wsrep_binlog_savepoint_rollback(thd, sv);
  DBUG_RETURN(rcode);
}

static int wsrep_rollback(handlerton *, THD *thd, bool all) {
  DBUG_ENTER("wsrep_rollback");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    DBUG_RETURN(0);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  switch (thd->wsrep_exec_mode) {
    case TOTAL_ORDER:
    case REPL_RECV:
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      WSREP_DEBUG("Avoiding wsrep rollback for failed DDL: %s",
                  WSREP_QUERY(thd));
      DBUG_RETURN(0);
    default:
      break;
  }

  if ((all || !thd_test_options(thd, OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN)) &&
      (thd->variables.wsrep_on && thd->wsrep_conflict_state != MUST_REPLAY)) {
    if (wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle)) {
      DBUG_PRINT("wsrep", ("setting rollback fail"));
      WSREP_ERROR(
          "Failed to executed wsrep-rollback"
          " (thd: %llu, schema: %s, SQL: %s)",
          (long long)thd->real_id, (thd->db().str ? thd->db().str : "(null)"),
          WSREP_QUERY(thd));
    }
    wsrep_cleanup_transaction(thd);
  }
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  DBUG_RETURN(0);
}

int wsrep_commit(handlerton *, THD *thd, bool all) {
  DBUG_ENTER("wsrep_commit");

  if (thd->wsrep_exec_mode == REPL_RECV) {
    DBUG_RETURN(0);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  if ((all || !thd_test_options(thd, OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN)) &&
      (thd->variables.wsrep_on && thd->wsrep_conflict_state != MUST_REPLAY)) {
    if (thd->wsrep_exec_mode == LOCAL_COMMIT) {
      DBUG_ASSERT(
          thd->get_ha_data(wsrep_hton->slot)->ha_info[all].is_trx_read_write());
      /*
        Call to wsrep->post_commit() (moved to wsrep_post_commit()) must
        be done only after commit has done for all involved htons.
      */
      DBUG_PRINT("wsrep", ("commit"));
      // WSREP_DEBUG("Committed local-transaction with global-seqno-no: %lu",
      //            thd->wsrep_trx_meta.gtid.seqno);
    } else {
      /*
        Transaction didn't go through wsrep->pre_commit() so just roll back
        possible changes to clean state.
      */
      if (wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle)) {
        DBUG_PRINT("wsrep", ("setting rollback fail"));
        WSREP_ERROR(
            "Failed to execute wsrep-rollback"
            " (thd: %llu, schema: %s, SQL: %s)",
            (long long)thd->real_id, (thd->db().str ? thd->db().str : "(null)"),
            WSREP_QUERY(thd));
      }
      wsrep_cleanup_transaction(thd);
    }
  }
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  DBUG_RETURN(0);
}

extern Rpl_filter *binlog_filter;
extern bool opt_log_slave_updates;

enum wsrep_trx_status wsrep_run_wsrep_commit(THD *thd, handlerton *, bool) {
  int rcode = -1;
  size_t data_len = 0;
  IO_CACHE_binlog_cache_storage *cache;
  int replay_round = 0;

  if (thd->get_stmt_da()->is_error()) {
    WSREP_DEBUG("commit issue, error: %d %s", thd->get_stmt_da()->mysql_errno(),
                thd->get_stmt_da()->message_text());
  }

  DBUG_ENTER("wsrep_run_wsrep_commit");
  DEBUG_SYNC(thd, "wsrep_before_replication");

  if (thd->slave_thread && !opt_log_slave_updates) DBUG_RETURN(WSREP_TRX_OK);

  if (thd->wsrep_exec_mode == REPL_RECV) {
    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    if (thd->wsrep_conflict_state == MUST_ABORT) {
      WSREP_DEBUG("WSREP: must abort for BF");

      DBUG_PRINT("wsrep", ("BF apply commit fail"));
      thd->wsrep_conflict_state = NO_CONFLICT;
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      //
      // TODO: test all calls of the rollback.
      // rollback must happen automagically innobase_rollback(hton, thd, 1);
      //
      DBUG_RETURN(WSREP_TRX_ERROR);
    }
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }

  if (thd->wsrep_exec_mode != LOCAL_STATE) DBUG_RETURN(WSREP_TRX_OK);

  if (thd->wsrep_consistency_check == CONSISTENCY_CHECK_RUNNING) {
    WSREP_DEBUG("commit for consistency check: %s", WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_OK);
  }

  THD_STAGE_INFO(thd, stage_wsrep_replicating_commit);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: replicating commit (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  if (thd->wsrep_conflict_state == MUST_ABORT) {
    DBUG_PRINT("wsrep", ("replicate commit fail"));
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    WSREP_DEBUG("innobase_commit, abort %s", WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_CERT_FAIL);
  }

  mysql_mutex_lock(&LOCK_wsrep_replaying);

  while (wsrep_replaying > 0 && thd->wsrep_conflict_state == NO_CONFLICT &&
         thd->killed == THD::NOT_KILLED && !connection_events_loop_aborted()) {
    mysql_mutex_unlock(&LOCK_wsrep_replaying);
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

    THD_STAGE_INFO(thd, stage_wsrep_waiting_on_replaying);
    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
             "wsrep: waiting to replay write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex = &LOCK_wsrep_replaying;
    thd->current_cond = &COND_wsrep_replaying;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    mysql_mutex_lock(&LOCK_wsrep_replaying);
    // Using timedwait is a hack to avoid deadlock in case if BF victim
    // misses the signal.
    struct timespec wtime;
    clock_gettime(CLOCK_REALTIME, &wtime);
    long prev_nsec = wtime.tv_nsec;
    wtime.tv_nsec = (wtime.tv_nsec + 1000000) % 1000000000;
    // If nsecs rolled over, increment seconds.
    wtime.tv_sec += (wtime.tv_nsec < prev_nsec ? 1 : 0);
    mysql_cond_timedwait(&COND_wsrep_replaying, &LOCK_wsrep_replaying, &wtime);

    if (replay_round++ % 100000 == 0)
      WSREP_DEBUG(
          "commit waiting for replaying: replayers %d, thd: (%u) "
          "conflict: %s (round: %d)",
          wsrep_replaying, thd->thread_id(),
          wsrep_get_conflict_state(thd->wsrep_conflict_state), replay_round);

    mysql_mutex_unlock(&LOCK_wsrep_replaying);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex = 0;
    thd->current_cond = 0;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    mysql_mutex_lock(&LOCK_wsrep_replaying);
  }
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  if (thd->wsrep_conflict_state == MUST_ABORT) {
    DBUG_PRINT("wsrep", ("replicate commit fail"));
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    WSREP_DEBUG("innobase_commit abort after replaying wait %s",
                WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_CERT_FAIL);
  }

  thd->wsrep_query_state = QUERY_COMMITTING;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  rcode = 0;

  cache = wsrep_get_trans_cache(thd, true);
  if (cache) {
    thd->binlog_flush_pending_rows_event(true);
    rcode = wsrep_write_cache(wsrep, thd, cache, &data_len);
    if (WSREP_OK != rcode) {
      WSREP_ERROR(
          "Failed to create write-set from binlog"
          " data_len: %zu, %d",
          data_len, rcode);
      DBUG_RETURN(WSREP_TRX_SIZE_EXCEEDED);
    }
  }

  if (data_len == 0) {
    if (thd->get_stmt_da()->is_ok() &&
        thd->get_stmt_da()->affected_rows() > 0 && !binlog_filter->is_on()) {
      WSREP_DEBUG(
          "empty rbr buffer, query: %s, "
          "affected rows: %llu, "
          "changed tables: %d, "
          "sql_log_bin: %d, "
          "wsrep status (%d %d %d)",
          WSREP_QUERY(thd), thd->get_stmt_da()->affected_rows(),
          stmt_has_updated_trans_table(
              thd->get_transaction()->ha_trx_info(Transaction_ctx::STMT)),
          thd->variables.sql_log_bin, thd->wsrep_exec_mode,
          thd->wsrep_query_state, thd->wsrep_conflict_state);
    } else {
      WSREP_DEBUG("empty rbr buffer, query: %s", WSREP_QUERY(thd));
    }
    thd->wsrep_query_state = QUERY_EXEC;
    DBUG_RETURN(WSREP_TRX_OK);
  }

  if (WSREP_UNDEFINED_TRX_ID == thd->wsrep_ws_handle.trx_id &&
      !thd->wsrep_replicate_GTID) {
    WSREP_WARN("SQL statement (%s) was not replicated (thd: %u)",
               WSREP_QUERY(thd), thd->thread_id());
    rcode = WSREP_TRX_FAIL;
  } else if (!rcode) {
    if (WSREP_OK == rcode) {
      if (wsrep_debug) {
        THD_STAGE_INFO(thd, stage_wsrep_pre_commit);
        snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
                 "wsrep: initiating pre-commit for write set (%lld)",
                 (long long)wsrep_thd_trx_seqno(thd));
        WSREP_DEBUG("%s", thd->wsrep_info);
        thd_proc_info(thd, thd->wsrep_info);
      }

      rcode = wsrep->replicate_pre_commit(
          wsrep, (wsrep_conn_id_t)thd->thread_id(), &thd->wsrep_ws_handle,
          WSREP_FLAG_COMMIT |
              ((thd->wsrep_PA_safe) ? 0ULL : WSREP_FLAG_PA_UNSAFE),
          &thd->wsrep_trx_meta);
    }

    if (rcode == WSREP_TRX_MISSING) {
      WSREP_WARN(
          "Unable to locate transaction handle in provider"
          " thd: %u, schema: %s, SQL: %s",
          thd->thread_id(), (thd->db().str ? thd->db().str : "(null)"),
          WSREP_QUERY(thd));
      rcode = WSREP_TRX_FAIL;
    } else if (rcode == WSREP_BF_ABORT) {
      WSREP_DEBUG("THD: %u, seqno %lld was BF aborted by provider, will replay",
                  thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno);
      mysql_mutex_lock(&thd->LOCK_wsrep_thd);
      thd->wsrep_conflict_state = MUST_REPLAY;
      DBUG_ASSERT(wsrep_thd_trx_seqno(thd) > 0);
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      mysql_mutex_lock(&LOCK_wsrep_replaying);
      wsrep_replaying++;
      WSREP_DEBUG("replaying increased: %d, thd: %u", wsrep_replaying,
                  thd->thread_id());
      mysql_mutex_unlock(&LOCK_wsrep_replaying);
    }
  } else {
    WSREP_ERROR("I/O error reading from thd's binlog iocache (errno=%d)",
                my_errno());
    DBUG_ASSERT(0);  // failure like this can not normally happen
    DBUG_RETURN(WSREP_TRX_ERROR);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  switch (rcode) {
    case 0:
      /*
        About MUST_ABORT: We assume that even if thd conflict state was set
        to MUST_ABORT, underlying transaction was not rolled back or marked
        as deadlock victim in QUERY_COMMITTING state. Conflict state is
        set to NO_CONFLICT and commit proceeds as usual.
      */
      if (thd->wsrep_conflict_state == MUST_ABORT) {
        thd->killed = THD::NOT_KILLED;
        thd->wsrep_conflict_state = NO_CONFLICT;
      }

      if (thd->wsrep_conflict_state != NO_CONFLICT) {
        WSREP_WARN("thd %u seqno %lld: conflict state %s after post commit",
                   thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno,
                   wsrep_get_conflict_state(thd->wsrep_conflict_state));
      }
      thd->wsrep_exec_mode = LOCAL_COMMIT;
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);
      /* Override XID iff it was generated by mysql */
      if (thd->get_transaction()->xid_state()->get_xid()->get_my_xid()) {
        wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                       thd->wsrep_trx_meta.gtid.uuid,
                       thd->wsrep_trx_meta.gtid.seqno);
      }

      THD_STAGE_INFO(thd, stage_wsrep_pre_commit_cert_passed);
      snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
               "wsrep: pre-commit/certification passed (%lld)",
               (long long)wsrep_thd_trx_seqno(thd));
      WSREP_DEBUG("%s", thd->wsrep_info);
      thd_proc_info(thd, thd->wsrep_info);

      DBUG_PRINT("wsrep", ("replicating commit success"));
      break;

    case WSREP_BF_ABORT:
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);

    case WSREP_TRX_FAIL:
      WSREP_DEBUG("commit action failed for reason: %s THD: %u Query: %s",
                  wsrep_get_wsrep_status((wsrep_status)rcode), thd->thread_id(),
                  WSREP_QUERY(thd));
      DBUG_PRINT("wsrep", ("replicating commit fail"));

      thd->wsrep_query_state = QUERY_EXEC;

      if (thd->wsrep_conflict_state == MUST_ABORT) {
        thd->wsrep_conflict_state = ABORTED;
      } else {
        WSREP_DEBUG("conflict state: %s",
                    wsrep_get_conflict_state(thd->wsrep_conflict_state));
        if (thd->wsrep_conflict_state == NO_CONFLICT) {
          thd->wsrep_conflict_state = CERT_FAILURE;
          WSREP_LOG_CONFLICT(NULL, thd, false);
        }
      }
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

      DBUG_RETURN(WSREP_TRX_CERT_FAIL);

    case WSREP_SIZE_EXCEEDED:
      WSREP_ERROR("Transaction size exceed set threshold.");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_SIZE_EXCEEDED);

    case WSREP_CONN_FAIL:
      WSREP_ERROR("Connection Failure.");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);

    default:
      WSREP_ERROR("Unknown connection failure");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);
  }

  thd->wsrep_query_state = QUERY_EXEC;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_RETURN(WSREP_TRX_OK);
}

enum wsrep_trx_status wsrep_replicate(THD *thd) {
  int rcode = -1;
  size_t data_len = 0;
  IO_CACHE_binlog_cache_storage *cache;
  int replay_round = 0;

  if (thd->get_stmt_da()->is_error()) {
    WSREP_DEBUG(
        "Error while initiating replication of write-set"
        " (code: %d, message: %s)",
        thd->get_stmt_da()->mysql_errno(), thd->get_stmt_da()->message_text());
  }

  DBUG_ENTER("wsrep_replicate");
  DEBUG_SYNC(thd, "wsrep_before_replication");

  /* Avoid replication of write-set through MySQL slave thread if
  log_slave_updates is not configured. */
  if (thd->slave_thread && !opt_log_slave_updates) DBUG_RETURN(WSREP_TRX_OK);

  /* wsrep_replicate is not meant to handle non-replicating DDL. */
  if (thd->wsrep_non_replicating_atomic_ddl) {
    DBUG_RETURN(WSREP_TRX_OK);
  }

  if (thd->wsrep_exec_mode == REPL_RECV) {
    mysql_mutex_lock(&thd->LOCK_wsrep_thd);

    if (thd->wsrep_conflict_state == MUST_ABORT) {
      WSREP_DEBUG("WSREP: Applier thread scheduled for forceful abort");
      thd->wsrep_conflict_state = NO_CONFLICT;
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);
    }
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }

  if (thd->wsrep_exec_mode != LOCAL_STATE) DBUG_RETURN(WSREP_TRX_OK);

  if (thd->wsrep_consistency_check == CONSISTENCY_CHECK_RUNNING) {
    WSREP_DEBUG("commit for consistency check: %s", WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_OK);
  }

  THD_STAGE_INFO(thd, stage_wsrep_replicating_commit);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "wsrep: replicating commit (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  if (thd->wsrep_conflict_state == MUST_ABORT) {
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    WSREP_DEBUG(
        "Forceful abort scheduled while initiate replication"
        " replication of write-set (%s)",
        WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_CERT_FAIL);
  }

  mysql_mutex_lock(&LOCK_wsrep_replaying);

  while (wsrep_replaying > 0 && thd->wsrep_conflict_state == NO_CONFLICT &&
         thd->killed == THD::NOT_KILLED && !connection_events_loop_aborted()) {
    mysql_mutex_unlock(&LOCK_wsrep_replaying);
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

    THD_STAGE_INFO(thd, stage_wsrep_waiting_on_replaying);
    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
             "wsrep: waiting to replay write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex = &LOCK_wsrep_replaying;
    thd->current_cond = &COND_wsrep_replaying;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    mysql_mutex_lock(&LOCK_wsrep_replaying);
    // Using timedwait is a hack to avoid deadlock in case if BF victim
    // misses the signal.
    struct timespec wtime;
    clock_gettime(CLOCK_REALTIME, &wtime);
    long prev_nsec = wtime.tv_nsec;
    wtime.tv_nsec = (wtime.tv_nsec + 1000000) % 1000000000;
    // If nsecs rolled over, increment seconds.
    wtime.tv_sec += (wtime.tv_nsec < prev_nsec ? 1 : 0);
    mysql_cond_timedwait(&COND_wsrep_replaying, &LOCK_wsrep_replaying, &wtime);

    if (replay_round++ % 100000 == 0)
      WSREP_DEBUG(
          "commit waiting for replaying: replayers %d, thd: (%u) "
          "conflict: %d (round: %d)",
          wsrep_replaying, thd->thread_id(), thd->wsrep_conflict_state,
          replay_round);

    mysql_mutex_unlock(&LOCK_wsrep_replaying);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex = 0;
    thd->current_cond = 0;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    mysql_mutex_lock(&LOCK_wsrep_replaying);
  }
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  if (thd->wsrep_conflict_state == MUST_ABORT) {
    DBUG_PRINT("wsrep", ("replicate commit fail"));
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    WSREP_DEBUG("innobase_commit abort after replaying wait %s",
                WSREP_QUERY(thd));
    DBUG_RETURN(WSREP_TRX_CERT_FAIL);
  }

  thd->wsrep_query_state = QUERY_COMMITTING;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  rcode = 0;
  cache = wsrep_get_trans_cache(thd, true);
  if (cache) {
    thd->binlog_flush_pending_rows_event(true);
    rcode = wsrep_write_cache(wsrep, thd, cache, &data_len);
    if (WSREP_OK != rcode) {
      WSREP_ERROR(
          "Failed to create write-set from binlog"
          " data_len: %zu, %d",
          data_len, rcode);
      DBUG_RETURN(WSREP_TRX_SIZE_EXCEEDED);
    }
  }

  if (data_len == 0) {
    if (thd->get_stmt_da()->is_ok() &&
        thd->get_stmt_da()->affected_rows() > 0 && !binlog_filter->is_on()) {
      WSREP_DEBUG(
          "empty rbr buffer, query: %s, "
          "affected rows: %llu, "
          "changed tables: %d, "
          "sql_log_bin: %d, "
          "wsrep status (%d %d %d)",
          WSREP_QUERY(thd), thd->get_stmt_da()->affected_rows(),
          stmt_has_updated_trans_table(
              thd->get_transaction()->ha_trx_info(Transaction_ctx::STMT)),
          thd->variables.sql_log_bin, thd->wsrep_exec_mode,
          thd->wsrep_query_state, thd->wsrep_conflict_state);
    } else {
      WSREP_DEBUG("empty rbr buffer, query: %s", WSREP_QUERY(thd));
    }
    thd->wsrep_query_state = QUERY_EXEC;
    DBUG_RETURN(WSREP_TRX_OK);
  }

  if (WSREP_UNDEFINED_TRX_ID == thd->wsrep_ws_handle.trx_id &&
      !thd->wsrep_replicate_GTID) {
    WSREP_WARN("SQL statement (%s) was not replicated (thd: %u)",
               WSREP_QUERY(thd), thd->thread_id());
    rcode = WSREP_TRX_FAIL;

  } else if (!rcode) {
    if (WSREP_OK == rcode) {
      THD_STAGE_INFO(thd, stage_wsrep_replicate);
      snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
               "wsrep: initiating replication for write set (%lld)",
               (long long)wsrep_thd_trx_seqno(thd));
      WSREP_DEBUG("%s", thd->wsrep_info);
      thd_proc_info(thd, thd->wsrep_info);

      rcode = wsrep->replicate(
          wsrep, (wsrep_conn_id_t)thd->thread_id(), &thd->wsrep_ws_handle,
          WSREP_FLAG_COMMIT |
              ((thd->wsrep_PA_safe) ? 0ULL : WSREP_FLAG_PA_UNSAFE),
          &thd->wsrep_trx_meta);
    }

    if (rcode == WSREP_TRX_MISSING) {
      WSREP_WARN(
          "Unable to locate transaction handle in provider"
          " thd: %u, schema: %s, SQL: %s",
          thd->thread_id(), (thd->db().str ? thd->db().str : "(null)"),
          WSREP_QUERY(thd));
      rcode = WSREP_TRX_FAIL;
    } else if (rcode == WSREP_BF_ABORT) {
      WSREP_DEBUG("THD: %u, seqno %lld was BF aborted by provider, will replay",
                  thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno);
      mysql_mutex_lock(&thd->LOCK_wsrep_thd);
      thd->wsrep_conflict_state = MUST_REPLAY;
      DBUG_ASSERT(wsrep_thd_trx_seqno(thd) > 0);
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      mysql_mutex_lock(&LOCK_wsrep_replaying);
      wsrep_replaying++;
      WSREP_DEBUG("replaying increased: %d, thd: %u", wsrep_replaying,
                  thd->thread_id());
      mysql_mutex_unlock(&LOCK_wsrep_replaying);
    }
  } else {
    WSREP_ERROR("I/O error reading from thd's binlog iocache (errno=%d)",
                my_errno());
    DBUG_ASSERT(0);  // failure like this can not normally happen
    DBUG_RETURN(WSREP_TRX_ERROR);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  switch (rcode) {
    case 0:
      /*
        About MUST_ABORT: We assume that even if thd conflict state was set
        to MUST_ABORT, underlying transaction was not rolled back or marked
        as deadlock victim in QUERY_COMMITTING state. Conflict state is
        set to NO_CONFLICT and commit proceeds as usual.
      */
      if (thd->wsrep_conflict_state == MUST_ABORT) {
        thd->killed = THD::NOT_KILLED;
        thd->wsrep_conflict_state = NO_CONFLICT;
      }

      if (thd->wsrep_conflict_state != NO_CONFLICT) {
        WSREP_WARN("thd %u seqno %lld: conflict state %s after post commit",
                   thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno,
                   wsrep_get_conflict_state(thd->wsrep_conflict_state));
      }

      /* We avoid updating the wsrep_exec_mode to LOCAL_COMMIT till we are done
      with pre-commit hook. */

      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);

      /* Override XID iff it was generated by mysql */
      if (thd->get_transaction()->xid_state()->get_xid()->get_my_xid()) {
        wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                       thd->wsrep_trx_meta.gtid.uuid,
                       thd->wsrep_trx_meta.gtid.seqno);
      }

      THD_STAGE_INFO(thd, stage_wsrep_write_set_replicated);
      snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
               "wsrep: write set replicated (%lld)",
               (long long)wsrep_thd_trx_seqno(thd));
      WSREP_DEBUG("%s", thd->wsrep_info);
      thd_proc_info(thd, thd->wsrep_info);

      DBUG_PRINT("wsrep", ("replicating commit success"));
      break;

    case WSREP_BF_ABORT:
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);

    case WSREP_TRX_FAIL:
      WSREP_DEBUG("commit action failed for reason: %s THD: %u Query: %s",
                  wsrep_get_wsrep_status((wsrep_status)rcode), thd->thread_id(),
                  WSREP_QUERY(thd));
      DBUG_PRINT("wsrep", ("replicating commit fail"));

      thd->wsrep_query_state = QUERY_EXEC;

      if (thd->wsrep_conflict_state == MUST_ABORT) {
        thd->wsrep_conflict_state = ABORTED;
      } else {
        WSREP_DEBUG("conflict state: %s",
                    wsrep_get_conflict_state(thd->wsrep_conflict_state));
        if (thd->wsrep_conflict_state == NO_CONFLICT) {
          thd->wsrep_conflict_state = CERT_FAILURE;
          WSREP_LOG_CONFLICT(NULL, thd, false);
        }
      }
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

      DBUG_RETURN(WSREP_TRX_CERT_FAIL);

    case WSREP_SIZE_EXCEEDED:
      WSREP_ERROR("transaction size exceeded");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_SIZE_EXCEEDED);

    case WSREP_CONN_FAIL:
      WSREP_ERROR("connection failure");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);

    default:
      WSREP_ERROR("unknown connection failure");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);
  }

  /* We avoid updating wsrep_query_state state to QUERY_EXEC
  that should be done as part of pre_commit_hook. */

  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_RETURN(WSREP_TRX_OK);
}

enum wsrep_trx_status wsrep_pre_commit(THD *thd) {
  int rcode;

  DBUG_ENTER("wsrep_pre_commit");

  DBUG_ASSERT(thd->wsrep_query_state == QUERY_COMMITTING);

  THD_STAGE_INFO(thd, stage_wsrep_pre_commit);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "wsrep: initiating pre-commit for write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  rcode = wsrep->pre_commit(
      wsrep, (wsrep_conn_id_t)thd->thread_id(), &thd->wsrep_ws_handle,
      WSREP_FLAG_COMMIT | ((thd->wsrep_PA_safe) ? 0ULL : WSREP_FLAG_PA_UNSAFE),
      &thd->wsrep_trx_meta);

  if (rcode == WSREP_TRX_MISSING) {
    WSREP_WARN(
        "Unable to locate transaction handle in provider"
        " thd: %u, schema: %s, SQL: %s",
        thd->thread_id(), (thd->db().str ? thd->db().str : "(null)"),
        WSREP_QUERY(thd));
    rcode = WSREP_TRX_FAIL;
  } else if (rcode == WSREP_BF_ABORT) {
    WSREP_DEBUG("thd %u seqno %lld BF aborted by provider, will replay",
                thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno);
    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    thd->wsrep_conflict_state = MUST_REPLAY;
    DBUG_ASSERT(wsrep_thd_trx_seqno(thd) > 0);
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    mysql_mutex_lock(&LOCK_wsrep_replaying);
    wsrep_replaying++;
    WSREP_DEBUG("replaying increased: %d, thd: %u", wsrep_replaying,
                thd->thread_id());
    mysql_mutex_unlock(&LOCK_wsrep_replaying);
  }

  DEBUG_SYNC(thd, "wsrep_after_replication");

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  switch (rcode) {
    case 0:
      /*
        About MUST_ABORT: We assume that even if thd conflict state was set
        to MUST_ABORT, underlying transaction was not rolled back or marked
        as deadlock victim in QUERY_COMMITTING state. Conflict state is
        set to NO_CONFLICT and commit proceeds as usual.
      */
      if (thd->wsrep_conflict_state == MUST_ABORT)
        thd->wsrep_conflict_state = NO_CONFLICT;

      if (thd->wsrep_conflict_state != NO_CONFLICT) {
        WSREP_WARN("thd %u seqno %lld: conflict state %s after post commit",
                   thd->thread_id(), (long long)thd->wsrep_trx_meta.gtid.seqno,
                   wsrep_get_conflict_state(thd->wsrep_conflict_state));
      }
      thd->wsrep_exec_mode = LOCAL_COMMIT;

      THD_STAGE_INFO(thd, stage_wsrep_pre_commit_cert_passed);
      snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
               "wsrep: pre-commit/certification passed (%lld)",
               (long long)wsrep_thd_trx_seqno(thd));
      WSREP_DEBUG("%s", thd->wsrep_info);
      thd_proc_info(thd, thd->wsrep_info);

      DBUG_PRINT("wsrep", ("pre-commit success"));
      break;

    case WSREP_BF_ABORT: {
      /* xid was populated post replicate but if transaction was aborted
      during pre-commit then reset xid back to undefined state (as per
      original protocol). */
      wsrep_trx_meta_t meta;
      meta.gtid = WSREP_GTID_UNDEFINED;
      wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                     meta.gtid.uuid, meta.gtid.seqno);
      DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno != WSREP_SEQNO_UNDEFINED);
    }
      // fallthrough

    case WSREP_PRECOMMIT_ABORT:
    case WSREP_TRX_FAIL:
      WSREP_DEBUG("pre-commit action failed for reason: %s THD: %u Query: %s",
                  wsrep_get_wsrep_status((wsrep_status)rcode), thd->thread_id(),
                  WSREP_QUERY(thd));
      DBUG_PRINT("wsrep", ("pre-commit fail"));

      thd->wsrep_query_state = QUERY_EXEC;

      if (thd->wsrep_conflict_state == MUST_ABORT) {
        thd->wsrep_conflict_state = ABORTED;
      } else {
        WSREP_DEBUG("conflict state: %s",
                    wsrep_get_conflict_state(thd->wsrep_conflict_state));
        if (thd->wsrep_conflict_state == NO_CONFLICT) {
          thd->wsrep_conflict_state = CERT_FAILURE;
          WSREP_LOG_CONFLICT(NULL, thd, false);
        }
      }
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

      DBUG_RETURN(WSREP_TRX_CERT_FAIL);

    case WSREP_SIZE_EXCEEDED:
      WSREP_ERROR("transaction size exceeded");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_SIZE_EXCEEDED);

    case WSREP_CONN_FAIL:
      WSREP_ERROR("connection failure");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);

    default:
      WSREP_ERROR("unknown connection failure");
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      DBUG_RETURN(WSREP_TRX_ERROR);
  }

  thd->wsrep_query_state = QUERY_EXEC;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_RETURN(WSREP_TRX_OK);
}

bool wsrep_replicate_GTID(THD *thd) {
  if (thd->slave_thread) {
    WSREP_DEBUG("GTID replication");
    DBUG_ASSERT(WSREP_UNDEFINED_TRX_ID == thd->wsrep_ws_handle.trx_id);
    (void)wsrep_ws_handle_for_trx(&thd->wsrep_ws_handle, thd->query_id);
    DBUG_ASSERT(WSREP_UNDEFINED_TRX_ID != thd->wsrep_ws_handle.trx_id);
    WSREP_DEBUG("slave trx using query ID %lu for replicating GTID",
                (long unsigned int)thd->wsrep_ws_handle.trx_id);

    enum wsrep_trx_status rcode = wsrep_run_wsrep_commit(thd, wsrep_hton, true);
    if (rcode) {
      /*
        TODO: should error here cause stopping of MySQL slave?
        Slave applying was totally filtered out, and fauílure in replicating
        GTID event, would cause a hole in GTID history in other cluster nodes

      */
      WSREP_INFO("GTID replication failed: %d", rcode);
      wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle);
      thd->wsrep_replicate_GTID = false;
      my_message(ER_ERROR_DURING_COMMIT,
                 "WSREP GTID replication was interrupted", MYF(0));
      return true;
    }
    wsrep_post_commit(thd, true);
  }
  thd->wsrep_replicate_GTID = false;

  return false;
}

static int wsrep_hton_init(void *p) {
  wsrep_hton = (handlerton *)p;
  wsrep_hton->state = SHOW_OPTION_YES;
  wsrep_hton->db_type = DB_TYPE_WSREP;
  wsrep_hton->savepoint_offset = sizeof(my_off_t);
  wsrep_hton->close_connection = wsrep_close_connection;
  wsrep_hton->savepoint_set = wsrep_savepoint_set;
  wsrep_hton->savepoint_rollback = wsrep_savepoint_rollback;
  wsrep_hton->commit = wsrep_commit;
  wsrep_hton->rollback = wsrep_rollback;
  wsrep_hton->prepare = wsrep_prepare;
  wsrep_hton->flags =
      HTON_NOT_USER_SELECTABLE | HTON_HIDDEN;  // todo: fix flags
  wsrep_hton->slot = 0;
  return 0;
}

struct st_mysql_storage_engine wsrep_storage_engine = {
    MYSQL_HANDLERTON_INTERFACE_VERSION};

mysql_declare_plugin(wsrep){
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &wsrep_storage_engine,
    "wsrep",
    "Codership Oy",
    "A pseudo storage engine to represent transactions in multi-master "
    "synchornous replication",
    PLUGIN_LICENSE_GPL,
    wsrep_hton_init, /* Plugin Init */
    NULL,            /* Plugin Check uninstall */
    NULL,            /* Plugin Deinit */
    0x0100 /* 1.0 */,
    NULL, /* status variables                */
    NULL, /* system variables                */
    NULL, /* config options                  */
    0,    /* flags                           */
} mysql_declare_plugin_end;
