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

#include "wsrep_var.h"

#include <cstdio>
#include <cstdlib>
#include "my_dir.h"
#include "mysqld.h"
#include "set_var.h"
#include "sql_class.h"
#include "sql_plugin.h"
#include "mysql/components/services/log_builtins.h"

#include "wsrep_priv.h"
#include "wsrep_thd.h"
#include "wsrep_xid.h"

#define WSREP_START_POSITION_ZERO "00000000-0000-0000-0000-000000000000:-1"
#define WSREP_CLUSTER_NAME "my_wsrep_cluster"

const char *wsrep_provider = 0;
const char *wsrep_provider_options = 0;
const char *wsrep_cluster_address = 0;
const char *wsrep_cluster_name = 0;
const char *wsrep_node_name = 0;
const char *wsrep_node_address = 0;
const char *wsrep_node_incoming_address = 0;
const char *wsrep_start_position = 0;
ulong wsrep_reject_queries;

int wsrep_init_vars() {
  wsrep_provider = my_strdup(key_memory_wsrep, WSREP_NONE, MYF(MY_WME));
  wsrep_provider_options = my_strdup(key_memory_wsrep, "", MYF(MY_WME));
  wsrep_cluster_address = my_strdup(key_memory_wsrep, "", MYF(MY_WME));
  wsrep_cluster_name =
      my_strdup(key_memory_wsrep, WSREP_CLUSTER_NAME, MYF(MY_WME));
  wsrep_node_name = my_strdup(key_memory_wsrep, "", MYF(MY_WME));
  wsrep_node_address = my_strdup(key_memory_wsrep, "", MYF(MY_WME));
  wsrep_node_incoming_address =
      my_strdup(key_memory_wsrep, WSREP_NODE_INCOMING_AUTO, MYF(MY_WME));
  wsrep_start_position =
      my_strdup(key_memory_wsrep, WSREP_START_POSITION_ZERO, MYF(MY_WME));

  global_system_variables.binlog_format = BINLOG_FORMAT_ROW;
  return 0;
}

/* Function checks if the new value for wsrep_on is valid.
Toggling of the value inside function or transaction is not allowed
@return false if no error encountered with check else return true. */
bool wsrep_on_check(sys_var *, THD *thd, set_var *var) {
  if (var->type == OPT_GLOBAL) return true;

  /* If in a stored function/trigger, it's too late to change wsrep_on. */
  if (thd->in_sub_stmt) {
    WSREP_WARN(
        "Cannot modify @@session.wsrep_on inside a stored function "
        " or trigger");
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
             var->save_result.ulonglong_value ? "ON" : "OFF");
    return true;
  }

  /* Make the session variable 'wsrep_on' read-only inside a transaction. */
  if (thd->in_active_multi_stmt_transaction()) {
    WSREP_WARN("Cannot modify @@session.wsrep_on inside a transaction");
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
             var->save_result.ulonglong_value ? "ON" : "OFF");
    return true;
  }

  return false;
}

bool wsrep_on_update(sys_var *, THD *thd, enum_var_type) {
  /* Note: toggle sql_log_bin if wsrep_on = off.
  wsrep_on = off suggest node-local operation. Avoid logging
  these operations to binlog as they could write events
  to binlog with MySQL XID that could duplicate with WSREP XID. */

  if (!thd->variables.wsrep_on &&
      thd->variables.wsrep_saved_binlog_state ==
          System_variables::wsrep_binlog_state_t::WSREP_BINLOG_NOTSET) {
    WSREP_WARN(
        "Toggling wsrep_on to OFF will affect sql_log_bin."
        " Check manual for more details");

    /* Cache the state of sql_bin_log before toggling it. */
    if (thd->variables.option_bits & OPTION_BIN_LOG) {
      thd->variables.wsrep_saved_binlog_state =
          System_variables::wsrep_binlog_state_t::WSREP_BINLOG_ENABLED;
    } else {
      thd->variables.wsrep_saved_binlog_state =
          System_variables::wsrep_binlog_state_t::WSREP_BINLOG_DISABLED;
    }

    thd->variables.option_bits &= ~OPTION_BIN_LOG;

  } else if (thd->variables.wsrep_on &&
             thd->variables.wsrep_saved_binlog_state !=
                 System_variables::wsrep_binlog_state_t::WSREP_BINLOG_NOTSET) {
    WSREP_WARN(
        "Toggling wsrep_on to ON will affect sql_log_bin."
        " Check manual for more details");

    /* Restore the state of sql_bin_log. */
    if (thd->variables.wsrep_saved_binlog_state ==
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_ENABLED) {
      thd->variables.option_bits |= OPTION_BIN_LOG;
    } else if (thd->variables.wsrep_saved_binlog_state ==
               System_variables::wsrep_binlog_state_t::WSREP_BINLOG_DISABLED) {
      thd->variables.option_bits &= ~OPTION_BIN_LOG;
    }
    thd->variables.wsrep_saved_binlog_state =
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_NOTSET;
  }

  return false;
}

