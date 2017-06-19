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

#include <mysqld.h>
#include <sql_base.h>
#include <sql_class.h>
#include <sql_parse.h>
#include "wsrep_priv.h"
#include "wsrep_thd.h"
#include "wsrep_sst.h"
#include "wsrep_utils.h"
#include "wsrep_var.h"
#include "wsrep_binlog.h"
#include "wsrep_applier.h"
#include "wsrep_xid.h"
#include <cstdio>
#include <cstdlib>
#include "log_event.h"
#include <rpl_slave.h>
#include "sql_base.h"		// TEMP_PREFIX 
#include "rpl_msr.h"           // channel_map
#ifdef HAVE_PSI_INTERFACE
#include <vector>
#include <map>
#include "pthread.h"
#include "mysql/psi/mysql_file.h"
#endif /* HAVE_PSI_INTERFACE */

#include "debug_sync.h"

wsrep_t *wsrep                  = NULL;
my_bool wsrep_emulate_bin_log   = FALSE; // activating parts of binlog interface
/* Sidno in global_sid_map corresponding to group uuid */
rpl_sidno wsrep_sidno= -1;
my_bool wsrep_preordered_opt= FALSE;

/* If set to true the said set statement is replicated across the cluster
using TOI */
bool     wsrep_replicate_set_stmt = false;

/*
 * Begin configuration options and their default values
 */

const char* wsrep_data_home_dir = NULL;
const char* wsrep_dbug_option   = "";

long    wsrep_slave_threads            = 1; // # of slave action appliers wanted
int     wsrep_slave_count_change       = 0; // # of appliers to stop or start
my_bool wsrep_debug                    = 0; // enable debug level logging
my_bool wsrep_convert_LOCK_to_trx      = 1; // convert locking sessions to trx
ulong   wsrep_retry_autocommit         = 5; // retry aborted autocommit trx
my_bool wsrep_auto_increment_control   = 1; // control auto increment variables
my_bool wsrep_drupal_282555_workaround = 1; // retry autoinc insert after dupkey
my_bool wsrep_incremental_data_collection = 0; // incremental data collection
ulong   wsrep_max_ws_size              = 1073741824UL;//max ws (RBR buffer) size
ulong   wsrep_max_ws_rows              = 65536; // max number of rows in ws
int     wsrep_to_isolation             = 0; // # of active TO isolation threads
my_bool wsrep_certify_nonPK            = 1; // certify, even when no primary key
long    wsrep_max_protocol_version     = 3; // maximum protocol version to use
ulong   wsrep_forced_binlog_format     = BINLOG_FORMAT_UNSPEC;
my_bool wsrep_recovery                 = 0; // recovery
my_bool wsrep_log_conflicts            = 0;
my_bool wsrep_desync                   = 0; // desynchronize the node from the
                                            // cluster
my_bool wsrep_load_data_splitting      = 1; // commit load data every 10K intervals
my_bool wsrep_restart_slave            = 0; // should mysql slave thread be
                                            // restarted, if node joins back
my_bool wsrep_restart_slave_activated  = 0; // node has dropped, and slave
                                            // restart will be needed
my_bool wsrep_slave_UK_checks          = 0; // slave thread does UK checks
my_bool wsrep_slave_FK_checks          = 0; // slave thread does FK checks

/* pxc-strict-mode help control behavior of experimental features like
myisam table replication, etc... */
ulong   pxc_strict_mode                = PXC_STRICT_MODE_ENFORCING;

/* pxc-maint-mode help put node in maintenance mode so that proxysql
can stop diverting queries to this node. */
ulong   pxc_maint_mode                = PXC_MAINT_MODE_DISABLED;
/* sleep for this period before delivering shutdown signal. */
ulong   pxc_maint_transition_period   = 30;

/* enables PXC SSL auto-config */
my_bool pxc_encrypt_cluster_traffic   = 0;

/*
 * End configuration options
 */

static wsrep_uuid_t cluster_uuid = WSREP_UUID_UNDEFINED;
static char         cluster_uuid_str[40]= { 0, };
static const char*  cluster_status_str[WSREP_VIEW_MAX] =
{
    "Primary",
    "non-Primary",
    "Disconnected"
};

static char provider_name[256]= { 0, };
static char provider_version[256]= { 0, };
static char provider_vendor[256]= { 0, };

/*
 * wsrep status variables
 */
my_bool     wsrep_connected          = FALSE;
my_bool     wsrep_ready              = FALSE; // node can accept queries
const char* wsrep_cluster_state_uuid = cluster_uuid_str;
long long   wsrep_cluster_conf_id    = WSREP_SEQNO_UNDEFINED;
const char* wsrep_cluster_status = cluster_status_str[WSREP_VIEW_DISCONNECTED];
long        wsrep_cluster_size       = 0;
long        wsrep_local_index        = -1;
long long   wsrep_local_bf_aborts    = 0;
const char* wsrep_provider_name      = provider_name;
const char* wsrep_provider_version   = provider_version;
const char* wsrep_provider_vendor    = provider_vendor;
/* End wsrep status variables */

wsrep_uuid_t     local_uuid   = WSREP_UUID_UNDEFINED;
wsrep_seqno_t    local_seqno  = WSREP_SEQNO_UNDEFINED;
wsp::node_status local_status;
long             wsrep_protocol_version = 3;

// Boolean denoting if server is in initial startup phase. This is needed
// to make sure that main thread waiting in wsrep_sst_wait() is signaled
// if there was no state gap on receiving first view event.
static my_bool   wsrep_startup = TRUE;

#ifdef HAVE_PSI_INTERFACE

/* Keys for mutexes and condition variables in galera library space. */
PSI_mutex_key
  key_LOCK_galera_cert,
  key_LOCK_galera_stats,
  key_LOCK_galera_dummy_gcs,
  key_LOCK_galera_service_thd,
  key_LOCK_galera_ist_receiver,

  key_LOCK_galera_local_monitor,
  key_LOCK_galera_apply_monitor,
  key_LOCK_galera_commit_monitor,
  key_LOCK_galera_async_sender_monitor,
  key_LOCK_galera_ist_receiver_monitor,

  key_LOCK_galera_sst,
  key_LOCK_galera_incoming,
  key_LOCK_galera_saved_state,
  key_LOCK_galera_trx_handle,
  key_LOCK_galera_wsdb_trx,
  key_LOCK_galera_wsdb_conn,
  key_LOCK_galera_gu_dbug_sync,
  key_LOCK_galera_profile,
  key_LOCK_galera_gcache,
  key_LOCK_galera_protstack,
  key_LOCK_galera_prodcons,
  key_LOCK_galera_gcommconn,
  key_LOCK_galera_recvbuf,
  key_LOCK_galera_mempool;

/* Sequence here should match with tag name sequence specified in wsrep_api.h */
PSI_mutex_info       all_galera_mutexes[]=
{
  { &key_LOCK_galera_cert, "LOCK_galera_cert", 0},
  { &key_LOCK_galera_stats, "LOCK_galera_stats", 0},
  { &key_LOCK_galera_dummy_gcs, "LOCK_galera_dummy_gcs", 0},
  { &key_LOCK_galera_service_thd, "LOCK_galera_service_thd", 0},
  { &key_LOCK_galera_ist_receiver, "LOCK_galera_ist_receiver", 0},
  { &key_LOCK_galera_local_monitor, "LOCK_galera_local_monitor", 0},
  { &key_LOCK_galera_apply_monitor, "LOCK_galera_apply_monitor", 0},
  { &key_LOCK_galera_commit_monitor, "LOCK_galera_commit_monitor", 0},
  { &key_LOCK_galera_async_sender_monitor, "LOCK_galera_async_sender_monitor", 0},
  { &key_LOCK_galera_ist_receiver_monitor, "LOCK_galera_ist_receiver_monitor", 0},
  { &key_LOCK_galera_sst, "LOCK_galera_sst", 0},
  { &key_LOCK_galera_incoming, "LOCK_galera_incoming", 0},
  { &key_LOCK_galera_saved_state, "LOCK_galera_saved_state", 0},
  { &key_LOCK_galera_trx_handle, "LOCK_galera_trx_handle", 0},
  { &key_LOCK_galera_wsdb_trx, "LOCK_galera_wsdb", 0},
  { &key_LOCK_galera_wsdb_conn, "LOCK_galera_wsdb_conn", 0},
  { &key_LOCK_galera_profile, "LOCK_galera_profile", 0},
  { &key_LOCK_galera_gcache, "LOCK_galera_gcache", 0},
  { &key_LOCK_galera_protstack, "LOCK_galera_protstack", 0},
  { &key_LOCK_galera_prodcons, "LOCK_galera_prodcons", 0},
  { &key_LOCK_galera_gcommconn, "LOCK_galera_gcommconn", 0},
  { &key_LOCK_galera_recvbuf, "LOCK_galera_recvbuf", 0},
  { &key_LOCK_galera_mempool, "LOCK_galera_mempool", 0}
};

PSI_cond_key
  key_COND_galera_dummy_gcs,
  key_COND_galera_service_thd,
  key_COND_galera_service_thd_flush,
  key_COND_galera_ist_receiver,
  key_COND_galera_ist_consumer,
  key_COND_galera_monitor_process1,
  key_COND_galera_monitor_process2,

  key_COND_galera_local_monitor,
  key_COND_galera_apply_monitor,
  key_COND_galera_commit_monitor,
  key_COND_galera_async_sender_monitor,
  key_COND_galera_ist_receiver_monitor,

  key_COND_galera_sst,
  key_COND_galera_gu_dbug_sync,
  key_COND_galera_prodcons,
  key_COND_galera_gcache,
  key_COND_galera_recvbuf;

PSI_cond_info       all_galera_condvars[]=
{
  { &key_COND_galera_dummy_gcs, "COND_galera_dummy_gcs", 0},
  { &key_COND_galera_service_thd, "COND_galera_service_thd", 0},
  { &key_COND_galera_service_thd_flush, "COND_galera_service_thd_flush", 0},
  { &key_COND_galera_ist_receiver, "COND_galera_ist_receiver", 0},
  { &key_COND_galera_ist_consumer, "COND_galera_ist_consumer", 0},
  { &key_COND_galera_local_monitor, "COND_galera_local_monitor", 0},
  { &key_COND_galera_apply_monitor, "COND_galera_apply_monitor", 0},
  { &key_COND_galera_commit_monitor, "COND_galera_commit_monitor", 0},
  { &key_COND_galera_async_sender_monitor, "COND_galera_async_sender_monitor", 0},
  { &key_COND_galera_ist_receiver_monitor, "COND_galera_ist_receiver_monitor", 0},
  { &key_COND_galera_sst, "COND_galera_sst", 0},
  { &key_COND_galera_prodcons, "COND_galera_prodcons", 0},
  { &key_COND_galera_gcache, "COND_galera_gcache", 0},
  { &key_COND_galera_recvbuf, "COND_galera_recvbuf", 0}
};

