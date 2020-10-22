/* Copyright (C) 2013 Codership Oy <info@codership.com>

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

#include "wsrep_thd.h"

#include "transaction.h"
#include "rpl_rli.h"
#include "log_event.h"
#include "sql_parse.h"
#include "sql_base.h" // close_thread_tables()
#include "mysqld.h"   // start_wsrep_THD();

static long long wsrep_bf_aborts_counter = 0;

int wsrep_show_bf_aborts (THD *thd, SHOW_VAR *var, char *buff)
{
    wsrep_local_bf_aborts = my_atomic_load64(&wsrep_bf_aborts_counter);
    var->type = SHOW_LONGLONG;
    var->value = (char*)&wsrep_local_bf_aborts;
    return 0;
}

/* must have (&thd->LOCK_wsrep_thd) */
void wsrep_client_rollback(THD *thd)
{
  WSREP_DEBUG("Initiating rollback. Transaction (query: %s) triggered by"
              " thread %u was BF aborted",
              WSREP_QUERY(thd), thd->thread_id());
  my_atomic_add64(&wsrep_bf_aborts_counter, 1);

  thd->wsrep_conflict_state= ABORTING;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  /* Check for comments in Relay_log_info::cleanup_context */
  trans_rollback_stmt(thd);

  trans_rollback(thd);

  if (thd->locked_tables_mode && thd->lock)
  {
    WSREP_DEBUG("Unlock tables, transaction triggered by thread id (%u)"
                " was BF aborted and rolled back", thd->thread_id());
    thd->locked_tables_list.unlock_locked_tables(thd);
    thd->variables.option_bits&= ~(OPTION_TABLE_LOCK);
  }

  if (thd->global_read_lock.is_acquired())
  {
    WSREP_DEBUG("Release Global Read Lock, transaction triggered by thread id (%u)"
                " was BF aborted and rolled back", thd->thread_id());
    thd->global_read_lock.unlock_global_read_lock(thd);
  }

  /* Release transactional metadata locks. */
  thd->mdl_context.release_transactional_locks();

  /* Release all user-locks. */
  mysql_ull_cleanup(thd);

  /* release explicit MDL locks */
  thd->mdl_context.release_explicit_locks();

  if (thd->get_binlog_table_maps())
  {
    WSREP_DEBUG("Clear binlog table map, transaction triggered by thread id (%u)"
                " was BF aborted and rolled back", thd->thread_id());
    thd->clear_binlog_table_maps();
  }
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  thd->wsrep_conflict_state= ABORTED;
}

#define NUMBER_OF_FIELDS_TO_IDENTIFY_COORDINATOR 1
#define NUMBER_OF_FIELDS_TO_IDENTIFY_WORKER 2
#include "rpl_info_factory.h"

static Relay_log_info* wsrep_relay_log_init(const char* log_fname)
{
  uint rli_option = INFO_REPOSITORY_DUMMY;
  Relay_log_info *rli= NULL;
  rli = Rpl_info_factory::create_rli(rli_option, false, "wsrep", true);
  if (!rli)
  {
    WSREP_ERROR("Failed to create Relay-Log for wsrep thread, aborting");
    unireg_abort(MYSQLD_ABORT_EXIT);
  }
  rli->set_rli_description_event(
      new Format_description_log_event(BINLOG_VERSION));

  rli->current_mts_submode= new Mts_submode_wsrep();
  return (rli);
}

