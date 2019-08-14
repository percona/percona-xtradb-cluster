/* Copyright (C) 2013-2015 Codership Oy <info@codership.com>

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

#include "wsrep_applier.h"

#include "wsrep_priv.h"
#include "wsrep_binlog.h" // wsrep_dump_rbr_buf()
#include "wsrep_xid.h"

#include "log_event.h" // class THD, EVENT_LEN_OFFSET, etc.
#include "debug_sync.h"

/*
  read the first event from (*buf). The size of the (*buf) is (*buf_len).
  At the end (*buf) is shitfed to point to the following event or NULL and
  (*buf_len) will be changed to account just being read bytes of the 1st event.
*/

static Log_event* wsrep_read_log_event(
    char **arg_buf, size_t *arg_buf_len,
    const Format_description_log_event *description_event)
{
  DBUG_ENTER("wsrep_read_log_event");
  char *head= (*arg_buf);

  uint data_len = uint4korr(head + EVENT_LEN_OFFSET);
  char *buf= (*arg_buf);
  const char *error= 0;
  Log_event *res=  0;

  res= Log_event::read_log_event(buf, data_len, &error, description_event, 0);

  if (!res)
  {
    DBUG_ASSERT(error != 0);
    sql_print_error("Error in Log_event::read_log_event(): "
                    "'%s', data_len: %d, event_type: %d",
                    error,data_len,head[EVENT_TYPE_OFFSET]);
  }
  (*arg_buf)+= data_len;
  (*arg_buf_len)-= data_len;
  DBUG_RETURN(res);
}

#include "transaction.h" // trans_commit(), trans_rollback()
#include "rpl_rli.h"     // class Relay_log_info;
#include "sql_base.h"    // close_temporary_table()

static inline void
wsrep_set_apply_format(THD* thd, Format_description_log_event* ev)
{
  if (thd->wsrep_apply_format)
  {
      delete (Format_description_log_event*)thd->wsrep_apply_format;
  }
  thd->wsrep_apply_format= ev;
}

static inline Format_description_log_event*
wsrep_get_apply_format(THD* thd)
{
  if (thd->wsrep_apply_format)
      return (Format_description_log_event*) thd->wsrep_apply_format;
  return thd->wsrep_rli->get_rli_description_event();
}

