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

#ifndef WSREP_MYSQLD_H
#define WSREP_MYSQLD_H

#include "mysqld.h"
#include "query_options.h"
#include "rpl_gtid.h"
#include "wsrep/src/wsrep_api.h"
#include "mysql_version.h"
// #include "sql/log.h"

#define WSREP_UNDEFINED_TRX_ID ULLONG_MAX

class set_var;
class THD;

enum wsrep_exec_mode { LOCAL_STATE, REPL_RECV, TOTAL_ORDER, LOCAL_COMMIT };

enum wsrep_exec_mode wsrep_thd_exec_mode(THD *thd);

const char *wsrep_get_exec_mode(wsrep_exec_mode state);
const char *wsrep_thd_get_exec_mode(THD *thd);
void wsrep_thd_set_exec_mode(THD *thd, enum wsrep_exec_mode mode);

enum wsrep_query_state {
  QUERY_IDLE,
  QUERY_EXEC,
  QUERY_COMMITTING,
  QUERY_EXITING,
  QUERY_ROLLINGBACK,
};

enum wsrep_query_state wsrep_thd_query_state(THD *thd);

const char *wsrep_get_query_state(wsrep_query_state state);
const char *wsrep_thd_get_query_state(THD *thd);
void wsrep_thd_set_query_state(THD *thd, enum wsrep_query_state state);

enum wsrep_conflict_state {
  NO_CONFLICT,
  MUST_ABORT,
  ABORTING,
  ABORTED,
  MUST_REPLAY,
  REPLAYING,
  REPLAYED,
  RETRY_AUTOCOMMIT,
  CERT_FAILURE,
};

enum wsrep_conflict_state wsrep_thd_conflict_state(THD *thd);

const char *wsrep_get_conflict_state(wsrep_conflict_state state);
const char *wsrep_thd_get_conflict_state(THD *thd);
void wsrep_thd_set_conflict_state(THD *thd, bool lock,
                                  enum wsrep_conflict_state state);

/* this while concept of consistency check was added so that pt-checksum
can run the command to consistency of the all the nodes of the cluster.
pt-checksum will fire DML with special version comment that will allow
execution of said DML in form of TOI. */
enum wsrep_consistency_check_mode {
  NO_CONSISTENCY_CHECK,
  CONSISTENCY_CHECK_DECLARED,
  CONSISTENCY_CHECK_RUNNING,
};

const char *wsrep_get_wsrep_status(wsrep_status status);

enum enum_wsrep_certification_rules {
  WSREP_CERTIFICATION_RULES_STRICT,
  WSREP_CERTIFICATION_RULES_OPTIMIZED
};

// Global wsrep parameters
extern wsrep_t *wsrep;

extern bool wsrep_replicate_set_stmt;

// MySQL wsrep options
extern const char *wsrep_provider;
extern const char *wsrep_provider_options;
extern const char *wsrep_cluster_name;
extern const char *wsrep_cluster_address;
extern const char *wsrep_node_name;
extern const char *wsrep_node_address;
extern const char *wsrep_node_incoming_address;
extern const char *wsrep_data_home_dir;
extern const char *wsrep_dbug_option;
extern long wsrep_slave_threads;
extern int wsrep_slave_count_change;
extern MYSQL_PLUGIN_IMPORT bool wsrep_debug;
extern ulong wsrep_retry_autocommit;
extern bool wsrep_auto_increment_control;
extern bool wsrep_incremental_data_collection;
extern const char *wsrep_start_position;
extern ulong wsrep_max_ws_size;
extern ulong wsrep_max_ws_rows;
extern const char *wsrep_notify_cmd;
extern bool wsrep_certify_nonPK;
extern ulong wsrep_certification_rules;
extern long wsrep_max_protocol_version;
extern long wsrep_protocol_version;
extern bool wsrep_desync;
extern ulong wsrep_reject_queries;
extern bool wsrep_recovery;
extern bool wsrep_log_conflicts;
extern bool wsrep_load_data_splitting;
extern bool wsrep_restart_slave;
extern bool wsrep_restart_slave_activated;
extern bool wsrep_slave_FK_checks;
extern bool wsrep_slave_UK_checks;
extern std::atomic<ulong> wsrep_running_threads;
extern ulong wsrep_RSU_commit_timeout;
extern bool wsrep_allow_server_session;

