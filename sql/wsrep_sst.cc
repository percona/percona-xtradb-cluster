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
#include <sql_class.h>
#include <sql_parse.h>
#include <sql_reload.h>
#include <mysql_version.h>
#include "debug_sync.h"
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
#include "sql_base.h"           // TEMP_PREFIX
#include "rpl_msr.h"           // channel_map
#include <functional>
#include <sstream>
#include <string>

/* We don't need PSI instrumentation for regex mutex, however
   Regex class has its constructor compiled conditionally.
   Disable instrumentation.
   Regex class uses c++11 nullptr. As we are in c++03 world
   let's define nullptr to satisfy compiler */
#ifdef HAVE_PSI_INTERFACE
#undef HAVE_PSI_INTERFACE
#endif
#define nullptr 0
#include <sql_regex.h>

/* Check if the string is valid. In method name we allow only:
   alpha-num
   underscore
   dash
   dot
   but we do not allow leading and trailing dash/dot.
   Method names have to be separated by colons (spaces before/after colon are allowed).
   Below regex may seem difficult, but in fact it is not:

   1. Any number of leading spaces

     2. Followed by  alpha-num character or underscore
       3. Followed by any number of underscore/dash/dot
       4. Followed by alpha-num character or underscore
       5. 3 - 4 group is optional
     6. Followed by colon (possibly with leading and/or trailing spaces)
     7. 2 - 6 group is optional

   8. Followed by alpha-num character or underscore
     9. Followed by any number of underscore/dash/dot
     10. Followed by alpha-num character or underscore
     11. 9 -10 group is optional

   11. Followed by any number of trailing spaces

   It is the same as:
   ^\s*(\w([-_.]*\w)*(\s*,\s*))*\w([-_.]*\w)*\s*$    */

#define COLON_REGEX "([[:blank:]]*,[[:blank:]]*)"
#define METHOD_REGEX "[[:alnum:]_]([-_.]*[[:alnum:]_])*"
#define SST_ALLOWED_METHODS_REGEX_PATTERN \
    "^[[:blank:]]*(" METHOD_REGEX COLON_REGEX ")*" METHOD_REGEX "[[:blank:]]*$"

static Regex sst_allowed_methods_regex;

/* We allow custom sst scripts, so data can be anything.

   We could create and maintain the list of forbidden
   characters and the ways they could be used to inject the command to
   the OS. However this approach seems to be too error prone.
   Instead of this we will just allow alpha-num + a few special characters
   (colon, slash, dot, underscore, square brackets, hyphen).

   It is the same as ^[\w:/.[\]-]+$    */

   /* For some reason regex engine is not able to handle the following pattern
   allowing square brackets:
   "^[[:alnum:]:/._[\\]-]+$"
   We will substitute square brackets with lt/gt for validation.
   Because of another regex engine problem, hyphen literal has to be located
   at the end of matching set (cannot be escaped when in the middle). */
static const char *sst_method_allowed_chars_regex_pattern=
    "^[[:alnum:]:/._<>-]+$";
static Regex sst_method_allowed_chars_regex;


extern const char wsrep_defaults_file[];
extern const char wsrep_defaults_group_suffix[];

#define WSREP_SST_OPT_ROLE     "--role"
#define WSREP_SST_OPT_ADDR     "--address"
#define WSREP_SST_OPT_AUTH     "--auth"
#define WSREP_SST_OPT_DATA     "--datadir"
#define WSREP_SST_OPT_CONF     "--defaults-file"
#define WSREP_SST_OPT_CONF_SUFFIX "--defaults-group-suffix"
#define WSREP_SST_OPT_PARENT   "--parent"
#define WSREP_SST_OPT_BINLOG   "--binlog"
#define WSREP_SST_OPT_VERSION  "--mysqld-version"

// mysqldump-specific options
#define WSREP_SST_OPT_USER     "--user"
#define WSREP_SST_OPT_PSWD     "--password"
#define WSREP_SST_OPT_HOST     "--host"
#define WSREP_SST_OPT_PORT     "--port"
#define WSREP_SST_OPT_LPORT    "--local-port"

// donor-specific
#define WSREP_SST_OPT_SOCKET   "--socket"
#define WSREP_SST_OPT_GTID     "--gtid"
#define WSREP_SST_OPT_BYPASS   "--bypass"

#define WSREP_SST_MYSQLDUMP       "mysqldump"
#define WSREP_SST_RSYNC           "rsync"
#define WSREP_SST_SKIP            "skip"
#define WSREP_SST_XTRABACKUP      "xtrabackup"
#define WSREP_SST_XTRABACKUP_V2   "xtrabackup-v2"
#define WSREP_SST_DEFAULT      WSREP_SST_XTRABACKUP_V2
#define WSREP_SST_ADDRESS_AUTO "AUTO"
#define WSREP_SST_AUTH_MASK    "********"

const char* wsrep_sst_method          = WSREP_SST_DEFAULT;
const char* wsrep_sst_receive_address = WSREP_SST_ADDRESS_AUTO;
const char* wsrep_sst_donor           = "";
      char* wsrep_sst_auth            = NULL;

#define WSREP_SST_ALLOWED_METHODS_DEFAULT WSREP_SST_MYSQLDUMP "," \
                                          WSREP_SST_RSYNC "," \
                                          WSREP_SST_SKIP "," \
                                          WSREP_SST_XTRABACKUP "," \
                                          WSREP_SST_XTRABACKUP_V2

const char *wsrep_sst_allowed_methods = WSREP_SST_ALLOWED_METHODS_DEFAULT;
static std::vector<std::string> allowed_sst_methods;

// container for real auth string
static const char* sst_auth_real      = NULL;
my_bool wsrep_sst_donor_rejects_queries = FALSE;