bool wsrep_causal_reads_update(sys_var *, THD *thd, enum_var_type) {
  if (thd->variables.wsrep_causal_reads) {
    thd->variables.wsrep_sync_wait |= WSREP_SYNC_WAIT_BEFORE_READ;
  } else {
    thd->variables.wsrep_sync_wait &= ~WSREP_SYNC_WAIT_BEFORE_READ;
  }

  // update global settings too.
  if (global_system_variables.wsrep_causal_reads) {
    global_system_variables.wsrep_sync_wait |= WSREP_SYNC_WAIT_BEFORE_READ;
  } else {
    global_system_variables.wsrep_sync_wait &= ~WSREP_SYNC_WAIT_BEFORE_READ;
  }
  return false;
}

bool wsrep_sync_wait_update(sys_var *, THD *thd, enum_var_type) {
  thd->variables.wsrep_causal_reads =
      thd->variables.wsrep_sync_wait & WSREP_SYNC_WAIT_BEFORE_READ;

  // update global settings too
  global_system_variables.wsrep_causal_reads =
      global_system_variables.wsrep_sync_wait & WSREP_SYNC_WAIT_BEFORE_READ;
  return false;
}

static int wsrep_start_position_verify(const char *start_str) {
  size_t start_len;
  wsrep_uuid_t uuid;
  ssize_t uuid_len;

  start_len = strlen(start_str);
  if (start_len < 34) return 1;

  uuid_len = wsrep_uuid_scan(start_str, start_len, &uuid);
  if (uuid_len < 0 || (start_len - uuid_len) < 2) return 1;

  if (start_str[uuid_len] != ':')  // separator should follow UUID
    return 1;

  char *endptr;
  wsrep_seqno_t const seqno __attribute__((unused))  // to avoid GCC warnings
  (strtoll(&start_str[uuid_len + 1], &endptr, 10));

  if (*endptr == '\0') return 0;  // remaining string was seqno

  return 1;
}

/* Function checks if the new value for start_position is valid.
@return false if no error encountered with check else return true. */
bool wsrep_start_position_check(sys_var *, THD *, set_var *var) {
  char start_pos_buf[FN_REFLEN];

  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1)))  // safety
    goto err;

  memcpy(start_pos_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  start_pos_buf[var->save_result.string_value.length] = 0;

  if (!wsrep_start_position_verify(start_pos_buf)) return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
           var->save_result.string_value.str ? var->save_result.string_value.str
                                             : "NULL");
  return true;
}

static void wsrep_set_local_position(THD *thd, const char *const value,
                                     size_t length, bool const sst) {
  wsrep_uuid_t uuid;
  size_t const uuid_len = wsrep_uuid_scan(value, length, &uuid);
  wsrep_seqno_t const seqno = strtoll(value + uuid_len + 1, NULL, 10);

  if (sst) {
    wsrep_sst_received(thd, uuid, seqno, NULL, 0);
  } else {
    local_uuid = uuid;
    local_seqno = seqno;
  }
}

bool wsrep_start_position_update(sys_var *, THD *thd, enum_var_type) {
  WSREP_INFO("Updating wsrep_start_position to: '%s'", wsrep_start_position);
  // since this value passed wsrep_start_position_check, don't check anything
  // here
  wsrep_set_local_position(thd, wsrep_start_position,
                           strlen(wsrep_start_position), true);
  return 0;
}

void wsrep_start_position_init(const char *val) {
  if (NULL == val || wsrep_start_position_verify(val)) {
    WSREP_ERROR("Bad initial value for wsrep_start_position: %s",
                (val ? val : ""));
    return;
  }

  wsrep_set_local_position(NULL, val, strlen(val), false);
}

static int get_provider_option_value(const char *opts, const char *opt_name,
                                     ulong *opt_value) {
  int ret = 1;
  ulong opt_value_tmp;
  char *opt_value_str, *s,
      *opts_copy = my_strdup(key_memory_wsrep, opts, MYF(MY_WME));

  if ((opt_value_str = strstr(opts_copy, opt_name)) == NULL) goto end;
  opt_value_str = strtok_r(opt_value_str, "=", &s);
  if (opt_value_str == NULL) goto end;
  opt_value_str = strtok_r(NULL, ";", &s);
  if (opt_value_str == NULL) goto end;

  opt_value_tmp = strtoul(opt_value_str, NULL, 10);
  if (errno == ERANGE) goto end;

  *opt_value = opt_value_tmp;
  ret = 0;

end:
  my_free(opts_copy);
  return ret;
}

