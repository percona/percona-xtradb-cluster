/* Copyright 2008 Codership Oy <http://www.codership.com>

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

#include <mysqld.h>
#include <sql_class.h>
#include "wsrep_mysqld.h"
#include "wsrep_priv.h"
#include <cstdio>
#include <cstdlib>

extern handlerton *binlog_hton;
extern int binlog_close_connection(handlerton *hton, THD *thd);
extern ulonglong thd_to_trx_id(THD *thd);

extern "C" int thd_binlog_format(const MYSQL_THD thd); 
// todo: share interface with ha_innodb.c 

enum wsrep_trx_status wsrep_run_wsrep_commit(THD *thd, handlerton *hton, bool all);

/*
  a post-commit cleanup on behalf of wsrep. Can't be a part of hton struct.
  Is called by THD::transactions.cleanup()
*/
void wsrep_cleanup_transaction(THD *thd)
{
  if (thd->wsrep_exec_mode == LOCAL_COMMIT)
  {
    if (thd->variables.wsrep_on &&
        thd->wsrep_conflict_state != MUST_REPLAY &&
        thd_sql_command((const THD*) thd) != SQLCOM_SELECT)
    {
      if (wsrep->post_commit(wsrep, &thd->wsrep_trx_handle))
      {
        DBUG_PRINT("wsrep", ("set committed fail"));
        WSREP_WARN("set committed fail: %lu %d", 
		   thd->real_id, thd->stmt_da->status());
      }
      thd_binlog_trx_reset(thd);
    }
    thd->wsrep_exec_mode= LOCAL_STATE;
  }
}

/*
  wsrep hton
*/
handlerton *wsrep_hton;

/*
  wsrep exploits binlog's caches even if binlogging itself is not 
  activated. In such case connection close needs calling
  actual binlog's method.
  Todo: split binlog hton from its caches to use ones by wsrep
  without referring to binlog's stuff.
*/
static int
wsrep_close_connection(handlerton*  hton, THD* thd)
{
  DBUG_ENTER("wsrep_close_connection");
  if (thd_get_ha_data(thd, binlog_hton) != NULL)
		binlog_hton->close_connection (binlog_hton, thd);
  DBUG_RETURN(0);
} 

/*
  prepare/wsrep_run_wsrep_commit can fail in two ways
  - certification test or an equivalent. As a result,
    the current transaction just rolls back
    Error codes:
           WSREP_TRX_ROLLBACK, WSREP_TRX_ERROR
  - a post-certification failure makes this server unable to
    commit its own WS and therefore the server must abort
*/
static int wsrep_prepare(handlerton *hton, THD *thd, bool all)
{
  DBUG_ENTER("wsrep_prepare");
  if ((all || 
      !thd_test_options(thd, OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN)) &&
      (thd->variables.wsrep_on && thd_sql_command((const THD*) thd) != SQLCOM_SELECT))
  {
    switch (wsrep_run_wsrep_commit(thd, hton, all))
    {
    case WSREP_TRX_OK:
      break;
    case WSREP_TRX_ROLLBACK:
    case WSREP_TRX_ERROR:
      DBUG_RETURN(1);
    }
  }
  DBUG_RETURN(0);
}

static int wsrep_savepoint_set(handlerton *hton, THD *thd,  void *sv)
{
  return 0;
}
static int wsrep_savepoint_rollback(handlerton *hton, THD *thd, void *sv)
{
  return 0;
}

static int wsrep_rollback(handlerton *hton, THD *thd, bool all)
{
  DBUG_ENTER("wsrep_rollback");
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  if ((all || !thd_test_options(thd, OPTION_NOT_AUTOCOMMIT | OPTION_BEGIN)) &&
      (thd->variables.wsrep_on && thd->wsrep_conflict_state != MUST_REPLAY))
  {
    if (wsrep->post_rollback(wsrep, &thd->wsrep_trx_handle))
    {
      DBUG_PRINT("wsrep", ("setting rollback fail"));
      WSREP_ERROR("settting rollback fail: %llu", thd_to_trx_id(thd));
    }
  }
  thd_binlog_trx_reset(thd); // @TODO: move into wsrep engine rollback
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  DBUG_RETURN(0);
}