PSI_thread_key
  key_THREAD_galera_service_thd,
  key_THREAD_galera_ist_receiver,
  key_THREAD_galera_ist_async_sender,
  key_THREAD_galera_writeset_checksum,
  key_THREAD_galera_gcache_removefile,
  key_THREAD_galera_receiver,
  key_THREAD_galera_gcommconn;

PSI_thread_info       all_galera_threads[]=
{
  { &key_THREAD_galera_service_thd, "THREAD_galera_service_thd", 0},
  { &key_THREAD_galera_ist_receiver, "THREAD_galera_ist_receiver", 0},
  { &key_THREAD_galera_ist_async_sender, "THREAD_galera_ist_async_sender", 0},
  { &key_THREAD_galera_writeset_checksum, "THREAD_galera_writeset_checksum", 0},
  { &key_THREAD_galera_gcache_removefile, "THREAD_galera_gcache_removefile", 0},
  { &key_THREAD_galera_receiver, "THREAD_galera_receiver", 0},
  { &key_THREAD_galera_gcommconn, "THREAD_galera_gcommconn", 0}
};

PSI_file_key
  key_FILE_galera_recordset,
  key_FILE_galera_ringbuffer,
  key_FILE_galera_gcache_page,
  key_FILE_galera_grastate,
  key_FILE_galera_gvwstate;

PSI_file_info       all_galera_files[]=
{
  { &key_FILE_galera_recordset, "FILE_galera_recordset", 0},
  { &key_FILE_galera_ringbuffer, "FILE_galera_ringbuffer", 0},
  { &key_FILE_galera_gcache_page, "FILE_galera_gcache_page", 0},
  { &key_FILE_galera_grastate, "FILE_galera_grastate", 0},
  { &key_FILE_galera_gvwstate, "FILE_galera_gvwstate", 0}
};

/* Vector to cache PSI key and mutex for corresponding galera mutex. */
typedef std::vector<void*> wsrep_psi_key_vec_t;
static wsrep_psi_key_vec_t wsrep_psi_key_vec;

/*!
 * @brief a callback to create PFS instrumented mutex/condition variables
 *
 *
 * @param type          mutex or condition variable
 * @param ops           add/init or remove/destory mutex/condition variable
 * @param tag           tag/name of instrument to monitor
 * @param value         created mutex or condition variable
 * @param alliedvalue   allied value for supporting operation.
                        for example: while waiting for cond-var corresponding
                        mutex is passes through this variable.
 * @param ts      time to wait for condition.
 */
static void wsrep_pfs_instr_cb(
    wsrep_pfs_instr_type_t        type,
    wsrep_pfs_instr_ops_t         ops,
    wsrep_pfs_instr_tag_t         tag,
    void**                        value __attribute__((unused)),
    void**                        alliedvalue __attribute__((unused)),
    const void*                   ts __attribute__((unused)))
{
  DBUG_ASSERT(!wsrep_psi_key_vec.empty());

  if (type == WSREP_PFS_INSTR_TYPE_MUTEX)
  {
    switch (ops)
    {
    case WSREP_PFS_INSTR_OPS_INIT:
    {
      PSI_mutex_key* key=
        reinterpret_cast<PSI_mutex_key*>(wsrep_psi_key_vec[tag]);

      mysql_mutex_t* mutex= NULL;
      mutex= (mysql_mutex_t*) my_malloc(
        PSI_NOT_INSTRUMENTED, sizeof(mysql_mutex_t), MYF(0));
      mysql_mutex_init(*key, mutex, MY_MUTEX_INIT_FAST);

      /* Begin a structure and m_mutex is first element this
      should hold true. To make this appear therotically good
      we could use map but that comes at cost of map operation
      and mutex as STL map are not thread safe. */
      assert (reinterpret_cast<void*>(mutex) ==
              reinterpret_cast<void*>(&(mutex->m_mutex)));

      *value= &(mutex->m_mutex);

      break;
    }

    case WSREP_PFS_INSTR_OPS_DESTROY:
    {
      mysql_mutex_t* mutex= reinterpret_cast<mysql_mutex_t*>(*value);
      assert(mutex != NULL);

      mysql_mutex_destroy(mutex);
      my_free(mutex);
      *value= NULL;

      break;
    }

    case WSREP_PFS_INSTR_OPS_LOCK:
    {
      mysql_mutex_t* mutex= reinterpret_cast<mysql_mutex_t*>(*value);
      assert(mutex != NULL);

      mysql_mutex_lock(mutex);

      break;
    }

    case WSREP_PFS_INSTR_OPS_UNLOCK:
    {
      mysql_mutex_t* mutex= reinterpret_cast<mysql_mutex_t*>(*value);
      assert(mutex != NULL);

      mysql_mutex_unlock(mutex);

      break;
    }

    default:
      assert(0);
      break;
    }
  }
  else if (type == WSREP_PFS_INSTR_TYPE_CONDVAR)
  {
    switch (ops)
    {
    case WSREP_PFS_INSTR_OPS_INIT:
    {
      PSI_cond_key* key=
        reinterpret_cast<PSI_cond_key*>(wsrep_psi_key_vec[tag]);

      mysql_cond_t* cond= NULL;
      cond= (mysql_cond_t*) my_malloc(
        PSI_NOT_INSTRUMENTED, sizeof(mysql_cond_t), MYF(0));
      mysql_cond_init(*key, cond);

      /* Begin a structure and m_cond is first element this
      should hold true. To make this appear therotically good
      we could use map but that comes at cost of map operation
      and mutex as STL map are not thread safe. */
      assert (reinterpret_cast<void*>(cond) ==
              reinterpret_cast<void*>(&(cond->m_cond)));

      *value= &(cond->m_cond);
      break;
    }

    case WSREP_PFS_INSTR_OPS_DESTROY:
    {
      mysql_cond_t* cond= reinterpret_cast<mysql_cond_t*>(*value);
      assert(cond != NULL);

      mysql_cond_destroy(cond);
      my_free(cond);
      *value= NULL;

      break;
    }

    case WSREP_PFS_INSTR_OPS_WAIT:
    {
      mysql_cond_t* cond= reinterpret_cast<mysql_cond_t*>(*value);
      mysql_mutex_t* mutex= reinterpret_cast<mysql_mutex_t*>(*alliedvalue);
      assert(cond != NULL);

      mysql_cond_wait(cond, mutex);

      break;
    }

    case WSREP_PFS_INSTR_OPS_TIMEDWAIT:
    {
      mysql_cond_t* cond= reinterpret_cast<mysql_cond_t*>(*value);
      mysql_mutex_t* mutex= reinterpret_cast<mysql_mutex_t*>(*alliedvalue);
      const timespec* wtime = reinterpret_cast<const timespec*>(ts);
      assert(cond != NULL && mutex != NULL);

      mysql_cond_timedwait(cond, mutex, wtime);

      break;
    }

    case WSREP_PFS_INSTR_OPS_SIGNAL:
    {
      mysql_cond_t* cond= reinterpret_cast<mysql_cond_t*>(*value);
      assert(cond != NULL);

      mysql_cond_signal(cond);

      break;
    }

    case WSREP_PFS_INSTR_OPS_BROADCAST:
    {
      mysql_cond_t* cond= reinterpret_cast<mysql_cond_t*>(*value);
      assert(cond != NULL);

      mysql_cond_broadcast(cond);

      break;
    }

    default:
      assert(0);
      break;
    }
  }
  else if (type == WSREP_PFS_INSTR_TYPE_THREAD)
  {
    switch (ops)
    {
    case WSREP_PFS_INSTR_OPS_INIT:
    {
      PSI_thread_key* key=
        reinterpret_cast<PSI_thread_key*>(wsrep_psi_key_vec[tag]);

      wsrep_pfs_register_thread(*key);
      break;
    }

    case WSREP_PFS_INSTR_OPS_DESTROY:
    {
      wsrep_pfs_delete_thread();
      break;
    }

    default:
      assert(0);
      break;
    }
  }
  else if (type == WSREP_PFS_INSTR_TYPE_FILE)
  {
    switch(ops)
    {
    case WSREP_PFS_INSTR_OPS_CREATE:
    {
      PSI_file_key* key=
        reinterpret_cast<PSI_thread_key*>(wsrep_psi_key_vec[tag]);

      File* fd= reinterpret_cast<File*> (*value);
      const char* name= reinterpret_cast<const char*> (ts);

      PSI_file_locker_state   state;
      struct PSI_file_locker* locker = NULL;

      wsrep_register_pfs_file_open_begin(
                &state, locker, *key, PSI_FILE_CREATE,
                name, __FILE__, __LINE__);

      wsrep_register_pfs_file_open_end(locker, *fd);

      break;
    }

    case WSREP_PFS_INSTR_OPS_OPEN:
    {
      PSI_file_key* key= 
        reinterpret_cast<PSI_thread_key*>(wsrep_psi_key_vec[tag]);
      
      File* fd= reinterpret_cast<File*> (*value);
      const char* name= reinterpret_cast<const char*> (ts);

      PSI_file_locker_state   state;
      struct PSI_file_locker* locker = NULL;

      wsrep_register_pfs_file_open_begin(
                &state, locker, *key, PSI_FILE_OPEN,
                name, __FILE__, __LINE__);

      wsrep_register_pfs_file_open_end(locker, *fd);

      break;
    }

    case WSREP_PFS_INSTR_OPS_CLOSE:
    {
      File* fd= reinterpret_cast<File*> (*value);

      PSI_file_locker_state   state;
      struct PSI_file_locker* locker = NULL;

      wsrep_register_pfs_file_io_begin(
                &state, locker, *fd, 0, PSI_FILE_CLOSE,
                __FILE__, __LINE__);

      wsrep_register_pfs_file_io_end(locker, 0);

      break;
    }

    case WSREP_PFS_INSTR_OPS_DELETE:
    {
      PSI_file_key* key=
        reinterpret_cast<PSI_thread_key*>(wsrep_psi_key_vec[tag]);

      PSI_file_locker_state   state;
      struct PSI_file_locker* locker = NULL;
      const char* name= reinterpret_cast<const char*> (ts);

      wsrep_register_pfs_file_close_begin(
                &state, locker, *key, PSI_FILE_DELETE,
                name, __FILE__, __LINE__);

      wsrep_register_pfs_file_close_end(locker, 0);

      break;
    }

    default:
      assert(0);
      break;
    }
  }
}
#endif /* HAVE_PSI_INTERFACE */