static void wsrep_prepare_bf_thd(THD *thd, struct wsrep_thd_shadow* shadow)
{
  shadow->options       = thd->variables.option_bits;
  shadow->server_status = thd->server_status;
  shadow->wsrep_exec_mode = thd->wsrep_exec_mode;
  shadow->vio           = thd->active_vio;
  shadow->rli_slave     = thd->rli_slave;
  // Disable general logging on applier threads
  thd->variables.option_bits |= OPTION_LOG_OFF;
  // Enable binlogging if opt_log_slave_updates is set
  if (opt_log_slave_updates)
    thd->variables.option_bits|= OPTION_BIN_LOG;
  else
    thd->variables.option_bits&= ~(OPTION_BIN_LOG);

#ifdef GALERA
  /*
    in 5.7, we declare applying to happen as with slave threads
    this set here is for replaying, when local threads will operate
    as slave for the duration of replaying
  */
  thd->slave_thread = TRUE;
#endif /* GALERA */

  if (!thd->wsrep_rli)
  {
    thd->wsrep_rli = wsrep_relay_log_init("wsrep_relay");
    thd->rli_slave = thd->wsrep_rli;
    thd->wsrep_rli->info_thd= thd;
    thd->init_for_queries(thd->wsrep_rli);
  }
  thd->wsrep_rli->info_thd = thd;

  thd->wsrep_exec_mode= REPL_RECV;
  thd->set_active_vio(0);
  thd->clear_error();

  shadow->tx_isolation        = thd->variables.tx_isolation;
  thd->variables.tx_isolation = ISO_READ_COMMITTED;
  thd->tx_isolation           = ISO_READ_COMMITTED;

  shadow->db = thd->db();
  thd->reset_db(NULL_CSTR);

  shadow->user_time = thd->user_time;
  shadow->row_count_func= thd->get_row_count_func();
}

static void wsrep_return_from_bf_mode(THD *thd, struct wsrep_thd_shadow* shadow)
{
  thd->variables.option_bits  = shadow->options;
  thd->server_status          = shadow->server_status;
  thd->wsrep_exec_mode        = shadow->wsrep_exec_mode;
  thd->set_active_vio(shadow->vio);
  thd->variables.tx_isolation = shadow->tx_isolation;
  thd->reset_db(shadow->db);
  thd->user_time              = shadow->user_time;
  assert(thd->rli_slave == thd->wsrep_rli);
  thd->rli_slave              = shadow->rli_slave;

  delete thd->wsrep_rli->current_mts_submode;
  thd->wsrep_rli->current_mts_submode = 0;
  delete thd->wsrep_rli;
  thd->wsrep_rli = 0;
  thd->slave_thread = (thd->rli_slave) ? TRUE : FALSE;
  thd->set_row_count_func(shadow->row_count_func);
}

void wsrep_replay_sp_transaction(THD* thd)
{
  DBUG_ENTER("wsrep_replay_sp_transaction");
  mysql_mutex_assert_owner(&thd->LOCK_wsrep_thd);
  DBUG_ASSERT(thd->wsrep_conflict_state == MUST_REPLAY);
  DBUG_ASSERT(thd->sp_runtime_ctx);
  DBUG_ASSERT(wsrep_thd_trx_seqno(thd) > 0);

  close_thread_tables(thd);
  if (thd->locked_tables_mode && thd->lock)
  {
    WSREP_DEBUG("releasing table lock for replaying (%u)",
                thd->thread_id());
    thd->locked_tables_list.unlock_locked_tables(thd);
    thd->variables.option_bits&= ~(OPTION_TABLE_LOCK);
  }
  thd->mdl_context.release_transactional_locks();

  THD *replay_thd= new THD(true, true);
  replay_thd->thread_stack= thd->thread_stack;

  struct wsrep_thd_shadow shadow;
  wsrep_prepare_bf_thd(replay_thd, &shadow);
  replay_thd->wsrep_trx_meta= thd->wsrep_trx_meta;
  replay_thd->wsrep_ws_handle= thd->wsrep_ws_handle;
  replay_thd->wsrep_ws_handle.trx_id= WSREP_UNDEFINED_TRX_ID;
  replay_thd->wsrep_conflict_state= REPLAYING;

  replay_thd->variables.option_bits|= OPTION_BEGIN;
  replay_thd->server_status|= SERVER_STATUS_IN_TRANS;

  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  thd->restore_globals();
  replay_thd->store_globals();
  wsrep_status_t rcode= wsrep->replay_trx(wsrep,
                                          &replay_thd->wsrep_ws_handle,
                                          (void*) replay_thd);

  wsrep_return_from_bf_mode(replay_thd, &shadow);
  replay_thd->restore_globals();
  delete replay_thd;

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);

  thd->store_globals();

  switch (rcode)
  {
  case WSREP_OK:
    {
      thd->wsrep_conflict_state= NO_CONFLICT;
      thd->killed= THD::NOT_KILLED;
      wsrep_status_t rcode= wsrep->post_commit(wsrep, &thd->wsrep_ws_handle);
      if (rcode != WSREP_OK)
      {
        WSREP_WARN("Post commit failed for SP replay: thd: %u error: %d",
                   thd->thread_id(), rcode);
      }
      /* As replaying the transaction was successful, an error must not
         be returned to client, so we need to reset the error state of
         the diagnostics area */
      thd->get_stmt_da()->reset_diagnostics_area();
      break;
    }
  case WSREP_TRX_FAIL:
    {
      thd->wsrep_conflict_state= ABORTED;
      wsrep_status_t rcode= wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle);
      if (rcode != WSREP_OK)
      {
        WSREP_WARN("Post rollback failed for SP replay: thd: %u error: %d",
                   thd->thread_id(), rcode);
      }
      if (thd->get_stmt_da()->is_set())
      {
        thd->get_stmt_da()->reset_diagnostics_area();
      }
      my_error(ER_LOCK_DEADLOCK, MYF(0));
      break;
    }
  default:
    WSREP_ERROR("trx_replay failed for: %d, schema: %s, query: %s",
                rcode,
                (thd->db().str ? thd->db().str : "(null)"),
                WSREP_QUERY(thd));
    /* we're now in inconsistent state, must abort */
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    unireg_abort(1);
    break;
  }

  wsrep_cleanup_transaction(thd);

  mysql_mutex_lock(&LOCK_wsrep_replaying);
  wsrep_replaying--;
  WSREP_DEBUG("replaying decreased: %d, thd: %u",
              wsrep_replaying, thd->thread_id());
  mysql_cond_broadcast(&COND_wsrep_replaying);
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  DBUG_VOID_RETURN;
}