static wsrep_cb_status_t wsrep_apply_events(THD*        thd,
                                            const void* events_buf,
                                            size_t      buf_len)
{
  char *buf= (char *)events_buf;
  int rcode= 0;
  int event= 1;

  DBUG_ENTER("wsrep_apply_events");

  DBUG_EXECUTE_IF("wsrep_applier_sleep_15",
  {
    static bool slept_once= false;
    if (!slept_once)
    {
      slept_once= true;
      WSREP_INFO("Applier sleeping 15 sec");
      my_sleep(15000000);
      WSREP_INFO("Applier slept 15 sec");
    }
  });

  if (thd->killed == THD::KILL_CONNECTION &&
      thd->wsrep_conflict_state != REPLAYING)
  {
    WSREP_INFO("Applier aborted. Skipping apply event while processing"
               " write-set: %lld",
               (long long) wsrep_thd_trx_seqno(thd));
    DBUG_RETURN(WSREP_CB_FAILURE);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  thd->wsrep_query_state= QUERY_EXEC;
  if (thd->wsrep_conflict_state!= REPLAYING)
    thd->wsrep_conflict_state= NO_CONFLICT;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  if (!buf_len)
    WSREP_DEBUG("Empty apply event found while processing write-set: %lld",
                (long long) wsrep_thd_trx_seqno(thd));

  while(buf_len)
  {
    int exec_res;
    Log_event* ev= wsrep_read_log_event(&buf, &buf_len,
                                        wsrep_get_apply_format(thd));

    if (!ev)
    {
      WSREP_ERROR("Applier could not read binlog event, seqno: %lld, len: %zu",
                  (long long)wsrep_thd_trx_seqno(thd), buf_len);
      rcode= 1;
      goto error;
    }

    switch (ev->get_type_code()) {
    case binary_log::FORMAT_DESCRIPTION_EVENT:
      wsrep_set_apply_format(thd, (Format_description_log_event*)ev);
      continue;
    /* If gtid_mode=OFF then async master-slave will generate ANONYMOUS GTID. */
    case binary_log::GTID_LOG_EVENT:
    case binary_log::ANONYMOUS_GTID_LOG_EVENT:
    {
      Gtid_log_event* gev= (Gtid_log_event*)ev;
      if (gev->get_gno() == 0)
      {
        /* Skip GTID log event to make binlog to generate LTID on commit */
        delete ev;
        continue;
      }
      /*
         gtid_pre_statement_checks will fail on the subsequent statement
         if the bits below are set. So we don't mark the thd to run in
         transaction mode yet, and assume there will be such a "BEGIN"
         log event that will set those appropriately.
      */
      thd->variables.option_bits&= ~OPTION_BEGIN;
      thd->server_status&= ~SERVER_STATUS_IN_TRANS;
      assert(event == 1);
      break;
    }
    default:
      break;
    }

    thd->server_id = ev->server_id; // use the original server id for logging
    thd->unmasked_server_id = ev->common_header->unmasked_server_id;
    thd->set_time();                // time the query
    wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                   thd->wsrep_trx_meta.gtid.uuid,
                   thd->wsrep_trx_meta.gtid.seqno);
    thd->lex->set_current_select(NULL);
    if (!ev->common_header->when.tv_sec)
      my_micro_time_to_timeval(my_micro_time(), &ev->common_header->when);
    ev->thd = thd; // because up to this point, ev->thd == 0

    set_timespec_nsec(&thd->wsrep_rli->ts_exec[0], 0);
    thd->wsrep_rli->stats_read_time += diff_timespec(&thd->wsrep_rli->ts_exec[0],
                                                     &thd->wsrep_rli->ts_exec[1]);

    /* MySQL slave "Sleeps if needed, and unlocks rli->data_lock."
     * at this point. But this does not apply for wsrep, we just do the unlock part
     * of sql_delay_event()
     *
     * if (sql_delay_event(ev, thd, rli))
     */
    //mysql_mutex_assert_owner(&rli->data_lock);
    //mysql_mutex_unlock(&rli->data_lock);

    exec_res = ev->apply_event(thd->wsrep_rli);
    DBUG_PRINT("info", ("exec_event result: %d", exec_res));

    if (exec_res)
    {
      WSREP_WARN("RBR event %d %s apply warning: %d, %lld",
                 event, ev->get_type_str(), exec_res,
                 (long long) wsrep_thd_trx_seqno(thd));
      rcode= exec_res;
      /* stop processing for the first error */
      delete ev;
      goto error;
    }

    set_timespec_nsec(&thd->wsrep_rli->ts_exec[1], 0);
    thd->wsrep_rli->stats_exec_time += diff_timespec(&thd->wsrep_rli->ts_exec[1],
                                          &thd->wsrep_rli->ts_exec[0]);

    DBUG_PRINT("info", ("wsrep apply_event error = %d", exec_res));
    event++;

    if (thd->wsrep_conflict_state!= NO_CONFLICT &&
        thd->wsrep_conflict_state!= REPLAYING)
      WSREP_WARN("conflict state after RBR event applying: %s, %lld",
                 wsrep_get_query_state(thd->wsrep_query_state),
                 (long long)wsrep_thd_trx_seqno(thd));

    if (thd->wsrep_conflict_state == MUST_ABORT) {
      WSREP_WARN("RBR event apply failed, rolling back: %lld",
                 (long long) wsrep_thd_trx_seqno(thd));
      /* Check for comments in Relay_log_info::cleanup_context */
      trans_rollback_stmt(thd);
      trans_rollback(thd);
      thd->locked_tables_list.unlock_locked_tables(thd);
      /* Release transactional metadata locks. */
      thd->mdl_context.release_transactional_locks();
      thd->wsrep_conflict_state= NO_CONFLICT;
      DBUG_RETURN(WSREP_CB_FAILURE);
    }

    switch (ev->get_type_code()) {
    case binary_log::ROWS_QUERY_LOG_EVENT:
      /*
        Setting binlog_rows_query_log_events to ON will generate
        ROW_QUERY_LOG_EVENT. This event logs an extra information while logging
        row information and so event should be kept infact till ROW_LOG_EVENT
        is processed and should be freed once ROW_LOG_EVENT is done.

        Keeping Rows_log event, it will be needed still, and will be deleted later
        in rli->cleanup_context()
        Also FORMAT_DESCRIPTION_EVENT is needed further, but it skipped from this loop
        by 'continue' above, and thus  avoids the following 'delete ev'
      */
      continue;
    default:
      delete ev;
      break;
    }
  }

 error:
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  thd->wsrep_query_state= QUERY_IDLE;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  assert(thd->wsrep_exec_mode== REPL_RECV);

  if (thd->killed == THD::KILL_CONNECTION)
    WSREP_INFO("applier aborted while processing write-set: %lld",
               (long long)wsrep_thd_trx_seqno(thd));

  if (rcode) DBUG_RETURN(WSREP_CB_FAILURE);
  DBUG_RETURN(WSREP_CB_SUCCESS);
}

