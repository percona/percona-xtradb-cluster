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
#include "global_threads.h" // LOCK_thread_count, etc.
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
  WSREP_DEBUG("client rollback due to BF abort for (%ld), query: %s",
              thd->thread_id, thd->query());

  my_atomic_add64(&wsrep_bf_aborts_counter, 1);

  thd->wsrep_conflict_state= ABORTING;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  trans_rollback(thd);

  if (thd->locked_tables_mode && thd->lock)
  {
    WSREP_DEBUG("unlocking tables for BF abort (%ld)", thd->thread_id);
    thd->locked_tables_list.unlock_locked_tables(thd);
    thd->variables.option_bits&= ~(OPTION_TABLE_LOCK);
  }

  if (thd->global_read_lock.is_acquired())
  {
    WSREP_DEBUG("unlocking GRL for BF abort (%ld)", thd->thread_id);
    thd->global_read_lock.unlock_global_read_lock(thd);
  }

  /* Release transactional metadata locks. */
  thd->mdl_context.release_transactional_locks();

  /* release explicit MDL locks */
  thd->mdl_context.release_explicit_locks();

  if (thd->get_binlog_table_maps())
  {
    WSREP_DEBUG("clearing binlog table map for BF abort (%ld)", thd->thread_id);
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
  rli = Rpl_info_factory::create_rli(rli_option, false);
  rli->set_rli_description_event(
      new Format_description_log_event(BINLOG_VERSION));

  return (rli);

#ifdef REMOVED
  Rpl_info_handler* handler_src= NULL;
  Rpl_info_handler* handler_dest= NULL;
  ulong *key_info_idx= NULL;
  const char *msg= "Failed to allocate memory for the relay log info "
                   "structure";

  DBUG_ENTER("Rpl_info_factory::create_rli");

  if (!(rli= new Relay_log_info(false
#ifdef HAVE_PSI_INTERFACE
                                ,&key_relay_log_info_run_lock,
                                &key_relay_log_info_data_lock,
                                &key_relay_log_info_sleep_lock,
                                &key_relay_log_info_data_cond,
                                &key_relay_log_info_start_cond,
                                &key_relay_log_info_stop_cond,
                                &key_relay_log_info_sleep_cond
#endif /* HAVE_PSI_INTERFACE */
                               )))
    goto err;

  if (!(key_info_idx= new ulong[NUMBER_OF_FIELDS_TO_IDENTIFY_COORDINATOR]))
     goto err;
  key_info_idx[0]= server_id;
  rli->set_idx_info(key_info_idx, NUMBER_OF_FIELDS_TO_IDENTIFY_COORDINATOR);

  if(Rpl_info_factory::init_rli_repositories(rli, rli_option, &handler_src,
                                             &handler_dest, &msg))
    goto err;

  if (Rpl_info_factory::decide_repository(rli, rli_option, &handler_src,
                                          &handler_dest, &msg))
    goto err;

  DBUG_RETURN(rli);
err:
  delete handler_src;
  delete handler_dest;
  delete []key_info_idx;
  if (rli)
  {
    /*
      The handler was previously deleted so we need to remove
      any reference to it.
    */
    rli->set_idx_info(NULL, 0);
    rli->set_rpl_info_handler(NULL);
    rli->set_rpl_info_type(INVALID_INFO_REPOSITORY);
    delete rli;
  }
  WSREP_ERROR("Error creating relay log info: %s.", msg);
  DBUG_RETURN(NULL);
#endif /* REMOVED */
}

static void wsrep_prepare_bf_thd(THD *thd, struct wsrep_thd_shadow* shadow)
{
  shadow->options       = thd->variables.option_bits;
  shadow->server_status = thd->server_status;
  shadow->wsrep_exec_mode = thd->wsrep_exec_mode;
  shadow->vio           = thd->net.vio;

  if (opt_log_slave_updates)
    thd->variables.option_bits|= OPTION_BIN_LOG;
  else
    thd->variables.option_bits&= ~(OPTION_BIN_LOG);

  if (!thd->wsrep_rli) thd->wsrep_rli= wsrep_relay_log_init("wsrep_relay");
  thd->wsrep_rli->info_thd = thd;

  thd->wsrep_exec_mode= REPL_RECV;
  thd->net.vio= 0;
  thd->clear_error();

  shadow->tx_isolation        = thd->variables.tx_isolation;
  thd->variables.tx_isolation = ISO_READ_COMMITTED;
  thd->tx_isolation           = ISO_READ_COMMITTED;

  shadow->db            = thd->db;
  shadow->db_length     = thd->db_length;
  thd->reset_db(NULL, 0);
}