void wsrep_replay_transaction(THD *thd)
{
  DBUG_ENTER("wsrep_replay_transaction");
  /* checking if BF trx must be replayed */
  if (thd->wsrep_conflict_state== MUST_REPLAY) {
    DBUG_ASSERT(wsrep_thd_trx_seqno(thd));
    if (thd->wsrep_exec_mode!= REPL_RECV) {
      if (thd->get_stmt_da()->is_sent())
      {
        WSREP_ERROR("Transaction replay issue, thd has reported status already");
      }

      /* PS reprepare observer should have been removed already
         open_table() will fail if we have dangling observer here
       */
      if (thd->get_reprepare_observer() && wsrep_log_conflicts)
      {
        WSREP_WARN("Found dangling observer in replaying of transaction"
                   " (thr %u %lld %s)",
                   thd->thread_id(), thd->query_id, thd->query().str);
      }

      struct da_shadow
      {
          enum Diagnostics_area::enum_diagnostics_status status;
          ulonglong affected_rows;
          ulonglong last_insert_id;
          char message[MYSQL_ERRMSG_SIZE];
      };
      struct da_shadow da_status;
      da_status.status= thd->get_stmt_da()->status();
      if (da_status.status == Diagnostics_area::DA_OK)
      {
        da_status.affected_rows= thd->get_stmt_da()->affected_rows();
        da_status.last_insert_id= thd->get_stmt_da()->last_insert_id();
        strmake(da_status.message,
                thd->get_stmt_da()->message_text(),
                sizeof(da_status.message)-1);
      }

      thd->get_stmt_da()->reset_diagnostics_area();

      thd->wsrep_conflict_state= REPLAYING;
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

      mysql_reset_thd_for_next_command(thd);
      thd->killed= THD::NOT_KILLED;
      close_thread_tables(thd);
      if (thd->locked_tables_mode && thd->lock)
      {
        WSREP_DEBUG("Release table locks owned by thread (%u)."
                    "Transaction will be replayed",
                    thd->thread_id());
        thd->locked_tables_list.unlock_locked_tables(thd);
        thd->variables.option_bits&= ~(OPTION_TABLE_LOCK);
      }
      thd->mdl_context.release_transactional_locks();
      /*
        Replaying will call MYSQL_START_STATEMENT when handling
        BEGIN Query_log_event so end statement must be called before
        replaying.
      */
      MYSQL_END_STATEMENT(thd->m_statement_psi, thd->get_stmt_da());
      thd->m_statement_psi= NULL;
      thd->m_digest= NULL;

      THD_STAGE_INFO(thd, stage_wsrep_replaying_trx);
      snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
               "wsrep: replaying write set (%lld)",
               (long long)wsrep_thd_trx_seqno(thd));
      WSREP_DEBUG("%s", thd->wsrep_info);
      thd_proc_info(thd, thd->wsrep_info);

      WSREP_DEBUG("Replaying transaction for SQL query: %s with write-set: %lld",
                  WSREP_QUERY(thd),
                  (long long)wsrep_thd_trx_seqno(thd));

      struct wsrep_thd_shadow shadow;
      wsrep_prepare_bf_thd(thd, &shadow);

      /* From trans_begin() */
      thd->variables.option_bits|= OPTION_BEGIN;
      thd->server_status|= SERVER_STATUS_IN_TRANS;

      int rcode = wsrep->replay_trx(wsrep,
                                    &thd->wsrep_ws_handle,
                                    (void *)thd);

      wsrep_return_from_bf_mode(thd, &shadow);
      if (thd->wsrep_conflict_state != REPLAYING)
        WSREP_WARN("Lost replaying mode: %d", thd->wsrep_conflict_state );

      mysql_mutex_lock(&thd->LOCK_wsrep_thd);

      switch (rcode)
      {
      case WSREP_OK:
        thd->wsrep_conflict_state= NO_CONFLICT;
        wsrep->post_commit(wsrep, &thd->wsrep_ws_handle);
        WSREP_DEBUG("Transaction replay completed successfully (%u %llu)",
                    thd->thread_id(), (long long)thd->real_id);
        if (thd->get_stmt_da()->is_sent())
        {
          WSREP_WARN("Replay completed with success");
        }
        else if (thd->get_stmt_da()->is_set())
        {
          if (thd->get_stmt_da()->status() != Diagnostics_area::DA_OK)
          {
            WSREP_WARN("Replay completed with error %d",
                       thd->get_stmt_da()->status());
          }
        }
        else
        {
          if (da_status.status == Diagnostics_area::DA_OK)
          {
            my_ok(thd,
                  da_status.affected_rows,
                  da_status.last_insert_id,
                  da_status.message);
          }
          else
          {
            my_ok(thd);
          }
        }
        break;
      case WSREP_TRX_FAIL:
        if (thd->get_stmt_da()->is_sent())
        {
          WSREP_ERROR("Transaction replay failed, thd has reported status");
        }
        else
        {
          WSREP_DEBUG("Transaction replay with write-set: %lld failed."
                      " Rolling back", (long long)wsrep_thd_trx_seqno(thd));
          //my_error(ER_LOCK_DEADLOCK, MYF(0), "wsrep aborted transaction");
        }
        thd->wsrep_conflict_state= ABORTED;
        wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle);
        break;
      default:
        WSREP_ERROR("Failed to replay transaction for: %d, schema: %s, query: %s",
                    rcode,
                    (thd->db().str ? thd->db().str : "(null)"),
                    WSREP_QUERY(thd));
        /* we're now in inconsistent state, must abort */
	mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
        unireg_abort(1);
        break;
      }

      wsrep_cleanup_transaction(thd);

      mysql_mutex_lock(&LOCK_wsrep_replaying);
      wsrep_replaying--;
      WSREP_DEBUG("Decreasing replay counter: %d, thd: %u",
                  wsrep_replaying, thd->thread_id());
      mysql_cond_broadcast(&COND_wsrep_replaying);
      mysql_mutex_unlock(&LOCK_wsrep_replaying);
    }
  }
  DBUG_VOID_RETURN;
}

