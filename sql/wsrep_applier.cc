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

  if (thd->killed == THD::KILL_CONNECTION &&
      thd->wsrep_conflict_state != REPLAYING)
  {
    WSREP_INFO("applier has been aborted, skipping apply_rbr: %lld",
               (long long) wsrep_thd_trx_seqno(thd));
    DBUG_RETURN(WSREP_CB_FAILURE);
  }

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  wsrep_thd_set_query_state(thd, QUERY_EXEC);
  if (thd->wsrep_conflict_state!= REPLAYING)
    thd->wsrep_conflict_state= NO_CONFLICT;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  if (!buf_len) WSREP_DEBUG("empty rbr buffer to apply: %lld",
                            (long long) wsrep_thd_trx_seqno(thd));

  while(buf_len)
  {
    int exec_res;
    Log_event* ev= wsrep_read_log_event(&buf, &buf_len,
                                        wsrep_get_apply_format(thd));

    if (!ev)
    {
      WSREP_ERROR("applier could not read binlog event, seqno: %lld, len: %zu",
                  (long long)wsrep_thd_trx_seqno(thd), buf_len);
      rcode= 1;
      goto error;
    }

    switch (ev->get_type_code()) {
    case FORMAT_DESCRIPTION_EVENT:
      wsrep_set_apply_format(thd, (Format_description_log_event*)ev);
      continue;
    case GTID_LOG_EVENT:
    {
      Gtid_log_event* gev= (Gtid_log_event*)ev;
      if (gev->get_gno() == 0)
      {
        /* Skip GTID log event to make binlog to generate LTID on commit */
        delete ev;
        continue;
      }
      break;
    }
    default:
      break;
    }

    thd->server_id = ev->server_id; // use the original server id for logging
    thd->set_time();                // time the query
    wsrep_xid_init(&thd->transaction.xid_state.xid,
                   thd->wsrep_trx_meta.gtid.uuid,
                   thd->wsrep_trx_meta.gtid.seqno);
    thd->lex->current_select= 0;
    if (!ev->when.tv_sec)
      my_micro_time_to_timeval(my_micro_time(), &ev->when);
    ev->thd = thd;
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
    event++;

    if (thd->wsrep_conflict_state!= NO_CONFLICT &&
        thd->wsrep_conflict_state!= REPLAYING)
      WSREP_WARN("conflict state after RBR event applying: %d, %lld",
                 thd->wsrep_query_state, (long long)wsrep_thd_trx_seqno(thd));

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
    case ROWS_QUERY_LOG_EVENT:
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
  wsrep_thd_set_query_state(thd, QUERY_IDLE);
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  assert(thd->wsrep_exec_mode== REPL_RECV);

  if (thd->killed == THD::KILL_CONNECTION)
    WSREP_INFO("applier aborted: %lld", (long long)wsrep_thd_trx_seqno(thd));

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

#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "applying write set %lld: %p, %zu",
           (long long)wsrep_thd_trx_seqno(thd), buf, buf_len);
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "applying write set");
#endif /* WSREP_PROC_INFO */

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

#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "applied write set %lld", (long long)wsrep_thd_trx_seqno(thd));
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "applied write set");
#endif /* WSREP_PROC_INFO */

  if (WSREP_CB_SUCCESS != rcode)
  {
    wsrep_dump_rbr_buf(thd, buf, buf_len);
  }

  TABLE *tmp;
  while ((tmp = thd->temporary_tables))
  {
    WSREP_DEBUG("Applier %lu, has temporary tables: %s.%s",
                  thd->thread_id, 
                  (tmp->s) ? tmp->s->db.str : "void",
                  (tmp->s) ? tmp->s->table_name.str : "void");
      close_temporary_table(thd, tmp, 1, 1);    
  }

  return rcode;
}

static wsrep_cb_status_t wsrep_commit(THD* const thd)
{
#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "committing %lld", (long long)wsrep_thd_trx_seqno(thd));
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "committing");
#endif /* WSREP_PROC_INFO */

  wsrep_cb_status_t const rcode(trans_commit(thd) ?
                                WSREP_CB_FAILURE : WSREP_CB_SUCCESS);

#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "committed %lld", (long long)wsrep_thd_trx_seqno(thd));
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "committed");
#endif /* WSREP_PROC_INFO */

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
#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "rolling back %lld", (long long)wsrep_thd_trx_seqno(thd));
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "rolling back");
#endif /* WSREP_PROC_INFO */

  /* Check for comments in Relay_log_info::cleanup_context */
  trans_rollback_stmt(thd);

  wsrep_cb_status_t const rcode(trans_rollback(thd) ?
                                WSREP_CB_FAILURE : WSREP_CB_SUCCESS);

#ifdef WSREP_PROC_INFO
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info) - 1,
           "rolled back %lld", (long long)wsrep_thd_trx_seqno(thd));
  thd_proc_info(thd, thd->wsrep_info);
#else
  thd_proc_info(thd, "rolled back");
#endif /* WSREP_PROC_INFO */
  thd->wsrep_rli->cleanup_context(thd, 0);
  thd->variables.gtid_next.set_automatic();

  return rcode;
}

wsrep_cb_status_t wsrep_commit_cb(void*         const     ctx,
                                  uint32_t      const     flags,
                                  const wsrep_trx_meta_t* meta,
                                  wsrep_bool_t* const     exit,
                                  bool          const     commit)
{
  THD* const thd((THD*)ctx);

  assert(meta->gtid.seqno == wsrep_thd_trx_seqno(thd));

  wsrep_cb_status_t rcode;

  if (commit)
    rcode = wsrep_commit(thd);
  else
    rcode = wsrep_rollback(thd);

  wsrep_set_apply_format(thd, NULL);
  thd->mdl_context.release_transactional_locks();
  free_root(thd->mem_root,MYF(MY_KEEP_PREALLOC));
  thd->tx_isolation= (enum_tx_isolation) thd->variables.tx_isolation;

  if (wsrep_slave_count_change < 0 && commit && WSREP_CB_SUCCESS == rcode)
  {
    mysql_mutex_lock(&LOCK_wsrep_slave_threads);
    if (wsrep_slave_count_change < 0)
    {
      wsrep_slave_count_change++;
      WSREP_DEBUG("Closing applier thread, to close %d", abs(wsrep_slave_count_change));
      *exit = true;
    }
    mysql_mutex_unlock(&LOCK_wsrep_slave_threads);
  }

  if (thd->wsrep_applier)
  {
    /* From trans_begin() */
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