static void wsrep_log_cb(wsrep_log_level_t level, const char *msg) {
  switch (level) {
  case WSREP_LOG_INFO:
    sql_print_information("WSREP: %s", msg);
    break;
  case WSREP_LOG_WARN:
    sql_print_warning("WSREP: %s", msg);
    break;
  case WSREP_LOG_ERROR:
  case WSREP_LOG_FATAL:
    sql_print_error("WSREP: %s", msg);
    break;
  case WSREP_LOG_DEBUG:
    if (wsrep_debug) sql_print_information ("[Debug] WSREP: %s", msg);
  default:
    break;
  }
}

static void wsrep_log_states (wsrep_log_level_t   const level,
                              const wsrep_uuid_t* const group_uuid,
                              wsrep_seqno_t       const group_seqno,
                              const wsrep_uuid_t* const node_uuid,
                              wsrep_seqno_t       const node_seqno)
{
  char uuid_str[37];
  char msg[256];

  wsrep_uuid_print (group_uuid, uuid_str, sizeof(uuid_str));
  snprintf (msg, 255, "WSREP: Group state: %s:%lld",
            uuid_str, (long long)group_seqno);
  wsrep_log_cb (level, msg);

  wsrep_uuid_print (node_uuid, uuid_str, sizeof(uuid_str));
  snprintf (msg, 255, "WSREP: Local state: %s:%lld",
            uuid_str, (long long)node_seqno);
  wsrep_log_cb (level, msg);
}

void wsrep_init_sidno(const wsrep_uuid_t& wsrep_uuid)
{
  /* generate new Sid map entry from inverted uuid */
  rpl_sid sid;
  wsrep_uuid_t ltid_uuid;

  for (size_t i= 0; i < sizeof(ltid_uuid.data); ++i)
  {
      ltid_uuid.data[i] = ~wsrep_uuid.data[i];
  }

  sid.copy_from(ltid_uuid.data);
  global_sid_lock->wrlock();
  wsrep_sidno= global_sid_map->add_sid(sid);
  WSREP_INFO("Initialized wsrep sidno %d", wsrep_sidno);
  global_sid_lock->unlock();
}

static wsrep_cb_status_t
wsrep_view_handler_cb (void*                    app_ctx,
                       void*                    recv_ctx,
                       const wsrep_view_info_t* view,
                       const char*              state,
                       size_t                   state_len,
                       void**                   sst_req,
                       size_t*                  sst_req_len)
{
  THD* thd = (THD*)recv_ctx;
  *sst_req     = NULL;
  *sst_req_len = 0;

  wsrep_member_status_t new_status= local_status.get();

  if (memcmp(&cluster_uuid, &view->state_id.uuid, sizeof(wsrep_uuid_t)))
  {
    memcpy(&cluster_uuid, &view->state_id.uuid, sizeof(cluster_uuid));

    wsrep_uuid_print (&cluster_uuid, cluster_uuid_str,
                      sizeof(cluster_uuid_str));
  }

  wsrep_cluster_conf_id= view->view;
  wsrep_cluster_status= cluster_status_str[view->status];
  wsrep_cluster_size= view->memb_num;
  wsrep_local_index= view->my_idx;

  WSREP_INFO("New cluster view: global state: %s:%lld, view# %lld: %s, "
             "number of nodes: %ld, my index: %ld, protocol version %d",
             wsrep_cluster_state_uuid, (long long)view->state_id.seqno,
             (long long)wsrep_cluster_conf_id, wsrep_cluster_status,
             wsrep_cluster_size, wsrep_local_index, view->proto_ver);

  /* Proceed further only if view is PRIMARY */
  if (WSREP_VIEW_PRIMARY != view->status) {
    wsrep_ready_set(FALSE);
    new_status= WSREP_MEMBER_UNDEFINED;
    /* Always record local_uuid and local_seqno in non-prim since this
     * may lead to re-initializing provider and start position is
     * determined according to these variables */
    // WRONG! local_uuid should be the last primary configuration uuid we were
    // a member of. local_seqno should be updated in commit calls.
    // local_uuid= cluster_uuid;
    // local_seqno= view->first - 1;
    goto out;
  }

  switch (view->proto_ver)
  {
  case 0:
  case 1:
  case 2:
  case 3:
      // version change
      if (view->proto_ver != wsrep_protocol_version)
      {
          my_bool wsrep_ready_saved= wsrep_ready;
          wsrep_ready_set(FALSE);
          WSREP_INFO("closing client connections for "
                     "protocol change %ld -> %d",
                     wsrep_protocol_version, view->proto_ver);
          wsrep_close_client_connections(TRUE, false);
          wsrep_protocol_version= view->proto_ver;
          wsrep_ready_set(wsrep_ready_saved);
      }
      break;
  default:
      WSREP_ERROR("Unsupported application protocol version: %d",
                  view->proto_ver);
      unireg_abort(1);
  }

  if (view->state_gap)
  {
    WSREP_WARN("Gap in state sequence. Need state transfer.");

    /* After that wsrep will call wsrep_sst_prepare. */
    /* keep ready flag 0 until we receive the snapshot */
    wsrep_ready_set(FALSE);

    /* Close client connections to ensure that they don't interfere
     * with SST. Necessary only if storage engines are initialized
     * before SST.
     * TODO: Just killing all ongoing transactions should be enough
     * since wsrep_ready is OFF and no new transactions can start.
     */
    if (!wsrep_before_SE())
    {
      WSREP_DEBUG("[debug]: closing client connections for PRIM");
      wsrep_close_client_connections(TRUE, false);
    }

    ssize_t const req_len= wsrep_sst_prepare (sst_req, thd);

    if (req_len < 0)
    {
      WSREP_ERROR("SST preparation failed: %zd (%s)", -req_len,
                  strerror(-req_len));
      new_status= WSREP_MEMBER_UNDEFINED;
    }
    else
    {
      assert(sst_req != NULL);
      *sst_req_len= req_len;
      new_status= WSREP_MEMBER_JOINER;
    }
  }
  else
  {
    /*
     *  NOTE: Initialize wsrep_group_uuid here only if it wasn't initialized
     *  before - OR - it was reinitilized on startup (lp:992840)
     */
    if (wsrep_startup)
    {
      if (wsrep_before_SE())
      {
        wsrep_SE_init_grab();
        // Signal mysqld init thread to continue
        wsrep_sst_complete (&cluster_uuid, view->state_id.seqno, false);
        // and wait for SE initialization
        wsrep_SE_init_wait(thd);
      }
      else
      {
        local_uuid=  cluster_uuid;
        local_seqno= view->state_id.seqno;
      }
      /* Init storage engine XIDs from first view */
      wsrep_set_SE_checkpoint(local_uuid, local_seqno);
      wsrep_init_sidno(local_uuid);
      new_status= WSREP_MEMBER_JOINED;
    }

    // just some sanity check
    if (memcmp (&local_uuid, &cluster_uuid, sizeof (wsrep_uuid_t)))
    {
      WSREP_ERROR("Undetected state gap. Can't continue.");
      wsrep_log_states(WSREP_LOG_FATAL, &cluster_uuid, view->state_id.seqno,
                       &local_uuid, -1);
      unireg_abort(1);
    }
  }

  if (wsrep_auto_increment_control)
  {
    global_system_variables.auto_increment_offset= view->my_idx + 1;
    global_system_variables.auto_increment_increment= view->memb_num;
  }

  { /* capabilities may be updated on new configuration */
    uint64_t const caps(wsrep->capabilities (wsrep));

    my_bool const idc((caps & WSREP_CAP_INCREMENTAL_WRITESET) != 0);
    if (TRUE == wsrep_incremental_data_collection && FALSE == idc)
    {
      WSREP_WARN("Unsupported protocol downgrade: "
                 "incremental data collection disabled. Expect abort.");
    }
    wsrep_incremental_data_collection = idc;
  }

out:
  if (view->status == WSREP_VIEW_PRIMARY) wsrep_startup= FALSE;
  local_status.set(new_status, view);

  return WSREP_CB_SUCCESS;
}

void wsrep_ready_set (my_bool x)
{
  WSREP_DEBUG("Setting wsrep_ready to %s", (x ? "true" : "false"));
  if (mysql_mutex_lock (&LOCK_wsrep_ready)) abort();
  if (wsrep_ready != x)
  {
    wsrep_ready= x;
    mysql_cond_signal (&COND_wsrep_ready);
  }
  mysql_mutex_unlock (&LOCK_wsrep_ready);
}

// Wait until wsrep has reached ready state
void wsrep_ready_wait ()
{
  if (mysql_mutex_lock (&LOCK_wsrep_ready)) abort();
  while (!wsrep_ready)
  {
    WSREP_INFO("Waiting to reach ready state");
    mysql_cond_wait (&COND_wsrep_ready, &LOCK_wsrep_ready);
  }
  WSREP_INFO("Ready state reached");
  mysql_mutex_unlock (&LOCK_wsrep_ready);
}

static void wsrep_synced_cb(void* app_ctx)
{
  WSREP_INFO("Synchronized with group, ready for connections");
  bool signal_main= false;
  if (mysql_mutex_lock (&LOCK_wsrep_ready)) abort();
  if (!wsrep_ready)
  {
    wsrep_ready= TRUE;
    mysql_cond_signal (&COND_wsrep_ready);
    signal_main= true;

  }
  local_status.set(WSREP_MEMBER_SYNCED);
  mysql_mutex_unlock (&LOCK_wsrep_ready);

  if (signal_main)
  {
      wsrep_SE_init_grab();
      // Signal mysqld init thread to continue
      wsrep_sst_complete (&local_uuid, local_seqno, false);
      // and wait for SE initialization
      /* we don't have recv_ctx (THD*) here */
      wsrep_SE_init_wait(current_thd);
  }
  if (wsrep_restart_slave_activated)
  {
    int rcode;
    WSREP_INFO("Restarting MySQL Slave");
    wsrep_restart_slave_activated= FALSE;
    channel_map.rdlock();
    if ((rcode = start_slave_threads(1 /* need mutex */,
                            0 /* no wait for start*/,
                            active_mi,
                       	    SLAVE_SQL)))
    {
      WSREP_WARN("Failed to create mysql-slave threads: %d", rcode);
    }
    channel_map.unlock();
  }
}