/* Applier/Slave Threads. Controlled using wsrep_slave_threads configuration
variable. These are worker thread capable of applying write-set in parallel.
Parallelism is use to explore efficency. */
static void wsrep_replication_process(THD *thd)
{
  int rcode;
  DBUG_ENTER("wsrep_replication_process");

  THD_STAGE_INFO(thd, stage_wsrep_applier_idle);

  struct wsrep_thd_shadow shadow;

  wsrep_prepare_bf_thd(thd, &shadow);

  /* This should be set as part of trans_begin and not explicitly.
  Note sure why Galera thought of setting it here but it conflicts
  as the first BEGIN statement is now intepreted wrongly as statement
  inside an active transaction. */
  /* From trans_begin() */
  thd->variables.option_bits|= OPTION_BEGIN;
  thd->server_status|= SERVER_STATUS_IN_TRANS;

  rcode = wsrep->recv(wsrep, (void *)thd);
  DBUG_PRINT("wsrep",("wsrep_repl returned: %d", rcode));

  WSREP_INFO("applier thread exiting (code:%d)", rcode);

  switch (rcode) {
  case WSREP_OK:
  case WSREP_NOT_IMPLEMENTED:
  case WSREP_CONN_FAIL:
    /* provider does not support slave operations / disconnected from group,
     * just close applier thread */
    break;
  case WSREP_NODE_FAIL:
    /* data inconsistency => SST is needed */
    /* Note: we cannot just blindly restart replication here,
     * SST might require server restart if storage engines must be
     * initialized after SST */
    WSREP_ERROR("Node consistency compromised. Aborting");
    wsrep_kill_mysql(thd);
    break;
  case WSREP_WARNING:
  case WSREP_TRX_FAIL:
  case WSREP_TRX_MISSING:
    /* these suggests a bug in provider code */
    WSREP_WARN("bad return from recv() call: %d", rcode);
    /* fall through to node shutdown */
    // fallthrough
  case WSREP_FATAL:
    /* Cluster connectivity is lost.
     *
     * If applier was killed on purpose (KILL_CONNECTION), we
     * avoid mysql shutdown. This is because the killer will then handle
     * shutdown processing (or replication restarting)
     */
    if (thd->killed != THD::KILL_CONNECTION)
    {
      wsrep_kill_mysql(thd);
    }
    break;
  }

  wsrep_close_applier(thd);

  TABLE *tmp;
  while ((tmp = thd->temporary_tables))
  {
    WSREP_WARN("Applier %u, has temporary tables at exit: %s.%s",
               thd->thread_id(), 
               (tmp->s) ? tmp->s->db.str : "void",
               (tmp->s) ? tmp->s->table_name.str : "void");
  }
  wsrep_return_from_bf_mode(thd, &shadow);

  DBUG_VOID_RETURN;
}

