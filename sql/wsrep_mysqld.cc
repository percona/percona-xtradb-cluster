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

#include "binlog.h"
#include "handler.h"
#include "mysql/plugin.h"
#include "mysqld.h"
#include "rpl_replica.h"
#include "sql/dd/cache/dictionary_client.h"  // dd::cache::Dictionary_client
#include "sql/dd/dd_view.h"
#include "sql/dd/dictionary.h"
#include "sql/dd/types/table.h"
#include "sql/dd_sql_view.h"  // prepare_view_tables_list
#include "sql/key_spec.h"
#include "sql/raii/sentry.h"
#include "sql/server_component/mysql_server_keyring_lockable_imp.h"
#include "sql/sql_alter.h"
#include "sql/sql_lex.h"
#include "sql/ssl_init_callback.h"
#include "sql/thd_raii.h"
#include "sql_base.h"
#include "sql_class.h"
#include "sql_parse.h"
#include "sql_plugin.h"  // SHOW_MY_BOOL

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#include "log_event.h"
#include "mysql/components/services/log_builtins.h"
#include "rpl_msr.h"  // channel_map
#include "sp_head.h"
#include "sql_base.h"  // TEMP_PREFIX
#include "sql_db.h"    // check_schema_readonly
#include "wsrep_applier.h"
#include "wsrep_binlog.h"
#include "wsrep_priv.h"
#include "wsrep_sst.h"
#include "wsrep_thd.h"
#include "wsrep_utils.h"
#include "wsrep_var.h"
#include "wsrep_xid.h"

#include "my_inttypes.h"
#include "service_wsrep.h"
#include "transaction.h"
#include "wsrep_schema.h"
#include "wsrep_server_state.h"
#include "wsrep_trans_observer.h"

#ifdef HAVE_PSI_INTERFACE
#include <map>
#include <vector>
#include "mysql/psi/mysql_file.h"
#include "pthread.h"
#endif /* HAVE_PSI_INTERFACE */

#include "debug_sync.h"

#include "wsrep_master_key_manager.h"
static bool wsrep_init_master_key();
static void wsrep_deinit_master_key();

/* wsrep-lib */
Wsrep_server_state *Wsrep_server_state::m_instance;

/* If binlogging is off then in theory replication can't be done as write-set
is made using binlog. In order to allow this use-case PXC/Galera support
emulation binlogging that is enabled if default binlogging is off. */
bool wsrep_emulate_bin_log = false;

/* Streaming Replication */
const char *wsrep_fragment_units[] = {"bytes", "rows", "statements", NullS};
const char *wsrep_SR_store_types[] = {"none", "table", NullS};

/* sidno in global_sid_map corresponding to galera uuid */
rpl_sidno wsrep_sidno = -1;

/* Begin configuration options and their default values */

const char *wsrep_data_home_dir = NULL;
const char *wsrep_dbug_option = "";
uint wsrep_min_log_verbosity = 3;
long wsrep_slave_threads = 1;           // # of slave action appliers wanted
int wsrep_slave_count_change = 0;       // # of appliers to stop or start
ulong wsrep_debug = 0;                  // enable debug level logging
ulong wsrep_retry_autocommit = 5;       // retry aborted autocommit trx
bool wsrep_auto_increment_control = 1;  // control auto increment variables
bool wsrep_incremental_data_collection = 0;  // incremental data collection
ulong wsrep_max_ws_size = 1073741824UL;      // max ws (RBR buffer) size
ulong wsrep_max_ws_rows = 65536;             // max number of rows in ws
int wsrep_to_isolation = 0;                  // # of active TO isolation threads
bool wsrep_certify_nonPK = 1;  // certify, even when no primary key
ulong wsrep_certification_rules = WSREP_CERTIFICATION_RULES_STRICT;
long wsrep_max_protocol_version = 4;  // maximum protocol version to use
long int wsrep_protocol_version = wsrep_max_protocol_version;
ulong wsrep_trx_fragment_unit = WSREP_FRAG_BYTES;
// unit for fragment size
ulong wsrep_SR_store_type = WSREP_SR_STORE_TABLE;
uint wsrep_ignore_apply_errors = 0;

bool wsrep_recovery = 0;  // recovery
bool wsrep_log_conflicts = 0;
bool wsrep_desync = 0;               // desynchronize the node from the cluster
bool wsrep_load_data_splitting = 1;  // commit load data every 10K intervals
bool wsrep_restart_slave = 0;        // should mysql slave thread be
                                     // restarted, if node joins back
bool wsrep_restart_slave_activated = 0;  // node has dropped, and slave
                                         // restart will be needed
bool wsrep_slave_UK_checks = 0;          // slave thread does UK checks
bool wsrep_slave_FK_checks = 0;          // slave thread does FK checks

/* wait for x micro-secs to allow active connection to commit before
starting RSU */
ulong wsrep_RSU_commit_timeout = 5000;

/* pxc-strict-mode help control behavior of experimental features like
myisam table replication, etc... */
ulong pxc_strict_mode = PXC_STRICT_MODE_ENFORCING;

/* pxc-maint-mode help put node in maintenance mode so that proxysql
can stop diverting queries to this node. */
ulong pxc_maint_mode = PXC_MAINT_MODE_DISABLED;

/* wsrep-pxc-maint-mode controls whenever maintenance mode has been set by
 * Wsrep_server_service::log_view.
 * This blocks users from changing pxc_maint_mode
 */
bool wsrep_pxc_maint_mode_forced = false;

/* sleep for this period before delivering shutdown signal. */
ulong pxc_maint_transition_period = 30;

/* enables PXC SSL auto-config */
bool pxc_encrypt_cluster_traffic = 0;

ulong wsrep_gcache_encrypt = WSREP_ENCRYPT_MODE_NONE;
ulong wsrep_disk_pages_encrypt = WSREP_ENCRYPT_MODE_NONE;

/* force flush of error message if error is detected at early stage
   during SST or other initialization. */
bool pxc_force_flush_error_message = false;

/* End configuration options */

static wsrep_uuid_t node_uuid = WSREP_UUID_UNDEFINED;
static char cluster_uuid_str[40] = {
    0,
};
static char provider_name[256] = {
    0,
};
static char provider_version[256] = {
    0,
};
static char provider_vendor[256] = {
    0,
};

/* Set to true on successful connect and false on disconnect. */
bool wsrep_connected = false;

// wsrep status variable - start
bool wsrep_ready = false;  // node can accept queries
const char *wsrep_cluster_state_uuid = cluster_uuid_str;
long long wsrep_cluster_conf_id = WSREP_SEQNO_UNDEFINED;
const char *wsrep_cluster_status = "Disconnected";
long wsrep_cluster_size = 0;
long wsrep_local_index = -1;
long long wsrep_local_bf_aborts = 0;
const char *wsrep_provider_name = provider_name;
const char *wsrep_provider_version = provider_version;
const char *wsrep_provider_vendor = provider_vendor;
char *wsrep_provider_capabilities = NULL;
char *wsrep_cluster_capabilities = NULL;
// wsrep status variable - end

wsrep_uuid_t local_uuid = WSREP_UUID_UNDEFINED;
wsrep_seqno_t local_seqno = WSREP_SEQNO_UNDEFINED;
wsp::node_status local_status;

// Boolean denoting whether to allow server session commands to execute
// irrespective of the local state.
bool wsrep_allow_server_session = false;

Wsrep_schema *wsrep_schema = 0;

#ifdef HAVE_PSI_INTERFACE

/* Keys for mutexes and condition variables in galera library space. */
PSI_mutex_key key_LOCK_galera_cert;
PSI_mutex_key key_LOCK_galera_stats;
PSI_mutex_key key_LOCK_galera_dummy_gcs;
PSI_mutex_key key_LOCK_galera_service_thd;
PSI_mutex_key key_LOCK_galera_ist_receiver;

PSI_mutex_key key_LOCK_galera_local_monitor;
PSI_mutex_key key_LOCK_galera_apply_monitor;
PSI_mutex_key key_LOCK_galera_commit_monitor;
PSI_mutex_key key_LOCK_galera_async_sender_monitor;
PSI_mutex_key key_LOCK_galera_ist_receiver_monitor;

PSI_mutex_key key_LOCK_galera_sst;
PSI_mutex_key key_LOCK_galera_incoming;
PSI_mutex_key key_LOCK_galera_saved_state;
PSI_mutex_key key_LOCK_galera_trx_handle;
PSI_mutex_key key_LOCK_galera_wsdb_trx;

PSI_mutex_key key_LOCK_galera_wsdb_conn;
PSI_mutex_key key_LOCK_galera_gu_dbug_sync;
PSI_mutex_key key_LOCK_galera_profile;
PSI_mutex_key key_LOCK_galera_gcache;
PSI_mutex_key key_LOCK_galera_protstack;
PSI_mutex_key key_LOCK_galera_prodcons;
PSI_mutex_key key_LOCK_galera_gcommconn;

PSI_mutex_key key_LOCK_galera_recvbuf;
PSI_mutex_key key_LOCK_galera_mempool;

/* Sequence here should match with tag name sequence specified in wsrep_api.h */
PSI_mutex_info all_galera_mutexes[] = {
    {&key_LOCK_galera_cert, "LOCK_galera_cert", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_stats, "LOCK_galera_stats", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_dummy_gcs, "LOCK_galera_dummy_gcs", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_service_thd, "LOCK_galera_service_thd", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_ist_receiver, "LOCK_galera_ist_receiver", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_local_monitor, "LOCK_galera_local_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_apply_monitor, "LOCK_galera_apply_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_commit_monitor, "LOCK_galera_commit_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_async_sender_monitor, "LOCK_galera_async_sender_monitor",
     0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_ist_receiver_monitor, "LOCK_galera_ist_receiver_monitor",
     0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_sst, "LOCK_galera_sst", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_incoming, "LOCK_galera_incoming", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_saved_state, "LOCK_galera_saved_state", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_trx_handle, "LOCK_galera_trx_handle", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_wsdb_trx, "LOCK_galera_wsdb", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_wsdb_conn, "LOCK_galera_wsdb_conn", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_profile, "LOCK_galera_profile", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_gcache, "LOCK_galera_gcache", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_protstack, "LOCK_galera_protstack", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_prodcons, "LOCK_galera_prodcons", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_gcommconn, "LOCK_galera_gcommconn", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_LOCK_galera_recvbuf, "LOCK_galera_recvbuf", 0, 0, PSI_DOCUMENT_ME},
    {&key_LOCK_galera_mempool, "LOCK_galera_mempool", 0, 0, PSI_DOCUMENT_ME}};

PSI_cond_key key_COND_galera_dummy_gcs;
PSI_cond_key key_COND_galera_service_thd;
PSI_cond_key key_COND_galera_service_thd_flush;
PSI_cond_key key_COND_galera_ist_receiver;
PSI_cond_key key_COND_galera_ist_consumer;
PSI_cond_key key_COND_galera_monitor_process1;
PSI_cond_key key_COND_galera_monitor_process2;

PSI_cond_key key_COND_galera_local_monitor;
PSI_cond_key key_COND_galera_apply_monitor;
PSI_cond_key key_COND_galera_commit_monitor;
PSI_cond_key key_COND_galera_async_sender_monitor;
PSI_cond_key key_COND_galera_ist_receiver_monitor;

PSI_cond_key key_COND_galera_sst;
PSI_cond_key key_COND_galera_gu_dbug_sync;
PSI_cond_key key_COND_galera_prodcons;
PSI_cond_key key_COND_galera_gcache;
PSI_cond_key key_COND_galera_recvbuf;

PSI_cond_info all_galera_condvars[] = {
    {&key_COND_galera_dummy_gcs, "COND_galera_dummy_gcs", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_service_thd, "COND_galera_service_thd", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_service_thd_flush, "COND_galera_service_thd_flush", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_ist_receiver, "COND_galera_ist_receiver", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_ist_consumer, "COND_galera_ist_consumer", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_local_monitor, "COND_galera_local_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_apply_monitor, "COND_galera_apply_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_commit_monitor, "COND_galera_commit_monitor", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_COND_galera_async_sender_monitor, "COND_galera_async_sender_monitor",
     0, 0, PSI_DOCUMENT_ME},
    {&key_COND_galera_ist_receiver_monitor, "COND_galera_ist_receiver_monitor",
     0, 0, PSI_DOCUMENT_ME},
    {&key_COND_galera_sst, "COND_galera_sst", 0, 0, PSI_DOCUMENT_ME},
    {&key_COND_galera_prodcons, "COND_galera_prodcons", 0, 0, PSI_DOCUMENT_ME},
    {&key_COND_galera_gcache, "COND_galera_gcache", 0, 0, PSI_DOCUMENT_ME},
    {&key_COND_galera_recvbuf, "COND_galera_recvbuf", 0, 0, PSI_DOCUMENT_ME}};

PSI_thread_key key_THREAD_galera_service_thd;
PSI_thread_key key_THREAD_galera_ist_receiver;
PSI_thread_key key_THREAD_galera_ist_async_sender;
PSI_thread_key key_THREAD_galera_writeset_checksum;
PSI_thread_key key_THREAD_galera_gcache_removefile;
PSI_thread_key key_THREAD_galera_receiver;
PSI_thread_key key_THREAD_galera_gcommconn;

PSI_thread_info all_galera_threads[] = {
    {&key_THREAD_galera_service_thd, "THREAD_galera_service_thd", "galera_srvc",
     0, 0, PSI_DOCUMENT_ME},
    {&key_THREAD_galera_ist_receiver, "THREAD_galera_ist_receiver", "ist_rcv",
     0, 0, PSI_DOCUMENT_ME},
    {&key_THREAD_galera_ist_async_sender, "THREAD_galera_ist_async_sender",
     "ist_sender", 0, 0, PSI_DOCUMENT_ME},
    {&key_THREAD_galera_writeset_checksum, "THREAD_galera_writeset_checksum",
     "wset_check", 0, 0, PSI_DOCUMENT_ME},
    {&key_THREAD_galera_gcache_removefile, "THREAD_galera_gcache_removefile",
     "gcache_rm", 0, 0, PSI_DOCUMENT_ME},
    {&key_THREAD_galera_receiver, "THREAD_galera_receiver", "galera_recv", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_THREAD_galera_gcommconn, "THREAD_galera_gcommconn", "galera_gcomm", 0,
     0, PSI_DOCUMENT_ME}};

PSI_file_key key_FILE_galera_recordset;
PSI_file_key key_FILE_galera_ringbuffer;
PSI_file_key key_FILE_galera_gcache_page;
PSI_file_key key_FILE_galera_grastate;
PSI_file_key key_FILE_galera_gvwstate;

PSI_file_info all_galera_files[] = {
    {&key_FILE_galera_recordset, "FILE_galera_recordset", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_FILE_galera_ringbuffer, "FILE_galera_ringbuffer", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_FILE_galera_gcache_page, "FILE_galera_gcache_page", 0, 0,
     PSI_DOCUMENT_ME},
    {&key_FILE_galera_grastate, "FILE_galera_grastate", 0, 0, PSI_DOCUMENT_ME},
    {&key_FILE_galera_gvwstate, "FILE_galera_gvwstate", 0, 0, PSI_DOCUMENT_ME}};