static void wsrep_init_position()
{
  /* read XIDs from storage engines */
  wsrep_uuid_t uuid;
  wsrep_seqno_t seqno;
  wsrep_get_SE_checkpoint(uuid, seqno);

  if (!memcmp(&uuid, &WSREP_UUID_UNDEFINED, sizeof(wsrep_uuid_t)))
  {
    WSREP_INFO("No pre-stored wsrep-start position found."
               " Skipping position initialization.");
    return;
  }

  char uuid_str[40] = {0, };
  wsrep_uuid_print(&uuid, uuid_str, sizeof(uuid_str));
  WSREP_INFO("Found pre-stored initial position: %s:%lld",
             uuid_str, (long long)seqno);

  if (!memcmp(&local_uuid, &WSREP_UUID_UNDEFINED, sizeof(local_uuid)) &&
      local_seqno == WSREP_SEQNO_UNDEFINED)
  {
    // Initial state
    local_uuid= uuid;
    local_seqno= seqno;
  }
  else if (memcmp(&local_uuid, &uuid, sizeof(local_uuid)) ||
           local_seqno != seqno)
  {
    WSREP_WARN("Initial position was provided by configuration or SST."
               " Avoid overriding this position.");
  }
}

int wsrep_init()
{
  int rcode= -1;

  wsrep_ready_set(FALSE);
  assert(wsrep_provider);

  wsrep_init_position();

  if ((rcode= wsrep_load(wsrep_provider, &wsrep, wsrep_log_cb)) != WSREP_OK)
  {
    if (strcasecmp(wsrep_provider, WSREP_NONE))
    {
      WSREP_ERROR("Failed to load wsrep_provider (%s). Error: %s (code: %d)."
                  " Reverting to no provider.",
                  wsrep_provider, strerror(rcode), rcode);
      strcpy((char*)wsrep_provider, WSREP_NONE); // damn it's a dirty hack
      return wsrep_init();
    }
    else /* this is for recursive call above */
    {
      WSREP_ERROR("Could not revert to no provider: %s (%d). Need to abort.",
                  strerror(rcode), rcode);
      unireg_abort(1);
    }
  }

  if (strlen(wsrep_provider) == 0 ||
      !strcmp(wsrep_provider, WSREP_NONE))
  {
    // enable normal operation in case no provider is specified
    wsrep_ready_set(TRUE);
    global_system_variables.wsrep_on = 0;
    wsrep_init_args args;
    args.logger_cb = wsrep_log_cb;
    args.options = (wsrep_provider_options) ?
            wsrep_provider_options : "";
    rcode = wsrep->init(wsrep, &args);
    if (rcode)
    {
      DBUG_PRINT("wsrep",("wsrep::init() failed: %d", rcode));
      WSREP_ERROR("Failed to initialize wsrep_provider (reason: %d)."
                  " Must shutdown", rcode);
      wsrep->free(wsrep);
      free(wsrep);
      wsrep = NULL;
    }
    return rcode;
  }
  else
  {
    global_system_variables.wsrep_on = 1;
    strncpy(provider_name,
            wsrep->provider_name,    sizeof(provider_name) - 1);
    strncpy(provider_version,
            wsrep->provider_version, sizeof(provider_version) - 1);
    strncpy(provider_vendor,
            wsrep->provider_vendor,  sizeof(provider_vendor) - 1);
  }

  /* wsrep_cluster_name is restricted to WSREP_CLUSTER_NAME_MAX_LEN characters
  only. For now galera supports only UTF-8 so WSREP_CLUSTER_NAME_MAX_LEN is ok
  on that front too. This limitation is indirectly enforced by limitation of
  gcomm */
  size_t const wsrep_len = strlen (wsrep_cluster_name);
  if (wsrep_len > WSREP_CLUSTER_NAME_MAX_LEN)
  {
    rcode = 1;
    WSREP_ERROR("wsrep_cluster_name too long (%zu)", wsrep_len);
    WSREP_ERROR("Failed to initialize wsrep_provider (reason:%d)."
                " Must shutdown", rcode);
    wsrep->free(wsrep);
    free(wsrep);
    wsrep = NULL;
    return rcode;
  }

  if (strlen (wsrep_node_name) > WSREP_HOSTNAME_LENGTH)
  {
    rcode = 1;
    WSREP_ERROR("wsrep_node_name too long (%zu)", strlen(wsrep_node_name));
    WSREP_ERROR("Failed to initialize wsrep_provider (reason:%d)."
                " Must shutdown", rcode);
    wsrep->free(wsrep);
    free(wsrep);
    wsrep = NULL;
    return rcode;
  }

  if (!wsrep_data_home_dir || strlen(wsrep_data_home_dir) == 0)
    wsrep_data_home_dir = mysql_real_data_home;

  char node_addr[512]= { 0, };
  size_t const node_addr_max= sizeof(node_addr) - 1;
  if (!wsrep_node_address || !strcmp(wsrep_node_address, ""))
  {
    size_t const ret= wsrep_guess_ip(node_addr, node_addr_max);
    if (!(ret > 0 && ret < node_addr_max))
    {
      WSREP_WARN("Failed to guess base node address. Set it explicitly via "
                 "wsrep_node_address.");
      node_addr[0]= '\0';
    }
  }
  else
  {
    strncpy(node_addr, wsrep_node_address, node_addr_max);
  }

  char inc_addr[512]= { 0, };
  size_t const inc_addr_max= sizeof (inc_addr);
  if ((!wsrep_node_incoming_address ||
       !strcmp (wsrep_node_incoming_address, WSREP_NODE_INCOMING_AUTO)))
  {
    unsigned int my_bind_ip= INADDR_ANY; // default if not set
    if (my_bind_addr_str && strlen(my_bind_addr_str))
    {
      my_bind_ip= wsrep_check_ip(my_bind_addr_str);
    }

    if (INADDR_ANY != my_bind_ip)
    {
      if (INADDR_NONE != my_bind_ip && INADDR_LOOPBACK != my_bind_ip)
      {
        snprintf(inc_addr, inc_addr_max, "%s:%u",
                 my_bind_addr_str, (int)mysqld_port);
      } // else leave inc_addr an empty string - mysqld is not listening for
        // client connections on network interfaces.
    }
    else // mysqld binds to 0.0.0.0, take IP from wsrep_node_address if possible
    {
      size_t const node_addr_len= strlen(node_addr);
      if (node_addr_len > 0)
      {
        const char* const colon= strrchr(node_addr, ':');
        if (strchr(node_addr, ':') == colon) // 1 or 0 ':'
        {
          size_t const ip_len= colon ? colon - node_addr : node_addr_len;
          if (ip_len + 7 /* :55555\0 */ < inc_addr_max)
          {
            memcpy (inc_addr, node_addr, ip_len);
            snprintf(inc_addr + ip_len, inc_addr_max - ip_len, ":%u",
                     (int)mysqld_port);
          }
          else
          {
            WSREP_WARN("Guessing address for incoming client connections: "
                       "address too long.");
            inc_addr[0]= '\0';
          }
        }
        else
        {
          WSREP_WARN("Guessing address for incoming client connections: "
                     "too many colons :) .");
          inc_addr[0]= '\0';
        }
      }

      if (!strlen(inc_addr))
      {
          WSREP_WARN("Guessing address for incoming client connections failed. "
                     "Try setting wsrep_node_incoming_address explicitly.");
      }
    }
  }
  else if (!strchr(wsrep_node_incoming_address, ':')) // no port included
  {
    if ((int)inc_addr_max <=
        snprintf(inc_addr, inc_addr_max, "%s:%u",
                 wsrep_node_incoming_address,(int)mysqld_port))
    {
      WSREP_WARN("Guessing address for incoming client connections: "
                 "address too long.");
      inc_addr[0]= '\0';
    }
  }
  else
  {
    size_t const need = strlen (wsrep_node_incoming_address);
    if (need >= inc_addr_max) {
      WSREP_WARN("wsrep_node_incoming_address too long: %zu", need);
      inc_addr[0]= '\0';
    }
    else {
      memcpy (inc_addr, wsrep_node_incoming_address, need);
    }
  }

#ifdef HAVE_PSI_INTERFACE
  if (wsrep_psi_key_vec.empty())
  {
    /* Register all galera mutexes. This is one-time activity and so
    avoid re-doing it if the provider is re-initialized. */
    const char*    category= "galera";
    unsigned int   count;

    count= array_elements(all_galera_mutexes);
    mysql_mutex_register(category, all_galera_mutexes, count);

    for (unsigned int i= 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
        reinterpret_cast<void*>(all_galera_mutexes[i].m_key));

    count= array_elements(all_galera_condvars);
    mysql_cond_register(category, all_galera_condvars, count);

    for (unsigned int i= 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
        reinterpret_cast<void*>(all_galera_condvars[i].m_key));

    count= array_elements(all_galera_threads);
    mysql_thread_register(category, all_galera_threads, count);

    for (unsigned int i= 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
        reinterpret_cast<void*>(all_galera_threads[i].m_key));

    count= array_elements(all_galera_files);
    mysql_file_register(category, all_galera_files, count);

    for (unsigned int i= 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
        reinterpret_cast<void*>(all_galera_files[i].m_key));
  }
#endif /* HAVE_PSI_INTERFACE */


  const char* provider_options= wsrep_provider_options;
  char buffer[4096];

  if (pxc_encrypt_cluster_traffic)
  {
    if (opt_ssl_ca == 0 || *opt_ssl_ca == 0 ||
        opt_ssl_cert == 0 || *opt_ssl_cert == 0 ||
        opt_ssl_key == 0 || *opt_ssl_key == 0)
    {
      WSREP_ERROR("ssl-ca, ssl-cert, and ssl-key must all be defined"
                  " to use encrypted mode traffic. Unable to configure SSL."
                  " Must shutdown.");
      rcode = 1;
      wsrep->free(wsrep);
      free(wsrep);
      wsrep = NULL;
      return rcode;
    }
    // Append the SSL options to the end of the provider
    // options strings (so that it overrides the SSL values
    // provided by the user).
    my_snprintf(buffer, sizeof(buffer), "%s%ssocket.ssl_key=%s;socket.ssl_ca=%s;socket.ssl_cert=%s",
        provider_options ? provider_options : "",
        (provider_options && *provider_options) ? ";" : "",
        opt_ssl_key,
        opt_ssl_ca,
        opt_ssl_cert
      );
    buffer[sizeof(buffer)-1] = 0;
    provider_options = buffer;
  }

  struct wsrep_init_args wsrep_args;

  struct wsrep_gtid const state_id = { local_uuid, local_seqno };

  wsrep_args.data_dir        = wsrep_data_home_dir;
  wsrep_args.node_name       = (wsrep_node_name) ? wsrep_node_name : "";
  wsrep_args.node_address    = node_addr;
  wsrep_args.node_incoming   = inc_addr;
  wsrep_args.options         = (provider_options) ?
                                provider_options : "";
  wsrep_args.proto_ver       = wsrep_max_protocol_version;

  wsrep_args.state_id        = &state_id;

  wsrep_args.logger_cb       = wsrep_log_cb;
  wsrep_args.view_handler_cb = wsrep_view_handler_cb;
  wsrep_args.apply_cb        = wsrep_apply_cb;
  wsrep_args.commit_cb       = wsrep_commit_cb;
  wsrep_args.unordered_cb    = wsrep_unordered_cb;
  wsrep_args.sst_donate_cb   = wsrep_sst_donate_cb;
  wsrep_args.synced_cb       = wsrep_synced_cb;
  wsrep_args.abort_cb        = wsrep_abort_cb;
  wsrep_args.pfs_instr_cb    = NULL;
#ifdef HAVE_PSI_INTERFACE
  wsrep_args.pfs_instr_cb    = wsrep_pfs_instr_cb;
#endif /* HAVE_PSI_INTERFACE */

  rcode = wsrep->init(wsrep, &wsrep_args);

  if (rcode)
  {
    DBUG_PRINT("wsrep",("wsrep::init() failed: %d", rcode));
    WSREP_ERROR("Failed to initialize wsrep_provider (reason:%d)."
                " Must shutdown", rcode);
    wsrep->free(wsrep);
    free(wsrep);
    wsrep = NULL;
  }

  return rcode;
}