/* Function checks if the new value for sst_method is valid.
@return false if no error encountered with check else return true. */
bool wsrep_sst_method_check (sys_var *self, THD* thd, set_var* var)
{
  if ((! var->save_result.string_value.str) ||
      (var->save_result.string_value.length == 0 ))
  {
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
             var->save_result.string_value.str ?
             var->save_result.string_value.str : "NULL");
    return true;
  }

  if (!strcmp (var->save_result.string_value.str, WSREP_SST_MYSQLDUMP))
  {
    WSREP_WARN("Percona-XtraDB-Cluster has deprecated SST through mysqldump."
               " Percona-XtraDB-Cluster recommends using xtrabackup."
               " Please switch to use xtrabackup or rsync.");
    push_warning_printf(
      thd, Sql_condition::SL_WARNING, ER_UNKNOWN_ERROR,
      "Percona-XtraDB-Cluster has deprecated SST through mysqldump."
      " Percona-XtraDB-Cluster recommends using xtrabackup."
      " Please switch to use xtrabackup or rsync.");
  }

  return false;
}

bool wsrep_sst_method_update (sys_var *self, THD* thd, enum_var_type type)
{
    return 0;
}

#if 0
static bool sst_receive_address_check (const char* str)
{
    if (!strncasecmp(str, "127.0.0.1", strlen("127.0.0.1")) ||
        !strncasecmp(str, "localhost", strlen("localhost")))
    {
        return 1;
    }

    return 0;
}
#endif

/* Function checks if the new value for sst_recieve_address is valid.
@return false if no error encountered with check else return true. */
bool wsrep_sst_receive_address_check (sys_var *self, THD* thd, set_var* var)
{
  char addr_buf[FN_REFLEN];

  if ((! var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1))) // safety
  {
    goto err;
  }

  memcpy(addr_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  addr_buf[var->save_result.string_value.length]= 0;

#if 0
  if (sst_receive_address_check(addr_buf))
  {
    goto err;
  }
#endif

  return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name.str,
           var->save_result.string_value.str ?
           var->save_result.string_value.str : "NULL");
  return true;
}

bool wsrep_sst_receive_address_update (sys_var *self, THD* thd,
                                       enum_var_type type)
{
    return 0;
}

bool wsrep_sst_auth_check (sys_var *self, THD* thd, set_var* var)
{
    return 0;
}

void wsrep_sst_auth_free()
{
  if (wsrep_sst_auth) { my_free((void *) wsrep_sst_auth); }
  if (sst_auth_real) { my_free((void *) sst_auth_real); }
  wsrep_sst_auth= NULL;
  sst_auth_real= NULL;
}

static bool sst_auth_real_set (const char* value)
{
  const char* v= NULL;

  if (value)
  {
    v= my_strdup(key_memory_wsrep, value, MYF(0));
  }
  else                                          // its NULL
  {
    wsrep_sst_auth_free();
    return 0;
  }

  if (v)
  {
    // set sst_auth_real
    if (sst_auth_real) { my_free((void *) sst_auth_real); }
    sst_auth_real = v;

    // mask wsrep_sst_auth
    if (strlen(sst_auth_real))
    {
      if (wsrep_sst_auth) { my_free((void*) wsrep_sst_auth); }
      wsrep_sst_auth= my_strdup(key_memory_wsrep, WSREP_SST_AUTH_MASK, MYF(0));
    }
    return 0;
  }
  return 1;
}

bool wsrep_sst_auth_update (sys_var *self, THD* thd, enum_var_type type)
{
    return sst_auth_real_set (wsrep_sst_auth);
}

void wsrep_sst_auth_init (const char* value)
{
    if (wsrep_sst_auth == value) wsrep_sst_auth = NULL;
    if (value) sst_auth_real_set (value);
}

bool  wsrep_sst_donor_check (sys_var *self, THD* thd, set_var* var)
{
  return 0;
}

bool wsrep_sst_donor_update (sys_var *self, THD* thd, enum_var_type type)
{
    return 0;
}

bool wsrep_before_SE()
{
  return (wsrep_provider != NULL
          && strcmp (wsrep_provider,   WSREP_NONE)
          && strcmp (wsrep_sst_method, WSREP_SST_SKIP)
          && strcmp (wsrep_sst_method, WSREP_SST_MYSQLDUMP));
}

/* Helper class for splitting string tokens separated with
   commas. Spaces are stripped. */
struct Splitter : public std::unary_function<char, void> {
  Splitter(std::vector<std::string> &vec) : vec_(vec) {}

  void flush()
  {
    if (tmp_.length() > 0)
    {
      vec_.push_back(tmp_);
      tmp_.clear();
    }
  }
  void operator()(char ch)
  {
    if (ch == ',')
    {
      vec_.push_back(tmp_);
      tmp_.clear();
    }
    else
    {
      if (ch != ' ')
      {
        tmp_+= ch;
      }
    }
  }
  std::vector<std::string> &vec_;
  std::string               tmp_;
};

bool wsrep_setup_allowed_sst_methods()
{
  sst_allowed_methods_regex.compile(SST_ALLOWED_METHODS_REGEX_PATTERN,
                                    MY_REG_EXTENDED | MY_REG_NOSUB,
                                    &my_charset_latin1);
  sst_method_allowed_chars_regex.compile(
      sst_method_allowed_chars_regex_pattern, MY_REG_EXTENDED | MY_REG_NOSUB,
      &my_charset_latin1);

  std::string methods(wsrep_sst_allowed_methods);

  if (!sst_allowed_methods_regex.match(methods))
  {
    WSREP_ERROR(
        "Wrong format of --wsrep-sst-allowed-methods parameter value: %s",
        wsrep_sst_allowed_methods);
    return true;
  }

  // split it into tokens and strip
  Splitter splitter(allowed_sst_methods);
  std::for_each(methods.begin(), methods.end(), splitter).flush();

  return false;
}


static bool            sst_complete = false;
static bool            sst_needed   = false;

void wsrep_sst_grab ()
{
  WSREP_INFO("Preparing to initiate SST/IST");
  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  sst_complete = false;
  mysql_mutex_unlock (&LOCK_wsrep_sst);
}