/* Vector to cache PSI key and mutex for corresponding galera mutex. */
typedef std::vector<void *> wsrep_psi_key_vec_t;
static wsrep_psi_key_vec_t wsrep_psi_key_vec;

LEX_CSTRING PXC_INTERNAL_SESSION_USER = {
    STRING_WITH_LEN("mysql.pxc.internal.session")};
LEX_CSTRING PXC_INTERNAL_SESSION_HOST = {STRING_WITH_LEN("localhost")};

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
void wsrep_pfs_instr_cb(wsrep_pfs_instr_type_t type, wsrep_pfs_instr_ops_t ops,
                        wsrep_pfs_instr_tag_t tag,
                        void **value __attribute__((unused)),
                        void **alliedvalue __attribute__((unused)),
                        const void *ts __attribute__((unused))) {
  assert(!wsrep_psi_key_vec.empty());

  if (type == WSREP_PFS_INSTR_TYPE_MUTEX) {
    switch (ops) {
      case WSREP_PFS_INSTR_OPS_INIT: {
        PSI_mutex_key *key =
            reinterpret_cast<PSI_mutex_key *>(wsrep_psi_key_vec[tag]);

        mysql_mutex_t *mutex = NULL;
        mutex = (mysql_mutex_t *)my_malloc(PSI_NOT_INSTRUMENTED,
                                           sizeof(mysql_mutex_t), MYF(0));
        mysql_mutex_init(*key, mutex, MY_MUTEX_INIT_FAST);

        /* Begin a structure and m_mutex is first element this
        should hold true. To make this appear therotically good
        we could use map but that comes at cost of map operation
        and mutex as STL map are not thread safe. */
        assert(reinterpret_cast<void *>(mutex) ==
               reinterpret_cast<void *>(&(mutex->m_mutex)));

        *value = &(mutex->m_mutex);

        break;
      }

      case WSREP_PFS_INSTR_OPS_DESTROY: {
        mysql_mutex_t *mutex = reinterpret_cast<mysql_mutex_t *>(*value);
        assert(mutex != NULL);

        mysql_mutex_destroy(mutex);
        my_free(mutex);
        *value = NULL;

        break;
      }

      case WSREP_PFS_INSTR_OPS_LOCK: {
        mysql_mutex_t *mutex = reinterpret_cast<mysql_mutex_t *>(*value);
        assert(mutex != NULL);

        mysql_mutex_lock(mutex);

        break;
      }

      case WSREP_PFS_INSTR_OPS_UNLOCK: {
        mysql_mutex_t *mutex = reinterpret_cast<mysql_mutex_t *>(*value);
        assert(mutex != NULL);

        mysql_mutex_unlock(mutex);

        break;
      }

      default:
        assert(0);
        break;
    }
  } else if (type == WSREP_PFS_INSTR_TYPE_CONDVAR) {
    switch (ops) {
      case WSREP_PFS_INSTR_OPS_INIT: {
        PSI_cond_key *key =
            reinterpret_cast<PSI_cond_key *>(wsrep_psi_key_vec[tag]);

        mysql_cond_t *cond = NULL;
        cond = (mysql_cond_t *)my_malloc(PSI_NOT_INSTRUMENTED,
                                         sizeof(mysql_cond_t), MYF(0));
        mysql_cond_init(*key, cond);

        /* Begin a structure and m_cond is first element this
        should hold true. To make this appear therotically good
        we could use map but that comes at cost of map operation
        and mutex as STL map are not thread safe. */
        assert(reinterpret_cast<void *>(cond) ==
               reinterpret_cast<void *>(&(cond->m_cond)));

        *value = &(cond->m_cond);
        break;
      }

      case WSREP_PFS_INSTR_OPS_DESTROY: {
        mysql_cond_t *cond = reinterpret_cast<mysql_cond_t *>(*value);
        assert(cond != NULL);

        mysql_cond_destroy(cond);
        my_free(cond);
        *value = NULL;

        break;
      }

      case WSREP_PFS_INSTR_OPS_WAIT: {
        mysql_cond_t *cond = reinterpret_cast<mysql_cond_t *>(*value);
        mysql_mutex_t *mutex = reinterpret_cast<mysql_mutex_t *>(*alliedvalue);
        assert(cond != NULL);

        mysql_cond_wait(cond, mutex);

        break;
      }

      case WSREP_PFS_INSTR_OPS_TIMEDWAIT: {
        mysql_cond_t *cond = reinterpret_cast<mysql_cond_t *>(*value);
        mysql_mutex_t *mutex = reinterpret_cast<mysql_mutex_t *>(*alliedvalue);
        const timespec *wtime = reinterpret_cast<const timespec *>(ts);
        assert(cond != NULL && mutex != NULL);

        mysql_cond_timedwait(cond, mutex, wtime);

        break;
      }

      case WSREP_PFS_INSTR_OPS_SIGNAL: {
        mysql_cond_t *cond = reinterpret_cast<mysql_cond_t *>(*value);
        assert(cond != NULL);

        mysql_cond_signal(cond);

        break;
      }

      case WSREP_PFS_INSTR_OPS_BROADCAST: {
        mysql_cond_t *cond = reinterpret_cast<mysql_cond_t *>(*value);
        assert(cond != NULL);

        mysql_cond_broadcast(cond);

        break;
      }

      default:
        assert(0);
        break;
    }
  } else if (type == WSREP_PFS_INSTR_TYPE_THREAD) {
    switch (ops) {
      case WSREP_PFS_INSTR_OPS_INIT: {
        PSI_thread_key *key =
            reinterpret_cast<PSI_thread_key *>(wsrep_psi_key_vec[tag]);

        wsrep_pfs_register_thread(*key);
        break;
      }

      case WSREP_PFS_INSTR_OPS_DESTROY: {
        wsrep_pfs_delete_thread();
        break;
      }

      default:
        assert(0);
        break;
    }
  } else if (type == WSREP_PFS_INSTR_TYPE_FILE) {
    switch (ops) {
      case WSREP_PFS_INSTR_OPS_CREATE: {
        PSI_file_key *key =
            reinterpret_cast<PSI_thread_key *>(wsrep_psi_key_vec[tag]);

        File *fd = reinterpret_cast<File *>(*value);
        const char *name = reinterpret_cast<const char *>(ts);

        PSI_file_locker_state state;
        struct PSI_file_locker *locker = NULL;

        wsrep_register_pfs_file_open_begin(
            &state, locker, *key, PSI_FILE_CREATE, name, __FILE__, __LINE__);

        wsrep_register_pfs_file_open_end(locker, *fd);

        break;
      }

      case WSREP_PFS_INSTR_OPS_OPEN: {
        PSI_file_key *key =
            reinterpret_cast<PSI_thread_key *>(wsrep_psi_key_vec[tag]);

        File *fd = reinterpret_cast<File *>(*value);
        const char *name = reinterpret_cast<const char *>(ts);

        PSI_file_locker_state state;
        struct PSI_file_locker *locker = NULL;

        wsrep_register_pfs_file_open_begin(&state, locker, *key, PSI_FILE_OPEN,
                                           name, __FILE__, __LINE__);

        wsrep_register_pfs_file_open_end(locker, *fd);

        break;
      }

      case WSREP_PFS_INSTR_OPS_CLOSE: {
        File *fd = reinterpret_cast<File *>(*value);

        PSI_file_locker_state state;
        struct PSI_file_locker *locker = NULL;

        wsrep_register_pfs_file_io_begin(&state, locker, *fd, 0, PSI_FILE_CLOSE,
                                         __FILE__, __LINE__);

        wsrep_register_pfs_file_io_end(locker, 0);

        break;
      }

      case WSREP_PFS_INSTR_OPS_DELETE: {
        PSI_file_key *key =
            reinterpret_cast<PSI_thread_key *>(wsrep_psi_key_vec[tag]);

        PSI_file_locker_state state;
        struct PSI_file_locker *locker = NULL;
        const char *name = reinterpret_cast<const char *>(ts);

        wsrep_register_pfs_file_close_begin(
            &state, locker, *key, PSI_FILE_DELETE, name, __FILE__, __LINE__);

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

static void wsrep_log_cb(wsrep::log::level level, const char *,
                         const char *msg) {
  switch (level) {
    case wsrep::log::info:
      WSREP_GALERA_LOG(INFORMATION_LEVEL, msg);
      break;
    case wsrep::log::warning:
      WSREP_GALERA_LOG(WARNING_LEVEL, msg);
      break;
    case wsrep::log::error:
      if (!Wsrep_server_state::instance().is_initialized_unprotected()) {
        pxc_force_flush_error_message = true;
        flush_error_log_messages();
      }
      WSREP_GALERA_LOG(ERROR_LEVEL, msg);
      break;
    case wsrep::log::debug:
      if (wsrep_debug) WSREP_GALERA_LOG(INFORMATION_LEVEL, msg);
    default:
      break;
  }
}

void wsrep_init_sidno(const wsrep::id &uuid) {
  /*
    Protocol versions starting from 4 use group gtid as it is.
    For lesser protocol versions generate new Sid map entry from inverted
    uuid.
  */
  rpl_sid sid;
  if (wsrep_protocol_version >= 4) {
    memcpy((void *)&sid, (const uchar *)uuid.data(), 16);
  } else {
    wsrep_uuid_t ltid_uuid;
    for (size_t i = 0; i < sizeof(ltid_uuid.data); ++i) {
      ltid_uuid.data[i] = ~((const uchar *)uuid.data())[i];
    }
    memcpy((void *)&sid, (const uchar *)ltid_uuid.data, 16);
  }

  global_sid_lock->wrlock();
  wsrep_sidno = global_sid_map->add_sid(sid);
  WSREP_INFO("Initialized wsrep sidno %d", wsrep_sidno);
  global_sid_lock->unlock();
}

bool wsrep_init_schema(THD *thd) {
  assert(!wsrep_schema);

  /*
   PXC upgrade requires modifications to some InnoDB tables.
   When the server is started without innodb, or without a read-write innodb,
   missing this user is a non-issue, since we won't participate in SST anyway.
   */
  handlerton *ddse = ha_resolve_by_legacy_type(thd, DB_TYPE_INNODB);
  if (ddse->is_dict_readonly && ddse->is_dict_readonly()) {
    LogErr(WARNING_LEVEL, ER_DD_NO_WRITES_NO_REPOPULATION, "InnoDB", " ");
    return false;
  }

  WSREP_INFO("wsrep_init_schema_and_SR %p", wsrep_schema);
  if (!wsrep_schema) {
    wsrep_schema = new Wsrep_schema();
    if (wsrep_schema->init(thd)) {
      WSREP_ERROR("Failed to init wsrep schema");
      return true;
    }
  }

  return false;
}

void wsrep_deinit_schema() {
  delete wsrep_schema;
  wsrep_schema = 0;
}

void wsrep_recover_sr_from_storage(THD *orig_thd) {
  switch (wsrep_SR_store_type) {
    case WSREP_SR_STORE_TABLE:
      if (!wsrep_schema) {
        WSREP_ERROR(
            "Wsrep schema not initialized when trying to recover "
            "streaming transactions");
        unireg_abort(1);
      }
      if (wsrep_schema->recover_sr_transactions(orig_thd)) {
        WSREP_ERROR("Failed to recover SR transactions from schema");
        unireg_abort(1);
      }
      break;
    default:
      /* */
      WSREP_ERROR("Unsupported wsrep SR store type: %lu", wsrep_SR_store_type);
      unireg_abort(1);
      break;
  }
}

/** Export the WSREP provider's capabilities as a human readable string.
 * The result is saved in a dynamically allocated string of the form:
 * :cap1:cap2:cap3:
 */
static void wsrep_capabilities_export(wsrep_cap_t const cap, char **str) {
  static const char *names[] = {
      /* Keep in sync with wsrep/wsrep_api.h WSREP_CAP_* macros. */
      "MULTI_MASTER",  "CERTIFICATION",     "PARALLEL_APPLYING",
      "TRX_REPLAY",    "ISOLATION",         "PAUSE",
      "CAUSAL_READS",  "CAUSAL_TRX",        "INCREMENTAL_WRITESET",
      "SESSION_LOCKS", "DISTRIBUTED_LOCKS", "CONSISTENCY_CHECK",
      "UNORDERED",     "ANNOTATION",        "PREORDERED",
      "STREAMING",     "SNAPSHOT",          "NBO",
  };

  std::string s;
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    if (cap & (1ULL << i)) {
      if (s.empty()) {
        s = ":";
      }
      s += names[i];
      s += ":";
    }
  }

  /* A read from the string pointed to by *str may be started at any time,
   * so it must never point to free(3)d memory or non '\0' terminated string. */

  char *const previous = *str;

  *str = strdup(s.c_str());

  if (previous != NULL) {
    free(previous);
  }
}

/* Verifies that SE position is consistent with the group position
 * and initializes other variables */
void wsrep_verify_SE_checkpoint(const wsrep_uuid_t &uuid
                                __attribute__((unused)),
                                wsrep_seqno_t const seqno
                                __attribute__((unused))) {}

/*
  Wsrep is considered ready if
  1) Provider is not loaded (native mode)
  2) Server has reached synced state
  3) Server is in joiner mode and mysqldump SST method has been
     specified
  See Wsrep_server_service::log_state_change() for further details.
 */
bool wsrep_ready_get(void) {
  if (mysql_mutex_lock(&LOCK_wsrep_ready)) abort();
  bool ret = wsrep_ready;
  mysql_mutex_unlock(&LOCK_wsrep_ready);
  return ret;
}

int wsrep_show_ready(THD *, SHOW_VAR *var, char *buff) {
  var->type = SHOW_MY_BOOL;
  var->value = buff;
  *((bool *)buff) = wsrep_ready_get();
  return 0;
}

// Wait until wsrep has reached ready state
void wsrep_ready_wait() {
  if (mysql_mutex_lock(&LOCK_wsrep_ready)) abort();
  while (!wsrep_ready) {
    WSREP_DEBUG("Waiting to reach ready state");
    mysql_cond_wait(&COND_wsrep_ready, &LOCK_wsrep_ready);
  }
  WSREP_DEBUG("Ready state reached");
  mysql_mutex_unlock(&LOCK_wsrep_ready);
}

void wsrep_update_cluster_state_uuid(const char *uuid) {
  strncpy(cluster_uuid_str, uuid, sizeof(cluster_uuid_str) - 1);
  cluster_uuid_str[sizeof(cluster_uuid_str) - 1] = '\0';
}

static void wsrep_init_position() {}

/****************************************************************************
                         Helpers for wsrep_init()
****************************************************************************/
static std::string wsrep_server_name() {
  std::string ret(wsrep_node_name ? wsrep_node_name : "");
  return ret;
}

static std::string wsrep_server_id() {
  /* using empty server_id, which enables view change handler to
     set final server_id later on
  */
  std::string ret("");
  return ret;
}

static std::string wsrep_server_node_address() {
  std::string ret;
  if (!wsrep_data_home_dir || strlen(wsrep_data_home_dir) == 0)
    wsrep_data_home_dir = mysql_real_data_home;

  /* Initialize node address */
  if (!wsrep_node_address || !strcmp(wsrep_node_address, "")) {
    char node_addr[512] = {
        0,
    };
    const size_t node_addr_max = sizeof(node_addr) - 1;
    size_t guess_ip_ret = wsrep_guess_ip(node_addr, node_addr_max);
    if (!(guess_ip_ret > 0 && guess_ip_ret < node_addr_max)) {
      WSREP_WARN(
          "Failed to guess base node address. Set it explicitly via "
          "wsrep_node_address.");
    } else {
      ret = node_addr;
    }
  } else {
    ret = wsrep_node_address;
  }
  return ret;
}

static std::string wsrep_server_incoming_address() {
  std::string ret;
  const std::string node_addr(wsrep_server_node_address());
  char inc_addr[512] = {
      0,
  };
  size_t const inc_addr_max = sizeof(inc_addr);

  /*
    In case wsrep_node_incoming_address is either not set or set to AUTO,
    we need to use mysqld's my_bind_addr_str:mysqld_port, lastly fallback
    to wsrep_node_address' value if mysqld's bind-address is not set either.
  */
  if ((!wsrep_node_incoming_address ||
       !strcmp(wsrep_node_incoming_address, WSREP_NODE_INCOMING_AUTO))) {
    bool is_ipv6 = false;
    unsigned int my_bind_ip = INADDR_ANY;  // default if not set
    char *single_addr = nullptr;

    if (my_bind_addr_str && strlen(my_bind_addr_str) &&
        strcmp(my_bind_addr_str, "*") != 0) {
      size_t single_addr_buf_size = strlen(my_bind_addr_str) + 1;
      single_addr =
          (char *)my_malloc(key_memory_wsrep, single_addr_buf_size, MYF(0));
      if (!single_addr) goto done;

      wsrep_get_single_address(my_bind_addr_str, single_addr,
                               single_addr_buf_size);

      my_bind_ip = wsrep_check_ip(single_addr, &is_ipv6);
    }

    if (INADDR_ANY != my_bind_ip) {
      /*
        If its a not a valid address, leave inc_addr as empty string. mysqld
        is not listening for client connections on network interfaces.
      */
      if (INADDR_NONE != my_bind_ip && INADDR_LOOPBACK != my_bind_ip) {
        const char *fmt = (is_ipv6) ? "[%s]:%u" : "%s:%u";
        // if we are here, single_addr is not nullptr
        snprintf(inc_addr, inc_addr_max, fmt, single_addr, mysqld_port);
      }
    } else /* mysqld binds to 0.0.0.0, try taking IP from wsrep_node_address. */
    {
      if (node_addr.size()) {
        size_t const ip_len =
            wsrep_host_len(node_addr.c_str(), node_addr.size());
        if (ip_len + 7 /* :55555\0 */ < inc_addr_max) {
          memcpy(inc_addr, node_addr.c_str(), ip_len);
          snprintf(inc_addr + ip_len, inc_addr_max - ip_len, ":%u",
                   (int)mysqld_port);
        } else {
          WSREP_WARN(
              "Guessing address for incoming client connections: "
              "address too long.");
        }
      }

      if (!strlen(inc_addr)) {
        WSREP_WARN(
            "Guessing address for incoming client connections failed. "
            "Try setting wsrep_node_incoming_address explicitly.");
        WSREP_INFO("Node addr: %s", node_addr.c_str());
      }
    }
    if (single_addr) my_free(single_addr);
  } else {
    wsp::Address addr(wsrep_node_incoming_address);

    if (!addr.is_valid()) {
      WSREP_WARN("Could not parse wsrep_node_incoming_address : %s",
                 wsrep_node_incoming_address);
      goto done;
    }

    /*
      In case port is not specified in wsrep_node_incoming_address, we use
      mysqld_port.
    */
    int port = (addr.get_port() > 0) ? addr.get_port() : (int)mysqld_port;
    const char *fmt = (addr.is_ipv6()) ? "[%s]:%u" : "%s:%u";

    snprintf(inc_addr, inc_addr_max, fmt, addr.get_address(), port);
  }

done:

  // inc_addr contains proper address, or is empty string
  ret = inc_addr;
  return ret;
}

static std::string wsrep_server_working_dir() {
  std::string ret;
  if (!wsrep_data_home_dir || strlen(wsrep_data_home_dir) == 0) {
    ret = mysql_real_data_home;
  } else {
    ret = wsrep_data_home_dir;
  }
  return ret;
}

static wsrep::gtid wsrep_server_initial_position() {
  wsrep::gtid ret;
  WSREP_DEBUG("Server initial position: %s", wsrep_start_position);
  std::istringstream is(wsrep_start_position);
  is >> ret;
  return ret;
}

/*
  Intitialize provider specific status variables
 */
static void wsrep_init_provider_status_variables() {
  const wsrep::provider &provider = Wsrep_server_state::instance().provider();
  strncpy(provider_name, provider.name().c_str(), sizeof(provider_name) - 1);
  strncpy(provider_version, provider.version().c_str(),
          sizeof(provider_version) - 1);
  strncpy(provider_vendor, provider.vendor().c_str(),
          sizeof(provider_vendor) - 1);

  /* All three buffers are zero-initialized on declaration, but
     let's avoid the confusion */
  provider_name[sizeof(provider_name) - 1] = '\0';
  provider_version[sizeof(provider_version) - 1] = '\0';
  provider_vendor[sizeof(provider_vendor) - 1] = '\0';
}

/**
   Updates provider params for variables which have an one to one mapping from
   server variable to provider params.

   Currently, we only have one such mapping. i.e,

        wsrep_max_ws_size => repl.max_ws_size

   If it is found that other server variables also have a corresponding param
   in provider, then add them in the end of the function.

   @return true if error, false otherwise
*/
bool wsrep_update_provider_params() {
  /*
     Check if repl.max_ws_size has been specified in the wsrep_provider_options
     in the cnf file.
  */
  ulong provider_max_ws_size;
  if (get_provider_option_value(wsrep_provider_options, "repl.max_ws_size",
                                &provider_max_ws_size) == 0) {
    /*
       If repl.max_ws_size is not defined in cnf file (or through command
       line), then set the repl.max_ws_size provider parameter from
       wsrep_max_ws_size.
    */
    wsrep_max_ws_size = provider_max_ws_size;
  }
  if (wsrep_max_ws_size_update(nullptr, nullptr, OPT_GLOBAL)) {
    return true;
  }
  return false;
}

int wsrep_init_server() {
  wsrep::log::logger_fn(wsrep_log_cb);
  try {
    std::string server_name;
    std::string server_id;
    std::string node_address;
    std::string incoming_address;
    std::string working_dir;
    wsrep::gtid initial_position;

    server_name = wsrep_server_name();
    server_id = wsrep_server_id();
    node_address = wsrep_server_node_address();
    incoming_address = wsrep_server_incoming_address();
    working_dir = wsrep_server_working_dir();
    initial_position = wsrep_server_initial_position();

    Wsrep_server_state::init_once(server_name, incoming_address, node_address,
                                  working_dir, initial_position,
                                  wsrep_max_protocol_version);
    Wsrep_server_state::instance().debug_log_level(wsrep_debug);
  } catch (const wsrep::runtime_error &e) {
    WSREP_ERROR("Failed to init wsrep server %s", e.what());
    return 1;
  } catch (const std::exception &e) {
    WSREP_ERROR("Failed to init wsrep server %s", e.what());
  }

#ifdef HAVE_PSI_INTERFACE
  if (wsrep_psi_key_vec.empty()) {
    /* Register all galera mutexes. This is one-time activity and so
    avoid re-doing it if the provider is re-initialized. */
    const char *category = "galera";
    unsigned int count;

    count = array_elements(all_galera_mutexes);
    mysql_mutex_register(category, all_galera_mutexes, count);

    for (unsigned int i = 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
          reinterpret_cast<void *>(all_galera_mutexes[i].m_key));

    count = array_elements(all_galera_condvars);
    mysql_cond_register(category, all_galera_condvars, count);

    for (unsigned int i = 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
          reinterpret_cast<void *>(all_galera_condvars[i].m_key));

    count = array_elements(all_galera_threads);
    mysql_thread_register(category, all_galera_threads, count);

    for (unsigned int i = 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
          reinterpret_cast<void *>(all_galera_threads[i].m_key));

    count = array_elements(all_galera_files);
    mysql_file_register(category, all_galera_files, count);

    for (unsigned int i = 0; i < count; ++i)
      wsrep_psi_key_vec.push_back(
          reinterpret_cast<void *>(all_galera_files[i].m_key));
  }
#endif /* HAVE_PSI_INTERFACE */

  return 0;
}

void wsrep_init_globals() {
  wsrep_init_sidno(Wsrep_server_state::instance().connected_gtid().id());
  if (WSREP_ON) {
    Wsrep_server_state::instance().initialized();
  }
}

void wsrep_deinit_server() {
  wsrep_deinit_schema();
  Wsrep_server_state::destroy();
}

static void override_galera_option(const std::string& option, const std::string& new_value, std::string& options) {
  size_t option_pos = options.find(option + "=");
  if (option_pos == std::string::npos) {
    option_pos = options.find(option + " ");
  }
  // option_pos == std::string::npos means there is no such
  // option in the string. We just need to add the new one.
  // All below logic still works.

  // pos points to the beginning of the option we want to override.
  // Find the separator
  size_t separator_pos = options.find(";", option_pos);

  // erase the option form the original string
  std::string modified_options = options.substr(0, option_pos);
  if (separator_pos != std::string::npos) {
    modified_options += options.substr(separator_pos+1);
  }

  // add the new value of the option at the end
  modified_options += ";" + option + "=" + new_value;

  options = modified_options;
}

static void setup_galera_encryption_params(char* provider_options, size_t buf_size) {
  std::string options(provider_options);
  static const std::string yes("yes");
  static const std::string no("no");

  if (wsrep_gcache_encrypt != WSREP_ENCRYPT_MODE_NONE) {
    const std::string& val = wsrep_gcache_encrypt == WSREP_ENCRYPT_MODE_ON ? yes : no;
    override_galera_option("gcache.encryption", val, options);
  }
  if (wsrep_disk_pages_encrypt != WSREP_ENCRYPT_MODE_NONE) {
    const std::string& val = wsrep_disk_pages_encrypt == WSREP_ENCRYPT_MODE_ON ? yes : no;
    override_galera_option("allocator.disk_pages_encryption", val, options);
  }

  snprintf(provider_options, buf_size, "%s", options.c_str());
  provider_options[buf_size] = 0;
}

int wsrep_init() {
  assert(wsrep_provider);

  wsrep_init_position();
  // wsrep_sst_auth_init();

  if (strlen(wsrep_provider) == 0 || !strcmp(wsrep_provider, WSREP_NONE)) {
    // enable normal operation in case no provider is specified
    global_system_variables.wsrep_on = 0;
    wsrep_provider_set = false;
    int err = Wsrep_server_state::instance().load_provider(
        wsrep_provider, wsrep_provider_options ? wsrep_provider_options : "");
    if (err) {
      DBUG_PRINT("wsrep", ("wsrep::init() failed: %d", err));
      WSREP_ERROR("wsrep::init() failed: %d, must shutdown", err);
    } else {
      wsrep_init_provider_status_variables();
    }
    return err;
  } else {
    /* There is provider. Initialize master key manager.
       Even if there is no kering plugin loaded, it will initialize,
       but will be useless. In such a case if Galera GCache encryption
       is enabled, it will fail on master key fetch/generate. */
    if (wsrep_init_master_key()) {
      WSREP_ERROR(
          "wsrep::init() master key initialization failed, must shutdown");
      return 1;
    }
  }

  global_system_variables.wsrep_on = 1;

  const char *provider_options = wsrep_provider_options;
  static const size_t buf_size = 4096;
  char buffer[buf_size];

  if (pxc_encrypt_cluster_traffic) {
    if (!server_main_callback.is_wsrep_context_initialized()) {
      WSREP_ERROR(
          "ssl-ca, ssl-cert, and ssl-key must all be defined"
          " to use encrypted mode traffic. Unable to configure SSL."
          " Must shutdown.");
      return 1;
    }

    char ssl_opts[buf_size];
    server_main_callback.populate_wsrep_ssl_options(ssl_opts, sizeof(ssl_opts));
    MY_COMPILER_GCC_DIAGNOSTIC_PUSH();
    MY_COMPILER_GCC_DIAGNOSTIC_IGNORE("-Wformat-truncation");
    snprintf(buffer, buf_size, "%s%s%s",
             provider_options ? provider_options : "",
             ((provider_options && *provider_options) ? ";" : ""), ssl_opts);
    MY_COMPILER_GCC_DIAGNOSTIC_POP();
  } else {
    snprintf(buffer, buf_size, "%s", provider_options ? provider_options : "");
  }
  buffer[buf_size] = 0;

  /* Setup GCache and WSCache encryption according to
      wsrep_gcache_encrypt and wsrep_disk_pages_encrypt.
      This is mainly for integration with MTR test suite
      which does not allow overriding of particular wsrep_provider_options'
      options via combinations file */
  setup_galera_encryption_params(buffer, buf_size);

  provider_options = buffer;

  if (!wsrep_data_home_dir || strlen(wsrep_data_home_dir) == 0)
    wsrep_data_home_dir = mysql_real_data_home;

  if (Wsrep_server_state::instance().load_provider(wsrep_provider,
                                                   provider_options)) {
    WSREP_ERROR("Failed to load provider");
    return 1;
  }

  if (Wsrep_server_state::get_provider().capabilities() == 0) {
    WSREP_ERROR(
        "Unable to discover capabilities for the given provider."
        " Check if protocol version is correctly configured");
    Wsrep_server_state::instance().unload_provider();
    return 1;
  }

  if (!wsrep_provider_is_SR_capable() &&
      global_system_variables.wsrep_trx_fragment_size > 0) {
    WSREP_ERROR(
        "The WSREP provider (%s) does not support streaming "
        "replication but wsrep_trx_fragment_size is set to a "
        "value other than 0 (%llu). Cannot continue. Either set "
        "wsrep_trx_fragment_size to 0 or use wsrep_provider that "
        "supports streaming replication.",
        wsrep_provider, global_system_variables.wsrep_trx_fragment_size);
    Wsrep_server_state::instance().unload_provider();
    return 1;
  }

  wsrep_init_provider_status_variables();

  if (wsrep_update_provider_params()) {
    WSREP_ERROR("Unable to update provider parameters.");
    Wsrep_server_state::instance().unload_provider();
    return 1;
  };

  wsrep_capabilities_export(
      Wsrep_server_state::instance().provider().capabilities(),
      &wsrep_provider_capabilities);

  WSREP_DEBUG("SR storage init for: %s",
              (wsrep_SR_store_type == WSREP_SR_STORE_TABLE) ? "table" : "void");

  return 0;
}

void wsrep_init_startup(bool sst_first) {
  if (wsrep_init()) unireg_abort(1);

  wsrep_thr_lock_init(wsrep_thd_is_BF, wsrep_abort_thd, wsrep_debug, wsrep_on);

  /* Skip replication start if dummy wsrep provider is loaded */
  if (!strcmp(wsrep_provider, WSREP_NONE)) return;

  /* Skip replication start if no cluster address */
  if (!wsrep_cluster_address || strlen(wsrep_cluster_address) == 0) return;

  if (!wsrep_start_replication()) unireg_abort(1);

  wsrep_create_rollbacker();
  wsrep_create_appliers(1);

  Wsrep_server_state &server_state = Wsrep_server_state::instance();
  /*
    If the SST happens before server initialization, wait until the server
    state reaches initializing. This indicates that
    either SST was not necessary or SST has been delivered.

    With mysqldump SST (!sst_first) wait until the server reaches
    joiner state and proceed to accepting connections.
  */
  try {
    if (sst_first) {
      server_state.wait_until_state(Wsrep_server_state::s_initializing);
    } else {
      server_state.wait_until_state(Wsrep_server_state::s_joiner);
    }
  } catch (const wsrep::runtime_error &e) {
    // While waiting for 'initializing' or 'joiner' state we got 'disconnecting'
    // state. It means that something went wrong during wsrep provider
    // initialization and we cannot recover anyway.
    unireg_abort(MYSQLD_ABORT_EXIT);
  }

  // We need to force keyring cache to repopulate
  // after SST, because SST might have add some keys.
  // GCache recovery won't work without this.
  if (keyring_status_no_error() && srv_keyring_load) {
    srv_keyring_load->load(opt_plugin_dir, mysql_real_data_home);
  }
  plugin_reinit_keyring();
}

void wsrep_deinit() {
  WSREP_DEBUG("wsrep_deinit");

  Wsrep_server_state::instance().unload_provider();
  provider_name[0] = '\0';
  provider_version[0] = '\0';
  provider_vendor[0] = '\0';

  if (wsrep_provider_capabilities != NULL) {
    char *p = wsrep_provider_capabilities;
    wsrep_provider_capabilities = NULL;
    free(p);
  }
  wsrep_deinit_master_key();
}

void wsrep_recover() {
#if 0
  if (wsrep_uuid_compare(&local_uuid, &WSREP_UUID_UNDEFINED) == 0 &&
      local_seqno == -2)
  {
    char uuid_str[40];
    wsrep_uuid_print(&local_uuid, uuid_str, sizeof(uuid_str));
    WSREP_INFO("Position %s:%lld given at startup, skipping position recovery",
               uuid_str, (long long)local_seqno);
    return;
  }
#endif
  wsrep::gtid gtid = wsrep_get_SE_checkpoint();
  std::ostringstream oss;
  oss << gtid;
  WSREP_INFO("Recovered position: %s", oss.str().c_str());
}

void wsrep_stop_replication(THD *thd, bool is_server_shutdown) {
  WSREP_INFO("Stop replication by %u", (thd) ? thd->thread_id() : 0);
  if (Wsrep_server_state::instance().state() !=
      Wsrep_server_state::s_disconnected) {
    WSREP_DEBUG("Disconnect provider");
    Wsrep_server_state::instance().disconnect();
    Wsrep_server_state::instance().wait_until_state(
        Wsrep_server_state::s_disconnected);
    wsrep_connected = false;
  }

  /* my connection, should not terminate with wsrep_close_client_connection(),
     make transaction to rollback
  */
  if (thd && !thd->wsrep_applier) trans_rollback(thd);

  /*
   * On shutdown, let the normal mysqld shutdown code close the client
   * connections.  See signal_hand() in mysqld.cc.
   */
  if (!is_server_shutdown) wsrep_close_client_connections(true, false);

  /* wait until appliers have stopped */
  wsrep_wait_appliers_close(thd);

  node_uuid = WSREP_UUID_UNDEFINED;
  return;
}

void wsrep_shutdown_replication() {
  WSREP_INFO("Shutdown replication");
  if (Wsrep_server_state::instance().state() !=
      wsrep::server_state::s_disconnected) {
    WSREP_DEBUG("Disconnect provider");
    Wsrep_server_state::instance().disconnect();
    Wsrep_server_state::instance().wait_until_state(
        Wsrep_server_state::s_disconnected);
    wsrep_connected = false;
  }

#if 0
  // Avoid closing client connections if server is not being
  // SHUTDOWN only provider is being shutdown.
  // wsrep_shutdown_replication is invoked when server is being shutdown.
  wsrep_close_client_connections(true, true);
#endif

  /* wait until appliers have stopped */
  wsrep_wait_appliers_close(NULL);
  node_uuid = WSREP_UUID_UNDEFINED;
}

/* This one is set to true when --wsrep-new-cluster is found in the command
 * line arguments */
bool wsrep_new_cluster = false;
#define WSREP_NEW_CLUSTER "--wsrep-new-cluster"
/* Finds and hides --wsrep-new-cluster from the arguments list
 * by moving it to the end of the list and decrementing argument count */
void wsrep_filter_new_cluster(int *argc, char *argv[]) {
  int i;
  for (i = *argc - 1; i > 0; i--) {
    /* make a copy of the argument to convert possible underscores to hyphens.
     * the copy need not to be longer than WSREP_NEW_CLUSTER option */
    char arg[sizeof(WSREP_NEW_CLUSTER) + 1];
    strncpy(arg, argv[i], sizeof(arg) - 1);
    arg[sizeof(arg) - 1] = '\0';
    char *underscore(arg);
    while (NULL != (underscore = strchr(underscore, '_'))) *underscore = '-';

    if (!strcmp(arg, WSREP_NEW_CLUSTER)) {
      wsrep_new_cluster = true;
      *argc -= 1;
      /* preserve the order of remaining arguments AND
       * preserve the original argument pointers - just in case */
      char *wnc = argv[i];
      memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(argv[i]));
      argv[*argc] = wnc; /* this will be invisible to the rest of the program */
    }
  }
}

bool wsrep_start_replication() {
  int rcode;
  WSREP_DEBUG("wsrep_start_replication");

  /*
    if provider is trivial, don't even try to connect,
    but resume local node operation
  */
  if (strlen(wsrep_provider) == 0 || !strcmp(wsrep_provider, WSREP_NONE)) {
    // enable normal operation in case no provider is specified
    return true;
  }

  if (!wsrep_cluster_address || strlen(wsrep_cluster_address) == 0) {
    // if provider is non-trivial, but no address is specified, wait for address
    return true;
  }

  bool const bootstrap(true == wsrep_new_cluster);
  wsrep_new_cluster = false;

  WSREP_INFO("Starting replication");

  if ((rcode = Wsrep_server_state::instance().connect(
           wsrep_cluster_name, wsrep_cluster_address, wsrep_sst_donor,
           bootstrap))) {
    DBUG_PRINT("wsrep", ("wsrep_ptr->connect(%s) failed: %d",
                         wsrep_cluster_address, rcode));
    WSREP_ERROR(
        "Provider/Node (%s) failed to establish connection with cluster"
        " (reason: %d)",
        wsrep_cluster_address, rcode);
    return false;
  } else {
    wsrep_connected = true;
    try {
      std::string opts = Wsrep_server_state::instance().provider().options();
      wsrep_provider_options_init(opts.c_str());
    } catch (const wsrep::runtime_error &) {
      WSREP_WARN("Failed to get wsrep options");
    }
  }

  return true;
}

bool wsrep_must_sync_wait(THD *thd, uint mask) {
  bool ret;
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);
  ret = (thd->variables.wsrep_sync_wait & mask) && thd->wsrep_client_thread &&
        WSREP_ON && thd->variables.wsrep_on &&
        !(thd->variables.wsrep_dirty_reads &&
          !is_update_query(thd->lex->sql_command)) &&
        !thd->in_active_multi_stmt_transaction() &&
        thd->wsrep_trx().state() != wsrep::transaction::s_replaying &&
        thd->wsrep_cs().sync_wait_gtid().is_undefined();
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
  return ret;
}

bool wsrep_sync_wait(THD *thd, uint mask) {
  if (wsrep_must_sync_wait(thd, mask)) {
    WSREP_DEBUG(
        "wsrep_sync_wait: thd->variables.wsrep_sync_wait= %u, "
        "mask= %u, thd->variables.wsrep_on= %d",
        thd->variables.wsrep_sync_wait, mask, thd->variables.wsrep_on);

    /*
      This allows autocommit SELECTs and a first SELECT after SET AUTOCOMMIT=0
      TODO: modify to check if thd has locked any rows.
    */
    if (thd->wsrep_cs().sync_wait(-1)) {
      const char *msg;
      int err;

      /*
        Possibly relevant error codes:
        ER_CHECKREAD, ER_ERROR_ON_READ, ER_INVALID_DEFAULT, ER_EMPTY_QUERY,
        ER_FUNCTION_NOT_DEFINED, ER_NOT_ALLOWED_COMMAND, ER_NOT_SUPPORTED_YET,
        ER_FEATURE_DISABLED, ER_QUERY_INTERRUPTED
      */

      switch (thd->wsrep_cs().current_error()) {
        case wsrep::e_not_supported_error:
          msg =
              "synchronous reads by wsrep backend. "
              "Please unset wsrep_causal_reads variable.";
          err = ER_NOT_SUPPORTED_YET;
          break;
        default:
          msg = "Synchronous wait failed.";
          err = ER_LOCK_WAIT_TIMEOUT;  // HERE! NOTE: the above msg won't be
                                       // displayed
                                       //       with ER_LOCK_WAIT_TIMEOUT
      }
      my_error(err, MYF(0), msg);

      return true;
    }
  }

  return false;
}

enum wsrep::provider::status wsrep_sync_wait_upto_gtid(THD *thd
                                                       __attribute__((unused)),
                                                       wsrep_gtid_t *upto,
                                                       int timeout) {
  assert(upto);
  enum wsrep::provider::status ret;
  if (upto) {
    wsrep::gtid upto_gtid(wsrep::id(upto->uuid.data, sizeof(upto->uuid.data)),
                          wsrep::seqno(upto->seqno));
    ret = Wsrep_server_state::instance().wait_for_gtid(upto_gtid, timeout);
  } else {
    ret = Wsrep_server_state::instance().causal_read(timeout).second;
  }
  WSREP_DEBUG("wsrep_sync_wait_upto_gtid: %d", ret);
  return ret;
}

void wsrep_keys_free(wsrep_key_arr_t *key_arr) {
  for (size_t i = 0; i < key_arr->keys_len; ++i) {
    my_free(const_cast<wsrep_buf_t *>(key_arr->keys[i].key_parts));
  }
  my_free(key_arr->keys);
  key_arr->keys = 0;
  key_arr->keys_len = 0;
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

static bool wsrep_prepare_key_for_isolation(const char *db, const char *table,
                                            wsrep_buf_t *key, size_t *key_len) {
  if (*key_len < 2) return false;

  switch (wsrep_protocol_version) {
    case 0:
      *key_len = 0;
      break;
    case 1:
    case 2:
    case 3:
    case 4: {
      *key_len = 0;
      if (db) {
        key[*key_len].ptr = db;
        key[*key_len].len = strlen(db);
        ++(*key_len);
        if (table) {
          key[*key_len].ptr = table;
          key[*key_len].len = strlen(table);
          ++(*key_len);
        }
      }
      break;
    }
    default:
      assert(0);
      WSREP_ERROR("Unsupported protocol version: %ld", wsrep_protocol_version);
      unireg_abort(1);
      return false;
  }
  return true;
}

static bool wsrep_prepare_key_for_isolation(const char *db, const char *table,
                                            wsrep_key_arr_t *ka) {
  wsrep_key_t *tmp;
  tmp = (wsrep_key_t *)my_realloc(key_memory_wsrep, ka->keys,
                                  (ka->keys_len + 1) * sizeof(wsrep_key_t),
                                  MYF(MY_ALLOW_ZERO_PTR));
  if (!tmp) {
    WSREP_ERROR("Can't allocate memory for key_array");
    return false;
  }
  ka->keys = tmp;
  if (!(ka->keys[ka->keys_len].key_parts = (wsrep_buf_t *)my_malloc(
            key_memory_wsrep, sizeof(wsrep_buf_t) * 2, MYF(0)))) {
    WSREP_ERROR("Can't allocate memory for key_parts");
    return false;
  }
  ka->keys[ka->keys_len].key_parts_num = 2;
  ++ka->keys_len;
  if (!wsrep_prepare_key_for_isolation(
          db, table,
          const_cast<wsrep_buf_t *>(ka->keys[ka->keys_len - 1].key_parts),
          &ka->keys[ka->keys_len - 1].key_parts_num)) {
    WSREP_ERROR("Preparing keys for isolation failed");
    return false;
  }

  return true;
}

static bool wsrep_prepare_keys_for_alter_add_fk(const char *child_table_db,
                                                Alter_info *alter_info,
                                                wsrep_key_arr_t *ka) {
  for (const Key_spec *key : alter_info->key_list) {
    if (key->type == KEYTYPE_FOREIGN) {
      const Foreign_key_spec *fk_key = down_cast<const Foreign_key_spec *>(key);
      const char *db_name = fk_key->ref_db.str;
      const char *table_name = fk_key->ref_table.str;
      if (!db_name) {
        db_name = child_table_db;
      }
      if (!wsrep_prepare_key_for_isolation(db_name, table_name, ka)) {
        return false;
      }
    }
  }
  return true;
}

static bool wsrep_prepare_keys_for_isolation(THD *, const char *db,
                                             const char *table,
                                             const TABLE_LIST *table_list,
                                             Alter_info *alter_info,
                                             wsrep_key_arr_t *ka) {
  ka->keys = 0;
  ka->keys_len = 0;

  if (db || table) {
    TABLE_LIST tmp_table;

    memset(static_cast<void *>(&tmp_table), 0, sizeof(tmp_table));
    tmp_table.table_name = const_cast<char *>(table);
    tmp_table.db = const_cast<char *>(db);
    MDL_REQUEST_INIT(&tmp_table.mdl_request, MDL_key::GLOBAL, (db) ? db : "",
                     (table) ? table : "", MDL_INTENTION_EXCLUSIVE,
                     MDL_STATEMENT);

    if (!wsrep_prepare_key_for_isolation(db, table, ka)) goto err;
  }

  for (const TABLE_LIST *table_it = table_list; table_it;
       table_it = table_it->next_global) {
    if (!wsrep_prepare_key_for_isolation(table_it->db, table_it->table_name,
                                         ka))
      goto err;
  }

  if (alter_info && (alter_info->flags & Alter_info::ADD_FOREIGN_KEY)) {
    if (!wsrep_prepare_keys_for_alter_add_fk(table_list->db, alter_info, ka))
      goto err;
  }

  return false;

err:
  wsrep_keys_free(ka);
  return true;
}

/*
 * Prepare key list from db/table and table_list
 *
 * Return zero in case of success, 1 in case of failure.
 */
bool wsrep_prepare_keys_for_isolation(THD *thd, const char *db,
                                      const char *table,
                                      const TABLE_LIST *table_list,
                                      wsrep_key_arr_t *ka) {
  return wsrep_prepare_keys_for_isolation(thd, db, table, table_list, NULL, ka);
}

bool wsrep_prepare_key_for_innodb(const uchar *cache_key, size_t cache_key_len,
                                  const uchar *row_id, size_t row_id_len,
                                  wsrep_buf_t *key, size_t *key_len) {
  if (*key_len < 3) return false;

  *key_len = 0;
  switch (wsrep_protocol_version) {
    case 0: {
      key[0].ptr = cache_key;
      key[0].len = cache_key_len;

      *key_len = 1;
      break;
    }
    case 1:
    case 2:
    case 3:
    case 4: {
      key[0].ptr = cache_key;
      key[0].len =
          strlen(reinterpret_cast<char *>(const_cast<uchar *>(cache_key)));

      key[1].ptr =
          cache_key +
          strlen(reinterpret_cast<char *>(const_cast<uchar *>(cache_key))) + 1;
      key[1].len = strlen(static_cast<char *>(const_cast<void *>(key[1].ptr)));

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

wsrep::key wsrep_prepare_key_for_toi(const char *db, const char *table,
                                     enum wsrep::key::type type) {
  wsrep::key ret(type);
  assert(db);
  ret.append_key_part(db, strlen(db));
  if (table) ret.append_key_part(table, strlen(table));
  return ret;
}

wsrep::key_array wsrep_prepare_keys_for_alter_add_fk(const char *child_table_db,
                                                     Alter_info *alter_info)

{
  wsrep::key_array ret;

  for (const Key_spec *key : alter_info->key_list) {
    if (key->type == KEYTYPE_FOREIGN) {
      const Foreign_key_spec *fk_key = down_cast<const Foreign_key_spec *>(key);
      const char *db_name = fk_key->ref_db.str;
      const char *table_name = fk_key->ref_table.str;
      if (!db_name) {
        db_name = child_table_db;
      }
      ret.push_back(wsrep_prepare_key_for_toi(db_name, table_name,
                                              wsrep::key::exclusive));
    }
  }

  return ret;
}

wsrep::key_array wsrep_prepare_keys_for_toi(const char *db, const char *table,
                                            const TABLE_LIST *table_list,
                                            dd::Tablespace_table_ref_vec *trefs,
                                            Alter_info *alter_info,
                                            wsrep::key_array *fk_tables) {
  wsrep::key_array ret;
  if (db || table) {
    ret.push_back(wsrep_prepare_key_for_toi(db, table, wsrep::key::shared));
  }
  for (const TABLE_LIST *table_it = table_list; table_it;
       table_it = table_it->next_global) {
    ret.push_back(wsrep_prepare_key_for_toi(table_it->get_db_name(),
                                            table_it->get_table_name(),
                                            wsrep::key::shared));
  }
  if (alter_info && (alter_info->flags & Alter_info::ADD_FOREIGN_KEY)) {
    wsrep::key_array fk(wsrep_prepare_keys_for_alter_add_fk(
        (table_list ? table_list->get_db_name() : db), alter_info));
    if (!fk.empty()) {
      ret.insert(ret.end(), fk.begin(), fk.end());
    }
  }
  if (trefs) {
    for (auto &tref : *trefs) {
      ret.push_back(wsrep_prepare_key_for_toi(tref.m_schema_name.c_str(),
                                              tref.m_name.c_str(),
                                              wsrep::key::exclusive));
    }
  }
  if (fk_tables && !fk_tables->empty()) {
    ret.insert(ret.end(), fk_tables->begin(), fk_tables->end());
  }
  return ret;
}

/*
 * Iterate over the list of tables and for each non-temporary invoke the
 * action function.
 */
using action_fn = std::function<void(const dd::Table *)>;
static bool wsrep_do_action_for_tables(THD *thd, TABLE_LIST *tables,
                                       action_fn action) {
  if (!WSREP(thd) || !WSREP_CLIENT(thd)) return false;

  // If we fail to collect FK meta data, we set the error and return true.
  // That will trigger retry logic inside wsrep_dispatch_sql_command()
  for (auto table = tables; table; table = table->next_local) {
    if (!is_temporary_table(table)) {
      DBUG_EXECUTE_IF("wsrep_do_action_for_tables_lock_fail", {
        my_error(ER_LOCK_DEADLOCK, MYF(0));
        return true;
      });

      MDL_ticket *mdl_ticket = nullptr;
      if (dd::acquire_shared_table_mdl(thd, table->db, table->table_name, false,
                                       &mdl_ticket)) {
        WSREP_WARN(
            "Unable to acquire shared lock on db: %s, table: %s for FKs "
            "collection",
            table->db, table->table_name);
        my_error(ER_LOCK_DEADLOCK, MYF(0));
        return true;
      }

      DEBUG_SYNC(thd, "wsrep_do_action_for_tables_after_lock");

      {
        /*
          Dictionary_client does aggressive validation against MDLs locked
          during the time when DD objects are acquired.
          Keep the MDL lock as long as the Auto_releaser is alive
          and unlock just after Auto_releaser destruction.
        */
        raii::Sentry<> mdl_release_guard{
            [thd, mdl_ticket]() -> void { dd::release_mdl(thd, mdl_ticket); }};

        {
          const dd::Table *table_obj = nullptr;
          dd::cache::Dictionary_client::Auto_releaser releaser(
              thd->dd_client());
          if (thd->dd_client()->acquire(dd::String_type(table->db),
                                        dd::String_type(table->table_name),
                                        &table_obj)) {
            WSREP_WARN(
                "Unable to acquire dd_client for db: %s, table: %s for FKs "
                "collection",
                table->db, table->table_name);
            my_error(ER_LOCK_DEADLOCK, MYF(0));
            return true;
          }

          if (table_obj != nullptr) {
            action(table_obj);
          }
        }  // The end of Dictionary_client::Auto_releaser scope
      }    // The end of mdl_release_guard scope

      // We might have been aborted in the meantime by some other TOI
      if (thd->killed == THD::KILL_QUERY) {
        WSREP_WARN(
            "Unable to collect FKs metadata for db: %s, table: %s for FKs "
            "collection",
            table->db, table->table_name);
        my_error(ER_LOCK_DEADLOCK, MYF(0));
        return true;
      }
    }
  }

  return false;
}

/*
 * Collect wsrep keys corresponding to child tables
 */
bool wsrep_append_child_tables(THD *thd, TABLE_LIST *tables,
                               wsrep::key_array *keys) {
  return wsrep_do_action_for_tables(
      thd, tables, [keys](const dd::Table *table_def) {
        for (const auto fk : table_def->foreign_key_parents()) {
          keys->push_back(wsrep_prepare_key_for_toi(
              fk->child_schema_name().c_str(), fk->child_table_name().c_str(),
              wsrep::key::shared));
        }
      });
}

/*
 * Collect wsrep keys corresponding to parent tables
 */
bool wsrep_append_fk_parent_table(THD *thd, TABLE_LIST *tables,
                                  wsrep::key_array *keys) {
  return wsrep_do_action_for_tables(
      thd, tables, [keys](const dd::Table *table_def) {
        for (const auto fk : table_def->foreign_keys()) {
          keys->push_back(wsrep_prepare_key_for_toi(
              fk->referenced_table_schema_name().c_str(),
              fk->referenced_table_name().c_str(), wsrep::key::shared));
        }
      });
}

/*
 * Construct Query_log_Event from thd query and serialize it
 * into buffer.
 *
 * Return 0 in case of success, 1 in case of error.
 */
int wsrep_to_buf_helper(THD *thd, const char *query, uint query_len,
                        uchar **buf, size_t *buf_len) {
  IO_CACHE_binlog_cache_storage tmp_io_cache;

  if (tmp_io_cache.open(mysql_tmpdir, TEMP_PREFIX, 64 * 1024, 64 * 1024))
    return 1;

  int ret(0);

  /* if MySQL GTID event is set, we have to forward it in wsrep channel */

  /* Instead of allocating and copying over the gtid event from gtid event
  buffer buf is made to point to gtid event.
  Unfortunately gtid event is only long enough to hold gtid event and so
  append action below will cause realloc to happen. */
  if (!ret && thd->wsrep_gtid_event_buf) {
    *buf = (uchar *)thd->wsrep_gtid_event_buf;
    *buf_len = thd->wsrep_gtid_event_buf_len;
  }

  if ((thd->variables.option_bits & OPTION_BIN_LOG) == 0) {
    Intvar_log_event ev((uchar)binary_log::Intvar_event::BINLOG_CONTROL_EVENT,
                        0);
    if (ev.write(&tmp_io_cache)) ret = 1;
  }

  /* if there is prepare query, add event for it */
  if (!ret && thd->wsrep_TOI_pre_query) {
    Query_log_event ev(thd, thd->wsrep_TOI_pre_query,
                       thd->wsrep_TOI_pre_query_len, false, false, false, 0);
    if (ev.write(&tmp_io_cache)) ret = 1;
  }

  /* continue to append the actual query */
  Query_log_event ev(thd, query, query_len, false, false, false, 0);

  /* Below commands will insert the row in mysql.password_history table.
     They need to be replicated with usec part of the timestamp, because
     the timestamp is the part of primary key. If we've got only second
     resolution, we can end up with the PK conflict on replica and node
     failure.
     During async replication, query_start_usec_used flag is set when
     the user's password is validated, then the event is written to
     the binlog and replicated. Here we replicate TOI, so we are at
     the beginning of query processing.
     Let's play safe, and change the flag only for the time of creation
     of the event which is replicated. This way we will not affect non-wsrep
     related logic. If it decides to cut precision, it will be still possible.
   */
  bool query_start_usec_used_tmp = thd->query_start_usec_used;
  enum_sql_command command = thd->lex->sql_command;
  if (command == SQLCOM_CREATE_USER || command == SQLCOM_ALTER_USER ||
      command == SQLCOM_SET_PASSWORD) {
    thd->query_start_usec_used = true;
  }

  if (!ret && ev.write(&tmp_io_cache)) ret = 1;

  thd->query_start_usec_used = query_start_usec_used_tmp;

  if (!ret && wsrep_write_cache_buf(&tmp_io_cache, buf, buf_len)) ret = 1;

  /* Re-assigning so that the buf can be freed using gtid event buffer.
  Even if gtid event buffer is not allocated we still re-assign as free
  logic can then only rely on freeing of gtid event buffer. */
  thd->wsrep_gtid_event_buf = *buf;
  thd->wsrep_gtid_event_buf_len = *buf_len;

  tmp_io_cache.close();
  return ret;
}

#include "sql_show.h"

#define C_STRING_WITH_LEN(X) (const_cast<char *>(X)), ((sizeof(X) - 1))

static int create_view_query(THD *thd, uchar **buf, size_t *buf_len) {
  LEX *lex = thd->lex;
  auto *select_lex = lex->query_block;
  TABLE_LIST *first_table = select_lex->table_list.first;
  TABLE_LIST *views = first_table;

  String buff;
  const LEX_STRING command[3] = {{C_STRING_WITH_LEN("CREATE ")},
                                 {C_STRING_WITH_LEN("ALTER ")},
                                 {C_STRING_WITH_LEN("CREATE OR REPLACE ")}};

  buff.append(command[static_cast<int>(thd->lex->create_view_mode)].str,
              command[static_cast<int>(thd->lex->create_view_mode)].length);

  if (!lex->definer) {
    /*
      DEFINER-clause is missing; we have to create default definer in
      persistent arena to be PS/SP friendly.
      If this is an ALTER VIEW then the current user should be set as
      the definer.
    */
    Prepared_stmt_arena_holder ps_arena_holder(thd);

    if (!(lex->definer = create_default_definer(thd))) {
      WSREP_WARN("Failed to create default definer for view.");
    }
  }

  views->algorithm = lex->create_view_algorithm;
  views->definer.user = lex->definer->user;
  views->definer.host = lex->definer->host;
  views->view_suid = lex->create_view_suid;
  views->with_check = lex->create_view_check;

  view_store_options(thd, views, &buff);
  buff.append(STRING_WITH_LEN("VIEW "));
  /* Test if user supplied a db (ie: we did not use thd->db) */
  if (views->db && views->db[0] &&
      (thd->db().str == NULL || strcmp(views->db, thd->db().str))) {
    append_identifier(thd, &buff, views->db, views->db_length);
    buff.append('.');
  }
  append_identifier(thd, &buff, views->table_name, views->table_name_length);
  if (views->derived_column_names()) {
    int i = 0;
    for (auto name : *views->derived_column_names()) {
      buff.append(i++ ? ", " : "(");
      append_identifier(thd, &buff, name.str, name.length);
    }
    buff.append(')');
  }
  buff.append(STRING_WITH_LEN(" AS "));
  // buff.append(views->source.str, views->source.length);
  buff.append(thd->lex->create_view_query_block.str,
              thd->lex->create_view_query_block.length);
  // int errcode= query_error_code(thd, true);
  // if (thd->binlog_query(THD::STMT_QUERY_TYPE,
  //                      buff.ptr(), buff.length(), false, false, false, errcod
  return wsrep_to_buf_helper(thd, buff.ptr(), buff.length(), buf, buf_len);
}

/*
  Rewrite DROP TABLE for TOI. Temporary tables are eliminated from
  the query as they are visible only to client connection.

  TODO: See comments for sql_base.cc:drop_temporary_table() and refine
  the function to deal with transactional locked tables.
 */
static int wsrep_drop_table_query(THD *thd, uchar **buf, size_t *buf_len) {
  LEX *lex = thd->lex;
  auto *select_lex = lex->query_block;
  TABLE_LIST *first_table = select_lex->table_list.first;
  String buff;

  assert(!lex->drop_temporary);

  bool found_temp_table = false;
  for (TABLE_LIST *table = first_table; table; table = table->next_global) {
    if (find_temporary_table(thd, table->db, table->table_name)) {
      found_temp_table = true;
      break;
    }
  }

  if (found_temp_table) {
    buff.append("DROP TABLE ");
    if (lex->drop_if_exists) buff.append("IF EXISTS ");

    for (TABLE_LIST *table = first_table; table; table = table->next_global) {
      if (!find_temporary_table(thd, table->db, table->table_name)) {
        append_identifier(thd, &buff, table->db, strlen(table->db),
                          system_charset_info, thd->charset());
        buff.append(".");
        append_identifier(thd, &buff, table->table_name,
                          strlen(table->table_name), system_charset_info,
                          thd->charset());
        buff.append(",");
      }
    }

    /* Chop the last comma */
    buff.chop();
    buff.append(" /* generated by wsrep */");

    WSREP_DEBUG("Rewrote '%s' as '%s'", thd->query().str, buff.ptr());

    return wsrep_to_buf_helper(thd, buff.ptr(), buff.length(), buf, buf_len);
  } else {
    return wsrep_to_buf_helper(thd, thd->query().str, thd->query().length, buf,
                               buf_len);
  }
}

static bool wsrep_can_run_in_nbo(THD *thd, const char *db, const char *table,
                                 const TABLE_LIST *table_list) {
  if (thd->lex->sql_command != SQLCOM_ALTER_TABLE &&
      thd->lex->sql_command != SQLCOM_CREATE_INDEX &&
      thd->lex->sql_command != SQLCOM_DROP_INDEX)
    return false;

  if (table && !find_temporary_table(thd, db, table)) {
    return true;
  }

  LEX *lex = thd->lex;
  auto *select_lex = lex->query_block;
  TABLE_LIST *first_table = (select_lex ? select_lex->table_list.first : NULL);
  if (table_list && first_table) {
    for (TABLE_LIST *table_it = first_table; table_it;
         table_it = table_it->next_global) {
      if (!find_temporary_table(thd, table_it->db, table_it->table_name)) {
        return true;
      }
    }
  }
  return !(table || (table_list && first_table));
}
/*
  Decide if statement should run in TOI.

  Look if table or table_list contain temporary tables. If the
  statement affects only temporary tables,   statement should not run
  in TOI. If the table list contains mix of regular and temporary tables
  (DROP TABLE, OPTIMIZE, ANALYZE), statement should be run in TOI but
  should be rewritten at later time for replication to contain only
  non-temporary tables.
 */
static bool wsrep_can_run_in_toi(THD *thd, const char *db, const char *table,
                                 const TABLE_LIST *table_list) {
  /* compression dictionary is not table object that has temporary qualifier
  attached to it. Neither it is dependent on other object that needs
  validation. */
  if (thd->lex->sql_command == SQLCOM_CREATE_COMPRESSION_DICTIONARY ||
      thd->lex->sql_command == SQLCOM_DROP_COMPRESSION_DICTIONARY)
    return true;

  assert(!table || db);
  assert(table_list || db);

  LEX *lex = thd->lex;
  auto *select_lex = lex->query_block;
  TABLE_LIST *first_table = (select_lex ? select_lex->table_list.first : NULL);

  switch (lex->sql_command) {
    case SQLCOM_CREATE_TABLE:
      if (thd->lex->create_info->options & HA_LEX_CREATE_TMP_TABLE) {
        return false;
      }
      return true;

    case SQLCOM_CREATE_VIEW:

      assert(first_table); /* First table is view name */
      /*
        If any of the remaining tables refer to temporary table error
        is returned to client, so TOI can be skipped
      */
      for (TABLE_LIST *it = first_table->next_global; it;
           it = it->next_global) {
        if (find_temporary_table(thd, it)) {
          return false;
        }
      }
      return true;

    case SQLCOM_CREATE_TRIGGER:

#if 0
    /* Trigger statement is invoked with table_list with length = 1 */
    assert(!table_list);
#endif
      assert(first_table);

      if (find_temporary_table(thd, first_table)) {
        return false;
      }
      return true;

    default:
      if (table && !find_temporary_table(thd, db, table)) {
        return true;
      }

      if (table_list && first_table) {
        for (TABLE_LIST *table_it = first_table; table_it;
             table_it = table_it->next_global) {
          if (!find_temporary_table(thd, table_it->db, table_it->table_name)) {
            return true;
          }
        }
      }
      return !(table || (table_list && first_table));
  }

  return false;
}

static int wsrep_TOI_event_buf(THD *thd, uchar **buf, size_t *buf_len) {
  int err;
  switch (thd->lex->sql_command) {
    case SQLCOM_CREATE_VIEW:
      err = create_view_query(thd, buf, buf_len);
      break;
    case SQLCOM_CREATE_PROCEDURE:
    case SQLCOM_CREATE_SPFUNCTION:
      err = wsrep_create_sp(thd, buf, buf_len);
      break;
    case SQLCOM_CREATE_TRIGGER:
      err = wsrep_create_trigger_query(thd, buf, buf_len);
      break;
    case SQLCOM_CREATE_EVENT:
      err = wsrep_create_event_query(thd, buf, buf_len);
      break;
    case SQLCOM_ALTER_EVENT:
      err = wsrep_alter_event_query(thd, buf, buf_len);
      break;
    case SQLCOM_DROP_TABLE:
      err = wsrep_drop_table_query(thd, buf, buf_len);
      break;
#if 0
    case SQLCOM_CREATE_ROLE:
      if (sp_process_definer(thd)) {
        WSREP_WARN("Failed to set CREATE ROLE definer for TOI.");
      }
      /* fallthrough */
#endif
    default:
      err = wsrep_to_buf_helper(thd, thd->query().str, thd->query().length, buf,
                                buf_len);
      break;
  }

  return err;
}

static void wsrep_TOI_begin_failed(THD *thd,
                                   const wsrep_buf_t * /* const err */) {
  if (wsrep_thd_trx_seqno(thd) > 0) {
    /* GTID was granted and TO acquired - need to log event and release TO */
    if (wsrep_emulate_bin_log) wsrep_thd_binlog_trx_reset(thd);
    if (wsrep_write_dummy_event(thd, "TOI begin failed")) {
      goto fail;
    }
    wsrep::client_state &cs(thd->wsrep_cs());
    std::string const err(wsrep::to_c_string(cs.current_error()));
    wsrep::mutable_buffer err_buf;
    err_buf.push_back(err);
    int const ret = cs.leave_toi_local(err_buf);
    if (ret) {
      WSREP_ERROR(
          "Leaving critical section for failed TOI failed: thd: %lld, "
          "schema: %s, SQL: %s, rcode: %d wsrep_error: %s",
          (long long)thd->real_id, thd->db().str, thd->query().str, ret,
          err.c_str());
      goto fail;
    }
  }
  return;
fail:
  WSREP_ERROR("Failed to release TOI resources. Need to abort.");
  unireg_abort(1);
}

static void free_gtid_event_buf(THD *thd) {
  if (thd->wsrep_gtid_event_buf) my_free(thd->wsrep_gtid_event_buf);
  thd->wsrep_gtid_event_buf_len = 0;
  thd->wsrep_gtid_event_buf = NULL;
}

static void wsrep_NBO_begin_failed(THD *thd,
                                   const wsrep_buf_t * /* const err */) {
  DBUG_TRACE;
  if (wsrep_thd_trx_seqno(thd) > 0) {
    /* GTID was granted and TO acquired - need to log event and release TO */
    if (wsrep_emulate_bin_log) wsrep_thd_binlog_trx_reset(thd);
    if (wsrep_write_dummy_event(thd, "NBO begin failed")) {
      goto fail;
    }
    wsrep::client_state &cs(thd->wsrep_cs());
    std::string const err(wsrep::to_c_string(cs.current_error()));
    wsrep::mutable_buffer err_buf;
    err_buf.push_back(err);
    int const ret = cs.leave_toi_local(err_buf);
    if (ret) {
      WSREP_ERROR(
          "Leaving critical section for failed NBO failed: thd: %lld, "
          "schema: %s, SQL: %s, rcode: %d wsrep_error: %s",
          (long long)thd->real_id, thd->db().str, thd->query().str, ret,
          err.c_str());
      goto fail;
    }
  }
  return;
fail:
  WSREP_ERROR("Failed to release NBO resources. Need to abort.");
  unireg_abort(1);
}

/*
  This function decides whether a DB operation must be replicated if it is
  internally using a read only schema.

  If a schema is read only, then this function will disallow TOI operations on
  all database objects(tables, triggers, functions, views, procedures, events)
  inside the read only schemas except ALTER SCHEMA <schema> READ ONLY = 0
  and raises ER_SCHEMA_READ_ONLY.

  @param thd          Thread context.
  @param db_          Name of schema to check.
  @param table_list   List of tables to check

  @return false if the DB operation can continue, true if not.
*/
static bool is_operating_on_read_only_schema(THD *thd, const char *db_,
                                             const TABLE_LIST *table_list) {
  LEX *lex = thd->lex;
  HA_CREATE_INFO *create_info = lex->create_info;
  bool some_tables_in_read_only_schema = false;
  std::string first_read_only_schema;

  /* Check if the db is read only */
  if (db_) {
    if (check_schema_readonly_no_error(thd, db_)) {
      some_tables_in_read_only_schema = true;
      first_read_only_schema.assign(db_);
    }
  }

  /* Check if the first table is in read only schema */
  const TABLE_LIST *first_table = table_list;
  if (first_table && first_table->db) {
    if (check_schema_readonly_no_error(thd, first_table->db)) {
      some_tables_in_read_only_schema = true;
      first_read_only_schema.assign(first_table->db);
    }
  }

  if (!some_tables_in_read_only_schema && table_list) {
    /*
      Validate referencing views if it is a DROP TABLE on a table in a different
      readonly schema. CREATE TABLE LIKE is possible when the table referenced
      by a view is deleted and recreated.
    */
    if (thd->lex->sql_command == SQLCOM_DROP_TABLE ||
        thd->lex->sql_command == SQLCOM_CREATE_TABLE) {
      // Prepare the list of referencing views
      std::vector<TABLE_LIST *> views;
      MEM_ROOT mem_root(key_memory_wsrep, MEM_ROOT_BLOCK_SIZE);
      if (prepare_view_tables_list<dd::View_table>(
              thd, db_, table_list->table_name, false, &mem_root, &views))
        return true;

      if (!views.empty()) {
        for (const auto view : views) {
          if (check_schema_readonly_no_error(thd, view->db)) {
            some_tables_in_read_only_schema = true;
            first_read_only_schema.assign(view->db);
            break;
          }
        }
      }
    }

    /* Skip table list traversal for CREATE TABLE LIKE command */
    bool skip_traversal =
        (create_info && create_info->options & HA_LEX_CREATE_TABLE_LIKE);

    /* Go through the table_list to check if any of the tables are in read
       only schema. This can happen in case of multi table drop and rename.
     */
    for (const TABLE_LIST *table = table_list;
         table && !some_tables_in_read_only_schema && !skip_traversal;
         table = table->next_global) {
      if (check_schema_readonly_no_error(thd, table->db)) {
        some_tables_in_read_only_schema = true;
        first_read_only_schema.assign(table->db);
        break;
      }
    }
  }

  /*
    If the schema is in read_only state, then the only change allowed is to:

    - Turn off read_only, possibly along with other option changes.
    - Keep read_only turned on, i.e., a no-op. In this case, other options may
      not be changed in the same statement.

     This means we fail if:

     - query is not ALTER DB
     - query is CREATE DB with same DB name, this must result in
    ER_DB_CREATE_EXISTS
     - query is ALTER DB but
          - HA_CREATE_USED_READ_ONLY is not set.
          - Or if we have set other fields as well and set READ ONLY to true.

  */
  if (some_tables_in_read_only_schema) {
    /*
      We ignore read_only for CREATE SCHEMA to make sure we keep the existing
      error handling in case the schema exists already.
    */
    if (lex->sql_command == SQLCOM_CREATE_DB) return false;

    if (lex->sql_command != SQLCOM_ALTER_DB ||
        (create_info &&
         (!(create_info->used_fields & HA_CREATE_USED_READ_ONLY) ||
          ((create_info->used_fields & ~HA_CREATE_USED_READ_ONLY) &&
           create_info->schema_read_only)))) {
      my_error(ER_SCHEMA_READ_ONLY, MYF(0), first_read_only_schema.c_str());
      return true;
    }
  }
  return false;
}

/*
  returns:
   0: statement was replicated as TOI
   1: TOI replication was skipped
  -1: TOI replication failed
 */

static int wsrep_TOI_begin(THD *thd, const char *db_, const char *table_,
                           const TABLE_LIST *table_list,
                           dd::Tablespace_table_ref_vec *trefs,
                           Alter_info *alter_info,
                           wsrep::key_array *fk_tables) {
  uchar *buf(0);
  size_t buf_len(0);
  int buf_err;
  int rc;

  DEBUG_SYNC(thd, "wsrep_TOI_begin_before_wsrep_skip_wsrep_hton");
  mysql_mutex_lock(&thd->LOCK_thd_data);

  /*
    If event qualified for TOI replication then it shouldn't go through
    normal replication. It state is updated to reflect TOI that
    naturally disallow it from getting registered for normal replication.

    This is true for all cases except when sql_log_bin = 0 that would cause
    statement to skip TOI and there-by also skip setting of the TOI mode.
    This flow can cause event to get registered for normal replication.

    Flag (wsrep_skip_wsrep_hton) below help in skipping "TOI qualified but
    skipped event" from getting registered for normal replication.

    Setting wsrep_skip_wsrep_hton=true has a side-effect that this
    query/thread cannot be killed.
  */
  thd->wsrep_skip_wsrep_hton = true;
  bool is_thd_killed = thd->is_killed();

  mysql_mutex_unlock(&thd->LOCK_thd_data);
  DEBUG_SYNC(thd, "wsrep_TOI_begin_after_wsrep_skip_wsrep_hton");

  if (is_thd_killed) {
    free_gtid_event_buf(thd);
    WSREP_DEBUG(
        "Can't execute %s in TOI mode because the query has been killed",
        WSREP_QUERY(thd));
    return -1;
  }

  thd->wsrep_skip_wsrep_hton = true;
  if (wsrep_can_run_in_toi(thd, db_, table_, table_list) == false) {
    free_gtid_event_buf(thd);
    WSREP_DEBUG("Can't execute %s in TOI mode", WSREP_QUERY(thd));
    return 1;
  }

  WSREP_DEBUG(
      "Executing Query (%s) with write-set (%lld) and exec_mode: %s"
      " in TO Isolation mode",
      WSREP_QUERY(thd), (long long)wsrep_thd_trx_seqno(thd),
      wsrep_thd_client_mode_str(thd));

  buf_err = wsrep_TOI_event_buf(thd, &buf, &buf_len);
  if (buf_err) {
    /* Given the existing error handling setup, all errors with write-set
    are classified under single error code. It would be good to have a proper
    error code reporting mechanism. */
    free_gtid_event_buf(thd);

    WSREP_WARN(
        "Append/Write to writeset buffer failed (either due to IO "
        "issues (including memory allocation) or hitting a configured "
        "limit viz. write set size, etc.");
    my_error(ER_ERROR_DURING_COMMIT, MYF(0), WSREP_SIZE_EXCEEDED,
             "Failed to append TOI write-set");
    return -1;
  }

  struct wsrep_buf buff = {buf, buf_len};

  wsrep::key_array key_array = wsrep_prepare_keys_for_toi(
      db_, table_, table_list, trefs, alter_info, fk_tables);

  THD_STAGE_INFO(thd, stage_wsrep_preparing_for_TO_isolation);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: initiating TOI for write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  wsrep::client_state &cs(thd->wsrep_cs());
  int ret =
      cs.enter_toi_local(key_array, wsrep::const_buffer(buff.ptr, buff.len));

  if (ret) {
    assert(cs.current_error());
    WSREP_DEBUG("to_execute_start() failed for %u: %s, seqno: %lld",
                thd->thread_id(), WSREP_QUERY(thd),
                (long long)wsrep_thd_trx_seqno(thd));

    /* jump to error handler in mysql_execute_command() */
    switch (cs.current_error()) {
      case wsrep::e_size_exceeded_error:
        WSREP_WARN(
            "TO isolation failed for: %d, schema: %s, sql: %s. "
            "Maximum size exceeded.",
            ret, (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
        my_error(ER_ERROR_DURING_COMMIT, MYF(0), WSREP_SIZE_EXCEEDED);
        break;
      default:
        WSREP_WARN(
            "TO isolation failed for: %d, schema: %s, sql: %s. "
            "Check wsrep connection state and retry the query.",
            ret, (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
        if (!thd->is_error()) {
          my_error(ER_LOCK_DEADLOCK, MYF(0),
                   "WSREP replication failed. Check "
                   "your wsrep connection state and retry the query.");
        }
    }
    rc = -1;
  } else {
    WSREP_DEBUG(
        "Query (%s) with write-set (%lld) and exec_mode: %s"
        " replicated in TO Isolation mode",
        WSREP_QUERY(thd), (long long)wsrep_thd_trx_seqno(thd),
        wsrep_thd_client_mode_str(thd));

    THD_STAGE_INFO(thd, stage_wsrep_TO_isolation_initiated);
    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
             "wsrep: TO isolation initiated for write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);

    /* DDL transactions are now atomic, so append wsrep xid.
    This will ensure transactions are logged with the given xid. */
    bool atomic_ddl = is_atomic_ddl(thd, true);
    if (atomic_ddl) {
      wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                     thd->wsrep_cs().toi_meta().gtid());
    }

    ++wsrep_to_isolation;
    rc = 0;
  }

  // at this point buf and thd->wsrep_gtid_event_buf are equall
  // (see wsrep_to_buf_helper internals), so we will free only once.
  free_gtid_event_buf(thd);

  if (rc) wsrep_TOI_begin_failed(thd, NULL);

  return rc;
}

static void wsrep_TOI_end(THD *thd) {
  --wsrep_to_isolation;

  wsrep::client_state &client_state(thd->wsrep_cs());
  assert(wsrep_thd_is_local_toi(thd));
  WSREP_DEBUG("TO END: %lld: %s", client_state.toi_meta().seqno().get(),
              WSREP_QUERY(thd));

  THD_STAGE_INFO(thd, stage_wsrep_completed_TO_isolation);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: completed TOI write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  if (wsrep_thd_is_local_toi(thd)) {
    wsrep_set_SE_checkpoint(client_state.toi_meta().gtid());
    wsrep::mutable_buffer err;
    if (thd->is_error() && !wsrep_must_ignore_error(thd)) {
      wsrep_store_error(thd, err);
    }
    int const ret = client_state.leave_toi_local(err);

    if (!ret) {
      WSREP_DEBUG("TO END: %lld", client_state.toi_meta().seqno().get());
      /* Reset XID on completion of DDL transactions */
      bool atomic_ddl = is_atomic_ddl(thd, true);
      if (atomic_ddl) {
        thd->get_transaction()->xid_state()->get_xid()->reset();
      }
    } else {
      WSREP_WARN("TO isolation end failed for: %d, schema: %s, sql: %s", ret,
                 (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
    }
  }
}

static int wsrep_NBO_begin_phase_one(THD *thd, const char *db_,
                                     const char *table_,
                                     const TABLE_LIST *table_list,
                                     dd::Tablespace_table_ref_vec *trefs,
                                     Alter_info *alter_info,
                                     wsrep::key_array *fk_tables) {
  DBUG_TRACE;
  uchar *buf(0);
  size_t buf_len(0);
  int buf_err;
  int rc;

  WSREP_DEBUG("Starting NBO phase one");

  mysql_mutex_lock(&thd->LOCK_thd_data);

  bool is_thd_killed = thd->is_killed();

  thd->wsrep_skip_wsrep_hton = true;

  mysql_mutex_unlock(&thd->LOCK_thd_data);

  if (is_thd_killed) {
    WSREP_DEBUG(
        "Can't execute %s in NBO mode because the query has been killed",
        WSREP_QUERY(thd));
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->wsrep_skip_wsrep_hton = false;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
    return -1;
  }

  if (wsrep_can_run_in_nbo(thd, db_, table_, table_list) == false) {
    WSREP_DEBUG("Can't execute %s in NBO mode", WSREP_QUERY(thd));
    my_error(ER_NOT_SUPPORTED_YET, MYF(0),
             "this query in wsrep_OSU_method NBO");
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->wsrep_skip_wsrep_hton = false;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
    return -1;
  }

  WSREP_DEBUG(
      "Executing Query (%s) with write-set (%lld) and exec_mode: %s"
      " in NBO mode",
      WSREP_QUERY(thd), (long long)wsrep_thd_trx_seqno(thd),
      wsrep_thd_client_mode_str(thd));

  buf_err = wsrep_TOI_event_buf(thd, &buf, &buf_len);
  if (buf_err) {
    /* Given the existing error handling setup, all errors with write-set
    are classified under single error code. It would be good to have a proper
    error code reporting mechanism. */
    WSREP_WARN(
        "Append/Write to writeset buffer failed (either due to IO "
        "issues (including memory allocation) or hitting a configured "
        "limit viz. write set size, etc.");
    my_error(ER_ERROR_DURING_COMMIT, MYF(0), WSREP_SIZE_EXCEEDED,
             "Failed to append NBO write-set");
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->wsrep_skip_wsrep_hton = false;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
    return -1;
  }

  struct wsrep_buf buff = {buf, buf_len};

  wsrep::key_array key_array = wsrep_prepare_keys_for_toi(
      db_, table_, table_list, trefs, alter_info, fk_tables);

  THD_STAGE_INFO(thd, stage_wsrep_preparing_for_TO_isolation);
  snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
           "wsrep: initiating NBO for write set (%lld)",
           (long long)wsrep_thd_trx_seqno(thd));
  WSREP_DEBUG("%s", thd->wsrep_info);
  thd_proc_info(thd, thd->wsrep_info);

  wsrep::client_state &cs(thd->wsrep_cs());

  int ret = cs.begin_nbo_phase_one(key_array,
                                   wsrep::const_buffer(buff.ptr, buff.len));

  if (ret) {
    assert(cs.current_error());
    WSREP_DEBUG("begin_nbo_phase_one() failed for %u: %s, seqno: %lld",
                thd->thread_id(), WSREP_QUERY(thd),
                (long long)wsrep_thd_trx_seqno(thd));

    /* jump to error handler in mysql_execute_command() */
    switch (cs.current_error()) {
      default:
        WSREP_WARN(
            "NBO failed for: %d, schema: %s, sql: %s. "
            "Check wsrep connection state and retry the query.",
            ret, (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
        if (!thd->is_error()) {
          WSREP_WARN("Adjusting error to deadlock");
          my_error(ER_LOCK_DEADLOCK, MYF(0),
                   "WSREP replication failed. Check "
                   "your wsrep connection state and retry the query.");
        }
    }
    rc = -1;
    thd->wsrep_skip_wsrep_hton = false;
  } else {
    thd->wsrep_nbo.keys = key_array;

    WSREP_DEBUG(
        "Query (%s) with write-set (%lld) and exec_mode: %s"
        " replicated in NBO mode",
        WSREP_QUERY(thd), (long long)wsrep_thd_trx_seqno(thd),
        wsrep_thd_client_mode_str(thd));

    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
             "wsrep: NBO initiated for write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);

    ++wsrep_to_isolation;
    rc = 0;
  }

  free_gtid_event_buf(thd);

  if (rc) wsrep_NBO_begin_failed(thd, NULL);

  return rc;
}

void wsrep_NBO_end_phase_one(THD *thd) {
  if (!wsrep_thd_is_local_nbo(thd)) return;

  DBUG_TRACE;

  wsrep::mutable_buffer err;
  WSREP_DEBUG("Ending NBO phase one");
  int ret = thd->wsrep_cs().end_nbo_phase_one(err);
  WSREP_DEBUG("NBO phase one ended");
  if (ret) {
    assert(thd->wsrep_cs().current_error());
    WSREP_DEBUG("end_nbo_phase_one() failed for %u: %s, seqno: %lld",
                thd->thread_id(), WSREP_QUERY(thd),
                (long long)wsrep_thd_trx_seqno(thd));
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->wsrep_skip_wsrep_hton = false;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
  } else {
    bool atomic_ddl = is_atomic_ddl(thd, true);
    if (atomic_ddl) {
      wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                     thd->wsrep_cs().toi_meta().gtid());
    }
    if (wsrep_thd_is_local_nbo(thd)) {
      wsrep_set_SE_checkpoint(thd->wsrep_cs().nbo_meta().gtid());
    }
  }

  DBUG_EXECUTE_IF("sync.after_nbo_phase_one_begin", {
    const char act[] =
        "now "
        "wait_for signal.after_nbo_phase_one_begin";
    assert(!debug_sync_set_action(thd, STRING_WITH_LEN(act)));
  };);
}

int wsrep_NBO_begin_phase_two(THD *thd) {
  if (wsrep_thd_is_in_nbo(thd)) {
    DBUG_TRACE;
    wsrep::client_state &client_state(thd->wsrep_cs());
    WSREP_DEBUG("NBO phase two END start: %lld: %s",
                client_state.toi_meta().seqno().get(), WSREP_QUERY(thd));
    int ret = client_state.begin_nbo_phase_two(
        thd->wsrep_nbo.keys,
        std::chrono::steady_clock::now() +
            std::chrono::seconds(thd->variables.lock_wait_timeout));

    if (ret) {
      assert(client_state.current_error());
      WSREP_DEBUG("begin_nbo_phase_two() failed for %u: %s, seqno: %lld",
                  thd->thread_id(), WSREP_QUERY(thd),
                  (long long)wsrep_thd_trx_seqno(thd));

      /* jump to error handler in mysql_execute_command() */
      switch (client_state.current_error()) {
        default:
          WSREP_WARN(
              "NBO phase two failed for: %d, schema: %s, sql: %s. "
              "Check wsrep connection state and retry the query.",
              ret, (thd->db().str ? thd->db().str : "(null)"),
              WSREP_QUERY(thd));
      }

      mysql_mutex_lock(&thd->LOCK_thd_data);
      thd->wsrep_skip_wsrep_hton = false;
      mysql_mutex_unlock(&thd->LOCK_thd_data);
      return 1;
    }
  }

  return 0;
}

static void wsrep_NBO_end_phase_two(THD *thd) {
  DBUG_TRACE;
  --wsrep_to_isolation;

  wsrep::client_state &client_state(thd->wsrep_cs());
  WSREP_DEBUG("NBO phase two END: %lld: %s",
              client_state.nbo_meta().seqno().get(), WSREP_QUERY(thd));

  if (wsrep_thd_is_in_nbo(thd)) {
    THD_STAGE_INFO(thd, stage_wsrep_completed_TO_isolation);
    snprintf(thd->wsrep_info, sizeof(thd->wsrep_info),
             "wsrep: completed NBO write set (%lld)",
             (long long)wsrep_thd_trx_seqno(thd));
    WSREP_DEBUG("%s", thd->wsrep_info);
    thd_proc_info(thd, thd->wsrep_info);

    wsrep::mutable_buffer err;
    if (thd->is_error() && !wsrep_must_ignore_error(thd)) {
      wsrep_store_error(thd, err);
    }
    int ret = client_state.end_nbo_phase_two(err);

    if (!ret) {
      WSREP_DEBUG("NBO phase two END: %lld",
                  client_state.nbo_meta().seqno().get());
      /* Reset XID on completion of DDL transactions */
      bool atomic_ddl = is_atomic_ddl(thd, true);
      if (atomic_ddl) {
        thd->get_transaction()->xid_state()->get_xid()->reset();
      }
    } else {
      WSREP_WARN("NBO phase two end failed for: %d, schema: %s, sql: %s", ret,
                 (thd->db().str ? thd->db().str : "(null)"), WSREP_QUERY(thd));
    }
  }

  mysql_mutex_lock(&thd->LOCK_thd_data);
  thd->wsrep_skip_wsrep_hton = false;
  mysql_mutex_unlock(&thd->LOCK_thd_data);
}

static int wsrep_RSU_begin(THD *thd, const char *, const char *) {
  WSREP_DEBUG("RSU BEGIN: %lld, : %s", wsrep_thd_trx_seqno(thd),
              WSREP_QUERY(thd));
  if (thd->wsrep_cs().begin_rsu(wsrep_RSU_commit_timeout)) {
    WSREP_WARN("RSU begin failed");
  } else {
    thd->variables.wsrep_on = 0;
  }
  return 0;
}

static void wsrep_RSU_end(THD *thd) {
  WSREP_DEBUG("RSU END: %lld : %s", wsrep_thd_trx_seqno(thd), WSREP_QUERY(thd));
  if (thd->wsrep_cs().end_rsu()) {
    WSREP_WARN("Failed to end RSU, server may need to be restarted");
  }
  thd->variables.wsrep_on = 1;
}

int wsrep_to_isolation_begin(THD *thd, const char *db_, const char *table_,
                             const TABLE_LIST *table_list,
                             dd::Tablespace_table_ref_vec *trefs,
                             Alter_info *alter_info,
                             wsrep::key_array *fk_tables) {
  /*
    No isolation for applier or replaying threads.
   */
  if (!wsrep_thd_is_local(thd)) return 0;

  /*
    If plugin native tables are being created/dropped then skip TOI for such
    action as the main/parent ddl is already replicated and should cause
    same action on all nodes.
  */
  if (thd->is_plugin_fake_ddl()) return 0;

  /* Read only schems need to be handled in a different way */
  if (is_operating_on_read_only_schema(thd, db_, table_list)) return -1;

  /* Generally if node enters non-primary state then execution of DDL+DML
  is blocked on such node but there are some asynchronous pre-register
  action that can cause invocation of TOI. For example: DROP of event
  on completion of EVENT tenure is one such asynchronous action which
  doesn't need to be fired by user and so if node is asynchronous
  such such action should be blocked at TOI level. */
  if (!wsrep_ready) {
    WSREP_DEBUG(
        "WSREP replication failed."
        " Check your wsrep connection state and retry the query.");
    my_error(ER_LOCK_DEADLOCK, MYF(0),
             "WSREP replication failed. Check "
             "your wsrep connection state and retry the query.");
    return -1;
  }

  int ret = 0;
  mysql_mutex_lock(&thd->LOCK_wsrep_thd);

  if (thd->wsrep_trx().state() == wsrep::transaction::s_must_abort) {
    WSREP_INFO(
        "thread: %u  schema: %s  query: %s has been aborted due to "
        "multi-master conflict",
        thd->thread_id(), (thd->db().str ? thd->db().str : "(null)"),
        (thd->query().str) ? WSREP_QUERY(thd) : "void");
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    return WSREP_TRX_FAIL;
  }
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  assert(wsrep_thd_is_local(thd));
  assert(thd->wsrep_trx().ws_meta().seqno().is_undefined());

  /* TOI protection against FTWRL */
  if (thd->global_read_lock.can_acquire_protection()) {
    WSREP_DEBUG("Aborting TOI: Global Read-Lock (FTWRL) in place: %s %u",
                WSREP_QUERY(thd), thd->thread_id());
    return -1;
  }

  /* TOI protection against FLUSH TABLE <table> WITH READ LOCK
  that would pause the provider. */
  if (!thd->global_read_lock.provider_resumed()) {
    WSREP_DEBUG("Aborting TOI: Galera provider paused due to lock: %s %u",
                thd->query().str, thd->thread_id());
    my_error(ER_CANT_UPDATE_WITH_READLOCK, MYF(0));
    return -1;
  }

  if (wsrep_debug && thd->mdl_context.has_locks()) {
    WSREP_DEBUG("Thread holds MDL locks at TOI begin: %s %u", WSREP_QUERY(thd),
                thd->thread_id());
  }

  /*
    It makes sense to set auto_increment_* to defaults in TOI operations.
    Must be done before wsrep_TOI_begin() since Query_log_event encapsulating
    TOI statement and auto inc variables for wsrep replication is constructed
    there. Variables are reset back in THD::reset_for_next_command() before
    processing of next command.
   */
  if (wsrep_auto_increment_control) {
    thd->variables.auto_increment_offset = 1;
    thd->variables.auto_increment_increment = 1;
  }

  if (thd->variables.wsrep_on && wsrep_thd_is_local(thd)) {
    switch (thd->variables.wsrep_OSU_method) {
      case WSREP_OSU_TOI:
        ret = wsrep_TOI_begin(thd, db_, table_, table_list, trefs, alter_info,
                              fk_tables);
        break;
      case WSREP_OSU_RSU:
        ret = wsrep_RSU_begin(thd, db_, table_);
        break;
      case WSREP_OSU_NBO:
        ret = wsrep_NBO_begin_phase_one(thd, db_, table_, table_list, trefs,
                                        alter_info, fk_tables);
        break;
      default:
        WSREP_ERROR("Unsupported OSU method: %lu",
                    thd->variables.wsrep_OSU_method);
        ret = -1;
        break;
    }
    switch (ret) {
      case 0: /* wsrep_TOI_begin sould set toi mode */
        break;
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

bool wsrep_thd_is_in_to_isolation(THD *thd, bool flock) {
  bool status = false;
  if (thd) {
    if (flock) mysql_mutex_lock(&thd->LOCK_thd_data);

    status = thd->wsrep_skip_wsrep_hton;

    if (flock) mysql_mutex_unlock(&thd->LOCK_thd_data);
  }
  return status;
}

void wsrep_to_isolation_end(THD *thd) {
  /*
    If plugin native tables are being created/dropped then skip TOI for such
    action as the main/parent ddl is already replicated and should cause
    same action on all nodes.
  */
  if (thd->is_plugin_fake_ddl()) return;

  if (wsrep_thd_is_local_toi(thd)) {
    assert(thd->variables.wsrep_OSU_method == WSREP_OSU_TOI);
    wsrep_TOI_end(thd);
  } else if (wsrep_thd_is_in_nbo(thd)) {
    assert(thd->variables.wsrep_OSU_method == WSREP_OSU_NBO);
    wsrep_NBO_end_phase_two(thd);
  } else if (wsrep_thd_is_in_rsu(thd)) {
    assert(thd->variables.wsrep_OSU_method == WSREP_OSU_RSU);
    wsrep_RSU_end(thd);
  } else {
    assert(0);
  }
  if (wsrep_emulate_bin_log) wsrep_thd_binlog_trx_reset(thd);

  DEBUG_SYNC(thd, "wsrep_to_isolation_end_before_wsrep_skip_wsrep_hton");
  mysql_mutex_lock(&thd->LOCK_thd_data);

  thd->wsrep_skip_wsrep_hton = false;

  mysql_mutex_unlock(&thd->LOCK_thd_data);
  DEBUG_SYNC(thd, "wsrep_to_isolation_end_after_wsrep_skip_wsrep_hton");
}

#define WSREP_MDL_LOG(severity, msg, schema, schema_len, req, gra)           \
  WSREP_##severity(                                                          \
      "%s\n\n"                                                               \
      "schema:  %.*s\n"                                                      \
      "request: (thd-tid:%u \tseqno:%lld \texec-mode:%s, query-state:%s,"    \
      " conflict-state:%s)\n          cmd-code:%d %d \tquery:%s)\n\n"        \
      "granted: (thd-tid:%u \tseqno:%lld \texec-mode:%s, query-state:%s,"    \
      " conflict-state:%s)\n          cmd-code:%d %d \tquery:%s)\n",         \
      msg, schema_len, schema, req->thread_id(),                             \
                                                                             \
      (long long)wsrep_thd_trx_seqno(req), wsrep_thd_client_mode_str(req),   \
      wsrep_thd_client_state_str(req), wsrep_thd_transaction_state_str(req), \
      req->get_command(), req->lex->sql_command,                             \
      (req->rewritten_query().length()                                       \
           ? wsrep_thd_rewritten_query(req).c_ptr_safe()                     \
           : req->query().str),                                              \
                                                                             \
      gra->thread_id(), (long long)wsrep_thd_trx_seqno(gra),                 \
      wsrep_thd_client_mode_str(gra), wsrep_thd_client_state_str(gra),       \
      wsrep_thd_transaction_state_str(gra), gra->get_command(),              \
      gra->lex->sql_command,                                                 \
      (gra->rewritten_query().length()                                       \
           ? wsrep_thd_rewritten_query(gra).c_ptr_safe()                     \
           : gra->query().str));

bool wsrep_handle_mdl_conflict(const MDL_context *requestor_ctx,
                               MDL_ticket *ticket, const MDL_key *key) {
  if (!WSREP_ON) return false;

  THD *request_thd = requestor_ctx->wsrep_get_thd();
  THD *granted_thd = ticket->get_ctx()->wsrep_get_thd();

  /* Normally this code flow is called by background applier thread (bf_thd)
  but at times when a local thread is force aborted it would also call
  this flow as part of reschedule_waiter (on release of locks) for other
  thread handlers. In this use-case avoid processing the action and let
  respective thd take needed action. */
  if (!(request_thd == current_thd || granted_thd == current_thd)) {
    return false;
  }

  bool ret = false;

  const char *schema = key->db_name();
  int schema_len = key->db_name_length();

  mysql_mutex_lock(&request_thd->LOCK_wsrep_thd);

  if (wsrep_thd_is_toi(request_thd) || wsrep_thd_is_in_rsu(request_thd) ||
      wsrep_thd_is_applying(request_thd) || wsrep_thd_is_in_nbo(request_thd)) {
    mysql_mutex_unlock(&request_thd->LOCK_wsrep_thd);

    WSREP_MDL_LOG(DEBUG, "---------- MDL conflict --------", schema, schema_len,
                  request_thd, granted_thd);
    ticket->wsrep_report(wsrep_debug);

    mysql_mutex_lock(&granted_thd->LOCK_wsrep_thd);
    if (wsrep_thd_is_toi(granted_thd) || wsrep_thd_is_in_nbo(granted_thd) ||
        wsrep_thd_is_applying(granted_thd)) {
      if (wsrep_thd_is_SR(granted_thd) && !wsrep_thd_is_SR(request_thd)) {
        WSREP_MDL_LOG(INFO, "MDL conflict, DDL vs SR", schema, schema_len,
                      request_thd, granted_thd);
        mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
        wsrep_abort_thd(request_thd, granted_thd, true);
      } else {
        WSREP_MDL_LOG(INFO, "MDL BF-BF conflict", schema, schema_len,
                      request_thd, granted_thd);
        ticket->wsrep_report(true);
        mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
        unireg_abort(1);
      }
    } else if (granted_thd->lex->sql_command == SQLCOM_FLUSH) {
      WSREP_DEBUG(
          "BF thread waiting for local/victim thread (%u) FLUSH operation",
          granted_thd->thread_id());
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = false;
    } else if (!granted_thd->ull_hash.empty()) {
      /* If there are user-level-lock skip abort */
      WSREP_DEBUG(
          "BF thread waiting for user level lock held by victim/local thread "
          "(%u)",
          granted_thd->thread_id());
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = false;
    } else if (granted_thd->mdl_context.wsrep_has_explicit_locks()) {
      /* Starting MySQL-8.0, mysql flow may take MDL_EXPLICIT lock for DD
      access or change. Till 8.0, these locks use to normally correspond to
      user set explicit lock (LOCK/FLUSH,GET_LOCK, etc....) that are generally
      retained beyond transaction tenure. Given the new meaning or semantics
      just check for presence of EXPLICIT lock would not work so wsrep flow
      now check if the lock is marked as preemptable. */
      WSREP_DEBUG(
          "BF thread waiting for explicit lock held by victim/local thread "
          "(%u)",
          granted_thd->thread_id());
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = false;
    } else if (granted_thd->wsrep_allow_mdl_conflict) {
      WSREP_DEBUG("Background thread caused BF abort, conf %s",
                  wsrep_thd_transaction_state_str(granted_thd));
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd(request_thd, granted_thd, true);
      ret = false;
    } else if (request_thd->lex->sql_command == SQLCOM_DROP_TABLE) {
      WSREP_DEBUG("DROP caused BF abort, conf %s",
                  wsrep_thd_transaction_state_str(granted_thd));
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd(request_thd, granted_thd, true);
      ret = false;
#if 0
    } else if (granted_thd->wsrep_query_state == QUERY_COMMITTING) {
      WSREP_DEBUG("Aborting local thread (%u) in replication/commit state",
                  granted_thd->thread_id());
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd(request_thd, granted_thd, true);
      ret = false;
#endif /* 0 */
    } else {
      WSREP_MDL_LOG(DEBUG, "MDL conflict-> BF abort", schema, schema_len,
                    request_thd, granted_thd);
      ticket->wsrep_report(wsrep_debug);
      if (granted_thd->wsrep_trx().active()) {
        mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
        wsrep_abort_thd(request_thd, granted_thd, true);
      } else {
        /*
          Granted_thd is likely executing with wsrep_on=0. If the requesting
          thd is BF, BF abort and wait.
        */
        mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
        if (wsrep_thd_is_BF(request_thd, false)) {
          ha_wsrep_abort_transaction(request_thd, granted_thd, true);
        } else {
          WSREP_MDL_LOG(INFO, "MDL unknown BF-BF conflict", schema, schema_len,
                        request_thd, granted_thd);
          ticket->wsrep_report(true);
          unireg_abort(1);
        }
      }
    }
  } else {
    mysql_mutex_unlock(&request_thd->LOCK_wsrep_thd);
  }
  return ret;
}

bool wsrep_node_is_donor() {
  if (WSREP_ON) {
    return (Wsrep_server_state::instance().state() ==
            Wsrep_server_state::s_donor);
  }
  return false;
}

bool wsrep_node_is_synced() {
  /* If node is not running in cluster mode then node is always in sync state */
  if (WSREP_ON) {
    return (Wsrep_server_state::instance().state() ==
            Wsrep_server_state::s_synced);
  }
  return true;
}

bool wsrep_is_wsrep_channel_name(const char *channel_name) {
  return channel_name && !strcasecmp(channel_name, WSREP_CHANNEL_NAME);
}

void wsrep_last_committed_id(wsrep_gtid_t *gtid) {
  wsrep::gtid ret =
      Wsrep_server_state::instance().provider().last_committed_gtid();
  memcpy(gtid->uuid.data, ret.id().data(), sizeof(gtid->uuid.data));
  gtid->seqno = ret.seqno().get();
}

void wsrep_node_uuid(wsrep_uuid_t &uuid) { uuid = node_uuid; }

int wsrep_must_ignore_error(THD *thd) {
  const int error = thd->get_stmt_da()->mysql_errno();
  const uint flags = sql_command_flags[thd->lex->sql_command];

  assert(error);
  assert(wsrep_thd_is_toi(thd) || wsrep_thd_is_in_nbo(thd));

  if ((wsrep_ignore_apply_errors & WSREP_IGNORE_ERRORS_ON_DDL))
    goto ignore_error;

  if ((flags & CF_WSREP_MAY_IGNORE_ERRORS) &&
      (wsrep_ignore_apply_errors & WSREP_IGNORE_ERRORS_ON_RECONCILING_DDL)) {
    switch (error) {
      case ER_DB_DROP_EXISTS:
      case ER_BAD_TABLE_ERROR:
      case ER_CANT_DROP_FIELD_OR_KEY:
        goto ignore_error;
    }
  }

  return 0;

ignore_error:
  WSREP_WARN(
      "Ignoring error '%s' on query. "
      "Default database: '%s'. Query: '%s', Error_code: %d",
      thd->get_stmt_da()->message_text(), print_slave_db_safe(thd->db().str),
      WSREP_QUERY(thd), error);
  return 1;
}

int wsrep_ignored_error_code(Log_event *ev, int error) {
  const THD *thd = ev->thd;

  assert(error);
  assert(wsrep_thd_is_applying(thd) && !wsrep_thd_is_local_toi(thd));

  if ((wsrep_ignore_apply_errors & WSREP_IGNORE_ERRORS_ON_RECONCILING_DML)) {
    const int ev_type = ev->get_type_code();
    if ((ev_type == binary_log::DELETE_ROWS_EVENT ||
         ev_type == binary_log::DELETE_ROWS_EVENT_V1) &&
        error == ER_KEY_NOT_FOUND)
      goto ignore_error;
  }

  return 0;

ignore_error:
  WSREP_WARN("Ignoring error '%s' on %s event. Error_code: %d",
             thd->get_stmt_da()->message_text(), ev->get_type_str(), error);
  return 1;
}

bool wsrep_provider_is_SR_capable() {
  return Wsrep_server_state::has_capability(
      wsrep::provider::capability::streaming);
}

int wsrep_thd_retry_counter(const THD *thd) { return thd->wsrep_retry_counter; }

enum wsrep::streaming_context::fragment_unit wsrep_fragment_unit(ulong unit) {
  switch (unit) {
    case WSREP_FRAG_BYTES:
      return wsrep::streaming_context::bytes;
    case WSREP_FRAG_ROWS:
      return wsrep::streaming_context::row;
    case WSREP_FRAG_STATEMENTS:
      return wsrep::streaming_context::statement;
    default:
      assert(0);
      return wsrep::streaming_context::bytes;
  }
}

void wsrep_set_thd_proc_info(THD *thd, const char *str) {
  strncpy(thd->wsrep_info, str, (sizeof(thd->wsrep_info) - 1));
  thd->wsrep_info[sizeof(thd->wsrep_info) - 1] = '\0';
}

const char *wsrep_get_thd_proc_info(THD *thd) { return (thd->wsrep_info); }

/***** callbacks for wsrep service ************/

bool get_wsrep_recovery() { return wsrep_recovery; }

bool wsrep_consistency_check(THD *thd) {
  return thd->wsrep_consistency_check == CONSISTENCY_CHECK_RUNNING;
}

/*
 Returns non-const copy of thd->rewritten_query().

 PXC code uses rewritten_query.c_ptr_safe() in several places.
 As c_ptr_save() is non const method, it requires non const object.
 Here we return copy of thd->rewritten_query() instead of const-casting
 thd->rewritten_query() in place.
 This is to avoid interference of PXC specific part with generic server part.
 */
String wsrep_thd_rewritten_query(THD *thd) { return thd->rewritten_query(); }

static std::shared_ptr<MasterKeyManager> masterKeyManager;
static bool wsrep_init_master_key() {
  masterKeyManager = std::make_shared<MasterKeyManager>(
      [](const std::string &msg) { WSREP_ERROR("%s", msg.c_str()); });
  return masterKeyManager->Init();
}

static void wsrep_deinit_master_key() {
  masterKeyManager->DeInit();
  masterKeyManager.reset();
}

std::string wsrep_get_master_key(const std::string &keyId) {
  return masterKeyManager->GetKey(keyId);
}

bool wsrep_new_master_key(const std::string &keyId) {
  return masterKeyManager->GenerateKey(keyId);
}

bool wsrep_rotate_master_key() {
  wsrep::provider &provider = Wsrep_server_state::instance().provider();
  return (wsrep::provider::status::success != provider.rotate_gcache_key());
}