static bool create_wsrep_THD(PSI_thread_key key, wsrep_thd_processor_fun processor)
{
  my_thread_handle hThread;
  bool res= mysql_thread_create(
                            key, 
                            &hThread, &connection_attrib,
                            start_wsrep_THD, (void*)processor);
#ifdef WITH_WSREP_OUT
  /*
    MariaDB bug https://jira.mariadb.org/browse/MDEV-8208 has not been observed
    in this server version. However, if it surfaces, the server startup must be 
    should be delayed here until wsrep_running_threads count ascends
  */
#endif /* WITH_WSREP_OUT */
  return res;
}

/* Create applier threads based on configuration.
Applier thread are responsible for applying write-set. */
void wsrep_create_appliers(long threads)
{
  if (!wsrep_connected)
  {
    /* see wsrep_replication_start() for the logic */
    if (wsrep_cluster_address && strlen(wsrep_cluster_address) &&
        wsrep_provider && strcasecmp(wsrep_provider, "none"))
    {
      WSREP_ERROR("Trying to launch slave threads before creating "
                  "connection at '%s'", wsrep_cluster_address);
    }
    return;
  }

  long wsrep_threads=0;
  while (wsrep_threads++ < threads) {
    if (create_wsrep_THD(key_THREAD_wsrep_applier, wsrep_replication_process))
      WSREP_WARN("Can't create thread to manage wsrep replication");
  }
}