// Wait for end of SST
bool wsrep_sst_wait ()
{
  wsrep_seqno_t seqno;

  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  while (!sst_complete)
  {
    WSREP_INFO("Waiting for SST/IST to complete.");
    mysql_cond_wait (&COND_wsrep_sst, &LOCK_wsrep_sst);
  }

  // Save a local copy of a global variable prior to release
  // of the mutex - to ensure that the return code from this
  // function and a diagnostic message in the log will always
  // be consistent with each other:

  seqno = local_seqno;

  mysql_mutex_unlock (&LOCK_wsrep_sst);

  if (seqno >= 0)
  {
    WSREP_INFO("SST complete, seqno: %lld", (long long) seqno);
  }
  else
  {
    WSREP_ERROR("SST failed: %d (%s)",
                int(-seqno), strerror(-seqno));
  }

  return (seqno >= 0);
}

// Signal end of SST
void wsrep_sst_complete (const wsrep_uuid_t* sst_uuid,
                         wsrep_seqno_t       sst_seqno,
                         bool                needed)
{
  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  if (!sst_complete)
  {
    sst_complete = true;
    sst_needed   = needed;
    local_uuid   = *sst_uuid;
    local_seqno  = sst_seqno;
    mysql_cond_signal (&COND_wsrep_sst);
  }
  else
  {
    /* This can happen when called from wsrep_synced_cb().
       At the moment there is no way to check there
       if main thread is still waiting for signal,
       so wsrep_sst_complete() is called from there
       each time wsrep_ready changes from FALSE -> TRUE.
    */
    WSREP_DEBUG("Nobody is waiting for SST.");
  }
  mysql_mutex_unlock (&LOCK_wsrep_sst);
}

// True if wsrep awaiting sst_received callback:

static bool sst_awaiting_callback= false;

void wsrep_sst_received (wsrep_t*            const wsrep,
                         const wsrep_uuid_t& uuid,
                         wsrep_seqno_t       const seqno,
                         const void*         const state,
                         size_t              const state_len)
{
    wsrep_get_SE_checkpoint(local_uuid, local_seqno);

    if (memcmp(&local_uuid, &uuid, sizeof(wsrep_uuid_t)) ||
        local_seqno < seqno || seqno < 0)
    {
        wsrep_set_SE_checkpoint(uuid, seqno);
        local_uuid = uuid;
        local_seqno = seqno;
    }
    else if (local_seqno > seqno)
    {
        WSREP_WARN("SST postion is in the past: %lld."
                   " Current position: %lld. Can't continue.",
                   (long long)seqno, (long long)local_seqno);
        unireg_abort(1);
    }

    wsrep_init_sidno(uuid);

    if (wsrep)
    {
        int const rcode(seqno < 0 ? seqno : 0);
        wsrep_gtid_t const state_id = {
            uuid, (rcode ? WSREP_SEQNO_UNDEFINED : seqno)
        };
        if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
        sst_awaiting_callback = false;
        wsrep->sst_received(wsrep, &state_id, state, state_len, rcode);
        mysql_mutex_unlock (&LOCK_wsrep_sst);
    }
}

// Let applier threads to continue
void wsrep_sst_continue ()
{
  if (sst_needed)
  {
    WSREP_INFO("Signalling provider to continue on SST completion.");
    // local_uuid and local_seqno are global variables and are volatile
    wsrep_uuid_t  const sst_uuid  = local_uuid;
    wsrep_seqno_t const sst_seqno = local_seqno;
    wsrep_sst_received (wsrep, sst_uuid, sst_seqno, NULL, 0);
  }
}

struct sst_thread_arg
{
  const char*     cmd;
  char**          env;
  char*           ret_str;
  int             err;
  mysql_mutex_t   LOCK_wsrep_sst_thread;
  mysql_cond_t    COND_wsrep_sst_thread;

  sst_thread_arg (const char* c, char** e)
    : cmd(c), env(e), ret_str(0), err(-1)
  {
    mysql_mutex_init(key_LOCK_wsrep_sst_thread,
                     &LOCK_wsrep_sst_thread, MY_MUTEX_INIT_FAST);
    mysql_cond_init(key_COND_wsrep_sst_thread, &COND_wsrep_sst_thread);
  }

