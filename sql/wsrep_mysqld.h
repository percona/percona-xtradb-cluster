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

#ifndef WSREP_MYSQLD_H
#define WSREP_MYSQLD_H

//#include <my_global.h>
//#include <mysql_priv.h>
#include <mysql.h>
#include <sql_priv.h>
//#include <sql_base.h>
#include "../wsrep/wsrep_api.h"

class set_var;
class THD;

// Global wsrep parameters
extern wsrep_t*    wsrep;

// MySQL wsrep options
extern const char* wsrep_provider;
extern const char* wsrep_provider_options;
extern const char* wsrep_cluster_name;
extern const char* wsrep_cluster_address;
extern const char* wsrep_node_name;
extern const char* wsrep_node_incoming_address;
extern const char* wsrep_data_home_dir;
extern const char* wsrep_dbug_option;
extern long        wsrep_slave_threads;
extern long        wsrep_local_cache_size;
extern my_bool     wsrep_ws_persistency;
extern my_bool     wsrep_debug;
extern my_bool     wsrep_convert_LOCK_to_trx;
extern ulong       wsrep_retry_autocommit;
extern my_bool     wsrep_auto_increment_control;
extern my_bool     wsrep_drupal_282555_workaround;
extern my_bool     wsrep_incremental_data_collection;
extern const char* wsrep_sst_method;
extern const char* wsrep_sst_receive_address;
extern const char* wsrep_sst_auth;
extern const char* wsrep_sst_donor;
extern const char* wsrep_start_position;
extern long long   wsrep_max_ws_size;
extern long        wsrep_max_ws_rows;
extern const char* wsrep_notify_cmd;
extern my_bool     wsrep_certify_nonPK;

// MySQL status variables
extern my_bool     wsrep_ready;
extern const char* wsrep_cluster_state_uuid;
extern long long   wsrep_cluster_conf_id;
extern const char* wsrep_cluster_status;
extern long        wsrep_cluster_size;
extern long        wsrep_local_index;
extern int         wsrep_show_status(THD *thd, SHOW_VAR *var, char *buff);

// MySQL variables funcs
extern int  wsrep_init_vars();
extern bool wsrep_on_update (
  sys_var *self, THD* thd, enum_var_type var_type);
extern bool  wsrep_start_position_check   (
  sys_var *self, THD* thd, set_var* var);
extern bool wsrep_start_position_update  (
  sys_var *self, THD* thd, enum_var_type type);
extern void wsrep_start_position_default (THD* thd, enum_var_type var_type);
extern void 
wsrep_start_position_init    (const char* opt);
extern bool  wsrep_provider_check          (sys_var *self, THD* thd, set_var* var);
extern bool wsrep_provider_update         (sys_var *self, THD* thd, enum_var_type type);
extern void wsrep_provider_default        (THD* thd, enum_var_type var_type);
extern void wsrep_provider_init           (const char* opt);
extern bool  wsrep_cluster_address_check   (sys_var *self, THD* thd, set_var* var);
extern bool wsrep_cluster_address_update  (sys_var *self, THD* thd, enum_var_type type);
extern void wsrep_cluster_address_default (THD* thd, enum_var_type var_type);
extern void wsrep_cluster_address_init    (const char* opt);

extern bool wsrep_init_first(); // initialize wsrep before storage
                                // engines or after
extern int  wsrep_init();

extern void wsrep_close_client_connections();
extern void wsrep_close_appliers(THD *thd);
extern void wsrep_wait_appliers_close(THD *thd); 
extern void wsrep_create_appliers();

/* new defines */
extern void wsrep_stop_replication(THD *thd);
extern void wsrep_start_replication();