extern int wsrep_on(void *);

void wsrep_init_startup (bool first)
{
  if (wsrep_init()) unireg_abort(1);

  wsrep_thr_lock_init(wsrep_thd_is_BF, wsrep_abort_thd,
                      wsrep_debug, wsrep_convert_LOCK_to_trx, wsrep_on);

  /* Skip replication start if dummy wsrep provider is loaded */
  if (!strcmp(wsrep_provider, WSREP_NONE)) return;

  /* Skip replication start if no cluster address */
  if (!wsrep_cluster_address || strlen(wsrep_cluster_address) == 0) return;

  if (first) wsrep_sst_grab(); // do it so we can wait for SST below

  if (!wsrep_start_replication()) unireg_abort(1);

  wsrep_create_rollbacker();
  wsrep_create_appliers(1);

  if (first && !wsrep_sst_wait()) unireg_abort(1);// wait until SST is completed
}


void wsrep_deinit()
{
  wsrep_unload(wsrep);
  wsrep= 0;
  provider_name[0]=    '\0';
  provider_version[0]= '\0';
  provider_vendor[0]=  '\0';
}

void wsrep_recover()
{
#if 0
  if (!memcmp(&local_uuid, &WSREP_UUID_UNDEFINED, sizeof(wsrep_uuid_t)) &&
      local_seqno == -2)
  {
    char uuid_str[40];
    wsrep_uuid_print(&local_uuid, uuid_str, sizeof(uuid_str));
    WSREP_INFO("Position %s:%lld given at startup, skipping position recovery",
               uuid_str, (long long)local_seqno);
    return;
  }
#endif
  wsrep_uuid_t uuid;
  wsrep_seqno_t seqno;
  wsrep_get_SE_checkpoint(uuid, seqno);
  char uuid_str[40];
  wsrep_uuid_print(&uuid, uuid_str, sizeof(uuid_str));
  WSREP_INFO("Recovered position: %s:%lld", uuid_str, (long long)seqno);
}


void wsrep_stop_replication(THD *thd)
{
  WSREP_INFO("Stop replication");
  if (!wsrep)
  {
    WSREP_INFO("Provider was not loaded, in stop replication");
    return;
  }

  /* disconnect from group first to get wsrep_ready == FALSE */
  WSREP_DEBUG("Disconnecting Provider");
  wsrep->disconnect(wsrep);

  wsrep_connected= FALSE;

  wsrep_close_client_connections(TRUE, false);

  /* wait until appliers have stopped */
  wsrep_wait_appliers_close(thd);

  return;
}

/* This one is set to true when --wsrep-new-cluster is found in the command
 * line arguments */
my_bool wsrep_new_cluster= FALSE;
#define WSREP_NEW_CLUSTER "--wsrep-new-cluster"
/* Finds and hides --wsrep-new-cluster from the arguments list
 * by moving it to the end of the list and decrementing argument count */
void wsrep_filter_new_cluster (int* argc, char* argv[])
{
  int i;
  for (i= *argc - 1; i > 0; i--)
  {
    /* make a copy of the argument to convert possible underscores to hyphens.
     * the copy need not to be longer than WSREP_NEW_CLUSTER option */
    char arg[sizeof(WSREP_NEW_CLUSTER) + 1]= { 0, };
    strncpy(arg, argv[i], sizeof(arg) - 1);
    char* underscore(arg);
    while (NULL != (underscore= strchr(underscore, '_'))) *underscore= '-';

    if (!strcmp(arg, WSREP_NEW_CLUSTER))
    {
      wsrep_new_cluster= TRUE;
      *argc -= 1;
      /* preserve the order of remaining arguments AND
       * preserve the original argument pointers - just in case */
      char* wnc= argv[i];
      memmove(&argv[i], &argv[i + 1], (*argc - i)*sizeof(argv[i]));
      argv[*argc]= wnc; /* this will be invisible to the rest of the program */
    }
  }
}

bool wsrep_start_replication()
{
  wsrep_status_t rcode;

  /*
    if provider is trivial, don't even try to connect,
    but resume local node operation
  */
  if (strlen(wsrep_provider)== 0 ||
      !strcmp(wsrep_provider, WSREP_NONE))
  {
    // enable normal operation in case no provider is specified
    wsrep_ready_set(TRUE);
    return true;
  }

  if (!wsrep_cluster_address || strlen(wsrep_cluster_address)== 0)
  {
    // if provider is non-trivial, but no address is specified, wait for address
    wsrep_ready_set(FALSE);
    return true;
  }

  bool const bootstrap(TRUE == wsrep_new_cluster);
  wsrep_new_cluster= FALSE;

  WSREP_INFO("Starting replication");

  if ((rcode = wsrep->connect(wsrep,
                              wsrep_cluster_name,
                              wsrep_cluster_address,
                              wsrep_sst_donor,
                              bootstrap)))
  {
    DBUG_PRINT("wsrep",("wsrep->connect(%s) failed: %d",
                        wsrep_cluster_address, rcode));
    WSREP_ERROR("Provider/Node (%s) failed to establish connection with cluster"
                " (reason: %d)", wsrep_cluster_address, rcode);
    return false;
  }
  else
  {
    wsrep_connected= TRUE;

    char* opts= wsrep->options_get(wsrep);
    if (opts)
    {
      wsrep_provider_options_init(opts);
      free(opts);
    }
    else
    {
      WSREP_WARN("Failed to get wsrep options");
    }
  }

  return true;
}

bool wsrep_must_sync_wait (THD* thd, uint mask)
{
  return (thd->variables.wsrep_sync_wait & mask) &&
    thd->variables.wsrep_on &&
    !thd->in_active_multi_stmt_transaction() &&
    thd->wsrep_conflict_state != REPLAYING &&
    thd->wsrep_sync_wait_gtid.seqno == WSREP_SEQNO_UNDEFINED;
}

bool wsrep_sync_wait (THD* thd, uint mask)
{
  if (wsrep_must_sync_wait(thd, mask))
  {
    //WSREP_DEBUG("wsrep_sync_wait: thd->variables.wsrep_sync_wait = %u, mask = %u, SQL: %s",
    //            thd->variables.wsrep_sync_wait, mask, WSREP_QUERY(thd));
    // This allows autocommit SELECTs and a first SELECT after SET AUTOCOMMIT=0
    // TODO: modify to check if thd has locked any rows.
    wsrep_gtid_t  gtid;
    wsrep_status_t ret= wsrep->causal_read (wsrep, &gtid);

    if (unlikely(WSREP_OK != ret))
    {
      const char* msg;
      int err;

      // Possibly relevant error codes:
      // ER_CHECKREAD, ER_ERROR_ON_READ, ER_INVALID_DEFAULT, ER_EMPTY_QUERY,
      // ER_FUNCTION_NOT_DEFINED, ER_NOT_ALLOWED_COMMAND, ER_NOT_SUPPORTED_YET,
      // ER_FEATURE_DISABLED, ER_QUERY_INTERRUPTED

      switch (ret)
      {
      case WSREP_NOT_IMPLEMENTED:
        msg= "synchronous reads by wsrep backend. "
             "Please unset wsrep_causal_reads variable.";
        err= ER_NOT_SUPPORTED_YET;
        break;
      default:
        msg= "Synchronous wait failed.";
        err= ER_LOCK_WAIT_TIMEOUT; // NOTE: the above msg won't be displayed
                                   //       with ER_LOCK_WAIT_TIMEOUT
      }
      my_error(err, MYF(0), msg);

      return true;
    }
  }

  return false;
}

void wsrep_keys_free(wsrep_key_arr_t* key_arr)
{
    for (size_t i= 0; i < key_arr->keys_len; ++i)
    {
        my_free((void*)key_arr->keys[i].key_parts);
    }
    my_free(key_arr->keys);
    key_arr->keys= 0;
    key_arr->keys_len= 0;
}