enum enum_wsrep_reject_types {
  WSREP_REJECT_NONE,    /* nothing rejected */
  WSREP_REJECT_ALL,     /* reject all queries, with UNKNOWN_COMMAND error */
  WSREP_REJECT_ALL_KILL /* kill existing connections and reject all queries*/
};

enum enum_wsrep_OSU_method {
  WSREP_OSU_TOI,
  WSREP_OSU_RSU,
  WSREP_OSU_NONE,
};

enum enum_wsrep_sync_wait {
  WSREP_SYNC_WAIT_NONE = 0x0,
  // select, begin
  WSREP_SYNC_WAIT_BEFORE_READ = 0x1,
  WSREP_SYNC_WAIT_BEFORE_UPDATE_DELETE = 0x2,
  WSREP_SYNC_WAIT_BEFORE_INSERT_REPLACE = 0x4,
  WSREP_SYNC_WAIT_BEFORE_SHOW = 0x8,
  WSREP_SYNC_WAIT_MAX = 0xF
};

enum enum_pxc_strict_modes {
  PXC_STRICT_MODE_DISABLED = 0,
  PXC_STRICT_MODE_PERMISSIVE,
  PXC_STRICT_MODE_ENFORCING,
  PXC_STRICT_MODE_MASTER
};
extern ulong pxc_strict_mode;

enum enum_pxc_maint_modes {
  PXC_MAINT_MODE_DISABLED = 0,
  PXC_MAINT_MODE_SHUTDOWN,
  PXC_MAINT_MODE_MAINTENANCE,
};
extern ulong pxc_maint_mode;
extern ulong pxc_maint_transition_period;
extern bool pxc_encrypt_cluster_traffic;

extern bool pxc_force_flush_error_message;

// MySQL status variables
extern bool wsrep_new_cluster;
extern bool wsrep_connected;
extern bool wsrep_ready;
extern const char *wsrep_cluster_state_uuid;
extern long long wsrep_cluster_conf_id;
extern const char *wsrep_cluster_status;
extern long wsrep_cluster_size;
extern long wsrep_local_index;
extern long long wsrep_local_bf_aborts;
extern const char *wsrep_provider_name;
extern const char *wsrep_provider_version;
extern const char *wsrep_provider_vendor;

int wsrep_show_status(THD *thd, SHOW_VAR *var, char *buff);
int wsrep_show_ready(THD *thd, SHOW_VAR *var, char *buff);
void wsrep_free_status(THD *thd);

/* Filters out --wsrep-new-cluster oprtion from argv[]
 * should be called in the very beginning of main() */
void wsrep_filter_new_cluster(int *argc, char *argv[]);

int wsrep_init();
void wsrep_deinit();
void wsrep_recover();
bool wsrep_before_SE();  // initialize wsrep before storage
                         // engines (true) or after (false)
/* wsrep initialization sequence at startup
 * @param before wsrep_before_SE() value */
void wsrep_init_startup(bool before);

extern "C" wsrep_ws_handle_t *wsrep_thd_ws_handle(THD *thd);

extern "C" void wsrep_thd_mark_split_trx(THD *thd, bool mini_trx);

extern "C" void wsrep_thd_set_trx_to_replay(THD *thd, uint64 trx_id);

extern "C" void wsrep_thd_LOCK(THD *thd);
extern "C" void wsrep_thd_UNLOCK(THD *thd);

extern "C" uint32 wsrep_thd_wsrep_rand(THD *thd);

extern "C" time_t wsrep_thd_query_start(THD *thd);

extern "C" my_thread_id wsrep_thd_thread_id(THD *thd);

extern "C" int64_t wsrep_thd_trx_seqno(THD *thd);

extern "C" query_id_t wsrep_thd_query_id(THD *thd);

extern "C" wsrep_trx_id_t wsrep_thd_next_trx_id(THD *thd);
extern "C" wsrep_trx_id_t wsrep_thd_trx_id(THD *thd);

extern "C" const char *wsrep_thd_query(THD *thd);
extern "C" query_id_t wsrep_thd_wsrep_last_query_id(THD *thd);

extern "C" void wsrep_thd_set_wsrep_last_query_id(THD *thd, query_id_t id);

extern "C" void wsrep_thd_awake(THD *thd, bool signal);

extern "C" int wsrep_thd_retry_counter(THD *thd);