// MySQL logging functions don't seem to understand long long length modifer.
// This is a workaround. It also prefixes all messages with "WSREP"
#define WSREP_LOG(fun, ...)                                       \
    {                                                             \
        char msg[256] = {'\0'};                                   \
        snprintf(msg, sizeof(msg) - 1, ## __VA_ARGS__);           \
        fun("WSREP: %s", msg);                                    \
    }

#define WSREP_DEBUG(...)                                                \
    if (wsrep_debug)     WSREP_LOG(sql_print_information, ##__VA_ARGS__)
#define WSREP_INFO(...)  WSREP_LOG(sql_print_information, ##__VA_ARGS__)
#define WSREP_WARN(...)  WSREP_LOG(sql_print_warning,     ##__VA_ARGS__)
#define WSREP_ERROR(...) WSREP_LOG(sql_print_error,       ##__VA_ARGS__)

/*! Synchronizes applier thread start with init thread */
extern void wsrep_sst_grab();
/*! Init thread waits for SST completion */
extern void wsrep_sst_wait();
/*! Signals wsrep that initialization is complete, writesets can be applied */
extern void wsrep_sst_continue();

extern void wsrep_SE_init_grab(); /*! grab init critical section */
extern void wsrep_SE_init_wait(); /*! wait for SE init to complete */
extern void wsrep_SE_init_done(); /*! signal that SE init is complte */

extern void wsrep_ready_wait();
enum wsrep_trx_status {
    WSREP_TRX_OK,
    WSREP_TRX_ROLLBACK,
    WSREP_TRX_ERROR,
  };
enum wsrep_trx_status
wsrep_run_wsrep_commit(
    THD *thd, handlerton *hton, bool all);

void wsrep_replication_process(THD *thd);
void wsrep_rollback_process(THD *thd);
void wsrep_brute_force_killer(THD *thd);
int  wsrep_hire_brute_force_killer(THD *thd, uint64_t trx_id);
extern "C" int wsrep_thd_is_brute_force(void *thd_ptr);
extern "C" int wsrep_abort_thd(void *bf_thd_ptr, void *victim_thd_ptr);
extern "C" int wsrep_thd_in_locking_session(void *thd_ptr);
void *wsrep_prepare_bf_thd(THD *thd);
void wsrep_return_from_bf_mode(void *shadow, THD *thd);

#define WSREP_SET_DATABASE(wsrep, thd, db)                                \
  if (thd->variables.wsrep_on && thd->wsrep_exec_mode==LOCAL_STATE &&     \
      db && db[0])                                                        \
  {                                                                       \
    char query[85];                                                       \
    int query_len;                                                        \
    wsrep_status_t rcode;                                                 \
    memset(query, 0, 85);                                                 \
    query_len = snprintf(query, 84, "USE %s;", db);                       \
    rcode = wsrep->set_database(wsrep, thd->thread_id, query, query_len); \
    if (rcode != WSREP_OK) {                                              \
      WSREP_WARN(                                                         \
        "wsrep failed to set current db for connection: %lu, code: %d",   \
        thd->thread_id, rcode);                                           \
    }                                                                     \
  }

/* this is visible for client build so that innodb plugin gets this */
typedef struct wsrep_aborting_thd {
  struct wsrep_aborting_thd *next;
  THD *aborting_thd;
} *wsrep_aborting_thd_t;

extern mysql_mutex_t LOCK_wsrep_rollback;
extern mysql_cond_t COND_wsrep_rollback;
extern wsrep_aborting_thd_t wsrep_aborting_thd;
extern MYSQL_PLUGIN_IMPORT my_bool wsrep_debug;
extern my_bool wsrep_convert_LOCK_to_trx;
extern ulong   wsrep_retry_autocommit;
extern my_bool wsrep_emulate_bin_log;
extern my_bool wsrep_auto_increment_control;
extern my_bool wsrep_drupal_282555_workaround;
extern long long wsrep_max_ws_size;
extern long      wsrep_max_ws_rows;
extern int       wsrep_to_isolation;
extern my_bool wsrep_certify_nonPK;

extern PSI_mutex_key key_LOCK_wsrep_rollback;
extern PSI_cond_key key_COND_wsrep_rollback;

#endif /* WSREP_MYSQLD_H */