/*!
 * @param db      Database string
 * @param table   Table string
 * @param key     Array of wsrep_key_t
 * @param key_len In: number of elements in key array, Out: number of
 *                elements populated
 *
 * @return true if preparation was successful, otherwise false.
 */

static bool wsrep_prepare_key_for_isolation(const char* db,
                                            const char* table,
                                            wsrep_buf_t* key,
                                            size_t* key_len)
{
    if (*key_len < 2) return false;

    switch (wsrep_protocol_version)
    {
    case 0:
        *key_len= 0;
        break;
    case 1:
    case 2:
    case 3:
    {
        *key_len= 0;
        if (db)
        {
            // sql_print_information("%s.%s", db, table);
            if (db)
            {
                key[*key_len].ptr= db;
                key[*key_len].len= strlen(db);
                ++(*key_len);
                if (table)
                {
                    key[*key_len].ptr= table;
                    key[*key_len].len= strlen(table);
                    ++(*key_len);
                }
            }
        }
        break;
    }
    default:
        return false;
    }

    return true;
}

/* Prepare key list from db/table and table_list */
bool wsrep_prepare_keys_for_isolation(THD*              thd,
                                      const char*       db,
                                      const char*       table,
                                      const TABLE_LIST* table_list,
                                      wsrep_key_arr_t*  ka)
{
    ka->keys= 0;
    ka->keys_len= 0;

    extern TABLE* find_temporary_table(THD*, const TABLE_LIST*);

    if (db || table)
    {
        TABLE_LIST tmp_table;

        memset(&tmp_table, 0, sizeof(tmp_table));
        tmp_table.table_name= (char*)table;
        tmp_table.db= (char*)db;
	MDL_REQUEST_INIT(&tmp_table.mdl_request, MDL_key::GLOBAL, (db) ? db :  "",
                         (table) ? table : "",
                         MDL_INTENTION_EXCLUSIVE, MDL_STATEMENT);

        if (!table || !find_temporary_table(thd, &tmp_table))
        {
            if (!(ka->keys= (wsrep_key_t*)my_malloc(key_memory_wsrep, sizeof(wsrep_key_t), MYF(0))))
            {
                WSREP_ERROR("Failed to allocate memory to hold TO isolation keys");
                goto err;
            }
            ka->keys_len= 1;
            if (!(ka->keys[0].key_parts= (wsrep_buf_t*)
                  my_malloc(key_memory_wsrep, sizeof(wsrep_buf_t)*2, MYF(0))))
            {
                WSREP_ERROR("Failed to allocate memory to hold TO isolation keys");
                goto err;
            }
            ka->keys[0].key_parts_num= 2;
            if (!wsrep_prepare_key_for_isolation(
                    db, table,
                    (wsrep_buf_t*)ka->keys[0].key_parts,
                    &ka->keys[0].key_parts_num))
            {
                WSREP_ERROR("Failed to prepare keys for TO isolation");
                goto err;
            }
        }
    }

    for (const TABLE_LIST* table= table_list; table; table= table->next_global)
    {
        if (!find_temporary_table(thd, table))
        {
            wsrep_key_t* tmp;
            tmp= (wsrep_key_t*)my_realloc(
                key_memory_wsrep, ka->keys, (ka->keys_len + 1) * sizeof(wsrep_key_t), MYF(0));
            if (!tmp)
            {
                WSREP_ERROR("Failed to allocate memory to hold TO isolation keys");
                goto err;
            }
            ka->keys= tmp;
            if (!(ka->keys[ka->keys_len].key_parts= (wsrep_buf_t*)
                  my_malloc(key_memory_wsrep, sizeof(wsrep_buf_t)*2, MYF(0))))
            {
                WSREP_ERROR("Failed to allocate memory to hold TO isolation keys");
                goto err;
            }
            ka->keys[ka->keys_len].key_parts_num= 2;
            ++ka->keys_len;
            if (!wsrep_prepare_key_for_isolation(
                    table->db, table->table_name,
                    (wsrep_buf_t*)ka->keys[ka->keys_len - 1].key_parts,
                    &ka->keys[ka->keys_len - 1].key_parts_num))
            {
                WSREP_ERROR("Failed to prepare keys for TO isolation");
                goto err;
            }
        }
    }
    return true;
err:
    wsrep_keys_free(ka);
    return false;
}


bool wsrep_prepare_key_for_innodb(const uchar* cache_key,
                                  size_t cache_key_len,
                                  const uchar* row_id,
                                  size_t row_id_len,
                                  wsrep_buf_t* key,
                                  size_t* key_len)
{
    if (*key_len < 3) return false;

    *key_len= 0;
    switch (wsrep_protocol_version)
    {
    case 0:
    {
        key[0].ptr = cache_key;
        key[0].len = cache_key_len;

        *key_len = 1;
        break;
    }
    case 1:
    case 2:
    case 3:
    {
        key[0].ptr = cache_key;
        key[0].len = strlen( (char*)cache_key );

        key[1].ptr = cache_key + strlen( (char*)cache_key ) + 1;
        key[1].len = strlen( (char*)(key[1].ptr) );

        *key_len = 2;
        break;
    }
    default:
        return false;
    }

    key[*key_len].ptr = row_id;
    key[*key_len].len = row_id_len;
    ++(*key_len);

    return true;
}


/*
 * Construct Query_log_Event from thd query and serialize it
 * into buffer.
 *
 * Return 0 in case of success, 1 in case of error.
 */
int wsrep_to_buf_helper(
    THD* thd, const char *query, uint query_len, uchar** buf, size_t* buf_len)
{
  IO_CACHE tmp_io_cache;
  if (open_cached_file(&tmp_io_cache, mysql_tmpdir, TEMP_PREFIX,
                       65536, MYF(MY_WME)))
    return 1;
  int ret(0);

#ifdef TODO
  if (thd->variables.gtid_next.type == GTID_GROUP)
  {
      Gtid_log_event gtid_ev(thd, FALSE, &thd->variables.gtid_next);
      if (!gtid_ev.is_valid()) ret= 0;
      if (!ret && gtid_ev.write(&tmp_io_cache)) ret= 1;
  }
#endif

  /* if MySQL GTID event is set, we have to forward it in wsrep channel */

  /* Instead of allocating and copying over the gtid event from gtid event
  buffer buf is made to point to gtid event.
  Unfortunately gtid event is only long enough to hold gtid event and so
  append action below will cause realloc to happen. */
  if (!ret && thd->wsrep_gtid_event_buf)
  {
    *buf     = (uchar *)thd->wsrep_gtid_event_buf;
    *buf_len = thd->wsrep_gtid_event_buf_len;
  }

  /* if there is prepare query, add event for it */
  for (uint i = 0; i < thd->wsrep_TOI_pre_queries.size() && !ret; ++i)
  {
    Query_log_event ev(thd, thd->wsrep_TOI_pre_queries[i]->ptr(),
                       thd->wsrep_TOI_pre_queries[i]->length(),
		       FALSE, FALSE, FALSE, 0);
    if (ev.write(&tmp_io_cache)) ret= 1;
  }

  /* continue to append the actual query */
  Query_log_event ev(thd, query, query_len, FALSE, FALSE, FALSE, 0);
  if (!ret && ev.write(&tmp_io_cache)) ret= 1;
  if (!ret && wsrep_write_cache_buf(&tmp_io_cache, buf, buf_len)) ret= 1;

  /* Re-assigning so that the buf can be freed using gtid event buffer.
  Even if gtid event buffer is not allocated we still re-assign as free
  logic can then only rely on freeing of gtid event buffer. */
  thd->wsrep_gtid_event_buf= *buf;
  thd->wsrep_gtid_event_buf_len= *buf_len;

  close_cached_file(&tmp_io_cache);
  return ret;
}

#include "sql_show.h"
static int
create_view_query(THD *thd, uchar** buf, size_t* buf_len)
{
    LEX *lex= thd->lex;
    SELECT_LEX *select_lex= lex->select_lex;
    TABLE_LIST *first_table= select_lex->table_list.first;
    TABLE_LIST *views = first_table;

    String buff;
    const LEX_STRING command[3]=
      {{ C_STRING_WITH_LEN("CREATE ") },
       { C_STRING_WITH_LEN("ALTER ") },
       { C_STRING_WITH_LEN("CREATE OR REPLACE ") }};

    buff.append(command[thd->lex->create_view_mode].str,
                command[thd->lex->create_view_mode].length);

    if (!lex->definer)
    {
      /*
        DEFINER-clause is missing; we have to create default definer in
        persistent arena to be PS/SP friendly.
        If this is an ALTER VIEW then the current user should be set as
        the definer.
      */
      Prepared_stmt_arena_holder ps_arena_holder(thd);

      if (!(lex->definer= create_default_definer(thd)))
      {
        WSREP_WARN("Failed to create default definer for view.");
      }
    }

    views->algorithm    = lex->create_view_algorithm;
    views->definer.user = lex->definer->user;
    views->definer.host = lex->definer->host;
    views->view_suid    = lex->create_view_suid;
    views->with_check   = lex->create_view_check;

    view_store_options(thd, views, &buff);
    buff.append(STRING_WITH_LEN("VIEW "));
    /* Test if user supplied a db (ie: we did not use thd->db) */
    if (views->db && views->db[0] &&
        (thd->db().str == NULL || strcmp(views->db, thd->db().str)))
    {
      append_identifier(thd, &buff, views->db,
                        views->db_length);
      buff.append('.');
    }
    append_identifier(thd, &buff, views->table_name,
                      views->table_name_length);
    if (lex->view_list.elements)
    {
      List_iterator_fast<LEX_STRING> names(lex->view_list);
      LEX_STRING *name;
      int i;

      for (i= 0; (name= names++); i++)
      {
        buff.append(i ? ", " : "(");
        append_identifier(thd, &buff, name->str, name->length);
      }
      buff.append(')');
    }
    buff.append(STRING_WITH_LEN(" AS "));
    //buff.append(views->source.str, views->source.length);
    buff.append(thd->lex->create_view_select.str,
                thd->lex->create_view_select.length);
    //int errcode= query_error_code(thd, TRUE);
    //if (thd->binlog_query(THD::STMT_QUERY_TYPE,
    //                      buff.ptr(), buff.length(), FALSE, FALSE, FALSE, errcod
    return wsrep_to_buf_helper(thd, buff.ptr(), buff.length(), buf, buf_len);
}