static bool refresh_provider_options() {
  WSREP_DEBUG("refresh_provider_options: %s",
              (wsrep_provider_options) ? wsrep_provider_options : "null");

  try {
    std::string opts = Wsrep_server_state::instance().provider().options();
    wsrep_provider_options_init(opts.c_str());
    get_provider_option_value(wsrep_provider_options,
                              (char *)"repl.max_ws_size", &wsrep_max_ws_size);
    return false;
  } catch (...) {
    WSREP_ERROR("Failed to get provider options");
    return true;
  }
}

static int wsrep_provider_verify(const char *provider_str) {
  MY_STAT f_stat;
  char path[FN_REFLEN];

  if (!provider_str || strlen(provider_str) == 0) return 1;

  if (!strcmp(provider_str, WSREP_NONE)) return 0;

  if (!unpack_filename(path, provider_str)) return 1;

  /* check that provider file exists */
  memset(&f_stat, 0, sizeof(MY_STAT));
  if (!my_stat(path, &f_stat, MYF(0))) {
    return 1;
  }
  return 0;
}

/* Function checks if the new value for provider is valid.
@return false if no error encountered with check else return true. */
bool wsrep_provider_check(sys_var *, THD *thd, set_var *var) {
  char wsrep_provider_buf[FN_REFLEN];

  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1)))  // safety
    goto err;

  /* Changing wsrep_provider in middle of function/trigger is not allowed. */
  if (thd->in_sub_stmt) {
    WSREP_WARN(
        "Cannot modify wsrep_provider inside a stored function "
        " or trigger");
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
             var->save_result.string_value.str);
    return true;
  }

  /* Changing wsrep_provider in middle of transaction is not allowed. */
  if (thd->in_active_multi_stmt_transaction()) {
    WSREP_WARN("Cannot modify wsrep_provider inside a transaction");
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
             var->save_result.string_value.str);
    return true;
  }

  memcpy(wsrep_provider_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  wsrep_provider_buf[var->save_result.string_value.length] = 0;

  if (!wsrep_provider_verify(wsrep_provider_buf)) return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
           var->save_result.string_value.str ? var->save_result.string_value.str
                                             : "NULL");
  return true;
}

bool wsrep_provider_update(sys_var *, THD *thd, enum_var_type) {
  bool rcode = false;

  /* Ensure we free the stats that are allocated by galera-library.
  wsrep part doesn't know how to free them shouldn't be attempting it
  as it not allocated by wsrep part.
  Before unloading cleanup any stale reference and stats is one of it.
  Given that thd->variables.wsrep.on is turned-off it is safe to assume
  that no new stats queries will be allowed during this transition time. */
  wsrep_free_status(thd);

  WSREP_DEBUG("Updating wsrep_provider to: %s", wsrep_provider);

  /* stop replication is heavy operation, and includes closing all client
     connections. Closing clients may need to get LOCK_global_system_variables
     at least in MariaDB.

     Note: releasing LOCK_global_system_variables may cause race condition, if
     there can be several concurrent clients changing wsrep_provider
  */
  mysql_mutex_unlock(&LOCK_global_system_variables);
  wsrep_stop_replication(thd, false);

  /* provider status variables are allocated in provider library
     and need to freed here, otherwise a dangling reference to
     wsrep_status_vars would remain in THD
  */
  wsrep_free_status(thd);

  wsrep_deinit();

  char *tmp = strdup(wsrep_provider);  // wsrep_init() rewrites provider
                                       // when fails
  if (wsrep_init()) {
    my_error(ER_CANT_OPEN_LIBRARY, MYF(0), tmp);
    rcode = true;
  }
  free(tmp);

  // we sure don't want to use old address with new provider
  wsrep_cluster_address_init(NULL);
  wsrep_provider_options_init(NULL);
  if (!rcode) refresh_provider_options();

  /* locking order to be enforced is:
     1. LOCK_global_system_variables
     2. LOCK_wsrep_cluster_config
     => have to juggle mutexes to comply with this
  */

  mysql_mutex_unlock(&LOCK_wsrep_cluster_config);
  mysql_mutex_lock(&LOCK_global_system_variables);
  mysql_mutex_lock(&LOCK_wsrep_cluster_config);


  return rcode;
}