static void wsrep_rollback_process(THD *thd)
{
  DBUG_ENTER("wsrep_rollback_process");

  mysql_mutex_lock(&LOCK_wsrep_rollback);
  wsrep_aborting_thd= NULL;

  THD_STAGE_INFO(thd, stage_wsrep_in_rollback_thread);

  while (thd->killed == THD::NOT_KILLED) {

    THD_STAGE_INFO(thd, stage_wsrep_aborter_idle);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex= &LOCK_wsrep_rollback;
    thd->current_cond= &COND_wsrep_rollback;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    mysql_cond_wait(&COND_wsrep_rollback,&LOCK_wsrep_rollback);

    if (thd->killed != THD::NOT_KILLED)
    {
      WSREP_DEBUG("Rollback/Aborter thread cancelled");
      break;
    }

    WSREP_DEBUG("Rollback/Aborter thread wakes for signal."
                " Has some transaction rollback work to do.");

    THD_STAGE_INFO(thd, stage_wsrep_aborter_active);

    mysql_mutex_lock(&thd->LOCK_current_cond);
    thd->current_mutex= 0;
    thd->current_cond= 0;
    mysql_mutex_unlock(&thd->LOCK_current_cond);

    /* check for false alarms */
    if (!wsrep_aborting_thd)
      WSREP_DEBUG("Rollback/Aborter thread found empty abort queue");

    /* process all entries in the queue */
    while (wsrep_aborting_thd) {
      THD *aborting;
      wsrep_aborting_thd_t next = wsrep_aborting_thd->next;
      aborting = wsrep_aborting_thd->aborting_thd;
      my_free(wsrep_aborting_thd);
      wsrep_aborting_thd= next;
      /*
       * must release mutex, appliers my want to add more
       * aborting thds in our work queue, while we rollback
       */
      mysql_mutex_unlock(&LOCK_wsrep_rollback);

      mysql_mutex_lock(&aborting->LOCK_wsrep_thd);
      if (aborting->wsrep_conflict_state == ABORTED)
      {
        WSREP_DEBUG("Rollback/Aborter encounter already aborted thread"
                    " thread-id: %u (%llu) conflict-state: %s",
                    aborting->thread_id(), (long long)aborting->real_id,
                    wsrep_get_conflict_state(aborting->wsrep_conflict_state));

        mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);
        mysql_mutex_lock(&LOCK_wsrep_rollback);
        continue;
      }
      aborting->wsrep_conflict_state= ABORTING;

      mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);

      aborting->store_globals();

      mysql_mutex_lock(&aborting->LOCK_wsrep_thd);
      wsrep_client_rollback(aborting);
      WSREP_DEBUG("Rollback/Aborter thread successfully aborted thread"
                  " thread-id: %u (%llu) conflict-state: %s",
                  aborting->thread_id(), (long long)aborting->real_id,
                  wsrep_get_conflict_state(aborting->wsrep_conflict_state));
      mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);

      /* Clear the thread state, since the rollback thread is done with it */
      aborting->restore_globals();

      mysql_mutex_lock(&LOCK_wsrep_rollback);
    }
  }

  mysql_mutex_unlock(&LOCK_wsrep_rollback);

  mysql_mutex_lock(&thd->LOCK_thd_data);
  thd_proc_info(thd, "wsrep aborter shutting down");
  thd->current_mutex= 0;
  thd->current_cond=  0;
  mysql_mutex_unlock(&thd->LOCK_thd_data);

  sql_print_information("WSREP: rollbacker thread exiting");
  thd->store_globals();
  DBUG_PRINT("wsrep",("wsrep rollbacker thread exiting"));
  DBUG_VOID_RETURN;
}

void wsrep_create_rollbacker()
{
  if (wsrep_provider && strcasecmp(wsrep_provider, "none"))
  {
    /* create rollbacker */
    if (create_wsrep_THD(key_THREAD_wsrep_rollbacker, wsrep_rollback_process))
      WSREP_WARN("Failed to create wsrep rollback/aborter thread");
  }
}

void wsrep_thd_set_PA_safe(void *thd_ptr, my_bool safe)
{ 
  if (thd_ptr) 
  {
    THD* thd = (THD*)thd_ptr;
    thd->wsrep_PA_safe = safe;
  }
}

int wsrep_thd_conflict_state(void *thd_ptr, my_bool sync)
{ 
  int state = -1;
  if (thd_ptr) 
  {
    THD* thd = (THD*)thd_ptr;
    if (sync) mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    
    state = thd->wsrep_conflict_state;
    if (sync) mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }
  return state;
}

my_bool wsrep_thd_is_BF(void *thd_ptr, my_bool sync)
{ 
  my_bool status = FALSE;
  if (thd_ptr) 
  {
    THD* thd = (THD*)thd_ptr;
    if (sync) mysql_mutex_lock(&thd->LOCK_wsrep_thd);

    status = ((thd->wsrep_exec_mode == REPL_RECV)    ||
	      (thd->wsrep_exec_mode == TOTAL_ORDER));
    if (sync) mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }
  return status;
}