extern "C" void wsrep_handle_fatal_signal(int sig);

extern "C" void wsrep_thd_set_next_trx_id(THD *thd);

extern "C" void wsrep_set_thd_proc_info(THD *thd, const char *str);
extern "C" const char *wsrep_get_thd_proc_info(THD *thd);

extern "C" void wsrep_thd_auto_increment_variables(
    THD *, unsigned long long *offset, unsigned long long *increment);

extern void wsrep_close_client_connections(bool wait_to_end,
                                           bool server_shtudown);
extern int wsrep_wait_committing_connections_close(int wait_time);
extern void wsrep_close_applier(THD *thd);
extern void wsrep_wait_appliers_close(THD *thd);
extern void wsrep_close_applier_threads(int count);
extern void wsrep_kill_mysql(THD *thd);

/* new defines */
extern void wsrep_stop_replication(THD *thd, bool is_server_shutdown);
extern bool wsrep_start_replication();
extern bool wsrep_must_sync_wait(THD *thd,
                                 uint mask = WSREP_SYNC_WAIT_BEFORE_READ);
extern bool wsrep_sync_wait(THD *thd, uint mask = WSREP_SYNC_WAIT_BEFORE_READ);
extern int wsrep_check_opts(int argc, char *const *argv);
extern void wsrep_prepend_PATH(const char *path);
/* some inline functions are defined in wsrep_mysqld_inl.h */

/* Provide a wrapper of the WSREP_ON macro for plugins to use */
extern "C" bool wsrep_is_wsrep_on(void);

/* Other global variables */
extern wsrep_seqno_t wsrep_locked_seqno;

#define WSREP_ON                                           \
  ((global_system_variables.wsrep_on) && wsrep_provider && \
   strcmp(wsrep_provider, WSREP_NONE))

/* use xxxxxx_NNULL macros when thd pointer is guaranteed to be non-null to
 * avoid compiler warnings (GCC 6 and later) */
#define WSREP_NNULL(thd) \
  (WSREP_ON && (wsrep != NULL) && thd->variables.wsrep_on)

#define WSREP(thd) ((thd != NULL) && WSREP_NNULL(thd))

#define WSREP_CLIENT(thd) (WSREP(thd) && thd->wsrep_client_thread)

#define WSREP_EMULATE_BINLOG_NNULL(thd) \
  (WSREP_NNULL(thd) && wsrep_emulate_bin_log)

#define WSREP_EMULATE_BINLOG(thd) (WSREP(thd) && wsrep_emulate_bin_log)