void wsrep_provider_init(const char *value) {
  WSREP_DEBUG("wsrep_provider_init: %s -> %s",
              (wsrep_provider) ? wsrep_provider : "null",
              (value) ? value : "null");
  if (NULL == value || wsrep_provider_verify(value)) {
    WSREP_ERROR("Bad initial value for wsrep_provider: %s",
                (value ? value : ""));
    return;
  }

  if (wsrep_provider) my_free((void *)wsrep_provider);
  wsrep_provider = my_strdup(key_memory_wsrep, value, MYF(0));
}

bool wsrep_provider_options_check(sys_var *, THD *, set_var *) { return 0; }

bool wsrep_provider_options_update(sys_var *, THD *, enum_var_type) {

  if (!wsrep_provider_options) {
    WSREP_ERROR("Invalid value for wsrep_provider_options");
    my_message(ER_WRONG_ARGUMENTS, "Invalid value for wsrep_provider_options",
               MYF(0));
    return true;
  }

  enum wsrep::provider::status ret =
      Wsrep_server_state::instance().provider().options(wsrep_provider_options);

  if (ret)
  {
    WSREP_ERROR("Set options returned %d", ret);
    refresh_provider_options();
    return true;
  }
  return refresh_provider_options();
}

void wsrep_provider_options_init(const char *value) {
  if (wsrep_provider_options && wsrep_provider_options != value)
    my_free((void *)wsrep_provider_options);
  wsrep_provider_options =
      (value) ? my_strdup(key_memory_wsrep, value, MYF(0)) : NULL;
}

bool wsrep_reject_queries_update(sys_var *, THD *, enum_var_type) {
  switch (wsrep_reject_queries) {
    case WSREP_REJECT_NONE:
      WSREP_INFO("Allowing client queries due to manual setting");
      break;
    case WSREP_REJECT_ALL:
      WSREP_INFO("Rejecting client queries due to manual setting");
      break;
    case WSREP_REJECT_ALL_KILL:
      wsrep_close_client_connections(false, false);
      WSREP_INFO(
          "Rejecting client queries and killing connections due to manual "
          "setting");
      break;
    default:
      WSREP_INFO("Unknown value for wsrep_reject_queries: %lu",
                 wsrep_reject_queries);
      return true;
  }
  return false;
}

bool wsrep_debug_update(sys_var *, THD *, enum_var_type) {
  Wsrep_server_state::instance().debug_log_level(wsrep_debug);
  return false;
}

static int wsrep_cluster_address_verify(const char *) {
  /* There is no predefined address format, it depends on provider. */
  return 0;
}

/* Function checks if the new value for cluster_address is valid.
@return false if no error encountered with check else return true. */
bool wsrep_cluster_address_check(sys_var *, THD *, set_var *var) {
  char addr_buf[FN_REFLEN];

  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1)))  // safety
    goto err;

  memcpy(addr_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  addr_buf[var->save_result.string_value.length] = 0;

  if (!wsrep_cluster_address_verify(addr_buf)) return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
           var->save_result.string_value.str ? var->save_result.string_value.str
                                             : "NULL");
  return true;
}

bool wsrep_cluster_address_update(sys_var *, THD *thd, enum_var_type) {
  if (!Wsrep_server_state::instance().is_provider_loaded()) {
    WSREP_INFO(
        "WSREP (galera) provider is not loaded, can't re(start) replication.");
    return false;
  }

  /* stop replication is heavy operation, and includes closing all client
     connections. Closing clients may need to get LOCK_global_system_variables
     at least in MariaDB.

     Note: releasing LOCK_global_system_variables may cause race condition, if
     there can be several concurrent clients changing wsrep_provider
  */
  WSREP_DEBUG("wsrep_cluster_address_update: %s", wsrep_cluster_address);
  mysql_mutex_unlock(&LOCK_global_system_variables);
  wsrep_stop_replication(thd, false);

  if (wsrep_start_replication()) {
    wsrep_create_rollbacker();
    wsrep_create_appliers(1);
#if 0
    if (WSREP_ON && wsrep_connected) {
      Wsrep_server_state::instance().wait_until_state(
          Wsrep_server_state::s_initializing);
      Wsrep_server_state::instance().initialized();
    }
#endif
    wsrep_create_appliers(wsrep_slave_threads - 1);
  }

  /* locking order to be enforced is:
     1. LOCK_global_system_variables
     2. LOCK_wsrep_cluster_config
     => have to juggle mutexes to comply with this
  */

  mysql_mutex_unlock(&LOCK_wsrep_cluster_config);
  mysql_mutex_lock(&LOCK_global_system_variables);
  mysql_mutex_lock(&LOCK_wsrep_cluster_config);

  return false;
}

