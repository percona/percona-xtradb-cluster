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

#undef SAFE_MUTEX
#include <mysqld.h>
#include <sql_class.h>
#include "wsrep_priv.h"
#include <cstdio>
#include <cstdlib>

wsrep_t *wsrep                  = NULL;
my_bool wsrep_emulate_bin_log   = FALSE; // activating parts of binlog interface

/*
 * Begin configuration options and their default values
 */

const char* wsrep_data_home_dir = mysql_real_data_home;
const char* wsrep_cluster_name  = "my_wsrep_cluster";
const char* wsrep_node_name     = glob_hostname;
#define WSREP_NODE_INCOMING_AUTO "AUTO"
const char* wsrep_node_incoming_address = WSREP_NODE_INCOMING_AUTO;
const char* wsrep_dbug_option   = "";

long    wsrep_slave_threads            = 1; // # of slave action appliers wanted
long    wsrep_local_cache_size         = 20971520;// space allocd for local write sets
my_bool wsrep_ws_persistency           = 0; // policy for certified WS storage
my_bool wsrep_debug                    = 0; // enable debug level logging
my_bool wsrep_convert_LOCK_to_trx      = 1; // convert locking sessions to trx
ulong   wsrep_retry_autocommit         = 5; // retry aborted autocommit trx
my_bool wsrep_auto_increment_control   = 1; // control auto increment variables
my_bool wsrep_drupal_282555_workaround = 1; // retry autoinc insert after dupkey
my_bool wsrep_incremental_data_collection = 0; // incremental data collection
long long wsrep_max_ws_size            = 1073741824LL; //max ws (RBR buffer) size
long    wsrep_max_ws_rows              = 65536; // max number of rows in ws
int     wsrep_to_isolation             = 0; // # of active TO isolation threads
my_bool wsrep_certify_nonPK            = 1; // certify, even when no primary key
long    wsrep_max_protocol_version     = 1; // maximum protocol version to use

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
/* End wsrep status variables */


wsrep_uuid_t     local_uuid   = WSREP_UUID_UNDEFINED;
wsrep_seqno_t    local_seqno  = WSREP_SEQNO_UNDEFINED;
wsp::node_status local_status;
static long      wsrep_protocol_version = 1;

// action execute callback
extern wsrep_status_t wsrep_bf_apply_cb(void *ctx,
                                        wsrep_apply_data_t *data,
                                        wsrep_seqno_t global_seqno);

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

static void wsrep_log_states (wsrep_log_level_t level,
                              wsrep_uuid_t* group_uuid,
                              wsrep_seqno_t group_seqno,
                              wsrep_uuid_t* node_uuid,
                              wsrep_seqno_t node_seqno)
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