/*
  returns: 
   0: statement was replicated as TOI
   1: TOI replication was skipped
  -1: TOI replication failed 
 */
static int wsrep_TOI_begin(THD *thd, const char *db_, const char *table_,
                           const TABLE_LIST* table_list)
{
  wsrep_status_t ret(WSREP_WARNING);
  uchar* buf(0);
  size_t buf_len(0);
  int buf_err;

  WSREP_DEBUG("Executing Query (%s) with write-set (%lld) and exec_mode: %s"
              " in TO Isolation mode",
              WSREP_QUERY(thd),
              (long long)wsrep_thd_trx_seqno(thd),
              wsrep_get_exec_mode(thd->wsrep_exec_mode));

  switch (thd->lex->sql_command)
  {
  case SQLCOM_CREATE_VIEW:
    buf_err= create_view_query(thd, &buf, &buf_len);
    break;
  case SQLCOM_CREATE_PROCEDURE:
  case SQLCOM_CREATE_SPFUNCTION:
    buf_err= wsrep_create_sp(thd, &buf, &buf_len);
    break;
  case SQLCOM_CREATE_TRIGGER:
    buf_err= wsrep_create_trigger_query(thd, &buf, &buf_len);
    break;
  case SQLCOM_CREATE_EVENT:
    buf_err= wsrep_create_event_query(thd, &buf, &buf_len);
    break;
  case SQLCOM_ALTER_EVENT:
    buf_err= wsrep_alter_event_query(thd, &buf, &buf_len);
    break;
  default:
    buf_err= wsrep_to_buf_helper(thd, thd->query().str, thd->query().length, 
                                 &buf, &buf_len);
    break;
  }

  if (buf_err == 1) {
    /* Given the existing error handling setup, all errors with write-set
    are classified under single error code. It would be good to have a proper
    error code reporting mechanism. */
    WSREP_WARN("Append/Write to writeset buffer failed (either due to IO "
		"issues (including memory allocation) or hitting a configured "
                "limit viz. write set size, etc.");
    my_error(ER_ERROR_DURING_COMMIT, MYF(0), WSREP_SIZE_EXCEEDED);
    return -1;
  }

  wsrep_key_arr_t key_arr= {0, 0};
  struct wsrep_buf buff = { buf, buf_len };

  if (!buf_err                                                                &&
      wsrep_prepare_keys_for_isolation(thd, db_, table_, table_list, &key_arr)&&
      key_arr.keys_len > 0                                                    &&
      WSREP_OK == (ret = wsrep->to_execute_start(wsrep, (ulong)thd->thread_id(),
                                                 key_arr.keys, key_arr.keys_len,
                                                 &buff, 1,
                                                 &thd->wsrep_trx_meta)))
  {
    thd->wsrep_exec_mode= TOTAL_ORDER;
    wsrep_to_isolation++;

#ifdef SKIP_INNODB_HP
    /* set priority */
    thd->tx_priority = 1;
#endif

    if (buf) my_free(buf);
    /* thd->wsrep_gtid_event_buf was free'ed above, just set to NULL */
    thd->wsrep_gtid_event_buf_len = 0;
    thd->wsrep_gtid_event_buf     = NULL;
    wsrep_keys_free(&key_arr);

    WSREP_DEBUG("Query (%s) with write-set (%lld) and exec_mode: %s"
                " replicated in TO Isolation mode",
                WSREP_QUERY(thd),
                (long long)wsrep_thd_trx_seqno(thd),
                wsrep_get_exec_mode(thd->wsrep_exec_mode));

    THD_STAGE_INFO(thd, stage_wsrep_preparing_for_TO_isolation);
    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
             "wsrep: initiating TOI for write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);
  }
  else if (key_arr.keys_len > 0) {
    /* jump to error handler in mysql_execute_command() */
    WSREP_WARN("TO isolation failed for: %d, schema: %s, sql: %s. Check wsrep "
               "connection state and retry the query.",
               ret,
               (thd->db().str ? thd->db().str : "(null)"),
               (thd->query().str) ? WSREP_QUERY(thd) : "void");
    my_error(ER_LOCK_DEADLOCK, MYF(0), "WSREP replication failed. Check "
             "your wsrep connection state and retry the query.");

    if (buf) my_free(buf);
    /* thd->wsrep_gtid_event_buf was free'ed above, just set to NULL */
    thd->wsrep_gtid_event_buf_len = 0;
    thd->wsrep_gtid_event_buf     = NULL;
    wsrep_keys_free(&key_arr);

    return -1;
  }
  else {
    /* non replicated DDL, affecting temporary tables only */
    WSREP_DEBUG("TO isolation skipped for: %d, sql: %s."
                "Only temporary tables affected.",
                ret, WSREP_QUERY(thd));

    if (buf) my_free(buf);
    /* thd->wsrep_gtid_event_buf was free'ed above, just set to NULL */
    thd->wsrep_gtid_event_buf_len = 0;
    thd->wsrep_gtid_event_buf     = NULL;
    wsrep_keys_free(&key_arr);

    return 1;
  }

  if (thd->wsrep_gtid_event_buf) my_free(thd->wsrep_gtid_event_buf);
  thd->wsrep_gtid_event_buf_len = 0;
  thd->wsrep_gtid_event_buf     = NULL;
  return 0;
}

static void wsrep_TOI_end(THD *thd)
{
  wsrep_status_t ret;
  wsrep_to_isolation--;

#ifdef SKIP_INNODB_HP
  /* set priority back to normal */
  thd->tx_priority = 0;
#endif

  THD_STAGE_INFO(thd, stage_wsrep_completed_TO_isolation);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: completed TOI write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  wsrep_set_SE_checkpoint(thd->wsrep_trx_meta.gtid.uuid,
                          thd->wsrep_trx_meta.gtid.seqno);
  
  if (WSREP_OK == (ret = wsrep->to_execute_end(wsrep, (ulong)thd->thread_id()))) {
    WSREP_DEBUG("Completed query (%s) replication with write-set (%lld) and"
                " exec_mode: %s in TO Isolation mode",
                WSREP_QUERY(thd),
                (long long)wsrep_thd_trx_seqno(thd),
                wsrep_get_exec_mode(thd->wsrep_exec_mode));
  }
  else {
    WSREP_WARN("TO isolation end failed for: %d, schema: %s, sql: %s",
               ret,
               (thd->db().str ? thd->db().str : "(null)"),
               (thd->query().str) ? WSREP_QUERY(thd) : "void");
  }
}