void wsrep_cluster_address_init(const char *value) {
  WSREP_DEBUG("wsrep_cluster_address_init: %s -> %s",
              (wsrep_cluster_address) ? wsrep_cluster_address : "null",
              (value) ? value : "null");

  if (wsrep_cluster_address) my_free((void *)wsrep_cluster_address);
  wsrep_cluster_address =
      my_strdup(PSI_NOT_INSTRUMENTED, (value) ? value : "", MYF(0));
}

/* Function checks if the new value for cluster_name is valid.
@return false if no error encountered with check else return true. */
bool wsrep_cluster_name_check(sys_var *, THD *, set_var *var) {
  if (!var->save_result.string_value.str ||
      (var->save_result.string_value.length == 0) ||
      (var->save_result.string_value.length > WSREP_CLUSTER_NAME_MAX_LEN)) {
    my_error(
        ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
        (var->save_result.string_value.str ? var->save_result.string_value.str
                                           : "NULL"));
    return true;
  }
  return false;
}

bool wsrep_cluster_name_update(sys_var *, THD *, enum_var_type) {
  return 0;
}

/* Function checks if the new value for node_name is valid.
@return false if no error encountered with check else return true. */
bool wsrep_node_name_check(sys_var *, THD *, set_var *var) {
  // TODO: for now 'allow' 0-length string to be valid (default)
  if (!var->save_result.string_value.str) {
    my_error(
        ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
        (var->save_result.string_value.str ? var->save_result.string_value.str
                                           : "NULL"));
    return true;
  }
  return false;
}

bool wsrep_node_name_update(sys_var *, THD *, enum_var_type) { return 0; }

/* Function checks if the new value for node_address is valid.
TODO: do something more elaborate, like checking connectivity
@return false if no error encountered with check else return true. */
bool wsrep_node_address_check(sys_var *, THD *, set_var *var) {
  char addr_buf[FN_REFLEN];

  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1)))  // safety
    goto err;

  memcpy(addr_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  addr_buf[var->save_result.string_value.length] = 0;

  // TODO: for now 'allow' 0-length string to be valid (default)
  return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
           var->save_result.string_value.str ? var->save_result.string_value.str
                                             : "NULL");
  return true;
}

bool wsrep_node_address_update(sys_var *, THD *, enum_var_type) { return 0; }

void wsrep_node_address_init(const char *value) {
  if (wsrep_node_address && strcmp(wsrep_node_address, value))
    my_free((void *)wsrep_node_address);

  wsrep_node_address =
      (value) ? my_strdup(key_memory_wsrep, value, MYF(0)) : NULL;
}

static void wsrep_slave_count_change_update() {
  // wsrep_running_threads = appliers threads + rollbacker thread +
  //                         post rollback thread
  wsrep_slave_count_change = (wsrep_slave_threads - wsrep_running_threads + 2);
}

bool wsrep_slave_threads_update(sys_var *, THD *, enum_var_type) {
  wsrep_slave_count_change_update();
  if (wsrep_slave_count_change > 0) {
    WSREP_DEBUG("Creating %d applier threads, total %ld",
                wsrep_slave_count_change, wsrep_slave_threads);
    wsrep_create_appliers(wsrep_slave_count_change);
    wsrep_slave_count_change = 0;
  } else {
    WSREP_DEBUG("%d applier threads scheduled for closure",
                abs(wsrep_slave_count_change));
  }
  return false;
}

bool wsrep_desync_check(sys_var *, THD *thd, set_var *var) {
  if (!WSREP_ON) {
    my_message(ER_WRONG_ARGUMENTS, "WSREP (galera) not started", MYF(0));
    return true;
  }

  bool new_wsrep_desync = var->save_result.ulonglong_value;
  if (wsrep_desync == new_wsrep_desync) {
    if (new_wsrep_desync) {
      push_warning(thd, Sql_condition::SL_WARNING, ER_WRONG_VALUE_FOR_VAR,
                   "'wsrep_desync' is already ON.");
    } else {
      push_warning(thd, Sql_condition::SL_WARNING, ER_WRONG_VALUE_FOR_VAR,
                   "'wsrep_desync' is already OFF.");
    }
    return false;
  }

  /* Setting desync to on/off signals participation of node in flow control.
  It doesn't turn-off recieval of write-sets.
  If the node is already in paused state due to some previous action like FTWRL
  then avoid desync as superset action of pausing the node is already active. */

  if (!thd->global_read_lock.provider_resumed()) {
    char message[1024];
    sprintf(message,
            "Explictly desync/resync of already desynced/paused node"
            " is prohibited");
    my_message(ER_UNKNOWN_ERROR, message, MYF(0));
    return true;
  }
  int ret= 1;
  if (new_wsrep_desync) {
    ret= Wsrep_server_state::instance().provider().desync();
    if (ret) {
      WSREP_WARN ("SET desync failed %d for schema: %s, query: %s", ret,
                  thd->db().str, WSREP_QUERY(thd));
      my_error (ER_CANNOT_USER, MYF(0), "'desync'", thd->query());
      return true;
    }
  } else {
    ret= Wsrep_server_state::instance().provider().resync();
    if (ret != WSREP_OK) {
      WSREP_WARN ("SET resync failed %d for schema: %s, query: %s", ret,
                  thd->db().str, WSREP_QUERY(thd));
      my_error (ER_CANNOT_USER, MYF(0), "'resync'", thd->query());
      return true;
    }
  }
  return false;
}