static void wsrep_return_from_bf_mode(THD *thd, struct wsrep_thd_shadow* shadow)
{
  thd->variables.option_bits  = shadow->options;
  thd->server_status          = shadow->server_status;
  thd->wsrep_exec_mode        = shadow->wsrep_exec_mode;
  thd->net.vio                = shadow->vio;
  thd->variables.tx_isolation = shadow->tx_isolation;
  thd->reset_db(shadow->db, shadow->db_length);
}

void wsrep_replay_transaction(THD *thd)
{
  /* checking if BF trx must be replayed */
  if (thd->wsrep_conflict_state== MUST_REPLAY) {
    DBUG_ASSERT(wsrep_thd_trx_seqno(thd));
    if (thd->wsrep_exec_mode!= REPL_RECV) {
      if (thd->get_stmt_da()->is_sent())
      {
        WSREP_ERROR("replay issue, thd has reported status already");
      }
      thd->get_stmt_da()->reset_diagnostics_area();

      thd->wsrep_conflict_state= REPLAYING;
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

      mysql_reset_thd_for_next_command(thd);
      thd->killed= THD::NOT_KILLED;
      close_thread_tables(thd);
      if (thd->locked_tables_mode && thd->lock)
      {
        WSREP_DEBUG("releasing table lock for replaying (%ld)",
                    thd->thread_id);
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
      thd_proc_info(thd, "wsrep replaying trx");
      WSREP_DEBUG("replay trx: %s %lld",
                  thd->query() ? thd->query() : "void",
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
      if (thd->wsrep_conflict_state!= REPLAYING)
        WSREP_WARN("lost replaying mode: %d", thd->wsrep_conflict_state );

      mysql_mutex_lock(&thd->LOCK_wsrep_thd);

      switch (rcode)
      {
      case WSREP_OK:
        thd->wsrep_conflict_state= NO_CONFLICT;
        wsrep->post_commit(wsrep, &thd->wsrep_ws_handle);
        WSREP_DEBUG("trx_replay successful for: %ld %llu",
                    thd->thread_id, (long long)thd->real_id);
        if (thd->get_stmt_da()->is_sent())
        {
          WSREP_WARN("replay ok, thd has reported status");
        }
        else if (thd->get_stmt_da()->is_set())
        {
          if (thd->get_stmt_da()->status() != Diagnostics_area::DA_OK)
          {
            WSREP_WARN("replay ok, thd has error status %d",
                       thd->get_stmt_da()->status());
          }
        }
        else
        {
          my_ok(thd);
        }
        break;
      case WSREP_TRX_FAIL:
        if (thd->get_stmt_da()->is_sent())
        {
          WSREP_ERROR("replay failed, thd has reported status");
        }
        else
        {
          WSREP_DEBUG("replay failed, rolling back");
          //my_error(ER_LOCK_DEADLOCK, MYF(0), "wsrep aborted transaction");
        }
        thd->wsrep_conflict_state= ABORTED;
        wsrep->post_rollback(wsrep, &thd->wsrep_ws_handle);
        break;
      default:
        WSREP_ERROR("trx_replay failed for: %d, query: %s",
                    rcode, thd->query() ? thd->query() : "void");
        /* we're now in inconsistent state, must abort */
	mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
        unireg_abort(1);
        break;
      }

      wsrep_cleanup_transaction(thd);

      mysql_mutex_lock(&LOCK_wsrep_replaying);
      wsrep_replaying--;
      WSREP_DEBUG("replaying decreased: %d, thd: %lu",
                  wsrep_replaying, thd->thread_id);
      mysql_cond_broadcast(&COND_wsrep_replaying);
      mysql_mutex_unlock(&LOCK_wsrep_replaying);
    }
  }
}

static void wsrep_replication_process(THD *thd)
{
  int rcode;
  DBUG_ENTER("wsrep_replication_process");

  struct wsrep_thd_shadow shadow;

  wsrep_prepare_bf_thd(thd, &shadow);

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
    WSREP_ERROR("node consistency compromised, aborting");
    wsrep_kill_mysql(thd);
    break;
  case WSREP_WARNING:
  case WSREP_TRX_FAIL:
  case WSREP_TRX_MISSING:
    /* these suggests a bug in provider code */
    WSREP_WARN("bad return from recv() call: %d", rcode);
    /* fall through to node shutdown */
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

  mysql_mutex_lock(&LOCK_thread_count);
  wsrep_close_applier(thd);
  mysql_cond_broadcast(&COND_thread_count);
  mysql_mutex_unlock(&LOCK_thread_count);

  TABLE *tmp;
  while ((tmp = thd->temporary_tables))
  {
    WSREP_WARN("Applier %lu, has temporary tables at exit: %s.%s",
                  thd->thread_id, 
                  (tmp->s) ? tmp->s->db.str : "void",
                  (tmp->s) ? tmp->s->table_name.str : "void");
  }
  wsrep_return_from_bf_mode(thd, &shadow);
  DBUG_VOID_RETURN;
}

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
      assert(0);
    }
    return;
  }

  long wsrep_threads=0;
  pthread_t hThread;
  while (wsrep_threads++ < threads) {
    if (pthread_create(
      &hThread, &connection_attrib,
      start_wsrep_THD, (void*)wsrep_replication_process))
      WSREP_WARN("Can't create thread to manage wsrep replication");
  }
}