wsrep_cb_status_t wsrep_apply_cb(void* const             ctx,
                                 const void* const       buf,
                                 size_t const            buf_len,
                                 uint32_t const          flags,
                                 const wsrep_trx_meta_t* meta)
{
  THD* const thd((THD*)ctx);

  assert(thd->wsrep_apply_toi == false);

// Allow tests to block the applier thread using the DBUG facilities
  DBUG_EXECUTE_IF("sync.wsrep_apply_cb",
                 {
                   const char act[]=
                     "now "
                     "SIGNAL sync.wsrep_apply_cb_reached "
                     "WAIT_FOR signal.wsrep_apply_cb";
                   DBUG_ASSERT(!debug_sync_set_action(thd,
                                                      STRING_WITH_LEN(act)));
                 };);

  thd->wsrep_trx_meta = *meta;

  THD_STAGE_INFO(thd, stage_wsrep_applying_writeset);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: applying write-set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  /* tune FK and UK checking policy */
  if (wsrep_slave_UK_checks == FALSE) 
    thd->variables.option_bits|= OPTION_RELAXED_UNIQUE_CHECKS;
  else
    thd->variables.option_bits&= ~OPTION_RELAXED_UNIQUE_CHECKS;

  if (wsrep_slave_FK_checks == FALSE) 
    thd->variables.option_bits|= OPTION_NO_FOREIGN_KEY_CHECKS;
  else
    thd->variables.option_bits&= ~OPTION_NO_FOREIGN_KEY_CHECKS;

  if (flags & WSREP_FLAG_ISOLATION)
  {
    thd->wsrep_apply_toi= true;
    /*
      Don't run in transaction mode with TOI actions.
     */
    thd->variables.option_bits&= ~OPTION_BEGIN;
    thd->server_status&= ~SERVER_STATUS_IN_TRANS;
  }
  wsrep_cb_status_t rcode(wsrep_apply_events(thd, buf, buf_len));

  THD_STAGE_INFO(thd, stage_wsrep_applied_writeset);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: %s write set (%lld)",
           rcode == WSREP_CB_SUCCESS ? "applied" : "failed to apply",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  if (WSREP_CB_SUCCESS != rcode)
  {
    wsrep_dump_rbr_buf(thd, buf, buf_len);
  }

  TABLE *tmp;
  while ((tmp = thd->temporary_tables))
  {
    WSREP_DEBUG("Applier %u, has temporary tables: %s.%s",
                thd->thread_id(), 
                (tmp->s) ? tmp->s->db.str : "void",
                (tmp->s) ? tmp->s->table_name.str : "void");
    close_temporary_table(thd, tmp, 1, 1);    
  }

  return rcode;
}