bool wsrep_desync_update(sys_var *, THD *, enum_var_type) { return false; }

bool wsrep_max_ws_size_update(sys_var *, THD *, enum_var_type) {
  char max_ws_size_opt[128];
  snprintf(max_ws_size_opt, sizeof(max_ws_size_opt), "repl.max_ws_size=%lu",
           wsrep_max_ws_size);
  enum wsrep::provider::status ret =
      Wsrep_server_state::instance().provider().options(max_ws_size_opt);
  if (ret) {
    WSREP_ERROR("Set options returned %d", ret);
    return true;
  }
  return refresh_provider_options();
}

bool wsrep_trx_fragment_size_check(sys_var *, THD *thd, set_var *var) {

  if (!WSREP(thd)) {
    push_warning(thd, Sql_condition::SL_WARNING, ER_WRONG_VALUE_FOR_VAR,
                 "Cannot set 'wsrep_trx_fragment_size' to a value other than "
                 "0 because wsrep is switched off.");
    return true;
  }

  if (var->value == NULL) {
    return false;
  }

  const ulong new_trx_fragment_size = var->value->val_uint();

  if (!WSREP(thd) && new_trx_fragment_size > 0) {
    push_warning(thd, Sql_condition::SL_WARNING, ER_WRONG_VALUE_FOR_VAR,
                 "Cannot set 'wsrep_trx_fragment_size' to a value other than "
                 "0 because wsrep is switched off.");
    return true;
  }

  if (new_trx_fragment_size > 0 && !wsrep_provider_is_SR_capable()) {
    push_warning(thd, Sql_condition::SL_WARNING, ER_WRONG_VALUE_FOR_VAR,
                 "Cannot set 'wsrep_trx_fragment_size' to a value other than "
                 "0 because the wsrep_provider does not support streaming "
                 "replication.");
    return true;
  }

  if (wsrep_protocol_version < 4  && new_trx_fragment_size > 0) {
    push_warning (thd, Sql_condition::SL_WARNING,
                  ER_WRONG_VALUE_FOR_VAR,
                  "Cannot set 'wsrep_trx_fragment_size' to a value other than "
                  "0 because cluster is not yet operating in Galera 4 mode.");
    return true;
  }

  return false;
}

bool wsrep_trx_fragment_size_update(sys_var *, THD *thd, enum_var_type) {
  WSREP_DEBUG("wsrep_trx_fragment_size_update: %llu",
              thd->variables.wsrep_trx_fragment_size);
  if (thd->variables.wsrep_trx_fragment_size) {
    return thd->wsrep_cs().enable_streaming(
        wsrep_fragment_unit(thd->variables.wsrep_trx_fragment_unit),
        size_t(thd->variables.wsrep_trx_fragment_size));
  } else {
    thd->wsrep_cs().disable_streaming();
    return false;
  }
}

bool wsrep_trx_fragment_unit_update(sys_var *, THD *thd, enum_var_type) {
  WSREP_DEBUG("wsrep_trx_fragment_unit_update: %lu",
              thd->variables.wsrep_trx_fragment_unit);
  if (thd->variables.wsrep_trx_fragment_size) {
    return thd->wsrep_cs().enable_streaming(
        wsrep_fragment_unit(thd->variables.wsrep_trx_fragment_unit),
        size_t(thd->variables.wsrep_trx_fragment_size));
  }
  return false;
}

/*
 * Status variables stuff below
 */
static inline void wsrep_assign_to_mysql(SHOW_VAR *mysql,
                                         wsrep_stats_var *wsrep) {
  mysql->scope = SHOW_SCOPE_ALL;
  mysql->name = wsrep->name;
  switch (wsrep->type) {
    case WSREP_VAR_INT64:
      mysql->value = (char *)&wsrep->value._int64;
      mysql->type = SHOW_LONGLONG;
      break;
    case WSREP_VAR_STRING:
      mysql->value = (char *)&wsrep->value._string;
      mysql->type = SHOW_CHAR_PTR;
      break;
    case WSREP_VAR_DOUBLE:
      mysql->value = (char *)&wsrep->value._double;
      mysql->type = SHOW_DOUBLE;
      break;
  }
}