static int wsrep_RSU_begin(THD *thd, const char *db_, const char *table_)
{
  wsrep_status_t ret(WSREP_WARNING);
  wsrep_seqno_t seqno= WSREP_SEQNO_UNDEFINED;

  WSREP_DEBUG("Executing Query (%s) with write-set (%lld) and exec_mode: %s"
              " in RSU mode",
              WSREP_QUERY(thd),
              (long long)wsrep_thd_trx_seqno(thd),
              wsrep_get_exec_mode(thd->wsrep_exec_mode));

  ret = wsrep->desync(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("Desync of node failed in RSU Flow with error: %d"
               " for schema: %s, query: %s",
               ret, (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
    my_error(ER_LOCK_DEADLOCK, MYF(0));
    return(ret);
  }

  mysql_mutex_lock(&LOCK_wsrep_replaying);
  wsrep_replaying++;
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  if (wsrep_wait_committing_connections_close(5000))
  {
    /* no can do, bail out from DDL */
    WSREP_WARN("RSU failed due to pending transactions, schema: %s, query %s",
               (thd->db().str ? thd->db().str : "(null)"),
               WSREP_QUERY(thd));
    mysql_mutex_lock(&LOCK_wsrep_replaying);
    wsrep_replaying--;
    mysql_mutex_unlock(&LOCK_wsrep_replaying);

    ret = wsrep->resync(wsrep);
    if (ret != WSREP_OK)
    {
      WSREP_WARN("resync failed %d for schema: %s, query: %s",
                 ret, (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
    }

    my_error(ER_LOCK_DEADLOCK, MYF(0));
    return(1);
  }

  seqno = wsrep->pause(wsrep);
  if (seqno == WSREP_SEQNO_UNDEFINED)
  {
    WSREP_WARN("pause failed %lld for schema: %s, query: %s", (long long)seqno,
               (thd->db().length ? thd->db().str : "(null)"), WSREP_QUERY(thd));

    /* Pause fail so rollback desync action too. */
    wsrep->resync(wsrep);

    return(1);
  }
  thd->global_read_lock.pause_provider(true);
  WSREP_DEBUG("Provider paused for RSU processing at seqno: %lld",
              (long long)seqno);

  DEBUG_SYNC(thd,"wsrep_RSU_begin_acquired");
  return 0;
}

static void wsrep_RSU_end(THD *thd)
{
  wsrep_status_t ret(WSREP_OK);

  WSREP_DEBUG("Initiating RSU_end for write-set: %lld",
              (long long)wsrep_thd_trx_seqno(thd));

  mysql_mutex_lock(&LOCK_wsrep_replaying);
  wsrep_replaying--;
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  ret = wsrep->resume(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("resume failed %d for schema: %s, query: %s", ret,
               (thd->db().str ? thd->db().str : "(null)"),
               WSREP_QUERY(thd));
  }
  else
  {
    thd->global_read_lock.pause_provider(false);
  }

  ret = wsrep->resync(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("resync failed %d for schema: %s, query: %s", ret,
               (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
  }

  WSREP_DEBUG("Completed Query (%s) replication with write-set (%lld) and"
              " exec_mode: %s in RSU mode",
              WSREP_QUERY(thd),
              (long long)wsrep_thd_trx_seqno(thd),
              wsrep_get_exec_mode(thd->wsrep_exec_mode));
}

int wsrep_to_isolation_begin(THD *thd, const char *db_, const char *table_,
                             const TABLE_LIST* table_list)
{
  /*
    No isolation for applier or replaying threads.
   */
  if (thd->wsrep_exec_mode == REPL_RECV) return 0;

  /* Generally if node enters non-primary state then execution of DDL+DML
  is blocked on such node but there are some asynchronous pre-register
  action that can cause invocation of TOI. For example: DROP of event
  on completion of EVENT tenure is one such asynchronous action which
  doesn't need to be fired by user and so if node is asynchronous
  such such action should be blocked at TOI level. */
  if (!wsrep_ready)
  {
    WSREP_DEBUG("WSREP replication failed."
                " Check your wsrep connection state and retry the query.");
    my_error(ER_LOCK_DEADLOCK, MYF(0), "WSREP replication failed. Check "
	     "your wsrep connection state and retry the query.");
    return -1;
  }

  int ret= 0;
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);

  if (thd->wsrep_conflict_state == MUST_ABORT)
  {
    WSREP_INFO("thread: %u, schema: %s, query: %s has been aborted due to multi-master conflict",
               thd->thread_id(),
               (thd->db().str ? thd->db().str : "(null)"),
               (thd->query().str) ? WSREP_QUERY(thd) : "void");
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    return WSREP_TRX_FAIL;
  }
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_ASSERT(thd->wsrep_exec_mode == LOCAL_STATE);
  DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno == WSREP_SEQNO_UNDEFINED);

  if (thd->global_read_lock.can_acquire_protection())
  {
    WSREP_DEBUG("Aborting TOI: Global Read-Lock (FTWRL) in place: %s %u",
                WSREP_QUERY(thd), thd->thread_id());
    return -1;
  }

  if (!thd->global_read_lock.provider_resumed())
  {
    WSREP_DEBUG("Aborting TOI: Galera provider paused due to lock: %s %u",
                thd->query().str, thd->thread_id());
    my_error(ER_CANT_UPDATE_WITH_READLOCK, MYF(0));
    return -1;
  }

  if (wsrep_debug && thd->mdl_context.has_locks())
  {
    WSREP_DEBUG("Thread holds MDL locks at TOI begin: %s %u",
                WSREP_QUERY(thd), thd->thread_id());
  }

  /*
    It makes sense to set auto_increment_* to defaults in TOI operations.
    Must be done before wsrep_TOI_begin() since Query_log_event encapsulating
    TOI statement and auto inc variables for wsrep replication is constructed
    there. Variables are reset back in THD::reset_for_next_command() before
    processing of next command.
   */
  if (wsrep_auto_increment_control)
  {
    thd->variables.auto_increment_offset = 1;
    thd->variables.auto_increment_increment = 1;
  }

  if (thd->variables.wsrep_on && thd->wsrep_exec_mode==LOCAL_STATE)
  {
    switch (thd->variables.wsrep_OSU_method) {
    case WSREP_OSU_TOI: ret =  wsrep_TOI_begin(thd, db_, table_,
                                               table_list); break;
    case WSREP_OSU_RSU: ret =  wsrep_RSU_begin(thd, db_, table_); break;
    default:
      WSREP_ERROR("Unsupported OSU method: %lu",
                  thd->variables.wsrep_OSU_method);
      ret= -1;
      break;
    }
    switch (ret) {
    case 0:  thd->wsrep_exec_mode= TOTAL_ORDER; break;
    case 1:
      /* TOI replication skipped, treat as success */
      ret = 0;
      break;
    case -1:
      /* TOI replication failed, treat as error */
      break;
    }
  }
  return ret;
}

void wsrep_to_isolation_end(THD *thd)
{
  if (thd->wsrep_exec_mode == TOTAL_ORDER)
  {
    switch(thd->variables.wsrep_OSU_method)
    {
    case WSREP_OSU_TOI: wsrep_TOI_end(thd); break;
    case WSREP_OSU_RSU: wsrep_RSU_end(thd); break;
    default:
      WSREP_WARN("Unsupported wsrep OSU method at isolation end: %lu",
                 thd->variables.wsrep_OSU_method);
      break;
    }
    wsrep_cleanup_transaction(thd);
  }
}

#define WSREP_MDL_LOG(severity, msg, schema, schema_len, req, gra)             \
    WSREP_##severity(                                                          \
      "%s\n\n"                                                                 \
      "schema:  %.*s\n"                                                        \
      "request: (thd-tid:%u \tseqno:%lld \texec-mode:%s, query-state:%s,"      \
                 " conflict-state:%s)\n          cmd-code:%d %d \tquery:%s)\n\n"\
      "granted: (thd-tid:%u \tseqno:%lld \texec-mode:%s, query-state:%s,"      \
                 " conflict-state:%s)\n          cmd-code:%d %d \tquery:%s)\n",\
      msg, schema_len, schema,                                                 \
      req->thread_id(), (long long)wsrep_thd_trx_seqno(req),                   \
      wsrep_get_exec_mode(req->wsrep_exec_mode),                               \
      wsrep_get_query_state(req->wsrep_query_state),                           \
      wsrep_get_conflict_state(req->wsrep_conflict_state),                     \
      req->get_command(), req->lex->sql_command, req->query().str,             \
      gra->thread_id(), (long long)wsrep_thd_trx_seqno(gra),                   \
      wsrep_get_exec_mode(gra->wsrep_exec_mode),                               \
      wsrep_get_query_state(gra->wsrep_query_state),                           \
      wsrep_get_conflict_state(gra->wsrep_conflict_state),                     \
      gra->get_command(), gra->lex->sql_command, gra->query().str);

bool
wsrep_grant_mdl_exception(const MDL_context *requestor_ctx,
                          MDL_ticket *ticket,
                          const MDL_key *key
) {
  if (!WSREP_ON) return FALSE;

  THD *request_thd  = requestor_ctx->wsrep_get_thd();
  THD *granted_thd  = ticket->get_ctx()->wsrep_get_thd();
  bool ret          = FALSE;

  const char* schema= key->db_name();
  int schema_len= key->db_name_length();

  mysql_mutex_lock(&request_thd->LOCK_wsrep_thd);
  if (request_thd->wsrep_exec_mode == TOTAL_ORDER ||
      request_thd->wsrep_exec_mode == REPL_RECV)
  {
    mysql_mutex_unlock(&request_thd->LOCK_wsrep_thd);

    WSREP_MDL_LOG(DEBUG, "---------- MDL conflict --------", schema, schema_len,
                  request_thd, granted_thd);
    ticket->wsrep_report(wsrep_debug);

    mysql_mutex_lock(&granted_thd->LOCK_wsrep_thd);
    if (granted_thd->wsrep_exec_mode == TOTAL_ORDER ||
        granted_thd->wsrep_exec_mode == REPL_RECV)
    {
      WSREP_DEBUG("MDL BF-BF Conflict");
      ticket->wsrep_report(true);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = TRUE;
    }
    else if (granted_thd->lex->sql_command == SQLCOM_FLUSH ||
             granted_thd->mdl_context.wsrep_has_explicit_locks())
    {
      WSREP_DEBUG("BF thread waiting for FLUSH/explicit lock");
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = FALSE;
    }
    else if (request_thd->lex->sql_command == SQLCOM_DROP_TABLE)
    {
      WSREP_DEBUG("DROP TABLE triggered BF abort (conflict-state: %s)",
                  wsrep_get_conflict_state(granted_thd->wsrep_conflict_state));
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd((void*)request_thd, (void*)granted_thd, 1);
      ret = FALSE;
    }
    else if (granted_thd->wsrep_query_state == QUERY_COMMITTING)
    {
      WSREP_DEBUG("MDL granted but committing thread (%u) is aborted",
                  granted_thd->thread_id());
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd((void*)request_thd, (void*)granted_thd, 1);
      ret = FALSE;
    }
    else
    {
      WSREP_DEBUG("MDL conflict -> BF abort");
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd((void*)request_thd, (void*)granted_thd, 1);
      ret = FALSE;
    }
  }
  else
  {
    mysql_mutex_unlock(&request_thd->LOCK_wsrep_thd);
  }
  return ret;
}

bool wsrep_node_is_donor()
{
  return (WSREP_ON) ? (local_status.get() == WSREP_MEMBER_DONOR) : false;
}

bool wsrep_node_is_synced()
{
  return (WSREP_ON) ? (local_status.get() == WSREP_MEMBER_SYNCED) : false;
}

const char* wsrep_get_exec_mode(wsrep_exec_mode state)
{
  switch(state)
  {
  case LOCAL_STATE:
    return "LOCAL_STATE";
  case REPL_RECV:
    return "REPL_RECV";
  case TOTAL_ORDER:
    return "TOTAL_ORDER";
  case LOCAL_COMMIT:
    return "LOCAL_COMMIT";
  }
  return "NULL";
}

const char* wsrep_get_query_state(wsrep_query_state state)
{
  switch(state)
  {
  case QUERY_IDLE:
    return "QUERY_IDLE";
  case QUERY_EXEC:
    return "QUERY_EXEC";
  case QUERY_COMMITTING:
    return "QUERY_COMMITTING";
  case QUERY_EXITING:
    return "QUERY_EXITING";
  case QUERY_ROLLINGBACK:
    return "QUERY_ROLLINGBACK";
  }
  return "NULL";
}

const char* wsrep_get_conflict_state(wsrep_conflict_state state)
{
  switch (state)
  {
  case NO_CONFLICT:
    return "NO_CONFLICT";
  case MUST_ABORT:
    return "MUST_ABORT";
  case ABORTING:
    return "ABORTING";
  case ABORTED:
    return "ABORTED";
  case MUST_REPLAY:
    return "MUST_REPLAY";
  case REPLAYING:
    return "REPLAYING";
  case REPLAYED:
    return "REPLAYED";
  case RETRY_AUTOCOMMIT:
    return "RETRY_AUTOCOMMIT";
  case CERT_FAILURE:
    return "CERT_FAILURE";
  }

  return "NULL";
}

const char* wsrep_get_wsrep_status(wsrep_status status)
{
  switch(status)
  {
  case WSREP_OK:
    return "WSREP_OK";
  case WSREP_WARNING:
    return "WSREP_WARNING";
  case WSREP_TRX_MISSING:
    return "WSREP_TRX_MISSING";
  case WSREP_TRX_FAIL:
    return "WSREP_TRX_FAIL";
  case WSREP_BF_ABORT:
    return "WSREP_BF_ABORT";
  case WSREP_SIZE_EXCEEDED:
    return "WSREP_SIZE_EXCEEDED";
  case WSREP_CONN_FAIL:
    return "WSREP_CONN_FAIL";
  case WSREP_NODE_FAIL:
    return "WSREP_NODE_FAIL";
  case WSREP_FATAL:
    return "WSREP_FATAL";
  case WSREP_PRECOMMIT_ABORT:
    return "WSREP_PRECOMMIT_ABORT";
  case WSREP_NOT_IMPLEMENTED:
    return "NULL";
  }

  return "NULL";
}