#define WSREP_LOG(severity, fmt, ...)                              \
  do {                                                             \
    LogEvent()                                                     \
        .prio(severity)                                            \
        .subsys("WSREP")                                           \
        .component("WSREP")                                        \
        .source_line(__LINE__)                                     \
        .source_file(MY_BASENAME)                                  \
        .function(__FUNCTION__)                                    \
        .message(fmt, ##__VA_ARGS__);                              \
    if (pxc_force_flush_error_message) flush_error_log_messages(); \
  } while (0);

#define WSREP_DEBUG(fmt, ...) \
  if (wsrep_debug) WSREP_LOG(INFORMATION_LEVEL, fmt, ##__VA_ARGS__)
#define WSREP_INFO(fmt, ...) WSREP_LOG(INFORMATION_LEVEL, fmt, ##__VA_ARGS__)
#define WSREP_WARN(fmt, ...) WSREP_LOG(WARNING_LEVEL, fmt, ##__VA_ARGS__)

extern bool wsrep_is_SE_initialized();

#define WSREP_ERROR(fmt, ...)                                             \
  do {                                                                    \
    if (!wsrep_is_SE_initialized()) pxc_force_flush_error_message = true; \
    WSREP_LOG(ERROR_LEVEL, fmt, ##__VA_ARGS__)                            \
  } while (0);

#define WSREP_LOG_CONFLICT_THD(thd, role)                                      \
  WSREP_LOG(INFORMATION_LEVEL,                                                 \
            "%s: \n "                                                          \
            "  THD: %u, mode: %s, state: %s, conflict: %s, seqno: %lld\n "     \
            "  SQL: %s\n",                                                     \
            role, wsrep_thd_thread_id(thd), wsrep_thd_get_exec_mode(thd),      \
            wsrep_thd_get_query_state(thd), wsrep_thd_get_conflict_state(thd), \
            (long long)wsrep_thd_trx_seqno(thd), wsrep_thd_query(thd));

#define WSREP_LOG_CONFLICT(bf_thd, victim_thd, bf_abort)                      \
  if (wsrep_debug || wsrep_log_conflicts) {                                   \
    WSREP_LOG(INFORMATION_LEVEL, "--------- CONFLICT DETECTED --------");     \
    WSREP_LOG(INFORMATION_LEVEL,                                              \
              "cluster conflict due to %s for threads:\n",                    \
              (bf_abort) ? "high priority abort" : "certification failure");  \
    if (bf_thd) WSREP_LOG_CONFLICT_THD(bf_thd, "Winning thread");             \
    if (victim_thd) WSREP_LOG_CONFLICT_THD(victim_thd, "Victim thread");      \
  }

#define WSREP_QUERY(thd)                                   \
  ((!opt_general_log_raw) && thd->rewritten_query.length() \
       ? thd->rewritten_query.c_ptr_safe()                 \
       : thd->query().str)

// Use this for logging output received from the SST scripts
#define WSREP_SST_LOG(severity, s)                                 \
  do {                                                             \
    LogEvent()                                                     \
        .prio(severity)                                            \
        .subsys("WSREP-SST")                                       \
        .component("WSREP-SST")                                    \
        .source_line(__LINE__)                                     \
        .source_file(MY_BASENAME)                                  \
        .function(__FUNCTION__)                                    \
        .verbatim(s);                                              \
    if (pxc_force_flush_error_message) flush_error_log_messages(); \
  } while (0);

// Use this for output received from Galera (via the logging callback)
#define WSREP_GALERA_LOG(severity, s)                              \
  do {                                                             \
    LogEvent()                                                     \
        .prio(severity)                                            \
        .subsys("Galera")                                          \
        .component("Galera")                                       \
        .source_line(__LINE__)                                     \
        .source_file(MY_BASENAME)                                  \
        .function(__FUNCTION__)                                    \
        .verbatim(s);                                              \
    if (pxc_force_flush_error_message) flush_error_log_messages(); \
  } while (0);

extern bool wsrep_ready_get();
extern void wsrep_ready_wait();

enum wsrep_trx_status {
  WSREP_TRX_OK,
  WSREP_TRX_CERT_FAIL,     /* certification failure, must abort */
  WSREP_TRX_SIZE_EXCEEDED, /* trx size exceeded */
  WSREP_TRX_ERROR,         /* native mysql error */
};

extern enum wsrep_trx_status wsrep_run_wsrep_commit(THD *thd, handlerton *hton,
                                                    bool all);

extern enum wsrep_trx_status wsrep_replicate(THD *thd);
extern enum wsrep_trx_status wsrep_pre_commit(THD *thd);

class Ha_trx_info;
struct THD_TRANS;
void wsrep_register_hton(THD *thd, bool all);
void wsrep_interim_commit(THD *thd);
void wsrep_post_commit(THD *thd, bool all);
void wsrep_brute_force_killer(THD *thd);
int wsrep_hire_brute_force_killer(THD *thd, uint64_t trx_id);

extern "C" bool wsrep_consistency_check(void *thd_ptr);

/* this is visible for client build so that innodb plugin gets this */
typedef struct wsrep_aborting_thd {
  struct wsrep_aborting_thd *next;
  THD *aborting_thd;
} * wsrep_aborting_thd_t;

extern mysql_mutex_t LOCK_wsrep_ready;
extern mysql_cond_t COND_wsrep_ready;
extern mysql_mutex_t LOCK_wsrep_sst;
extern mysql_cond_t COND_wsrep_sst;
extern mysql_mutex_t LOCK_wsrep_sst_init;
extern mysql_cond_t COND_wsrep_sst_init;
extern mysql_mutex_t LOCK_wsrep_rollback;
extern mysql_cond_t COND_wsrep_rollback;
extern int wsrep_replaying;
extern mysql_mutex_t LOCK_wsrep_replaying;
extern mysql_cond_t COND_wsrep_replaying;
extern mysql_mutex_t LOCK_wsrep_slave_threads;
extern mysql_mutex_t LOCK_wsrep_desync;

extern wsrep_aborting_thd_t wsrep_aborting_thd;
extern bool wsrep_emulate_bin_log;
extern int wsrep_to_isolation;
extern rpl_sidno wsrep_sidno;

#ifdef HAVE_PSI_INTERFACE
extern PSI_mutex_key key_LOCK_wsrep_ready;
extern PSI_mutex_key key_COND_wsrep_ready;
extern PSI_mutex_key key_LOCK_wsrep_sst;
extern PSI_cond_key key_COND_wsrep_sst;
extern PSI_mutex_key key_LOCK_wsrep_sst_init;
extern PSI_cond_key key_COND_wsrep_sst_init;
extern PSI_mutex_key key_LOCK_wsrep_sst_thread;
extern PSI_cond_key key_COND_wsrep_sst_thread;
extern PSI_mutex_key key_LOCK_wsrep_rollback;
extern PSI_cond_key key_COND_wsrep_rollback;
extern PSI_mutex_key key_LOCK_wsrep_replaying;
extern PSI_cond_key key_COND_wsrep_replaying;
extern PSI_mutex_key key_LOCK_wsrep_slave_threads;
extern PSI_mutex_key key_LOCK_wsrep_desync;

extern PSI_mutex_key key_LOCK_wsrep_sst_thread;
extern PSI_cond_key key_COND_wsrep_sst_thread;

extern PSI_thread_key key_THREAD_wsrep_sst_joiner;
extern PSI_thread_key key_THREAD_wsrep_sst_donor;
extern PSI_thread_key key_THREAD_wsrep_sst_upgrade;
extern PSI_thread_key key_THREAD_wsrep_sst_logger;
extern PSI_thread_key key_THREAD_wsrep_applier;
extern PSI_thread_key key_THREAD_wsrep_rollbacker;
#endif /* HAVE_PSI_INTERFACE */
struct TABLE_LIST;
class Alter_info;
int wsrep_to_isolation_begin(THD *thd, const char *db_, const char *table_,
                             const TABLE_LIST *table_list,
                             Alter_info *alter_info = NULL);
void wsrep_to_isolation_end(THD *thd);
void wsrep_cleanup_transaction(THD *thd);
int wsrep_to_buf_helper(THD *thd, const char *query, uint query_len,
                        uchar **buf, size_t *buf_len);
int wsrep_create_sp(THD *thd, uchar **buf, size_t *buf_len);
int wsrep_create_trigger_query(THD *thd, uchar **buf, size_t *buf_len);
int wsrep_create_event_query(THD *thd, uchar **buf, size_t *buf_len);
int wsrep_alter_event_query(THD *thd, uchar **buf, size_t *buf_len);

bool wsrep_stmt_rollback_is_safe(THD *thd);

void wsrep_init_sidno(const wsrep_uuid_t &);
bool wsrep_node_is_donor();
bool wsrep_node_is_synced();
bool wsrep_replicate_GTID(THD *thd);

typedef struct wsrep_key_arr {
  wsrep_key_t *keys;
  size_t keys_len;
} wsrep_key_arr_t;
bool wsrep_prepare_keys_for_isolation(THD *thd, const char *db,
                                      const char *table,
                                      const TABLE_LIST *table_list,
                                      wsrep_key_arr_t *ka);
void wsrep_keys_free(wsrep_key_arr_t *key_arr);


/* In 8.0, channels now have names
   Reserve the 'wsrep' name for the WSREP replication channels
*/
constexpr char WSREP_CHANNEL_NAME[] = "wsrep";
bool wsrep_is_wsrep_channel_name(const char *channel_name);


/* In 8.0, a WSREP state file was added to keep track of information
   about WSREP that could be accessed by other processes (e.g. SST)

   This file is named 'wsrep_state.dat'
 */

/* Name of the file that holds metadata about the WSREP state.
 */
constexpr char WSREP_STATE_FILENAME[] = "wsrep_state.dat";

/* Version number for the state file format
 */
constexpr char WSREP_STATE_FILE_VERSION_NAME[] = "version";
constexpr char WSREP_STATE_FILE_VERSION[] = "1.0";

/* This identifies the PXC version of the datadir/schema.
 */
constexpr char WSREP_SCHEMA_VERSION_NAME[] = "wsrep_schema_version";
constexpr char WSREP_SCHEMA_VERSION[] = MYSQL_SERVER_VERSION;

#endif /* WSREP_MYSQLD_H */
