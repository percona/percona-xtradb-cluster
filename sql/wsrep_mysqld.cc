/* Copyright 2008-2015 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.x1

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <sql_plugin.h> // SHOW_MY_BOOL
#include <mysqld.h>
#include <sql_class.h>
#include <sql_parse.h>
#include "debug_sync.h"
#include <sql_base.h> /* find_temporary_table() */
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
#include "debug_sync.h"

wsrep_t *wsrep                  = NULL;
my_bool wsrep_emulate_bin_log   = FALSE; // activating parts of binlog interface
/* Sidno in global_sid_map corresponding to group uuid */
rpl_sidno wsrep_sidno= -1;
my_bool wsrep_preordered_opt= FALSE;

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
ulong   wsrep_certification_rules      = WSREP_CERTIFICATION_RULES_STRICT;
long    wsrep_max_protocol_version     = 3; // maximum protocol version to use
ulong   wsrep_forced_binlog_format     = BINLOG_FORMAT_UNSPEC;
my_bool wsrep_recovery                 = 0; // recovery
my_bool wsrep_log_conflicts            = 0;
ulong   wsrep_mysql_replication_bundle = 0;
my_bool wsrep_desync                   = 0; // desynchronize the node from the
                                            // cluster
my_bool wsrep_load_data_splitting      = 1; // commit load data every 10K intervals
my_bool wsrep_restart_slave            = 0; // should mysql slave thread be
                                            // restarted, if node joins back
my_bool wsrep_restart_slave_activated  = 0; // node has dropped, and slave
                                            // restart will be needed