static void wsrep_view_handler_cb (void* app_ctx,
                                   void* recv_ctx,
                                   wsrep_view_info_t* view,
                                   const char* state,
                                   size_t state_len,
                                   void** sst_req,
                                   ssize_t* sst_req_len)
{
  wsrep_member_status_t new_status= local_status.get();

  if (memcmp(&cluster_uuid, &view->id, sizeof(wsrep_uuid_t)))
  {
    cluster_uuid= view->id;
    wsrep_uuid_print (&cluster_uuid, cluster_uuid_str,
                      sizeof(cluster_uuid_str));
  }

  wsrep_cluster_conf_id= view->conf;
  wsrep_cluster_status= cluster_status_str[view->status];
  wsrep_cluster_size= view->memb_num;
  wsrep_local_index= view->my_idx;
  switch (view->proto_ver)
  {
  case 0:
  case 1:
      // version change
      if (view->proto_ver != wsrep_protocol_version)
      {
          my_bool wsrep_ready_saved= wsrep_ready;
          wsrep_ready= FALSE;
          WSREP_INFO("closing client connections for "
                     "protocol change %ld -> %d",
                     wsrep_protocol_version, view->proto_ver);
          wsrep_close_client_connections();
          wsrep_protocol_version= view->proto_ver;
          wsrep_ready= wsrep_ready_saved;
      }
      break;
  default:
      WSREP_ERROR("Unsupported application protocol version: %d",
                  view->proto_ver);
      abort();
  }

  WSREP_INFO("New cluster view: group UUID: %s, conf# %lld: %s, "
             "number of nodes: %ld, my index: %ld, first seqno: %lld, "
             "protocol version %d",
             wsrep_cluster_state_uuid, (long long)wsrep_cluster_conf_id,
             wsrep_cluster_status, wsrep_cluster_size,
             wsrep_local_index, (long long)view->first,
             view->proto_ver);

  /* Proceed further only if view is PRIMARY */
  if (WSREP_VIEW_PRIMARY != view->status) {
    wsrep_ready= FALSE;
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

  if (view->state_gap) {

    WSREP_WARN("Gap in state sequence. Need state transfer.");

    /* After that wsrep will call wsrep_sst_prepare. */
    /* keep ready flag 0 until we receive the snapshot */
    wsrep_ready= FALSE;

    /* Close client connections to ensure that they don't interfere
     * with SST */
    WSREP_DEBUG("[debug]: closing client connections for PRIM");
    wsrep_close_client_connections();

    *sst_req_len= wsrep_sst_prepare (sst_req);

    if (*sst_req_len < 0)
    {
      int err = *sst_req_len;
      WSREP_ERROR("SST preparation failed: %d (%s)", -err, strerror(-err));
      new_status= WSREP_MEMBER_UNDEFINED;
    }
    else
    {
      new_status= WSREP_MEMBER_JOINER;
    }
  }
  else
  {
    /*
     *  NOTE: Initialize wsrep_group_uuid here only if it wasn't initialized
     *  before.
     */
    if (!memcmp (&local_uuid, &WSREP_UUID_UNDEFINED, sizeof(wsrep_uuid_t)))
    {
      if (wsrep_init_first())
      {
        wsrep_SE_init_grab();
        // Signal init thread to continue
        wsrep_sst_complete (&cluster_uuid, view->first - 1, false);
        // and wait for SE initialization
        wsrep_SE_init_wait();
      }
      else
      {
        local_uuid=  cluster_uuid;
        local_seqno= view->first - 1;
      }

      new_status= WSREP_MEMBER_JOINED;
    }
    else // just some sanity check
    {
      if (memcmp (&local_uuid, &cluster_uuid, sizeof (wsrep_uuid_t)))
      {
        WSREP_ERROR("Undetected state gap. Can't continue.");
        wsrep_log_states (WSREP_LOG_FATAL, &cluster_uuid, view->first,
                          &local_uuid, -1);
        abort();
      }
    }
  }

  if (wsrep_auto_increment_control)
  {
    global_system_variables.auto_increment_offset= view->my_idx + 1;
    global_system_variables.auto_increment_increment= view->memb_num;
  }

out:

  local_status.set(new_status, view);
  free(view);
}

static pthread_mutex_t ready_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  ready_cond = PTHREAD_COND_INITIALIZER;

// Wait until wsrep has reached ready state
void wsrep_ready_wait ()
{
  if (pthread_mutex_lock (&ready_lock)) abort();
  while (!wsrep_ready)
    {
      WSREP_INFO("Waiting to reach ready state");
      pthread_cond_wait (&ready_cond, &ready_lock);
    }
  WSREP_INFO("ready state reached");
  pthread_mutex_unlock (&ready_lock);
}

static void wsrep_synced_cb(void* app_ctx)
{
  WSREP_INFO("Synchronized with group, ready for connections");
  if (pthread_mutex_lock (&ready_lock)) abort();
  if (!wsrep_ready)
  {
    wsrep_ready= TRUE;
    pthread_cond_signal (&ready_cond);
  }
  local_status.set(WSREP_MEMBER_SYNCED);
  pthread_mutex_unlock (&ready_lock);
}

void wsrep_init()
{
  int rcode= -1;

  wsrep_ready= FALSE;
  assert(wsrep_provider);
  assert(wsrep_cluster_address);

  if ((rcode= wsrep_load(wsrep_provider, &wsrep, wsrep_log_cb)) != WSREP_OK)
  {
    if (strcasecmp(wsrep_provider, WSREP_NONE))
    {
      WSREP_ERROR("wsrep_load(%s) failed: %s (%d). Reverting to no provider.",
                  wsrep_provider, strerror(rcode), rcode);
      strcpy((char*)wsrep_provider, WSREP_NONE); // damn it's an ugly hack
      wsrep_init();
    }
    else
    {
      WSREP_ERROR("Could not revert to no provider: %s (%d). Need to abort.",
                  strerror(rcode), rcode);
      unireg_abort(1);
    }
  }

  if (strlen(wsrep_provider)== 0 ||
      !strcmp(wsrep_provider, WSREP_NONE))
  {
    // enable normal operation in case no provider is specified
    wsrep_ready= TRUE;
  }

  struct wsrep_init_args wsrep_args;

  if (!wsrep_node_incoming_address ||
      !strcmp (wsrep_node_incoming_address, WSREP_NODE_INCOMING_AUTO)) {
    static char inc_addr[256];
    size_t inc_addr_max = sizeof (inc_addr);
    size_t ret = default_address (inc_addr, inc_addr_max);
    if (ret > 0 && ret < inc_addr_max) {
      wsrep_node_incoming_address = inc_addr;
    }
    else {
      wsrep_node_incoming_address = NULL;
    }
  }

  wsrep_args.data_dir        = mysql_real_data_home;
  wsrep_args.node_name       = wsrep_node_name;
  wsrep_args.node_incoming   = wsrep_node_incoming_address;
  wsrep_args.options         = wsrep_provider_options;
  wsrep_args.proto_ver       = wsrep_max_protocol_version;

  wsrep_args.state_uuid      = &local_uuid;
  wsrep_args.state_seqno     = local_seqno;

  wsrep_args.logger_cb       = wsrep_log_cb;
  wsrep_args.view_handler_cb = wsrep_view_handler_cb;
  wsrep_args.synced_cb       = wsrep_synced_cb;
  wsrep_args.bf_apply_cb     = wsrep_bf_apply_cb;
  wsrep_args.sst_donate_cb   = wsrep_sst_donate_cb;

  rcode = wsrep->init(wsrep, &wsrep_args);

  if (rcode)
  {
    DBUG_PRINT("wsrep",("wsrep::init() failed: %d", rcode));
    WSREP_ERROR("wsrep::init() failed: %d", rcode);
//    goto err;
  }
}

void wsrep_deinit()
{
  wsrep_unload(wsrep);
  wsrep= 0;
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
  WSREP_DEBUG("Provider disconnect");
  wsrep->disconnect(wsrep);

  wsrep_connected= FALSE;

  wsrep_close_client_connections();

  /* wait until appliers have stopped */
  wsrep_wait_appliers_close(thd);

  return;
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
    wsrep_ready = TRUE;
    return true;
  }

  if (strlen(wsrep_cluster_address)== 0)
  {
    // if provider is non-trivial, but no address is specified, wait for address
    wsrep_ready = FALSE;
    return true;
  }

  WSREP_INFO("Start replication");

  if ((rcode = wsrep->connect(wsrep,
                              wsrep_cluster_name,
                              wsrep_cluster_address,
                              wsrep_sst_donor)))
  {
    if (-ESOCKTNOSUPPORT == rcode)
    {
      DBUG_PRINT("wsrep",("unrecognized cluster address: '%s', rcode: %d",
                          wsrep_cluster_address, rcode));
      WSREP_ERROR("unrecognized cluster address: '%s', rcode: %d",
                  wsrep_cluster_address, rcode);
    }
    else
    {
      DBUG_PRINT("wsrep",("wsrep->connect() failed: %d", rcode));
      WSREP_ERROR("wsrep::connect() failed: %d", rcode);
    }

    return false;
  }
  else
  {
    wsrep_connected= TRUE;

    uint64_t caps = wsrep->capabilities (wsrep);

    wsrep_incremental_data_collection =
        (caps & WSREP_CAP_WRITE_SET_INCREMENTS);

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

bool
wsrep_causal_wait (THD* thd)
{
  if (thd->variables.wsrep_causal_reads && thd->variables.wsrep_on)
  {
    // This allows autocommit SELECTs and a first SELECT after SET AUTOCOMMIT=0
    // TODO: modify to check if thd has locked any rows.
    if (unlikely(thd->in_active_multi_stmt_transaction()))
    {
      my_error(ER_NOT_SUPPORTED_YET, MYF(0), "wsrep_causal_reads=ON "
               "inside transactions");
      return true;
    }

    wsrep_seqno_t  seqno;
    wsrep_status_t ret= wsrep->causal_read (wsrep, &seqno);

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
        msg= "consistent reads by wsrep backend. "
             "Please unset wsrep_causal_reads variable.";
        err= ER_NOT_SUPPORTED_YET;
        break;
      default:
        msg= "Causal wait failed.";
        err= ER_ERROR_ON_READ;
      }

      my_error(err, MYF(0), msg);

      return true;
    }
  }

  return false;
}