static SHOW_VAR mysql_status_vars[512 + 1];
static const int mysql_status_len = 512;

static void export_wsrep_status_to_mysql(THD *thd) {
  int wsrep_status_len, i;

  /* Avoid freeing the stats immediately on completion of the call
  instead free them on next invocation as the reference to status
  is feeded in MySQL show status array. Freeing them on completion
  will make these references invalid. */
  wsrep_free_status(thd);

  thd->wsrep_status_vars = Wsrep_server_state::instance().status();

  wsrep_status_len= thd->wsrep_status_vars.size();

  if (mysql_status_len < wsrep_status_len) wsrep_status_len = mysql_status_len;

  for (i = 0; i < wsrep_status_len; i++) {
    mysql_status_vars[i].name =
        (char *)thd->wsrep_status_vars[i].name().c_str();
    mysql_status_vars[i].value =
        (char *)thd->wsrep_status_vars[i].value().c_str();
    mysql_status_vars[i].type = SHOW_CHAR;
    mysql_status_vars[i].scope = SHOW_SCOPE_ALL;
  }

  mysql_status_vars[wsrep_status_len].name = NullS;
  mysql_status_vars[wsrep_status_len].value = NullS;
  mysql_status_vars[wsrep_status_len].type = SHOW_LONG;
}

int wsrep_show_status(THD *thd, SHOW_VAR *var, char *) {
  export_wsrep_status_to_mysql(thd);
  var->type = SHOW_ARRAY;
  var->value = (char *)&mysql_status_vars;
  return 0;
}

void wsrep_free_status(THD *thd) {
  thd->wsrep_status_vars.clear();
}

bool wsrep_replicate_myisam_check(sys_var *, THD *thd, set_var *var) {
  /* Trying to set wsrep_replicate_myisam to off. */
  if (var->save_result.ulonglong_value == 0) return false;

  /* If pxc-strict-mode >= ENFORCING we don't allow setting
  wsrep_replicate_myisam to ON. */
  bool block = false;

  switch (pxc_strict_mode) {
    case PXC_STRICT_MODE_DISABLED:
      break;
    case PXC_STRICT_MODE_PERMISSIVE:
      WSREP_WARN(
          "Percona-XtraDB-Cluster doesn't recommend use of MyISAM"
          " table replication feature"
          " with pxc_strict_mode = PERMISSIVE");
      push_warning_printf(
          thd, Sql_condition::SL_WARNING, ER_UNKNOWN_ERROR,
          "Percona-XtraDB-Cluster doesn't recommend use of MyISAM"
          " table replication feature"
          " with pxc_strict_mode = PERMISSIVE");
      break;
    case PXC_STRICT_MODE_ENFORCING:
    case PXC_STRICT_MODE_MASTER:
    default:
      block = true;
      WSREP_ERROR(
          "Percona-XtraDB-Cluster prohibits use of MyISAM"
          " table replication feature"
          " with pxc_strict_mode = ENFORCING or MASTER");
      char message[1024];
      sprintf(message,
              "Percona-XtraDB-Cluster prohibits use of MyISAM"
              " table replication feature"
              " with pxc_strict_mode = ENFORCING or MASTER");
      my_message(ER_UNKNOWN_ERROR, message, MYF(0));
      break;
  }

  return block;
}

static const char *pxc_strict_mode_to_string(ulong value) {
  switch (value) {
    case PXC_STRICT_MODE_DISABLED:
      return "DISABLED";
    case PXC_STRICT_MODE_PERMISSIVE:
      return "PERMISSIVE";
    case PXC_STRICT_MODE_ENFORCING:
      return "ENFORCING";
    case PXC_STRICT_MODE_MASTER:
      return "MASTER";
    default:
      return "NULL";
  }
}