my_bool wsrep_slave_UK_checks          = 0; // slave thread does UK checks
my_bool wsrep_slave_FK_checks          = 0; // slave thread does FK checks
ulong   wsrep_RSU_commit_timeout       = 5000; // wait for x micr-secs
                                               // to allow active connection to
                                               // commit before starting RSU.

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
  if (WSREP_VIEW_PRIMARY != view->status)
  {
#ifdef HAVE_QUERY_CACHE
    // query cache must be initialised by now
    query_cache.flush();
#endif /*HAVE_QUERY_CACHE*/
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

  wsrep_ready_set(TRUE);

  switch (view->proto_ver)
  {
  case 0:
  case 1:
  case 2:
  case 3:
      // version change
      if (view->proto_ver != wsrep_protocol_version)
      {
          my_bool wsrep_ready_saved= wsrep_ready_get();
          wsrep_ready_set(FALSE);
          WSREP_INFO("closing client connections for "
                     "protocol change %ld -> %d",
                     wsrep_protocol_version, view->proto_ver);
          wsrep_close_client_connections(TRUE);
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
        wsrep_close_client_connections(TRUE);
    }

    ssize_t const req_len= wsrep_sst_prepare (sst_req);

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
        // Signal mysqld init thread to continue
        wsrep_sst_complete (&cluster_uuid, view->state_id.seqno, false);
        // and wait for SE initialization
        if (wsrep_SE_init_wait() == WSREP_SE_INIT_RESULT_FAILURE)
        {
            WSREP_ERROR("Storage engine initialization failed.");
            return WSREP_CB_FAILURE;
        }
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
    WSREP_INFO("Auto Increment Offset/Increment re-align with cluster"
                " membership change (Offset: %lu -> %lu)"
                " (Increment: %lu -> %lu)",
                global_system_variables.auto_increment_offset,
                (long unsigned) view->my_idx + 1,
                global_system_variables.auto_increment_increment,
                (long unsigned) view->memb_num);

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

my_bool wsrep_ready_set (my_bool x)
{
  WSREP_INFO("Setting wsrep_ready to %s", (x ? "true" : "false"));
  if (mysql_mutex_lock (&LOCK_wsrep_ready)) abort();
  my_bool ret= (wsrep_ready != x);
  if (ret)
  {
    wsrep_ready= x;
    mysql_cond_signal (&COND_wsrep_ready);
  }
  mysql_mutex_unlock (&LOCK_wsrep_ready);
  return ret;
}

my_bool wsrep_ready_get (void)
{
  if (mysql_mutex_lock (&LOCK_wsrep_ready)) abort();
  my_bool ret= wsrep_ready;
  mysql_mutex_unlock (&LOCK_wsrep_ready);
  return ret;
}

int wsrep_show_ready(THD *thd, SHOW_VAR *var, char *buff)
{
  var->type= SHOW_MY_BOOL;
  var->value= buff;
  *((my_bool *)buff)= wsrep_ready_get();
  return 0;
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
  WSREP_INFO("ready state reached");
  mysql_mutex_unlock (&LOCK_wsrep_ready);
}

static void wsrep_synced_cb(void* app_ctx)
{
  WSREP_INFO("Synchronized with group, ready for connections");
  my_bool signal_main= wsrep_ready_set(TRUE);
  local_status.set(WSREP_MEMBER_SYNCED);

  if (signal_main)
  {
    // Signal mysqld init thread to continue
    wsrep_sst_complete (&local_uuid, local_seqno, false);
    // And wait for SE initialization. Return immediately in
    // case of failure, the server is going to shut down.
    if (wsrep_SE_init_wait() == WSREP_SE_INIT_RESULT_FAILURE)
      return;
  }
  if (wsrep_restart_slave_activated)
  {
    int rcode;
    WSREP_INFO("MySQL slave restart");
    wsrep_restart_slave_activated= FALSE;

    mysql_mutex_lock(&LOCK_active_mi);
    if ((rcode = start_slave_threads(1 /* need mutex */,
                            0 /* no wait for start*/,
                            active_mi,
                       	    SLAVE_SQL)))
    {
      WSREP_WARN("Failed to create slave threads: %d", rcode);
    }
    mysql_mutex_unlock(&LOCK_active_mi);

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
    WSREP_INFO("Read nil XID from storage engines, skipping position init");
    return;
  }

  char uuid_str[40] = {0, };
  wsrep_uuid_print(&uuid, uuid_str, sizeof(uuid_str));
  WSREP_INFO("Initial position: %s:%lld", uuid_str, (long long)seqno);

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
    WSREP_WARN("Initial position was provided by configuration or SST, "
               "avoiding override");
  }
}

int wsrep_init()
{
  int rcode= -1;

  wsrep_ready_set(FALSE);
  assert(wsrep_provider);

  // Reset global settings
  wsrep_incremental_data_collection= FALSE;

  wsrep_init_position();

  if ((rcode= wsrep_load(wsrep_provider, &wsrep, wsrep_log_cb)) != WSREP_OK)
  {
    if (strlen(wsrep_provider) == 0)
    {
      WSREP_ERROR("wsrep_load() failed to load the provider('%s'): %s (%d). Reverting to no provider.",
                  wsrep_provider, strerror(rcode), rcode);

      if (wsrep_provider) my_free((void *)wsrep_provider);
      wsrep_provider = my_strdup(WSREP_NONE, MYF(0)); // damn it's a dirty hack

      return wsrep_init();
    }
    else if (strcasecmp(wsrep_provider, WSREP_NONE))
    {
      WSREP_ERROR("wsrep_load() failed to load the provider('%s'): %s (%d). Need to abort.",
                  wsrep_provider, strerror(rcode), rcode);
      unireg_abort(1);
    }
    else /* this is for recursive call above */
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
      WSREP_ERROR("wsrep::init() failed: %d, must shutdown", rcode);
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
    WSREP_ERROR("wsrep::init() failed: %d, must shutdown", rcode);
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
        size_t const ip_len= wsrep_host_len(node_addr, node_addr_len);
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

  struct wsrep_init_args wsrep_args;

  struct wsrep_gtid const state_id = { local_uuid, local_seqno };

  wsrep_args.data_dir        = wsrep_data_home_dir;
  wsrep_args.node_name       = (wsrep_node_name) ? wsrep_node_name : "";
  wsrep_args.node_address    = node_addr;
  wsrep_args.node_incoming   = inc_addr;
  wsrep_args.options         = (wsrep_provider_options) ?
                                wsrep_provider_options : "";
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

  rcode = wsrep->init(wsrep, &wsrep_args);

  if (rcode)
  {
    DBUG_PRINT("wsrep",("wsrep::init() failed: %d", rcode));
    WSREP_ERROR("wsrep::init() failed: %d, must shutdown", rcode);
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
  wsrep_sst_auth_free();
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
  WSREP_DEBUG("Provider disconnect");
  wsrep->disconnect(wsrep);

  wsrep_connected= FALSE;

  wsrep_close_client_connections(TRUE);

  /* wait until appliers have stopped */
  wsrep_wait_appliers_close(thd);

  return;
}

/* This one is set to true when --wsrep-new-cluster is found in the command
 * line arguments */
static my_bool wsrep_new_cluster= FALSE;
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

  WSREP_INFO("Start replication");

  if ((rcode = wsrep->connect(wsrep,
                              wsrep_cluster_name,
                              wsrep_cluster_address,
                              wsrep_sst_donor,
                              bootstrap)))
  {
    DBUG_PRINT("wsrep",("wsrep->connect(%s) failed: %d",
                        wsrep_cluster_address, rcode));
    WSREP_ERROR("wsrep::connect(%s) failed: %d",
                wsrep_cluster_address, rcode);
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
    WSREP_DEBUG("wsrep_sync_wait: thd->variables.wsrep_sync_wait = %u, mask = %u",
                thd->variables.wsrep_sync_wait, mask);
    // This allows autocommit SELECTs and a first SELECT after SET AUTOCOMMIT=0
    // TODO: modify to check if thd has locked any rows.
    wsrep_status_t ret= wsrep->causal_read (wsrep, &thd->wsrep_sync_wait_gtid);

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
    break;
  }
  default:
    return false;
  }
  return true;
}


static bool wsrep_prepare_key_for_isolation(const char* db,
                                            const char* table,
                                            wsrep_key_arr_t* ka)
{
  wsrep_key_t* tmp;
  tmp= (wsrep_key_t*)my_realloc(ka->keys,
                                (ka->keys_len + 1) * sizeof(wsrep_key_t),
                                MYF(0));
  if (!tmp)
  {
    WSREP_ERROR("Can't allocate memory for key_array");
    return false;
  }
  ka->keys= tmp;
  if (!(ka->keys[ka->keys_len].key_parts= (wsrep_buf_t*)
        my_malloc(sizeof(wsrep_buf_t)*2, MYF(0))))
  {
    WSREP_ERROR("Can't allocate memory for key_parts");
    return false;
  }
  ka->keys[ka->keys_len].key_parts_num= 2;
  ++ka->keys_len;
  if (!wsrep_prepare_key_for_isolation(db, table,
                                       (wsrep_buf_t*)ka->keys[ka->keys_len - 1].key_parts,
                                       &ka->keys[ka->keys_len - 1].key_parts_num))
  {
    WSREP_ERROR("Preparing keys for isolation failed");
    return false;
  }

  return true;
}


static bool wsrep_prepare_keys_for_alter_add_fk(char* child_table_db,
                                                Alter_info* alter_info,
                                                wsrep_key_arr_t* ka)
{
  Key *key;
  List_iterator<Key> key_iterator(alter_info->key_list);
  while ((key= key_iterator++))
  {
    if (key->type == Key::FOREIGN_KEY)
    {
      Foreign_key *fk_key= (Foreign_key *)key;
      const char *db_name= fk_key->ref_db.str;
      const char *table_name= fk_key->ref_table.str;
      if (!db_name)
      {
        db_name= child_table_db;
      }
      if (!wsrep_prepare_key_for_isolation(db_name, table_name, ka))
      {
        return false;
      }
    }
  }
  return true;
}


static bool wsrep_prepare_keys_for_isolation(THD*              thd,
                                             const char*       db,
                                             const char*       table,
                                             const TABLE_LIST* table_list,
                                             Alter_info*       alter_info,
                                             wsrep_key_arr_t*  ka)
{
  ka->keys= 0;
  ka->keys_len= 0;

  if (db || table)
  {
    if (!wsrep_prepare_key_for_isolation(db, table, ka))
      goto err;
  }

  for (const TABLE_LIST* table= table_list; table; table= table->next_global)
  {
    if (!wsrep_prepare_key_for_isolation(table->db, table->table_name, ka))
      goto err;
  }

  if (alter_info && (alter_info->flags & Alter_info::ADD_FOREIGN_KEY))
  {
    if (!wsrep_prepare_keys_for_alter_add_fk(table_list->db, alter_info, ka))
      goto err;
  }

  return false;

err:
  wsrep_keys_free(ka);
  return true;
}


/* Prepare key list from db/table and table_list */
bool wsrep_prepare_keys_for_isolation(THD*              thd,
                                      const char*       db,
                                      const char*       table,
                                      const TABLE_LIST* table_list,
                                      wsrep_key_arr_t*  ka)
{
  return wsrep_prepare_keys_for_isolation(thd, db, table, table_list, NULL, ka);
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

  if (thd->variables.gtid_next.type == GTID_GROUP)
  {
      Gtid_log_event gtid_ev(thd, FALSE, &thd->variables.gtid_next);
      if (!gtid_ev.is_valid()) ret= 0;
      if (!ret && gtid_ev.write(&tmp_io_cache)) ret= 1;
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
  close_cached_file(&tmp_io_cache);
  return ret;
}

#include "sql_show.h"
static int
create_view_query(THD *thd, uchar** buf, size_t* buf_len)
{
    LEX *lex= thd->lex;
    SELECT_LEX *select_lex= &lex->select_lex;
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
        WSREP_WARN("view default definer issue");
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
        (thd->db == NULL || strcmp(views->db, thd->db)))
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
  Rewrite DROP TABLE for TOI. Temporary tables are eliminated from
  the query as they are visible only to client connection.

  TODO: See comments for sql_base.cc:drop_temporary_table() and refine
  the function to deal with transactional locked tables.
 */
static int wsrep_drop_table_query(THD* thd, uchar** buf, size_t* buf_len)
{

  LEX* lex= thd->lex;
  SELECT_LEX* select_lex= &lex->select_lex;
  TABLE_LIST* first_table= select_lex->table_list.first;
  String buff;

  DBUG_ASSERT(!lex->drop_temporary);

  bool found_temp_table= false;
  for (TABLE_LIST* table= first_table; table; table= table->next_global)
  {
    if (find_temporary_table(thd, table->db, table->table_name))
    {
      found_temp_table= true;
      break;
    }
  }

  if (found_temp_table)
  {
    buff.append("DROP TABLE ");
    if (lex->drop_if_exists)
      buff.append("IF EXISTS ");

    for (TABLE_LIST* table= first_table; table; table= table->next_global)
    {
      if (!find_temporary_table(thd, table->db, table->table_name))
      {
        append_identifier(thd, &buff, table->db, strlen(table->db),
                          system_charset_info, thd->charset());
        buff.append(".");
        append_identifier(thd, &buff, table->table_name,
                          strlen(table->table_name),
                          system_charset_info, thd->charset());
        buff.append(",");
      }
    }

    /* Chop the last comma */
    buff.chop();
    buff.append(" /* generated by wsrep */");

    WSREP_DEBUG("Rewrote '%s' as '%s'", thd->query(), buff.ptr());

    return wsrep_to_buf_helper(thd, buff.ptr(), buff.length(), buf, buf_len);
  }
  else
  {
    return wsrep_to_buf_helper(thd, thd->query(), thd->query_length(),
                               buf, buf_len);
  }
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
                                 const TABLE_LIST *table_list)
{
  DBUG_ASSERT(!table || db);
  DBUG_ASSERT(table_list || db);

  /* Only if binlog is enabled and user try to set sql_log_bin=0. */
  if (mysql_bin_log.is_open() && !(thd->variables.option_bits & OPTION_BIN_LOG))
    return false;

  LEX* lex= thd->lex;
  SELECT_LEX* select_lex= &lex->select_lex;
  TABLE_LIST* first_table= select_lex->table_list.first;

  switch (lex->sql_command)
  {
  case SQLCOM_CREATE_TABLE:
    DBUG_ASSERT(!table_list);
    if (thd->lex->create_info.options & HA_LEX_CREATE_TMP_TABLE)
    {
      return false;
    }
    return true;

  case SQLCOM_CREATE_VIEW:

    DBUG_ASSERT(!table_list);
    DBUG_ASSERT(first_table); /* First table is view name */
    /*
      If any of the remaining tables refer to temporary table error
      is returned to client, so TOI can be skipped
    */
    for (TABLE_LIST* it= first_table->next_global; it; it= it->next_global)
    {
      if (find_temporary_table(thd, it))
      {
        return false;
      }
    }
    return true;

  case SQLCOM_CREATE_TRIGGER:

#if 0
    /* Trigger statement is invoked with table_list with length = 1 */
    DBUG_ASSERT(!table_list);
#endif
    DBUG_ASSERT(first_table);

    if (find_temporary_table(thd, first_table))
    {
      return false;
    }
    return true;

  default:
    if (table && !find_temporary_table(thd, db, table))
    {
      return true;
    }

    if (table_list && first_table)
    {
      for (TABLE_LIST* table= first_table; table; table= table->next_global)
      {
        if (!find_temporary_table(thd, table->db, table->table_name))
        {
          return true;
        }
      }
    }
    return !(table || (table_list && first_table));
  }
}

/*
  returns: 
   0: statement was replicated as TOI
   1: TOI replication was skipped
  -1: TOI replication failed 
 */
static int wsrep_TOI_begin(THD *thd, char *db_, char *table_,
                           const TABLE_LIST* table_list,
                           Alter_info* alter_info)
{
  wsrep_status_t ret(WSREP_WARNING);
  uchar* buf(0);
  size_t buf_len(0);
  int buf_err;

  thd->wsrep_skip_wsrep_hton= true;
  if (wsrep_can_run_in_toi(thd, db_, table_, table_list) == false)
  {
    WSREP_DEBUG("No TOI for %s", WSREP_QUERY(thd));
    return 1;
  }

  WSREP_DEBUG("TO BEGIN: %lld, %d : %s", (long long)wsrep_thd_trx_seqno(thd),
              thd->wsrep_exec_mode, WSREP_QUERY(thd));
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
  case SQLCOM_DROP_TABLE:
    buf_err= wsrep_drop_table_query(thd, &buf, &buf_len);
    break;
  default:
    buf_err= wsrep_to_buf_helper(thd, thd->query(), thd->query_length(),
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
  if (WSREP(thd))
      thd_proc_info(thd, "Preparing for TO isolation");
  if (!buf_err                                                                  &&
      !wsrep_prepare_keys_for_isolation(thd, db_, table_,
                                        table_list, alter_info, &key_arr)       &&
      key_arr.keys_len > 0                                                      &&
      WSREP_OK == (ret = wsrep->to_execute_start(wsrep, thd->thread_id,
						 key_arr.keys, key_arr.keys_len,
						 &buff, 1,
						 &thd->wsrep_trx_meta)))
  {
    thd->wsrep_exec_mode= TOTAL_ORDER;
    wsrep_to_isolation++;
    if (buf) my_free(buf);
    wsrep_keys_free(&key_arr);
    WSREP_DEBUG("TO BEGIN: %lld, %d",(long long)wsrep_thd_trx_seqno(thd),
                thd->wsrep_exec_mode);
  }
  else if (key_arr.keys_len > 0) {
    /* jump to error handler in mysql_execute_command() */
    WSREP_WARN("TO isolation failed for: %d, schema: %s, sql: %s. Check wsrep "
               "connection state and retry the query.",
               ret, (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
    my_error(ER_LOCK_DEADLOCK, MYF(0), "WSREP replication failed. Check "
             "your wsrep connection state and retry the query.");
    if (buf) my_free(buf);
    wsrep_keys_free(&key_arr);
    wsrep_cleanup_transaction(thd);
    return -1;
  }
  else {
    /* non replicated DDL, affecting temporary tables only */
    WSREP_DEBUG("TO isolation skipped for: %d, sql: %s."
                "Only temporary tables affected.",
                ret, WSREP_QUERY(thd));
    if (buf) my_free(buf);
    wsrep_keys_free(&key_arr);
    return 1;
  }
  return 0;
}

static void wsrep_TOI_end(THD *thd) {
  wsrep_status_t ret;
  wsrep_to_isolation--;

  WSREP_DEBUG("TO END: %lld, %d : %s", (long long)wsrep_thd_trx_seqno(thd),
              thd->wsrep_exec_mode, WSREP_QUERY(thd));

  wsrep_set_SE_checkpoint(thd->wsrep_trx_meta.gtid.uuid,
                          thd->wsrep_trx_meta.gtid.seqno);
  WSREP_DEBUG("TO END: %lld, update seqno",
              (long long)wsrep_thd_trx_seqno(thd));
  
  if (WSREP_OK == (ret = wsrep->to_execute_end(wsrep, thd->thread_id))) {
    WSREP_DEBUG("TO END: %lld", (long long)wsrep_thd_trx_seqno(thd));
  }
  else {
    WSREP_WARN("TO isolation end failed for: %d, schema: %s, sql: %s",
               ret, (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
  }
}

static int wsrep_RSU_begin(THD *thd, char *db_, char *table_)
{
  wsrep_status_t ret(WSREP_WARNING);
  wsrep_seqno_t seqno= WSREP_SEQNO_UNDEFINED;

  WSREP_DEBUG("RSU BEGIN: %lld, %d : %s", (long long)wsrep_thd_trx_seqno(thd),
              thd->wsrep_exec_mode, WSREP_QUERY(thd));

  ret = wsrep->desync(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("RSU desync failed %d for schema: %s, query: %s",
               ret, (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
    my_error(ER_LOCK_DEADLOCK, MYF(0));
    return(-1);
  }

  mysql_mutex_lock(&LOCK_wsrep_replaying);
  wsrep_replaying++;
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  if (wsrep_wait_committing_connections_close(wsrep_RSU_commit_timeout))
  {
    /* no can do, bail out from DDL */
    WSREP_WARN("RSU failed due to pending transactions, schema: %s, query %s",
               (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
    mysql_mutex_lock(&LOCK_wsrep_replaying);
    wsrep_replaying--;
    mysql_mutex_unlock(&LOCK_wsrep_replaying);

    ret = wsrep->resync(wsrep);
    if (ret != WSREP_OK)
    {
      WSREP_WARN("resync failed %d for schema: %s, query: %s",
                 ret, (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
    }

    my_error(ER_LOCK_DEADLOCK, MYF(0));
    return(-1);
  }

  seqno = wsrep->pause(wsrep);
  if (seqno == WSREP_SEQNO_UNDEFINED)
  {
    WSREP_WARN("pause failed %lld for schema: %s, query: %s", (long long)seqno,
               (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));

    /* Pause fail so rollback desync action too. */
    wsrep->resync(wsrep);

    return(-1);
  }
  thd->global_read_lock.pause_provider(true);
  WSREP_DEBUG("paused at %lld", (long long)seqno);

  DEBUG_SYNC(thd,"wsrep_RSU_begin_acquired");
  return 0;
}

static void wsrep_RSU_end(THD *thd)
{
  wsrep_status_t ret(WSREP_OK);

  WSREP_DEBUG("RSU END: %lld, %d : %s", (long long)wsrep_thd_trx_seqno(thd),
               thd->wsrep_exec_mode, WSREP_QUERY(thd));

  mysql_mutex_lock(&LOCK_wsrep_replaying);
  wsrep_replaying--;
  mysql_mutex_unlock(&LOCK_wsrep_replaying);

  ret = wsrep->resume(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("resume failed %d for schema: %s, query: %s", ret,
               (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
  }
  else
  {
    thd->global_read_lock.pause_provider(false);
  }

  ret = wsrep->resync(wsrep);
  if (ret != WSREP_OK)
  {
    WSREP_WARN("resync failed %d for schema: %s, query: %s", ret,
               (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
  }
}

int wsrep_to_isolation_begin(THD *thd, char *db_, char *table_,
                             const TABLE_LIST* table_list,
                             Alter_info* alter_info)
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
    WSREP_INFO("thread: %lu, schema: %s, query: %s has been aborted due to multi-master conflict",
               thd->thread_id, (thd->db ? thd->db : "(null)"), WSREP_QUERY(thd));
    mysql_mutex_unlock(&thd->LOCK_wsrep_thd);
    return WSREP_TRX_FAIL;
  }
  mysql_mutex_unlock(&thd->LOCK_wsrep_thd);

  DBUG_ASSERT(thd->wsrep_exec_mode == LOCAL_STATE);
  DBUG_ASSERT(thd->wsrep_trx_meta.gtid.seqno == WSREP_SEQNO_UNDEFINED);

  if (thd->global_read_lock.can_acquire_protection())
  {
    WSREP_DEBUG("Aborting TOI: Global Read-Lock (FTWRL) in place: %s %lu",
                WSREP_QUERY(thd), thd->thread_id);
    return -1;
  }

  if (!thd->global_read_lock.provider_resumed())
  {
    WSREP_DEBUG("Aborting TOI: Galera provider paused due to lock: %s %lu",
                thd->query(), thd->thread_id);
    my_error(ER_CANT_UPDATE_WITH_READLOCK, MYF(0));
    return -1;
  }

  if (wsrep_debug && thd->mdl_context.has_locks())
  {
    WSREP_DEBUG("thread holds MDL locks at TI begin: %s %lu",
                WSREP_QUERY(thd), thd->thread_id);
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
    case WSREP_OSU_TOI:
      ret= wsrep_TOI_begin(thd, db_, table_, table_list, alter_info);
      DEBUG_SYNC(thd, "wsrep_after_toi_begin");
      break;
    case WSREP_OSU_RSU:
      ret= wsrep_RSU_begin(thd, db_, table_);
      break;
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

  if (thd->wsrep_skip_wsrep_hton)
    thd->wsrep_skip_wsrep_hton= false;
}

#define WSREP_MDL_LOG(severity, msg, schema, schema_len, req, gra)             \
    WSREP_##severity(                                                          \
      "%s\n"                                                                   \
      "schema:  %.*s\n"                                                        \
      "request: (%lu \tseqno %lld \twsrep (%d, %d, %d) cmd %d %d \t%s)\n"      \
      "granted: (%lu \tseqno %lld \twsrep (%d, %d, %d) cmd %d %d \t%s)",       \
      msg, schema_len, schema,                                                 \
      req->thread_id, (long long)wsrep_thd_trx_seqno(req),                     \
      req->wsrep_exec_mode, req->wsrep_query_state, req->wsrep_conflict_state, \
      req->get_command(), req->lex->sql_command,                               \
      (req->rewritten_query.length() ? req->rewritten_query.c_ptr_safe() : req->query()), \
      gra->thread_id, (long long)wsrep_thd_trx_seqno(gra),                     \
      gra->wsrep_exec_mode, gra->wsrep_query_state, gra->wsrep_conflict_state, \
      gra->get_command(), gra->lex->sql_command,                               \
      (gra->rewritten_query.length() ? gra->rewritten_query.c_ptr_safe() : gra->query()));

bool
wsrep_grant_mdl_exception(MDL_context *requestor_ctx,
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
    WSREP_MDL_LOG(DEBUG, "MDL conflict ", schema, schema_len,
                  request_thd, granted_thd);
    ticket->wsrep_report(wsrep_debug);

    mysql_mutex_lock(&granted_thd->LOCK_wsrep_thd);
    if (granted_thd->wsrep_exec_mode == TOTAL_ORDER ||
        granted_thd->wsrep_exec_mode == REPL_RECV)
    {
      WSREP_MDL_LOG(INFO, "MDL BF-BF conflict", schema, schema_len,
                    request_thd, granted_thd);
      ticket->wsrep_report(true);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = TRUE;
    }
    else if (granted_thd->lex->sql_command == SQLCOM_FLUSH ||
             granted_thd->mdl_context.wsrep_has_explicit_locks())
    {
      WSREP_DEBUG("BF thread waiting for FLUSH");
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      ret = FALSE;
    }
    else if (request_thd->lex->sql_command == SQLCOM_DROP_TABLE)
    {
      WSREP_DEBUG("DROP caused BF abort, conf %d", granted_thd->wsrep_conflict_state);
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd((void*)request_thd, (void*)granted_thd, 1);
      ret = FALSE;
    }
    else if (granted_thd->wsrep_query_state == QUERY_COMMITTING)
    {
      WSREP_DEBUG("mdl granted, but commiting thd abort scheduled");
      ticket->wsrep_report(wsrep_debug);
      mysql_mutex_unlock(&granted_thd->LOCK_wsrep_thd);
      wsrep_abort_thd((void*)request_thd, (void*)granted_thd, 1);
      ret = FALSE;
    }
    else
    {
      WSREP_MDL_LOG(DEBUG, "MDL conflict-> BF abort", schema, schema_len,
                    request_thd, granted_thd);
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