int wsrep_commit(handlerton *hton, THD *thd, bool all)
{
  DBUG_ENTER("wsrep_commit");
  
  DBUG_RETURN(0);
}

enum wsrep_trx_status
wsrep_run_wsrep_commit(
    THD *thd, handlerton *hton, bool all)
{
  int rcode         = -1;
  uint data_len     = 0;
  uchar *rbr_data   = NULL;
  IO_CACHE *cache;

  if (thd->stmt_da->is_error()) {
    WSREP_ERROR("commit issue, error: %d %s", 
                thd->stmt_da->sql_errno(), thd->stmt_da->message());
  }

  DBUG_ENTER("wsrep_run_wsrep_commit");
  if (thd->wsrep_exec_mode == REPL_RECV) {

    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    if (thd->wsrep_conflict_state == MUST_ABORT) {
      if (wsrep_debug)
	WSREP_INFO("WSREP: must abort for BF");
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
  if (thd->wsrep_exec_mode != LOCAL_STATE) {
    DBUG_RETURN(WSREP_TRX_OK);
  }

  DBUG_PRINT("wsrep", ("replicating commit"));

  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  if (thd->wsrep_conflict_state == MUST_ABORT) {
    DBUG_PRINT("wsrep", ("replicate commit fail"));
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    if (wsrep_debug) {
      WSREP_INFO("innobase_commit, abort %s",
		 (thd->query()) ? thd->query() : "void");
    }
    DBUG_RETURN(WSREP_TRX_ROLLBACK);
  }

  mysql_mutex_lock(&LOCK_wsrep_replaying);

  while (wsrep_replaying > 0 && thd->wsrep_conflict_state == NO_CONFLICT) {

    mysql_mutex_unlock(&LOCK_wsrep_replaying);
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

    mysql_mutex_lock(&thd->mysys_var->mutex);
    thd_proc_info(thd, "wsrep waiting on replaying");
    thd->mysys_var->current_mutex= &LOCK_wsrep_replaying;
    thd->mysys_var->current_cond=  &COND_wsrep_replaying;
    mysql_mutex_unlock(&thd->mysys_var->mutex);

    mysql_mutex_lock(&LOCK_wsrep_replaying);
    // Using timedwait is a hack to avoid deadlock in case if BF victim
    // misses the signal.
    struct timespec wtime = {0, 1000000};
    mysql_cond_timedwait(&COND_wsrep_replaying, &LOCK_wsrep_replaying,
                           &wtime);
    WSREP_DEBUG("commit waiting for replaying: %d, thd: (%lu) conflict: %d", 
		wsrep_replaying, thd->thread_id, thd->wsrep_conflict_state);

    mysql_mutex_unlock(&LOCK_wsrep_replaying);

    mysql_mutex_lock(&thd->mysys_var->mutex);
    thd->mysys_var->current_mutex= 0;
    thd->mysys_var->current_cond=  0;
    mysql_mutex_unlock(&thd->mysys_var->mutex);

    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    mysql_mutex_lock(&LOCK_wsrep_replaying);
  }
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  if (thd->wsrep_conflict_state == MUST_ABORT) {
    DBUG_PRINT("wsrep", ("replicate commit fail"));
    thd->wsrep_conflict_state = ABORTED;
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    WSREP_DEBUG("innobase_commit abort after replaying wait %s",
		(thd->query()) ? thd->query() : "void");
    DBUG_RETURN(WSREP_TRX_ROLLBACK);
  }  thd->wsrep_query_state = QUERY_COMMITTING;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  cache = get_trans_log(thd);
  rcode = 0;
  if (cache) {
    thd->binlog_flush_pending_rows_event(true);
    rcode = wsrep_write_cache(cache, &rbr_data, &data_len);
    if (rcode) {
      WSREP_ERROR("rbr write fail, data_len: %d, %d", 
		  data_len, rcode);
      if (data_len) my_free(rbr_data);
      DBUG_RETURN(WSREP_TRX_ROLLBACK);
    }

    if (thd_binlog_format(thd) != BINLOG_FORMAT_ROW) {
      if (data_len > 0) {
	data_len = 0;
	my_free(rbr_data);
	rbr_data = NULL;
      }
    }
  }
  if (!rcode) {
    rcode = wsrep->pre_commit(
			      wsrep,
			      (wsrep_conn_id_t)thd->thread_id,
			      &thd->wsrep_trx_handle,
			      rbr_data,
			      data_len,
			      &thd->wsrep_trx_seqno);
    if (rcode == WSREP_TRX_MISSING) {
      rcode = WSREP_OK;
    } else if (rcode == WSREP_BF_ABORT) {
      mysql_mutex_lock(&thd->LOCK_wsrep_thd);
      thd->wsrep_conflict_state = MUST_REPLAY;
      mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
      mysql_mutex_lock(&LOCK_wsrep_replaying);
      wsrep_replaying++;
      WSREP_DEBUG("replaying increased: %d, thd: %lu", 
		  wsrep_replaying, thd->thread_id);
      mysql_mutex_unlock(&LOCK_wsrep_replaying);
    }
  } else {
    const char *errmsg = 
      "I/O error reading from thd's binlog iocache";
    WSREP_ERROR("%s, errno=%d, io cache code=%d", 
		    errmsg, my_errno, cache->error);
    if (data_len) my_free(rbr_data);
    DBUG_ASSERT(0); // failure like this can not normally happen
    DBUG_RETURN(WSREP_TRX_ERROR);
  }

  if (data_len) {
    my_free(rbr_data);
  }

  switch(rcode) {
  case 0:
    thd->wsrep_exec_mode = LOCAL_COMMIT;
    DBUG_PRINT("wsrep", ("replicating commit success"));
    break;
  case WSREP_TRX_FAIL:
  case WSREP_BF_ABORT:
    WSREP_DEBUG("commit failed for reason: %d", rcode);
    DBUG_PRINT("wsrep", ("replicating commit fail"));

    mysql_mutex_lock(&thd->LOCK_wsrep_thd);
    thd->wsrep_query_state= QUERY_EXEC;

    if (thd->wsrep_conflict_state == MUST_ABORT) {
      thd->wsrep_conflict_state= ABORTED;
    }
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

    DBUG_RETURN(WSREP_TRX_ROLLBACK);

  case WSREP_CONN_FAIL:
    WSREP_ERROR("connection failure");
    DBUG_RETURN(WSREP_TRX_ERROR);
  default:
    WSREP_ERROR("unknown connection failure");
    DBUG_RETURN(WSREP_TRX_ERROR);
  }
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  thd->wsrep_query_state= QUERY_EXEC;
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_RETURN(WSREP_TRX_OK);
}


static int wsrep_hton_init(void *p)
{
  wsrep_hton= (handlerton *)p;
  wsrep_hton->state=opt_bin_log ? SHOW_OPTION_YES : SHOW_OPTION_NO;
  wsrep_hton->db_type=DB_TYPE_WSREP;
  wsrep_hton->savepoint_offset= sizeof(my_off_t);
  wsrep_hton->close_connection= wsrep_close_connection;
  wsrep_hton->savepoint_set= wsrep_savepoint_set;
  wsrep_hton->savepoint_rollback= wsrep_savepoint_rollback;
  wsrep_hton->commit= wsrep_commit;
  wsrep_hton->rollback= wsrep_rollback;
  wsrep_hton->prepare= wsrep_prepare;
  wsrep_hton->flags= HTON_NOT_USER_SELECTABLE | HTON_HIDDEN; // todo: fix flags
  return 0;
}



struct st_mysql_storage_engine wsrep_storage_engine=
{ MYSQL_HANDLERTON_INTERFACE_VERSION };


mysql_declare_plugin(wsrep)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &wsrep_storage_engine,
  "wsrep",
  "Codership Oy",
  "A pseudo storage engine to represent transactions in multi-master synchornous replication",
  PLUGIN_LICENSE_GPL,
  wsrep_hton_init, /* Plugin Init */
  NULL, /* Plugin Deinit */
  0x0100 /* 1.0 */,
  NULL,                       /* status variables                */
  NULL,                       /* system variables                */
  NULL                        /* config options                  */
}
mysql_declare_plugin_end;