static void wsrep_rollback_process(THD *thd)
{
  DBUG_ENTER("wsrep_rollback_process");

  mysql_mutex_lock(&LOCK_wsrep_rollback);
  wsrep_aborting_thd= NULL;

  while (thd->killed == THD::NOT_KILLED) {
    thd_proc_info(thd, "wsrep aborter idle");
    thd->mysys_var->current_mutex= &LOCK_wsrep_rollback;
    thd->mysys_var->current_cond=  &COND_wsrep_rollback;

    mysql_cond_wait(&COND_wsrep_rollback,&LOCK_wsrep_rollback);

    WSREP_DEBUG("WSREP rollback thread wakes for signal");

    mysql_mutex_lock(&thd->mysys_var->mutex);
    thd_proc_info(thd, "wsrep aborter active");
    thd->mysys_var->current_mutex= 0;
    thd->mysys_var->current_cond=  0;
    mysql_mutex_unlock(&thd->mysys_var->mutex);

    /* check for false alarms */
    if (!wsrep_aborting_thd)
    {
      WSREP_DEBUG("WSREP rollback thread has empty abort queue");
    }
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
      if (aborting->wsrep_conflict_state== ABORTED)
      {
        WSREP_DEBUG("WSREP, thd already aborted: %llu state: %d",
                    (long long)aborting->real_id,
                    aborting->wsrep_conflict_state);

        mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);
        mysql_mutex_lock(&LOCK_wsrep_rollback);
        continue;
      }
      aborting->wsrep_conflict_state= ABORTING;

      mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);

      aborting->store_globals();

      mysql_mutex_lock(&aborting->LOCK_wsrep_thd);
      wsrep_client_rollback(aborting);
      WSREP_DEBUG("WSREP rollbacker aborted thd: (%lu %llu)",
                  aborting->thread_id, (long long)aborting->real_id);
      mysql_mutex_unlock(&aborting->LOCK_wsrep_thd);

      mysql_mutex_lock(&LOCK_wsrep_rollback);
    }
  }

  mysql_mutex_unlock(&LOCK_wsrep_rollback);
  sql_print_information("WSREP: rollbacker thread exiting");

  DBUG_PRINT("wsrep",("wsrep rollbacker thread exiting"));
  DBUG_VOID_RETURN;
}

void wsrep_create_rollbacker()
{
  if (wsrep_provider && strcasecmp(wsrep_provider, "none"))
  {
    pthread_t hThread;
    /* create rollbacker */
    if (pthread_create( &hThread, &connection_attrib,
                        start_wsrep_THD, (void*)wsrep_rollback_process))
      WSREP_WARN("Can't create thread to manage wsrep rollback");
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
           bf_thd->wsrep_exec_mode == TOTAL_ORDER) )               &&
       victim_thd)
  {
    WSREP_DEBUG("wsrep_abort_thd, by: %llu, victim: %llu", (bf_thd) ?
                (long long)bf_thd->real_id : 0, (long long)victim_thd->real_id);
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
