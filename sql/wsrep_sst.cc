/* Copyright 2008-2011 Codership Oy <http://www.codership.com>

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
#include <set_var.h>
#include <sql_acl.h>
#include <sql_reload.h>
#include <sql_parse.h>
#include "wsrep_priv.h"
#include <cstdio>
#include <cstdlib>

#define WSREP_SST_MYSQLDUMP "mysqldump"
#define WSREP_SST_DEFAULT WSREP_SST_MYSQLDUMP
const char* wsrep_sst_method          = WSREP_SST_DEFAULT;
#define WSREP_SST_ADDRESS_AUTO "AUTO"
const char* wsrep_sst_receive_address = WSREP_SST_ADDRESS_AUTO;
const char* wsrep_sst_donor           = "";

static char  sst_auth[16] = { 0, };
const  char* wsrep_sst_auth = sst_auth;

// to show when wsrep_sst_auth is set
static const char* const sst_auth_set = "********";
// container for real auth string
static const char* sst_auth_real = 0;

bool wsrep_sst_auth_check (sys_var *self, THD* thd, set_var* var)
{
    if (!(thd->security_ctx->master_access & SUPER_ACL)) {
        my_error(ER_SPECIFIC_ACCESS_DENIED_ERROR, MYF(0), "SUPER");
        return 1;
    }
    return 0;
}

static bool sst_auth_real_set (const char* value)
{
    const char* v = strdup (value);

    if (v)
    {
        if (sst_auth_real) free (const_cast<char*>(sst_auth_real));
        sst_auth_real = v;

        memset (sst_auth, 0, sizeof(sst_auth));

        if (strlen(sst_auth_real))
            strncpy (sst_auth, sst_auth_set, sizeof(sst_auth) - 1);

        return 0;
    }

    return 1;
}

bool wsrep_sst_auth_update (sys_var *self, THD* thd, enum_var_type type)
{
    const char *latin= "latin1";
    LEX_STRING charset; 
    charset.str= (char *)latin;
    charset.length= strlen(latin);

    const char* value = (const char*)self->value_ptr(thd, type, &charset);

    return sst_auth_real_set (value);
}

void wsrep_sst_auth_default (THD* thd, enum_var_type var_type)
{
    sst_auth_real_set ("root:");
}

void wsrep_sst_auth_init (const char* value)
{
    if (value) sst_auth_real_set (value);
}

static wsrep_uuid_t cluster_uuid = WSREP_UUID_UNDEFINED;

bool wsrep_init_first()
{
  return strcmp (wsrep_sst_method, WSREP_SST_MYSQLDUMP);
}

static pthread_mutex_t sst_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sst_cond = PTHREAD_COND_INITIALIZER;
static bool            sst_complete = false;
static bool            sst_needed   = false;

void wsrep_sst_grab ()
{
    WSREP_INFO("wsrep_sst_grab()");
  if (pthread_mutex_lock (&sst_lock)) abort();
  sst_complete = false;
  pthread_mutex_unlock (&sst_lock);
}

// Wait for end of SST
bool wsrep_sst_wait ()
{
  if (pthread_mutex_lock (&sst_lock)) abort();
  while (!sst_complete)
  {
    WSREP_INFO("Waiting for SST to complete.");
    pthread_cond_wait (&sst_cond, &sst_lock);
  }

  if (local_seqno >= 0)
  {
    WSREP_INFO("SST complete, seqno: %lld", (long long) local_seqno);
  }
  else
  {
    WSREP_ERROR("SST failed: %d (%s)",
                int(-local_seqno), strerror(-local_seqno));
  }

  pthread_mutex_unlock (&sst_lock);

  return (local_seqno >= 0);
}

// Signal end of SST
void wsrep_sst_complete (wsrep_uuid_t* sst_uuid,
                         wsrep_seqno_t sst_seqno,
                         bool          needed)
{
  if (pthread_mutex_lock (&sst_lock)) abort();
  if (!sst_complete)
  {
    sst_complete = true;
    sst_needed   = needed;
    local_uuid   = *sst_uuid;
    local_seqno  = sst_seqno;
    pthread_cond_signal (&sst_cond);
  }
  else
  {
    WSREP_WARN("Nobody is waiting for SST.");
  }
  pthread_mutex_unlock (&sst_lock);
}

// Let applier threads to continue
void wsrep_sst_continue ()
{
  if (sst_needed)
  {
    WSREP_INFO("Signalling provider to continue.");
    wsrep->sst_received (wsrep, &local_uuid, local_seqno, NULL, 0);
  }
}

struct sst_thread_arg
{
  const char*     cmd;
  int             err;
  char*           ret_str;
  pthread_mutex_t lock;
  pthread_cond_t  cond;

  sst_thread_arg (const char* c) : cmd(c), err(-1), ret_str(0)
  {
    pthread_mutex_init (&lock, NULL);
    pthread_cond_init  (&cond, NULL);
  }

  ~sst_thread_arg()
  {
    pthread_cond_destroy  (&cond);
    pthread_mutex_unlock  (&lock);
    pthread_mutex_destroy (&lock);
  }
};

static int sst_scan_uuid_seqno (const char* str,
                                wsrep_uuid_t* uuid, wsrep_seqno_t* seqno)
{
  int offt = wsrep_uuid_scan (str, strlen(str), uuid);
  if (offt > 0 && strlen(str) > (unsigned int)offt && ':' == str[offt])
  {
    *seqno = strtoll (str + offt + 1, NULL, 10);
    if (*seqno != LLONG_MAX || errno != ERANGE)
    {
      return 0;
    }
  }

  WSREP_ERROR("Failed to parse uuid:seqno pair: '%s'", str);
  return EINVAL;
}

// get rid of trailing \n
static char* my_fgets (char* buf, size_t buf_len, FILE* stream)
{
   char* ret= fgets (buf, buf_len, stream);

   if (ret)
   {
       size_t len = strlen(ret);
       if (len > 0 && ret[len - 1] == '\n') ret[len - 1] = '\0';
   }

   return ret;
}

static void* sst_joiner_thread (void* a)
{
  sst_thread_arg* arg= (sst_thread_arg*) a;
  int err= 1;

  {
    const char magic[] = "ready";
    const size_t magic_len = sizeof(magic) - 1;
    const size_t out_len = 512;
    char out[out_len];

    WSREP_INFO("Running: '%s'", arg->cmd);

    wsp::process proc (arg->cmd, "r");

    if (proc.pipe() && !proc.error())
    {
      const char* tmp= my_fgets (out, out_len, proc.pipe());

      if (!tmp || strlen(tmp) < (magic_len + 2) ||
          strncasecmp (tmp, magic, magic_len))
      {
        WSREP_ERROR("Failed to read '%s <addr>' from: %s\n\tRead: '%s'",
                    magic, arg->cmd, tmp);
        proc.wait();
        if (proc.error()) err = proc.error();
      }
      else
      {
        err = 0;
      }
    }
    else
    {
      err = proc.error();
      WSREP_ERROR("Failed to execute: %s : %d (%s)",
                  arg->cmd, err, strerror(err));
    }

    // signal sst_prepare thread with ret code,
    // it will go on sending SST request
    pthread_mutex_lock   (&arg->lock);
    if (!err)
    {
      arg->ret_str = strdup (out + magic_len + 1);
      if (!arg->ret_str) err = ENOMEM;
    }
    arg->err = -err;
    pthread_cond_signal  (&arg->cond);
    pthread_mutex_unlock (&arg->lock); //! @note arg is unusable after that.

    if (err) return NULL; /* lp:808417 - return immediately, don't signal
                           * initializer thread to ensure single thread of
                           * shutdown. */

    wsrep_uuid_t  ret_uuid  = WSREP_UUID_UNDEFINED;
    wsrep_seqno_t ret_seqno = WSREP_SEQNO_UNDEFINED;

    // in case of successfull receiver start, wait for SST completion/end
    char* tmp = my_fgets (out, out_len, proc.pipe());

    proc.wait();
    err= EINVAL;

    if (!tmp)
    {
      WSREP_ERROR("Failed to read uuid:seqno from joiner script.");
      if (proc.error()) err = proc.error();
    }
    else
    {
      err= sst_scan_uuid_seqno (out, &ret_uuid, &ret_seqno);
    }

    if (err)
    {
      ret_uuid=  WSREP_UUID_UNDEFINED;
      ret_seqno= -err;
    }

    // Tell initializer thread that SST is complete
    wsrep_sst_complete (&ret_uuid, ret_seqno, true);
  }

  return NULL;
}