bool wsrep_prepare_key_for_isolation(const char* db,
                                     const char* table,
                                     wsrep_key_t* key,
                                     size_t* key_len)
{
    if (*key_len < 2) return false;

    switch (wsrep_protocol_version)
    {
    case 0:
        *key_len= 0;
        break;
    case 1:
    {
        *key_len= 0;
        if (db)
        {
            // sql_print_information("%s.%s", db, table);
            if (db)
            {
                key[*key_len].key= db;
                key[*key_len].key_len= strlen(db);
                ++(*key_len);
                if (table)
                {
                    key[*key_len].key=     table;
                    key[*key_len].key_len= strlen(table);
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


bool wsrep_prepare_key_for_innodb(const TABLE_SHARE* table_share,
                                  const uchar* row_id,
                                  size_t row_id_len,
                                  wsrep_key_t* key,
                                  size_t* key_len)
{
    if (*key_len < 3) return false;

    *key_len= 0;
    switch (wsrep_protocol_version)
    {
    case 0:
        key[*key_len].key=     table_share->table_cache_key.str;
        key[*key_len].key_len= table_share->table_cache_key.length;
        ++(*key_len);
        break;
    case 1:
    {
        key[*key_len].key=     table_share->db.str;
        key[*key_len].key_len= table_share->db.length;
        ++(*key_len);
        key[*key_len].key= table_share->table_name.str;
        key[*key_len].key_len= table_share->table_name.length;
        ++(*key_len);
        break;
    }
    default:
        return false;
    }

    key[*key_len].key= row_id;
    key[*key_len].key_len= row_id_len;
    ++(*key_len);

    return true;
}