bool pxc_strict_mode_check(sys_var *, THD *thd, set_var *var) {
  /* pxc-strict-mode can be changed only if node is cluster-node. */
  if (!(WSREP_ON)) {
    WSREP_ERROR("pxc_strict_mode can be changed only if node is cluster-node");

    char message[1024];
    sprintf(message,
            "pxc_strict_mode can be changed only if node is cluster-node");
    my_message(ER_UNKNOWN_ERROR, message, MYF(0));
    return true;
  }

  bool enforcing_strictness = false;
  bool block = false;

  if (pxc_strict_mode <= PXC_STRICT_MODE_PERMISSIVE &&
      var->save_result.ulonglong_value >= PXC_STRICT_MODE_ENFORCING)
    enforcing_strictness = true;

  if (enforcing_strictness) {
    /* wsrep_replicate_myisam shouldn't be OFF
    TODO: Mutex ordering ? */
    bool replicate_myisam;
    replicate_myisam = (global_system_variables.wsrep_replicate_myisam ||
                        thd->variables.wsrep_replicate_myisam);

    bool row_binlog_format;
    row_binlog_format =
        (global_system_variables.binlog_format == BINLOG_FORMAT_ROW &&
         thd->variables.binlog_format == BINLOG_FORMAT_ROW);

    bool safe_log_output;
    safe_log_output =
        ((log_output_options & LOG_NONE) || (log_output_options & LOG_FILE));

    bool serializable;
    serializable =
        (thd->tx_isolation == ISO_SERIALIZABLE ||
         global_system_variables.transaction_isolation == ISO_SERIALIZABLE);

    /* replicate_myisam = off
       row_binlog_format = true (row)
       safe_log_output = true (none/file)
       serializable = false */
    block = !(!replicate_myisam && row_binlog_format && safe_log_output &&
              !serializable);

    if (replicate_myisam)
      WSREP_ERROR(
          "Can't change pxc_strict_mode with wsrep_replicate_myisam"
          " turned ON");

    if (!row_binlog_format)
      WSREP_ERROR("Can't change pxc_strict_mode while binlog format != ROW");

    if (!safe_log_output)
      WSREP_ERROR("Can't change pxc_strict_mode while log_output != NONE/FILE");

    if (serializable)
      WSREP_ERROR(
          "Can't change pxc_strict_mode while isolation level is"
          " SERIALIZABLE");

    if (block) {
      char message[1024];
      sprintf(message, "Can't change pxc_strict_mode to %s as%s%s%s%s",
              pxc_strict_mode_to_string(var->save_result.ulonglong_value),
              (replicate_myisam ? " wsrep_replicate_myisam is ON" : ""),
              (!row_binlog_format ? " binlog_format != ROW" : ""),
              (!safe_log_output ? " log_output != NONE/FILE" : ""),
              (serializable ? " isolation level is SERIALIZABLE" : ""));
      my_message(ER_UNKNOWN_ERROR, message, MYF(0));
    }
  }

  return (block);
}

static const char *pxc_maint_mode_to_string(ulong value) {
  switch (value) {
    case PXC_MAINT_MODE_DISABLED:
      return "DISABLED";
    case PXC_MAINT_MODE_SHUTDOWN:
      return "SHUTDOWN";
    case PXC_MAINT_MODE_MAINTENANCE:
      return "MAINTENANCE";
    default:
      return "NULL";
  }
}

bool pxc_maint_mode_check(sys_var *, THD *, set_var *var) {
  /* pxc-maint-mode can be changed only if node is cluster-node. */
  if (!(WSREP_ON)) {
    WSREP_ERROR("pxc_maint_mode can be changed only if node is cluster-node");

    char message[1024];
    sprintf(message,
            "pxc_maint_mode can be changed only if node is cluster-node");
    my_message(ER_UNKNOWN_ERROR, message, MYF(0));
    return true;
  }

  /* Following transitions are allowed.
  DISABLED -> MAINTENANCE
  MAINTENANCE -> DISABLED
  All other combinations are blocked
  SHUTDOWN can't be set explictly. It is done when user initiate shutdown
  action. Once the node in shutdown mode toggling is not allowed. */

  bool not_shutdown = (pxc_maint_mode != PXC_MAINT_MODE_SHUTDOWN);
  bool to_maint =
      (pxc_maint_mode == PXC_MAINT_MODE_DISABLED &&
       var->save_result.ulonglong_value == PXC_MAINT_MODE_MAINTENANCE);
  bool back_from_maint =
      (pxc_maint_mode == PXC_MAINT_MODE_MAINTENANCE &&
       var->save_result.ulonglong_value == PXC_MAINT_MODE_DISABLED);
  bool retain_disable =
      (pxc_maint_mode == PXC_MAINT_MODE_DISABLED &&
       var->save_result.ulonglong_value == PXC_MAINT_MODE_DISABLED);

  bool allowed_transition =
      not_shutdown && (to_maint || back_from_maint || retain_disable);

  if (!allowed_transition) {
    char message[1024];
    sprintf(message, "Can't change pxc_maint_mode from %s to %s",
            pxc_maint_mode_to_string(pxc_maint_mode),
            pxc_maint_mode_to_string(var->save_result.ulonglong_value));
    my_message(ER_UNKNOWN_ERROR, message, MYF(0));
    return true;
  }

  return false;
}

bool pxc_maint_mode_update(sys_var *, THD *, enum_var_type) { return false; }