extern "C"
my_bool wsrep_thd_is_BF_or_commit(void *thd_ptr, my_bool sync)
{
  bool status = FALSE;
  if (thd_ptr) 
  {
    THD* thd = (THD*)thd_ptr;
    if (sync) mysql_mutex_lock(&thd->LOCK_wsrep_thd);

    status = ((thd->wsrep_exec_mode == REPL_RECV)    ||
	      (thd->wsrep_exec_mode == TOTAL_ORDER)  ||
	      (thd->wsrep_exec_mode == LOCAL_COMMIT));
    if (sync) mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }
  return status;
}

extern "C"
my_bool wsrep_thd_is_local(void *thd_ptr, my_bool sync)
{
  bool status = FALSE;
  if (thd_ptr) 
  {
    THD* thd = (THD*)thd_ptr;
    if (sync) mysql_mutex_lock(&thd->LOCK_wsrep_thd);

    status = (thd->wsrep_exec_mode == LOCAL_STATE);
    if (sync) mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  }
  return status;
}

int wsrep_abort_thd(void *bf_thd_ptr, void *victim_thd_ptr, my_bool signal)
{
  THD *victim_thd = (THD *) victim_thd_ptr;
  THD *bf_thd     = (THD *) bf_thd_ptr;
  DBUG_ENTER("wsrep_abort_thd");

  if ( (WSREP(bf_thd) ||
         ( (WSREP_ON || bf_thd->variables.wsrep_OSU_method == WSREP_OSU_RSU) &&
           bf_thd->wsrep_exec_mode == TOTAL_ORDER) )                         &&
       victim_thd)
  {
    WSREP_DEBUG("BF thread %u (%llu) aborting Vicitim thread %u (%llu)",
                bf_thd->thread_id(), (long long) bf_thd->real_id,
                victim_thd->thread_id(), (long long) victim_thd->real_id);
    ha_wsrep_abort_transaction(bf_thd, victim_thd, signal);
  }
  else
  {
    WSREP_DEBUG("wsrep_abort_thd not effective: %p %p", bf_thd, victim_thd);
  }

  DBUG_RETURN(1);
}

int wsrep_thd_in_locking_session(void *thd_ptr)
{
  if (thd_ptr && ((THD *)thd_ptr)->in_lock_tables) {
    return 1;
  }
  return 0;
}

bool wsrep_thd_has_explicit_locks(THD *thd)
{
  assert(thd);
  return (thd->mdl_context.wsrep_has_explicit_locks());
}

/*
  Get auto increment variables for THD. Use global settings for
  applier threads.
 */
extern "C"
void wsrep_thd_auto_increment_variables(THD* thd,
                                        unsigned long long* offset,
                                        unsigned long long* increment)
{
  if (thd->wsrep_exec_mode == REPL_RECV &&
      thd->wsrep_conflict_state != REPLAYING)
  {
    *offset= global_system_variables.auto_increment_offset;
    *increment= global_system_variables.auto_increment_increment;
  }
  else
  {
    *offset= thd->variables.auto_increment_offset;
    *increment= thd->variables.auto_increment_increment;
  }
}

bool wsrep_safe_to_persist_xid(THD* thd)
{
  /* Rollback of transaction too also invoke persist of XID.
  Avoid persisting XID in this use-case. */
  
  /* This method is the part of performance improvement
  (introduce in commi c01c75da8ec67197e5b2f86edbdd9541df717faa)
  Before upstream wsrep_5.7.30-25.22 merge we never got here in case of 
  recovery, because xid was unconditionally reset in trx_get_trx_by_xid_low()
  and then we skipped this call in trx_write_serialisation_history()
  Togheter with wsrep_5.7.30-25.22 merge we have conditional xid reset in trx_get_trx_by_xid_low()
  and as it is WSREP xid, we are not resetting it, so we get here. However, we do not have
  thd context. In this case skip the optimization. */
  if (!thd) return true;

  bool safe_to_persist_xid= false;
  if (thd->wsrep_conflict_state == NO_CONFLICT      ||
      thd->wsrep_conflict_state == REPLAYING        ||
      thd->wsrep_conflict_state == RETRY_AUTOCOMMIT)
    safe_to_persist_xid= true;
  return(safe_to_persist_xid);
}