static ssize_t sst_prepare_other (const char*  method,
                                  const char*  addr_in,
                                  const char** addr_out)
{
  ssize_t cmd_len= 1024;
  char    cmd_str[cmd_len];
  const char* sst_dir;

  if (wsrep_data_home_dir && strlen(wsrep_data_home_dir))
    sst_dir= wsrep_data_home_dir;
  else
    sst_dir= mysql_real_data_home;

  int ret= snprintf (cmd_str, cmd_len,
                     "wsrep_sst_%s 'joiner' '%s' '%s' '%s' '%d' 2>sst.err",
                     method, addr_in, sst_auth_real, sst_dir, (int)getpid());

  if (ret < 0 || ret >= cmd_len)
  {
    WSREP_ERROR("sst_prepare_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  pthread_t tmp;
  sst_thread_arg arg(cmd_str);
  pthread_mutex_lock (&arg.lock);
  pthread_create (&tmp, NULL, sst_joiner_thread, &arg);
  pthread_cond_wait (&arg.cond, &arg.lock);

  *addr_out= arg.ret_str;

  if (!arg.err)
    ret = strlen(*addr_out);
  else
  {
    assert (arg.err < 0);
    ret = arg.err;
  }

  pthread_detach (tmp);

  return ret;
}

//extern ulong my_bind_addr;
extern uint  mysqld_port;

/*! Just tells donor where ti sent mysqldump */
static ssize_t sst_prepare_mysqldump (const char*  addr_in,
                                      const char** addr_out)
{
  ssize_t ret = strlen (addr_in);

  if (!strrchr(addr_in, ':'))
  {
    ssize_t s = ret + 7;
    char* tmp = (char*) malloc (s);

    if (tmp)
    {
      ret= snprintf (tmp, s, "%s:%u", addr_in, mysqld_port);

      if (ret > 0 && ret < s)
      {
        *addr_out= tmp;
        return ret;
      }
      if (ret > 0) /* buffer too short */ ret = -EMSGSIZE;
      free (tmp);
    }
    else {
      ret= -ENOMEM;
    }

    sql_print_error ("WSREP: Could not prepare state transfer request: "
                     "adding default port failed: %zd.", ret);
  }
  else {
    *addr_out= addr_in;
  }

  return ret;
}

ssize_t wsrep_sst_prepare (void** msg)
{
  const ssize_t ip_max= 256;
  char ip_buf[ip_max];
  const char* addr_in=  NULL;
  const char* addr_out= NULL;

  // Figure out SST address. Common for all SST methods
  if (wsrep_sst_receive_address &&
    strcmp (wsrep_sst_receive_address, WSREP_SST_ADDRESS_AUTO)) {
    addr_in= wsrep_sst_receive_address;
  }
  else
  {
    ssize_t ret= default_ip (ip_buf, ip_max);
    if (ret && ret < ip_max)
    {
      addr_in= ip_buf;
    }
    else
    {
      WSREP_ERROR("Could not prepare state transfer request: "
                  "unable to determine address to accept state transfer at.");
//      return -EADDRNOTAVAIL;
      unireg_abort(1);
    }
  }

  ssize_t addr_len= -ENOSYS;
  if (!strcmp(wsrep_sst_method, WSREP_SST_MYSQLDUMP))
  {
    addr_len= sst_prepare_mysqldump (addr_in, &addr_out);
    if (addr_len < 0) unireg_abort(1); // return addr_len;
  }
  else
  {
    /*! A heuristic workaround until we learn how to stop and start engines */
    if (sst_complete)
    {
      // we already did SST at initializaiton, now engines are running
      WSREP_ERROR("'%s' SST requires server restart", wsrep_sst_method);
//      return -ERESTART;
      unireg_abort(1);
    }

    addr_len = sst_prepare_other (wsrep_sst_method, addr_in, &addr_out);
    if (addr_len < 0)
    {
      WSREP_ERROR("Failed to prepare for '%s' SST. Unrecoverable.",
                   wsrep_sst_method);
//      return addr_len;
      unireg_abort(1);
    }
  }

  size_t method_len = strlen(wsrep_sst_method);
  size_t msg_len    = method_len + addr_len + 2 /* + auth_len + 1*/;

  *msg = malloc (msg_len);
  if (NULL != *msg) {
    char* msg_ptr = (char*)*msg;
    strcpy (msg_ptr, wsrep_sst_method);
    msg_ptr += method_len + 1;
    strcpy (msg_ptr, addr_out);

    sql_print_information ("[DEBUG] WSREP: Prepared SST request: %s|%s",
                               (char*)*msg, msg_ptr);
  }
  else {
    msg_len = -ENOMEM;
    unireg_abort(1);
  }

  if (addr_out != addr_in) /* malloc'ed */ free ((char*)addr_out);

  return msg_len;
}

// helper method for donors
static int sst_run_shell (const char* cmd_str, int max_tries)
{
  int ret = 0;

  for (int tries=1; tries <= max_tries; tries++)
  {
    wsp::process proc (cmd_str, "r");

    if (NULL != proc.pipe())
    {
      proc.wait();
    }

    if ((ret = proc.error()))
    {
      WSREP_ERROR("Try %d/%d: '%s' failed: %d (%s)",
                  tries, max_tries, proc.str, ret, strerror(ret));
      sleep (1);
    }
    else
    {
      WSREP_DEBUG("SST script successfully completed.");
      break;
    }
  }

  return -ret;
}

static int sst_mysqldump_check_addr (const char* user, const char* pswd,
                                     const char* host, const char* port)
{
  return 0;
}

static int sst_donate_mysqldump (const char*         addr,
                                 const wsrep_uuid_t* uuid,
                                 const char*         uuid_str,
                                 wsrep_seqno_t       seqno)
{
  size_t host_len;
  const char* port = strchr (addr, ':');

  if (port)
  {
    port += 1;
    host_len = port - addr;
  }
  else
  {
    port = "";
    host_len = strlen (addr) + 1;
  }

  char host[host_len];

  strncpy (host, addr, host_len - 1);
  host[host_len - 1] = '\0';

  const char* auth = sst_auth_real;
  const char* pswd = strchr (auth, ':');
  size_t user_len;

  if (pswd)
  {
    pswd += 1;
    user_len = pswd - auth;
  }
  else
  {
    pswd = "";
    user_len = strlen (auth) + 1;
  }

  char user[user_len];

  strncpy (user, auth, user_len - 1);
  user[user_len - 1] = '\0';

  int ret = sst_mysqldump_check_addr (user, pswd, host, port);
  if (!ret)
  {
    size_t cmd_len= 1024;
    char   cmd_str[cmd_len];

    snprintf (cmd_str, cmd_len,
              "wsrep_sst_mysqldump '%s' '%s' '%s' '%s' '%u' '%s' '%lld'",
              user, pswd, host, port, mysqld_port, uuid_str, (long long)seqno);

    WSREP_DEBUG("Running: '%s'", cmd_str);

    ret= sst_run_shell (cmd_str, 3);
  }

  wsrep->sst_sent (wsrep, uuid, ret ? ret : seqno);

  return ret;
}

wsrep_seqno_t wsrep_locked_seqno= WSREP_SEQNO_UNDEFINED;

static int sst_flush_tables(THD* thd)
{
  WSREP_INFO("Flushing tables for SST...");

  int not_used;
  
  int  err= reload_acl_and_cache(thd, REFRESH_TABLES | REFRESH_READ_LOCK,
                                 (TABLE_LIST*) 0, &not_used);
//  Other possible flags: REFRESH_LOG, REFRESH_THREADS, REFRESH_HOSTS,
//                        REFRESH_FAST, REFRESH_STATUS, REFRESH_SLAVE,
//                        REFRESH_MASTER, REFRESH_QUERY_CACHE_FREE,
//                        REFRESH_DES_KEY_FILE

  if (err)
  {
    WSREP_ERROR("Failed to flush and lock tables: %d (%s)", err,strerror(err));
  }
  else
  {
    /* make sure logs are flushed after global read lock acquired */
    err= reload_acl_and_cache(thd, REFRESH_LOG, (TABLE_LIST*) 0, &not_used);
  }

  if (err)
  {
    WSREP_ERROR("Failed to flush redo logs: %d (%s)", err, strerror(err));
  }
  else
  {
    WSREP_INFO("Tables flushed.");
    const char base_name[]= "tables_flushed";
    ssize_t const full_len= strlen(mysql_real_data_home) + strlen(base_name)+2;
    char real_name[full_len];
    sprintf(real_name, "%s/%s", mysql_real_data_home, base_name);
    char tmp_name[full_len + 4];
    sprintf(tmp_name, "%s.tmp", real_name);

    FILE* file= fopen(tmp_name, "w+");
    if (0 == file)
    {
      err= errno;
      WSREP_ERROR("Failed to open '%s': %d (%s)", tmp_name, err,strerror(err));
    }
    else
    {
      fprintf(file, "%s:%lld\n",
              wsrep_cluster_state_uuid, (long long)wsrep_locked_seqno);
      fsync(fileno(file));
      fclose(file);
      if (rename(tmp_name, real_name) == -1)
      {
        err= errno;
        WSREP_ERROR("Failed to rename '%s' to '%s': %d (%s)",
                     tmp_name, real_name, err,strerror(err));
      }
    }
  }

  return err;
}

static void sst_disallow_writes (THD* thd, bool yes)
{
  char query_str[64] = { 0, };
  ssize_t const query_max = sizeof(query_str) - 1;
  snprintf (query_str, query_max, "SET GLOBAL innodb_disallow_writes=%d",
            yes ? 1 : 0);

  thd->set_query(query_str, strlen(query_str));

  Parser_state ps;
  mysql_parse(thd, thd->query(), thd->query_length(), &ps);
  if (thd->is_error())
  {
    int const err= thd->stmt_da->sql_errno();
    WSREP_WARN ("error executing '%s': %d (%s)%s",
                query_str, err, thd->stmt_da->message(),
                err == ER_UNKNOWN_SYSTEM_VARIABLE ? 
                ". Was mysqld built with --with-innodb-disallow-writes ?" : "");
    thd->clear_error();
  }
}

static void* sst_donor_thread (void* a)
{
  sst_thread_arg* arg= (sst_thread_arg*)a;

  WSREP_INFO("Running: '%s'", arg->cmd);

  int  err= 1;
  bool locked= false;

  const char*  out= NULL;
  const size_t out_len= 128;
  char         out_buf[out_len];

  wsrep_uuid_t  ret_uuid= WSREP_UUID_UNDEFINED;
  wsrep_seqno_t ret_seqno= WSREP_SEQNO_UNDEFINED; // seqno of complete SST

  wsp::thd thd;
  wsp::process proc(arg->cmd, "r");

  err= proc.error();

/* Inform server about SST script startup and release TO isolation */
  pthread_mutex_lock   (&arg->lock);
  arg->err = -err;
  pthread_cond_signal  (&arg->cond);
  pthread_mutex_unlock (&arg->lock); //! @note arg is unusable after that.

  if (proc.pipe() && !err)
  {
wait_signal:
    out= my_fgets (out_buf, out_len, proc.pipe());

    if (out)
    {
      const char magic_flush[]= "flush tables";
      const char magic_cont[]= "continue";
      const char magic_done[]= "done";

      if (!strcasecmp (out, magic_flush))
      {
        err= sst_flush_tables (thd.ptr);
        if (!err)
        {
          sst_disallow_writes (thd.ptr, true);
          locked= true;
          goto wait_signal;
        }
      }
      else if (!strcasecmp (out, magic_cont))
      {
        sst_disallow_writes (thd.ptr, false);
        thd.ptr->global_read_lock.unlock_global_read_lock (thd.ptr);
        locked= false;
        err=  0;
        goto wait_signal;
      }
      else if (!strncasecmp (out, magic_done, strlen(magic_done)))
      {
        err= sst_scan_uuid_seqno (out + strlen(magic_done) + 1,
                                  &ret_uuid, &ret_seqno);
      }
      else
      {
        WSREP_WARN("Received unknown signal: '%s'", out);
      }
    }
    else
    {
      WSREP_ERROR("Failed to read from: %s", arg->cmd);
    }
    if (err && proc.error()) err= proc.error();
  }
  else
  {
    WSREP_ERROR("Failed to execute: %s : %d (%s)",arg->cmd, err, strerror(err));
  }

  if (locked) // don't forget to unlock server before return
  {
    sst_disallow_writes (thd.ptr, false);
    thd.ptr->global_read_lock.unlock_global_read_lock (thd.ptr);
  }

  // signal to donor that SST is over
  wsrep->sst_sent (wsrep, &ret_uuid, err ? -err : ret_seqno);
  proc.wait();

  return NULL;
}

static int sst_donate_other (const char*   method,
                             const char*   addr,
                             const char*   uuid,
                             wsrep_seqno_t seqno)
{
  ssize_t cmd_len = 1024;
  char    cmd_str[cmd_len];

  int ret= snprintf (cmd_str, cmd_len,
                     "wsrep_sst_%s 'donor' '%s' '%s' '%s' '%s' '%lld' 2>sst.err"
                     ,method, addr, sst_auth_real, mysql_real_data_home,
                     uuid, (long long) seqno);
  if (ret < 0 || ret >= cmd_len)
  {
    WSREP_ERROR("sst_donate_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  pthread_t tmp;
  sst_thread_arg arg(cmd_str);
  pthread_mutex_lock (&arg.lock);
  pthread_create (&tmp, NULL, sst_donor_thread, &arg);
  pthread_cond_wait (&arg.cond, &arg.lock);

  WSREP_INFO("sst_donor_thread signaled with %d", arg.err);
  return arg.err;
}

int wsrep_sst_donate_cb (void* app_ctx, void* recv_ctx,
                         const void* msg, size_t msg_len,
                         const wsrep_uuid_t*     current_uuid,
                         wsrep_seqno_t           current_seqno,
                         const char* state, size_t state_len)
{
  /* This will be reset when sync callback is called.
   * Should we set wsrep_ready to FALSE here too? */
//  wsrep_notify_status(WSREP_MEMBER_DONOR);
  local_status.set(WSREP_MEMBER_DONOR);

  const char* method = (char*)msg;
  size_t method_len  = strlen (method);
  const char* data   = method + method_len + 1;

  char uuid_str[37];
  wsrep_uuid_print (current_uuid, uuid_str, sizeof(uuid_str));

  int ret;
  if (!strcmp (WSREP_SST_MYSQLDUMP, method))
  {
    ret = sst_donate_mysqldump (data, current_uuid, uuid_str, current_seqno);
  }
  else
  {
    ret = sst_donate_other (method, data, uuid_str, current_seqno);
  }

  return (ret > 0 ? 0 : ret);
}

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  init_cond = PTHREAD_COND_INITIALIZER;

void wsrep_SE_init_grab()
{
  if (pthread_mutex_lock (&init_lock)) abort();
}

void wsrep_SE_init_wait()
{
  pthread_cond_wait (&init_cond, &init_lock);
  pthread_mutex_unlock (&init_lock);
}

void wsrep_SE_init_done()
{
  pthread_cond_signal (&init_cond);
  pthread_mutex_unlock (&init_lock);
}