static wsrep_cb_status_t wsrep_commit(THD* const thd)
{
  THD_STAGE_INFO(thd, stage_wsrep_committing);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: committing write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  if (!thd->get_transaction()->is_empty(Transaction_ctx::STMT))
  {
    WSREP_INFO("Applier statement commit needed");
    if (trans_commit_stmt(thd))
    {
      WSREP_WARN("Applier statement commit failed");
    }
  }
  wsrep_cb_status_t const rcode(trans_commit(thd) ?
                                WSREP_CB_FAILURE : WSREP_CB_SUCCESS);

  THD_STAGE_INFO(thd, stage_wsrep_committed);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: %s write set (%lld)",
           (rcode == WSREP_CB_SUCCESS ? "committed" : "failed to commit"),
            (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  if (WSREP_CB_SUCCESS == rcode)
  {
    thd->wsrep_rli->cleanup_context(thd, 0);
    thd->variables.gtid_next.set_automatic();
    if (thd->wsrep_apply_toi)
    {
      wsrep_set_SE_checkpoint(thd->wsrep_trx_meta.gtid.uuid,
                              thd->wsrep_trx_meta.gtid.seqno);
    }
  }

  return rcode;
}

static wsrep_cb_status_t wsrep_rollback(THD* const thd)
{
  THD_STAGE_INFO(thd, stage_wsrep_rolling_back);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: rolling back write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  if (!thd->get_transaction()->is_empty(Transaction_ctx::STMT))
  {
    WSREP_INFO("Applier statement rollback needed");
    if (trans_rollback_stmt(thd))
    {
      WSREP_WARN("Applier statement rollback failed");
    }
  }
  wsrep_cb_status_t const rcode(trans_rollback(thd) ?
                                WSREP_CB_FAILURE : WSREP_CB_SUCCESS);

  THD_STAGE_INFO(thd, stage_wsrep_rolled_back);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: %s write set (%lld)",
           rcode == WSREP_CB_SUCCESS ? "rolled back" : "failed to rollback",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  thd->wsrep_rli->cleanup_context(thd, 0);
  thd->variables.gtid_next.set_automatic();

  return rcode;
}

wsrep_cb_status_t wsrep_commit_cb(void*         const     ctx,
                                  const void*             trx_handle,
                                  uint32_t      const     flags,
                                  const wsrep_trx_meta_t* meta,
                                  wsrep_bool_t* const     exit,
                                  bool          const     commit)
{
  THD* const thd((THD*)ctx);

  /* Applier transaction delays entering CommitMonitor so
  cache the needed params that can aid entering CommitMonitor
  post prepare stage.
  Replay of local transaction uses the same path as applying of
  transaction but CommitMonitor protocol is different for it. */
  if (trx_handle && thd->wsrep_conflict_state != REPLAYING)
    thd->wsrep_ws_handle.opaque= const_cast<void*>(trx_handle);

  assert(meta->gtid.seqno == wsrep_thd_trx_seqno(thd));

  wsrep_cb_status_t rcode;

  if (commit)
    rcode = wsrep_commit(thd);
  else
    rcode = wsrep_rollback(thd);

  wsrep_set_apply_format(thd, NULL);
  thd->mdl_context.release_transactional_locks();

  if (thd->wsrep_applier)
  {
    /* This function is meant to initiate a commit after write-set has been
    applied. This generally will be done by applier thread to replicate
    action that some other node has executed.
    There is an exception here: A local action may also follow this route
    of applying write-set followed by commit if local transaction is forced
    to rollback. Galera replay_transaction flow will try to use write-set
    in that case instead of re-running the complete transaction.
    In latter case it is not adviable to free the mem_root as follow-up
    action associated with user thread will need it and so this action
    is executed only when wsrep_applier = true which suggest true applier
    (Note: this flags is set to flase for user thread even though it is using
     write-set to complete the action) */
    free_root(thd->mem_root,MYF(MY_KEEP_PREALLOC));
  }

  thd->tx_isolation= (enum_tx_isolation) thd->variables.tx_isolation;

  if (wsrep_slave_count_change < 0 && commit && WSREP_CB_SUCCESS == rcode)
  {
    mysql_mutex_lock(&LOCK_wsrep_slave_threads);
    if (wsrep_slave_count_change < 0)
    {
      wsrep_slave_count_change++;
      WSREP_DEBUG("Closing applier thread(s). Yet to close %d",
                  abs(wsrep_slave_count_change));
      *exit = true;
    }
    mysql_mutex_unlock(&LOCK_wsrep_slave_threads);
  }

  if (thd->wsrep_applier)
  {
    /* From trans_begin(). Also check the comment at line:134 when/why these bits are unset. */
    thd->variables.option_bits|= OPTION_BEGIN;
    thd->server_status|= SERVER_STATUS_IN_TRANS;
    thd->wsrep_apply_toi= false;
  }

  return rcode;
}


wsrep_cb_status_t wsrep_unordered_cb(void*       const ctx,
                                     const void* const data,
                                     size_t      const size)
{
    return WSREP_CB_SUCCESS;
}