  ~sst_thread_arg()
  {
    mysql_cond_destroy  (&COND_wsrep_sst_thread);
    mysql_mutex_unlock  (&LOCK_wsrep_sst_thread);
    mysql_mutex_destroy (&LOCK_wsrep_sst_thread);
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

/*
  Generate opt_binlog_opt_val for sst_donate_other(), sst_prepare_other().

  Returns zero on success, negative error code otherwise.

  String containing binlog name is stored in param ret if binlog is enabled
  and GTID mode is on, otherwise empty string. Returned string should be
  freed with my_free().
 */
static int generate_binlog_opt_val(char** ret)
{
  DBUG_ASSERT(ret);
  *ret= NULL;
  if (opt_bin_log && get_gtid_mode(GTID_MODE_LOCK_NONE) > GTID_MODE_OFF)
  {
    assert(opt_bin_logname);
    *ret= strcmp(opt_bin_logname, "0") ?
        my_strdup(key_memory_wsrep, opt_bin_logname, MYF(0)) :
        my_strdup(key_memory_wsrep, "", MYF(0));
  }
  else
  {
    *ret= my_strdup(key_memory_wsrep, "", MYF(0));
  }
  if (!*ret) return -ENOMEM;
  return 0;
}

// Pointer to SST process object:

static wsp::process * sst_process= static_cast<wsp::process*>(NULL);

// SST cancellation flag:

static bool sst_cancelled= false;

void wsrep_sst_cancel (bool call_wsrep_cb)
{
  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  if (!sst_cancelled)
  {
    sst_cancelled=true;
    /*
      When we launched the SST process, then we need
      to terminate it before exit from the parent (server)
      process:
    */
    if (sst_process)
    {
      sst_process->terminate();
      sst_process = NULL;
    }
    /*
      If this is a normal shutdown, then we need to notify
      the wsrep provider about completion of the SST, to
      prevent infinite waitng in the wsrep provider after
      the SST process was canceled:
    */
    if (call_wsrep_cb && sst_awaiting_callback)
    {
      WSREP_INFO("Signalling cancellation of the SST request.");
      wsrep_gtid_t const state_id = {
          WSREP_UUID_UNDEFINED,
          WSREP_SEQNO_UNDEFINED
      };
      sst_awaiting_callback = false;
      wsrep->sst_received(wsrep, &state_id, NULL, 0, -ECANCELED);
    }
  }
  mysql_mutex_unlock (&LOCK_wsrep_sst);
}

/*
  Handling of fatal signals: SIGABRT, SIGTERM, etc.
*/
extern "C" void wsrep_handle_fatal_signal (int sig)
{
  wsrep_sst_cancel(false);
}

/*
  This callback is invoked in the case of abnormal
  termination of the wsrep provider:
*/
void wsrep_abort_cb (void)
{
  wsrep_sst_cancel(false);
}

static void* sst_joiner_thread (void* a)
{
  sst_thread_arg* arg= (sst_thread_arg*) a;
  int err= 1;

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_register_thread(key_THREAD_wsrep_sst_joiner);
#endif /* HAVE_PSI_INTERFACE */

  {
    const char magic[] = "ready";
    const size_t magic_len = sizeof(magic) - 1;
    const size_t out_len = 512;
    char out[out_len];

    WSREP_INFO("Initiating SST/IST transfer on JOINER side (%s)", arg->cmd);

    // Launch the SST script and save pointer to its process:

    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    wsp::process proc (arg->cmd, "r", arg->env);
    sst_process = &proc;
    mysql_mutex_unlock (&LOCK_wsrep_sst);

    if (proc.pipe() && !proc.error())
    {
      const char* tmp= my_fgets (out, out_len, proc.pipe());

      if (!tmp || strlen(tmp) < (magic_len + 2) ||
          strncasecmp (tmp, magic, magic_len))
      {
        if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
        // Print error message if SST is not cancelled:
        if (!sst_cancelled)
        {
          // Null-pointer is not valid argument for %s formatting (even
          // though it is supported by many compilers):
           WSREP_ERROR("Failed to read '%s <addr>' from: %s\n\tRead: '%s'",
                       magic, arg->cmd, tmp ? tmp : "(null)");
        }
        // Clear the pointer to SST process:
        sst_process = NULL;
        mysql_mutex_unlock (&LOCK_wsrep_sst);
        proc.wait();
        if (proc.error()) err = proc.error();
      }
      else
      {
        // Do not clear the pointer to SST process. It will be useful if 
        // we decide to interrupt it.
        err = 0;
      }
    }
    else
    {
      // Clear the pointer to SST process:
      if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
      sst_process = NULL;
      mysql_mutex_unlock (&LOCK_wsrep_sst);
      err = proc.error();
      WSREP_ERROR("Failed to execute: %s : %d (%s)",
                  arg->cmd, err, strerror(err));
    }

    // signal sst_prepare thread with ret code,
    // it will go on sending SST request
    mysql_mutex_lock   (&arg->LOCK_wsrep_sst_thread);
    if (!err)
    {
      arg->ret_str = strdup (out + magic_len + 1);
      if (!arg->ret_str) err = ENOMEM;
    }
    arg->err = -err;
    mysql_cond_signal  (&arg->COND_wsrep_sst_thread);
    mysql_mutex_unlock (&arg->LOCK_wsrep_sst_thread); //! @note arg is unusable after that.

    if (err)
    {
      if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
      sst_process = NULL;
      mysql_mutex_unlock (&LOCK_wsrep_sst);

      return NULL; /* lp:808417 - return immediately, don't signal
                    * initializer thread to ensure single thread of
                    * shutdown. */
    }
    wsrep_uuid_t  ret_uuid  = WSREP_UUID_UNDEFINED;
    wsrep_seqno_t ret_seqno = WSREP_SEQNO_UNDEFINED;

    // in case of successfull receiver start, wait for SST completion/end
    char* tmp = my_fgets (out, out_len, proc.pipe());

    proc.wait();
    err= EINVAL;

    if (!tmp || proc.error())
    {
      WSREP_ERROR("Failed to read uuid:seqno from joiner script.");
      if (proc.error())
      {
        err= proc.error();
        char errbuf[MYSYS_STRERROR_SIZE];
        WSREP_ERROR("SST script aborted with error %d (%s)",
                    err, my_strerror(errbuf, sizeof(errbuf), err));
      }
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

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  sst_process = NULL;
  mysql_mutex_unlock (&LOCK_wsrep_sst);

  return NULL;
}

#define WSREP_SST_AUTH_ENV "WSREP_SST_OPT_AUTH"

static int sst_append_auth_env(wsp::env& env, const char* sst_auth)
{
  int const sst_auth_size= strlen(WSREP_SST_AUTH_ENV) + 1 /* = */
    + (sst_auth ? strlen(sst_auth) : 0) + 1 /* \0 */;

  wsp::string sst_auth_str(sst_auth_size); // for automatic cleanup on return
  if (!sst_auth_str()) return -ENOMEM;

  int ret= snprintf(sst_auth_str(), sst_auth_size, "%s=%s",
                    WSREP_SST_AUTH_ENV, sst_auth ? sst_auth : "");

  if (ret < 0 || ret >= sst_auth_size)
  {
    WSREP_ERROR("sst_append_auth_env(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  env.append(sst_auth_str());
  return -env.error();
}

static ssize_t sst_prepare_other (const char*  method,
                                  const char*  sst_auth,
                                  const char*  addr_in,
                                  const char** addr_out)
{
  int const cmd_len= 4096;
  wsp::string cmd_str(cmd_len);

  if (!cmd_str())
  {
    WSREP_ERROR("sst_prepare_other(): could not allocate cmd buffer of %d bytes",
                cmd_len);
    return -ENOMEM;
  }

  const char* binlog_opt= "";
  char* binlog_opt_val= NULL;

  int ret;
  if ((ret= generate_binlog_opt_val(&binlog_opt_val)))
  {
    WSREP_ERROR("sst_prepare_other(): generate_binlog_opt_val() failed: %d",
                ret);
    return ret;
  }
  if (strlen(binlog_opt_val)) binlog_opt= WSREP_SST_OPT_BINLOG;

  ret= snprintf (cmd_str(), cmd_len,
                 "wsrep_sst_%s "
                 WSREP_SST_OPT_ROLE" 'joiner' "
                 WSREP_SST_OPT_ADDR" '%s' "
                 WSREP_SST_OPT_DATA" '%s' "
                 WSREP_SST_OPT_CONF" '%s' "
                 WSREP_SST_OPT_CONF_SUFFIX" '%s' "
                 WSREP_SST_OPT_PARENT" '%d' "
                 WSREP_SST_OPT_VERSION" '%s' "
                 " %s '%s' ",
                 method, addr_in, mysql_real_data_home,
                 wsrep_defaults_file, wsrep_defaults_group_suffix,
                 (int)getpid(), MYSQL_SERVER_VERSION MYSQL_SERVER_SUFFIX_DEF,
                 binlog_opt, binlog_opt_val);
  my_free(binlog_opt_val);

  if (ret < 0 || ret >= cmd_len)
  {
    WSREP_ERROR("sst_prepare_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  wsp::env env(NULL);
  if (env.error())
  {
    WSREP_ERROR("sst_prepare_other(): env. var ctor failed: %d", -env.error());
    return -env.error();
  }

  if ((ret= sst_append_auth_env(env, sst_auth)))
  {
    WSREP_ERROR("sst_prepare_other(): appending auth failed: %d", ret);
    return ret;
  }

  pthread_t tmp;
  sst_thread_arg arg(cmd_str(), env());
  mysql_mutex_lock (&arg.LOCK_wsrep_sst_thread);
  ret = pthread_create(&tmp, NULL, sst_joiner_thread, &arg);
  if (ret)
  {
    WSREP_ERROR("sst_prepare_other(): pthread_create() failed: %d (%s)",
                ret, strerror(ret));
    return -ret;
  }
  mysql_cond_wait (&arg.COND_wsrep_sst_thread, &arg.LOCK_wsrep_sst_thread);

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

extern uint  mysqld_port;

/*! Just tells donor where to send mysqldump */
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

    WSREP_ERROR ("Could not prepare state transfer request: "
                 "adding default port failed: %zd.", ret);
  }
  else {
    *addr_out= addr_in;
  }

  return ret;
}

static bool SE_initialized = false;

ssize_t wsrep_sst_prepare (void** msg, THD* thd)
{
  const ssize_t ip_max= 256;
  char ip_buf[ip_max];
  const char* addr_in=  NULL;
  const char* addr_out= NULL;

  if (!strcmp(wsrep_sst_method, WSREP_SST_SKIP))
  {
    ssize_t ret = strlen(WSREP_STATE_TRANSFER_TRIVIAL) + 1;
    *msg = strdup(WSREP_STATE_TRANSFER_TRIVIAL);
    if (!msg)
    {
      WSREP_ERROR("Could not allocate %zd bytes for state request", ret);
      unireg_abort(1);
    }
    return ret;
  }

  // Figure out SST address. Common for all SST methods
  if (wsrep_sst_receive_address &&
      strcmp (wsrep_sst_receive_address, WSREP_SST_ADDRESS_AUTO))
  {
    addr_in= wsrep_sst_receive_address;
  }
  else if (wsrep_node_address && strlen(wsrep_node_address))
  {
    size_t const addr_len= strlen(wsrep_node_address);
    size_t const host_len= wsrep_host_len(wsrep_node_address, addr_len);

    if (host_len < addr_len)
    {
      strncpy (ip_buf, wsrep_node_address, host_len);
      ip_buf[host_len]= '\0';
      addr_in= ip_buf;
    }
    else
    {
      addr_in= wsrep_node_address;
    }
  }
  else
  {
    ssize_t ret= wsrep_guess_ip (ip_buf, ip_max);

    if (ret && ret < ip_max)
    {
      addr_in= ip_buf;
    }
    else
    {
      WSREP_ERROR("Could not prepare state transfer request: "
                  "failed to guess address to accept state transfer at. "
                  "wsrep_sst_receive_address must be set manually.");
      if (thd) delete thd;
      unireg_abort(1);
    }
  }

  ssize_t addr_len= -ENOSYS;
  if (!strcmp(wsrep_sst_method, WSREP_SST_MYSQLDUMP))
  {
    addr_len= sst_prepare_mysqldump (addr_in, &addr_out);
    if (addr_len < 0) unireg_abort(1);
  }
  else
  {
    /*! A heuristic workaround until we learn how to stop and start engines */
    if (SE_initialized)
    {
      // we already did SST at initializaiton, now engines are running
      // sql_print_information() is here because the message is too long
      // for WSREP_INFO.
      sql_print_information ("WSREP: "
                 "You have configured '%s' state snapshot transfer method "
                 "which cannot be performed on a running server. "
                 "Wsrep provider won't be able to fall back to it "
                 "if other means of state transfer are unavailable. "
                 "In that case you will need to restart the server.",
                 wsrep_sst_method);
      *msg = 0;
      return 0;
    }

    addr_len = sst_prepare_other (wsrep_sst_method, sst_auth_real,
                                  addr_in, &addr_out);
    if (addr_len < 0)
    {
      WSREP_ERROR("Failed to prepare for '%s' SST. Unrecoverable.",
                   wsrep_sst_method);
      unireg_abort(1);
    }
  }

  size_t const method_len(strlen(wsrep_sst_method));
  size_t const msg_len   (method_len + addr_len + 2 /* + auth_len + 1*/);

  *msg = malloc (msg_len);
  if (NULL != *msg) {
    char* const method_ptr(reinterpret_cast<char*>(*msg));
    strcpy (method_ptr, wsrep_sst_method);
    char* const addr_ptr(method_ptr + method_len + 1);
    strcpy (addr_ptr, addr_out);

    WSREP_INFO ("Prepared SST/IST request: %s|%s", method_ptr, addr_ptr);

    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    sst_awaiting_callback = true;
    mysql_mutex_unlock (&LOCK_wsrep_sst);
  }
  else {
    WSREP_ERROR("Failed to allocate SST request of size %zu. Can't continue.",
                msg_len);
    unireg_abort(1);
  }

  if (addr_out != addr_in) /* malloc'ed */ free ((char*)addr_out);

  return msg_len;
}

// helper method for donors
static int sst_run_shell (const char* cmd_str, char** env, int max_tries)
{
  int ret = 0;

  for (int tries=1; tries <= max_tries; tries++)
  {
    // Launch the SST script and save pointer to its process:

    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    wsp::process proc (cmd_str, "r", env);
    sst_process = &proc;
    mysql_mutex_unlock (&LOCK_wsrep_sst);

    if (proc.pipe() && !proc.error())
    {
      proc.wait();
    }

    // Clear the pointer to SST process:

    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock (&LOCK_wsrep_sst);

    if ((ret = proc.error()))
    {
      // Try again if SST is not cancelled:
      if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
      if (!sst_cancelled)
      {
         mysql_mutex_unlock (&LOCK_wsrep_sst);
         WSREP_ERROR("Try %d/%d: '%s' failed: %d (%s)",
                     tries, max_tries, proc.cmd(), ret, strerror(ret));
         sleep (1);
      }
      else
      {
         mysql_mutex_unlock (&LOCK_wsrep_sst);
      }
    }
    else
    {
      WSREP_DEBUG("SST script successfully completed.");
      break;
    }
  }

  return -ret;
}

static void sst_reject_queries(my_bool close_conn)
{
    wsrep_ready_set (FALSE); // this will be resotred when donor becomes synced
    WSREP_INFO("Rejecting client queries for the duration of SST.");
    if (TRUE == close_conn) wsrep_close_client_connections(FALSE, false);
}

static int sst_donate_mysqldump (const char*         addr,
                                 const wsrep_uuid_t* uuid,
                                 const char*         uuid_str,
                                 wsrep_seqno_t       seqno,
                                 bool                bypass,
                                 char**              env) // carries auth info
{
  int const cmd_len= 4096;
  wsp::string  cmd_str(cmd_len);

  if (!cmd_str())
  {
    WSREP_ERROR("sst_donate_mysqldump(): "
                "could not allocate cmd buffer of %d bytes", cmd_len);
    return -ENOMEM;
  }

  if (!bypass && wsrep_sst_donor_rejects_queries) sst_reject_queries(TRUE);

  int ret= snprintf (cmd_str(), cmd_len,
                     "wsrep_sst_mysqldump "
                     WSREP_SST_OPT_ADDR" '%s' "
                     WSREP_SST_OPT_LPORT" '%u' "
                     WSREP_SST_OPT_SOCKET" '%s' "
                     WSREP_SST_OPT_CONF" '%s' "
                     WSREP_SST_OPT_GTID" '%s:%lld' "
                     WSREP_SST_OPT_VERSION" '%s' "
                     "%s",
                     addr, mysqld_port, mysqld_unix_port,
                     wsrep_defaults_file, uuid_str,
                     (long long)seqno,
                     MYSQL_SERVER_VERSION MYSQL_SERVER_SUFFIX_DEF,
                     bypass ? " " WSREP_SST_OPT_BYPASS : "");

  if (ret < 0 || ret >= cmd_len)
  {
    WSREP_ERROR("sst_donate_mysqldump(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  WSREP_INFO("Initiating SST/IST transfer on DONOR side (%s)", cmd_str());

  ret= sst_run_shell (cmd_str(), env, 3);

  wsrep_gtid_t const state_id = { *uuid, (ret ? WSREP_SEQNO_UNDEFINED : seqno)};

  wsrep->sst_sent (wsrep, &state_id, ret);

  return ret;
}

wsrep_seqno_t wsrep_locked_seqno= WSREP_SEQNO_UNDEFINED;

static int run_sql_command(THD *thd, const char *query)
{
  thd->set_query((char *)query, strlen(query));

  Parser_state ps;
  if (ps.init(thd, thd->query().str, thd->query().length))
  {
    WSREP_ERROR("SST query: %s failed", query);
    return -1;
  }

  mysql_parse(thd, &ps, false);
  if (thd->is_error())
  {
    int const err= thd->get_stmt_da()->mysql_errno();
    WSREP_WARN ("error executing '%s': %d (%s)%s",
                query, err, thd->get_stmt_da()->message_text(),
                err == ER_UNKNOWN_SYSTEM_VARIABLE ?
                ". Was mysqld built with --with-innodb-disallow-writes ?" : "");
    thd->clear_error();
    return -1;
  }
  return 0;
}

static int sst_flush_tables(THD* thd)
{
  WSREP_INFO("Flushing tables for SST...");

  int err;
  int not_used;
  if (run_sql_command(thd, "FLUSH TABLES WITH READ LOCK"))
  {
    WSREP_ERROR("Failed to flush and lock tables");
    err = ECANCELED;
  }
  else
  {
    /* make sure logs are flushed after global read lock acquired */
    err= reload_acl_and_cache(thd, REFRESH_ENGINE_LOG | REFRESH_BINARY_LOG,
			      (TABLE_LIST*) 0, &not_used);
  }

  if (err)
  {
    WSREP_ERROR("Failed to flush tables: %d (%s)", err, strerror(err));
  }
  else
  {
    WSREP_INFO("Table flushing completed.");
    const char base_name[]= "tables_flushed";
    ssize_t const full_len= strlen(mysql_real_data_home) + strlen(base_name)+2;
    char *real_name= static_cast<char *>(alloca(full_len));
    sprintf(real_name, "%s/%s", mysql_real_data_home, base_name);
    char *tmp_name= static_cast<char *>(alloca(full_len + 4));
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

  if (run_sql_command(thd, query_str))
  {
    WSREP_ERROR("Failed to disallow InnoDB writes");
  }
}

static void* sst_donor_thread (void* a)
{
  sst_thread_arg* arg= (sst_thread_arg*)a;

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_register_thread(key_THREAD_wsrep_sst_donor);
#endif /* HAVE_PSI_INTERFACE */

  WSREP_INFO("Initiating SST/IST transfer on DONOR side (%s)", arg->cmd);

  int  err= 1;
  bool locked= false;

  const char*  out= NULL;
  const size_t out_len= 128;
  char         out_buf[out_len];

  wsrep_uuid_t  ret_uuid= WSREP_UUID_UNDEFINED;
  wsrep_seqno_t ret_seqno= WSREP_SEQNO_UNDEFINED; // seqno of complete SST

  wsp::thd thd(FALSE); // we turn off wsrep_on for this THD so that it can
                       // operate with wsrep_ready == OFF
  thd.ptr->wsrep_sst_donor= true;

  // Launch the SST script and save pointer to its process:

  if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
  wsp::process proc(arg->cmd, "r", arg->env);
  sst_process = &proc;
  mysql_mutex_unlock (&LOCK_wsrep_sst);

  err= proc.error();

/* Inform server about SST script startup and release TO isolation */
  mysql_mutex_lock   (&arg->LOCK_wsrep_sst_thread);
  arg->err = -err;
  mysql_cond_signal  (&arg->COND_wsrep_sst_thread);
  mysql_mutex_unlock (&arg->LOCK_wsrep_sst_thread); //! @note arg is unusable after that.

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
        if (locked)
        {
          sst_disallow_writes (thd.ptr, false);
          thd.ptr->global_read_lock.unlock_global_read_lock (thd.ptr);
          locked= false;
        }
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
      if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
      // Print error message if SST is not cancelled:
#if 0
      if (!sst_cancelled)
      {
         WSREP_ERROR("Failed to read from: %s", proc.cmd());
      }
#endif
      // Clear the pointer to SST process:
      sst_process = NULL;
      mysql_mutex_unlock (&LOCK_wsrep_sst);
      proc.wait();
      goto skip_clear_pointer;
    }
    
    // Clear the pointer to SST process:
    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock (&LOCK_wsrep_sst);

skip_clear_pointer:
    if (!err && proc.error()) err= proc.error();
  }
  else
  {
    // Clear the pointer to SST process:
    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock (&LOCK_wsrep_sst);
    WSREP_ERROR("Failed to execute: %s : %d (%s)",
                proc.cmd(), err, strerror(err));
  }

  if (locked) // don't forget to unlock server before return
  {
    sst_disallow_writes (thd.ptr, false);
    thd.ptr->global_read_lock.unlock_global_read_lock (thd.ptr);
  }

  // signal to donor that SST is over
  struct wsrep_gtid const state_id = {
      ret_uuid, err ? WSREP_SEQNO_UNDEFINED : ret_seqno
  };
  wsrep->sst_sent (wsrep, &state_id, -err);
  proc.wait();

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

  return NULL;
}



static int sst_donate_other (const char*   method,
                             const char*   addr,
                             const char*   uuid,
                             wsrep_seqno_t seqno,
                             bool          bypass,
                             char**        env) // carries auth info
{
  int const cmd_len= 4096;
  wsp::string  cmd_str(cmd_len);

  if (!cmd_str())
  {
    WSREP_ERROR("sst_donate_other(): "
                "could not allocate cmd buffer of %d bytes", cmd_len);
    return -ENOMEM;
  }

  const char* binlog_opt= "";
  char* binlog_opt_val= NULL;

  int ret;
  if ((ret= generate_binlog_opt_val(&binlog_opt_val)))
  {
    WSREP_ERROR("sst_donate_other(): generate_binlog_opt_val() failed: %d",ret);
    return ret;
  }
  if (strlen(binlog_opt_val)) binlog_opt= WSREP_SST_OPT_BINLOG;

  ret= snprintf (cmd_str(), cmd_len,
                 "wsrep_sst_%s "
                 WSREP_SST_OPT_ROLE" 'donor' "
                 WSREP_SST_OPT_ADDR" '%s' "
                 WSREP_SST_OPT_SOCKET" '%s' "
                 WSREP_SST_OPT_DATA" '%s' "
                 WSREP_SST_OPT_CONF" '%s' "
                 WSREP_SST_OPT_CONF_SUFFIX" '%s' "
                 WSREP_SST_OPT_VERSION" '%s' "
                 " %s '%s' "
                 WSREP_SST_OPT_GTID" '%s:%lld' "
                 "%s",
                 method, addr, mysqld_unix_port, mysql_real_data_home,
                 wsrep_defaults_file, wsrep_defaults_group_suffix,
                 MYSQL_SERVER_VERSION MYSQL_SERVER_SUFFIX_DEF,
                 binlog_opt, binlog_opt_val,
                 uuid, (long long) seqno,
                 bypass ? " " WSREP_SST_OPT_BYPASS : "");
  my_free(binlog_opt_val);

  if (ret < 0 || ret >= cmd_len)
  {
    WSREP_ERROR("sst_donate_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  if (!bypass && wsrep_sst_donor_rejects_queries) sst_reject_queries(FALSE);

  pthread_t tmp;
  sst_thread_arg arg(cmd_str(), env);
  mysql_mutex_lock (&arg.LOCK_wsrep_sst_thread);
  ret = pthread_create(&tmp, NULL, sst_donor_thread, &arg);
  if (ret)
  {
    WSREP_ERROR("sst_donate_other(): pthread_create() failed: %d (%s)",
                ret, strerror(ret));
    return -ret;
  }
  mysql_cond_wait (&arg.COND_wsrep_sst_thread, &arg.LOCK_wsrep_sst_thread);

  WSREP_INFO("DONOR thread signaled with %d", arg.err);
  pthread_detach(tmp);
  return arg.err;
}

/*
  Validate SST request string.
  The protocol is: method\0data\0

  For xtrabackup-v2 it is: method\0addresspath\0
  like:
  "xtrabackup-v2\000127.0.0.1:4444/xtrabackup_sst//1"
*/
static bool is_sst_request_valid(const std::string &msg)
{
  size_t method_len= strlen(msg.c_str());

  if (method_len == 0)
  {
    return false;
  }

  std::string method= msg.substr(0, method_len);

  // Is this method allowed?
  std::vector<std::string>::iterator res= std::find(
      allowed_sst_methods.begin(), allowed_sst_methods.end(), method);
  if (res == allowed_sst_methods.end())
  {
    return false;
  }

  const char *data_ptr= msg.c_str() + method_len + 1;
  size_t      data_len= strlen(data_ptr);

  // method + null + data + null
  if (method_len + 1 + data_len + 1 != msg.length())
  {
    // Someone tries to piggyback after 2nd null
    return false;
  }

  if (data_len > 0)
  {
    std::string data= msg.substr(method_len + 1, data_len);

    // first make sure we don't have lt/gt which are not allowed
    if (std::string::npos != data.find('<') ||
        std::string::npos != data.find('>'))
    {
        return false;
    }

    // square brackets workaround
    std::replace(data.begin(), data.end(), '[', '<');
    std::replace(data.begin(), data.end(), ']', '>');

    return sst_method_allowed_chars_regex.match(data);
  }
  return true;
}

/* Helper functor class for conversion of std::string
   containing null characters into printable string */
struct StringBuilder : public std::unary_function<char, void> {
  StringBuilder(std::ostringstream &ss) : ss_(ss) {}

  void operator()(char ch)
  {
    if (ch != 0)
      ss_ << ch;
    else
      ss_ << "<nullptr>";
  }
  std::ostringstream &ss_;
};

wsrep_cb_status_t wsrep_sst_donate_cb (void* app_ctx, void* recv_ctx,
                                       const void* msg, size_t msg_len,
                                       const wsrep_gtid_t* current_gtid,
                                       const char* state, size_t state_len,
                                       bool bypass)
{
  /* This will be reset when sync callback is called.
   * Should we set wsrep_ready to FALSE here too? */
//  wsrep_notify_status(WSREP_MEMBER_DONOR);
  local_status.set(WSREP_MEMBER_DONOR);

  std::string message(reinterpret_cast<const char *>(msg), msg_len);
  if (!is_sst_request_valid(message))
  {
    std::ostringstream ss;
    StringBuilder      stringBuilder(ss);
    std::for_each(message.begin(), message.end(), stringBuilder);
    WSREP_ERROR("Invalid sst_request: %s", ss.str().c_str());
    return WSREP_CB_FAILURE;
  }

  const char* method = (char*)msg;
  size_t method_len  = strlen (method);
  const char* data   = method + method_len + 1;

  char uuid_str[37];
  wsrep_uuid_print (&current_gtid->uuid, uuid_str, sizeof(uuid_str));

  DBUG_EXECUTE_IF("wsrep_sst_donate_cb_fails", 
  {
    return WSREP_CB_FAILURE;
  });

  wsp::env env(NULL);
  if (env.error())
  {
    WSREP_ERROR("wsrep_sst_donate_cb(): env var ctor failed: %d", -env.error());
    return WSREP_CB_FAILURE;
  }

  /* Wait for wsrep-SE to initialize that also signals
  completion of init_server_component which is important before we initiate
  any meangiful action especially DONOR action from this node. */
  while (!wsrep_is_SE_initialized())
  {
    sleep(1);
    THD* applier_thd= static_cast<THD*>(recv_ctx);
    if (applier_thd->killed == THD::KILL_CONNECTION)
      return WSREP_CB_FAILURE;
  }

  int ret;
  if ((ret= sst_append_auth_env(env, sst_auth_real)))
  {
    WSREP_ERROR("wsrep_sst_donate_cb(): appending auth env failed: %d", ret);
    return WSREP_CB_FAILURE;
  }

  if (!strcmp (WSREP_SST_MYSQLDUMP, method))
  {
    WSREP_WARN("Percona-XtraDB-Cluster has deprecated SST through mysqldump."
               " Percona-XtraDB-Cluster recommends using xtrabackup."
               " Please switch to use xtrabackup or rsync.");
    ret = sst_donate_mysqldump(data, &current_gtid->uuid, uuid_str,
                               current_gtid->seqno, bypass, env());
  }
  else
  {
    ret = sst_donate_other(method, data, uuid_str,
                           current_gtid->seqno, bypass, env());
  }

  /* Above methods should return 0 in case of success and negative value
   * in case of failure. If we have any positive value here it means that we
   * handle errors in above functions in the wrong way */
  DBUG_ASSERT(ret <= 0); 
  return (ret >= 0 ? WSREP_CB_SUCCESS : WSREP_CB_FAILURE);
}

void wsrep_SE_init_grab()
{
  if (mysql_mutex_lock (&LOCK_wsrep_sst_init)) abort();
}

void wsrep_SE_init_wait(THD* thd)
{
  while (SE_initialized == false && thd->killed == THD::NOT_KILLED)
  {
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->current_cond= &COND_wsrep_sst_init;
    thd->current_mutex= &LOCK_wsrep_sst_init;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
    
    mysql_cond_wait (&COND_wsrep_sst_init, &LOCK_wsrep_sst_init);

    if (thd->killed != THD::NOT_KILLED)
    {
      WSREP_DEBUG("SE init waiting canceled");
      break;
    }
    mysql_mutex_lock(&thd->LOCK_thd_data);
    thd->current_cond= NULL;
    thd->current_mutex= NULL;
    mysql_mutex_unlock(&thd->LOCK_thd_data);
  }
  mysql_mutex_unlock (&LOCK_wsrep_sst_init);

  mysql_mutex_lock(&thd->LOCK_thd_data);
  thd->current_cond= NULL;
  thd->current_mutex= NULL;
  mysql_mutex_unlock(&thd->LOCK_thd_data);
}

void wsrep_SE_init_done()
{
  mysql_cond_signal (&COND_wsrep_sst_init);
  mysql_mutex_unlock (&LOCK_wsrep_sst_init);
}

void wsrep_SE_initialized()
{
  SE_initialized = true;
}

bool wsrep_is_SE_initialized()
{
  return SE_initialized;
}
