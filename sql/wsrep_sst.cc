/* Copyright 2008-2017 Codership Oy <http://www.codership.com>

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

#include "wsrep_sst.h"
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <sstream>
#include "debug_sync.h"
#include "log_event.h"
#include "my_rnd.h"
#include "mysql/components/services/log_builtins.h"
#include "mysql_version.h"
#include "mysqld.h"
#include "rpl_msr.h"  // channel_map
#include "rpl_replica.h"
#include "sql/auth/auth_common.h"
#include "sql/sql_lex.h"
#include "sql_base.h"  // TEMP_PREFIX
#include "sql_class.h"
#include "sql_parse.h"
#include "sql_reload.h"
#include "srv_session.h"
#include "wsrep_applier.h"
#include "wsrep_binlog.h"
#include "wsrep_priv.h"
#include "wsrep_service.h"
#include "wsrep_thd.h"
#include "wsrep_utils.h"
#include "wsrep_var.h"
#include "wsrep_xid.h"

extern const char wsrep_defaults_file[];
extern const char *wsrep_defaults_group_suffix;

#define WSREP_SST_OPT_ROLE "--role"
#define WSREP_SST_OPT_ADDR "--address"
#define WSREP_SST_OPT_AUTH "--auth"
#define WSREP_SST_OPT_DATA "--datadir"
#define WSREP_SST_OPT_BASEDIR "--basedir"
#define WSREP_SST_OPT_PLUGINDIR "--plugindir"
#define WSREP_SST_OPT_CONF "--defaults-file"
#define WSREP_SST_OPT_CONF_SUFFIX "--defaults-group-suffix"
#define WSREP_SST_OPT_PARENT "--parent"
#define WSREP_SST_OPT_BINLOG "--binlog"
#define WSREP_SST_OPT_VERSION "--mysqld-version"
#define WSREP_SST_OPT_DEBUG "--debug"

// mysqldump-specific options
#define WSREP_SST_OPT_HOST "--host"
#define WSREP_SST_OPT_PORT "--port"
#define WSREP_SST_OPT_LPORT "--local-port"

// donor-specific
#define WSREP_SST_OPT_SOCKET "--socket"
#define WSREP_SST_OPT_GTID "--gtid"
#define WSREP_SST_OPT_BYPASS "--bypass"

#define WSREP_SST_RSYNC "rsync"
#define WSREP_SST_MYSQLDUMP "mysqldump"
#define WSREP_SST_ONLY_IST "ist_only"
#define WSREP_SST_XTRABACKUP "xtrabackup"
#define WSREP_SST_XTRABACKUP_V2 "xtrabackup-v2"
#define WSREP_SST_DEFAULT WSREP_SST_XTRABACKUP_V2
#define WSREP_SST_ADDRESS_AUTO "AUTO"

#define WSREP_STATE_TRANSFER_NO_SST ""

const char *wsrep_sst_method = WSREP_SST_DEFAULT;
const char *wsrep_sst_receive_address = WSREP_SST_ADDRESS_AUTO;
const char *wsrep_sst_donor = "";

#define WSREP_SST_ALLOWED_METHODS_DEFAULT WSREP_SST_XTRABACKUP_V2
const char *wsrep_sst_allowed_methods = WSREP_SST_ALLOWED_METHODS_DEFAULT;
static std::vector<std::string> allowed_sst_methods;

bool wsrep_sst_donor_rejects_queries = false;

/* Function checks if the new value for sst_method is valid.
@return false if no error encountered with check else return true. */
bool wsrep_sst_method_check(sys_var *self, THD *, set_var *var) {
  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length == 0)) {
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), self->name.str,
             var->save_result.string_value.str
                 ? var->save_result.string_value.str
                 : "NULL");
    return true;
  }

  if (strcmp(var->save_result.string_value.str, WSREP_SST_XTRABACKUP_V2) != 0) {
    my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), self->name.str,
             var->save_result.string_value.str
                 ? var->save_result.string_value.str
                 : "NULL");
    return true;
  }

  return false;
}

bool wsrep_sst_method_update(sys_var *, THD *, enum_var_type) { return 0; }

/* Function checks if the new value for sst_recieve_address is valid.
@return false if no error encountered with check else return true. */
bool wsrep_sst_receive_address_check(sys_var *self, THD *, set_var *var) {
  char addr_buf[FN_REFLEN];

  if ((!var->save_result.string_value.str) ||
      (var->save_result.string_value.length > (FN_REFLEN - 1)))  // safety
  {
    goto err;
  }

  memcpy(addr_buf, var->save_result.string_value.str,
         var->save_result.string_value.length);
  addr_buf[var->save_result.string_value.length] = 0;

  return false;

err:
  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), self->name.str,
           var->save_result.string_value.str ? var->save_result.string_value.str
                                             : "NULL");
  return true;
}

bool wsrep_sst_receive_address_update(sys_var *, THD *, enum_var_type) {
  return 0;
}

bool wsrep_sst_donor_check(sys_var *, THD *, set_var *) { return 0; }

bool wsrep_sst_donor_update(sys_var *, THD *, enum_var_type) { return 0; }

bool wsrep_before_SE() {
  return (wsrep_provider != NULL && strcmp(wsrep_provider, WSREP_NONE));
}

bool wsrep_setup_allowed_sst_methods() {
  std::string methods(wsrep_sst_allowed_methods);

  /* Check if the string is valid. In method name we allow only:
     alpha-num
     underscore
     dash
     dot
     but we do not allow leading and trailing dash/dot.
     Method names have to be separated by colons (spaces before/after colon are
     allowed). Below regex may seem difficult, but in fact it is not:

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

     11. Followed by any number of trailing spaces */

  static std::regex validate_regex(
      "\\s*(\\w([-_.]*\\w)*(\\s*,\\s*))*\\w([-_.]*\\w)*\\s*",
      std::regex::nosubs);

  if (!std::regex_match(methods, validate_regex)) {
    WSREP_FATAL(
        "Wrong format of --wsrep-sst-allowed-methods parameter value: %s",
        wsrep_sst_allowed_methods);
    return true;
  }

  // split it into tokens and trim
  static std::regex split_regex("[^,\\s][^,]*[^,\\s]");
  for (auto it =
           std::sregex_iterator(methods.begin(), methods.end(), split_regex);
       it != std::sregex_iterator(); ++it) {
    allowed_sst_methods.push_back(it->str());
  }

  return false;
}

// True if wsrep awaiting sst_received callback:
static bool sst_awaiting_callback = false;

bool wsrep_sst_in_progress() {
    if (mysql_mutex_lock (&LOCK_wsrep_sst)) abort();
    bool in_progress = sst_awaiting_callback;
    mysql_mutex_unlock (&LOCK_wsrep_sst);
    return in_progress;
}

// Signal end of SST
static int wsrep_sst_complete(THD *thd, int const rcode) {
  Wsrep_client_service client_service(thd, thd->wsrep_cs());
  return Wsrep_server_state::instance().sst_received(client_service, rcode,
                                                     &sst_awaiting_callback);
}

/*
If wsrep provider is loaded, inform that the new state snapshot
has been received. Also update the local checkpoint.
*/
bool wsrep_sst_received(THD *thd, const wsrep_uuid_t &uuid,
                        wsrep_seqno_t const seqno,
                        const void *const state __attribute__((unused)),
                        size_t const state_len __attribute__((unused))) {
  /*
    To keep track of whether the local uuid:seqno should be updated. Also, note
    that local state (uuid:seqno) is updated/checkpointed only after we get an
    OK from wsrep provider. By doing so, the values remain consistent across
    the server & wsrep provider.
  */
  /*
    TODO: Handle backwards compatibility. WSREP API v25 does not have
    wsrep schema.
  */
  /*
    Logical SST methods (mysqldump etc) don't update InnoDB sys header.
    Reset the SE checkpoint before recovering view in order to avoid
    sanity check failure.
   */
  wsrep::gtid const sst_gtid(wsrep::id(uuid.data, sizeof(uuid.data)),
                             wsrep::seqno(seqno));

  if (!wsrep_before_SE()) {
    wsrep_set_SE_checkpoint(wsrep::gtid::undefined());
    wsrep_set_SE_checkpoint(sst_gtid);
  }
  wsrep_verify_SE_checkpoint(uuid, seqno);

  /*
    Both wsrep_init_SR() and wsrep_recover_view() may use
    wsrep thread pool. Restore original thd context before returning.
  */
  if (thd) {
    wsrep_store_threadvars(thd);
  }

  if (WSREP_ON) {
    int const rcode(seqno < 0 ? seqno : 0);
    return (0 != wsrep_sst_complete(thd, rcode));
  }

  return false;
}

// get rid of trailing \n
static char *my_fgets(char *buf, size_t buf_len, FILE *stream) {
  char *ret = fgets(buf, buf_len, stream);

  if (ret) {
    size_t len = strlen(ret);
    if (len > 0 && ret[len - 1] == '\n') ret[len - 1] = '\0';
  }

  return ret;
}

struct sst_thread_arg {
  const char *cmd;
  char **env;
  char *ret_str;
  int err;

  mysql_mutex_t LOCK_wsrep_sst_thread;
  mysql_cond_t COND_wsrep_sst_thread;

  sst_thread_arg(const char *c, char **e)
      : cmd(c), env(e), ret_str(0), err(-1) {
    mysql_mutex_init(key_LOCK_wsrep_sst_thread, &LOCK_wsrep_sst_thread,
                     MY_MUTEX_INIT_FAST);
    mysql_cond_init(key_COND_wsrep_sst_thread, &COND_wsrep_sst_thread);
  }

  ~sst_thread_arg() {
    mysql_cond_destroy(&COND_wsrep_sst_thread);
    mysql_mutex_unlock(&LOCK_wsrep_sst_thread);
    mysql_mutex_destroy(&LOCK_wsrep_sst_thread);
  }
};

struct sst_logger_thread_arg {
  /* This is the READ end of the stderr pipe */
  FILE *err_pipe;

  sst_logger_thread_arg(FILE *ferr) : err_pipe(ferr) {}

  ~sst_logger_thread_arg() {
    if (err_pipe) fclose(err_pipe);
    err_pipe = NULL;
  }
};

static enum loglevel string_to_loglevel(const char *s) {
  if (strncmp(s, "ERR:", 4) == 0)
    return ERROR_LEVEL;
  else if (strncmp(s, "WRN:", 4) == 0)
    return WARNING_LEVEL;
  else if (strncmp(s, "INF:", 4) == 0)
    return INFORMATION_LEVEL;
  else if (strncmp(s, "DBG:", 4) == 0)
    return INFORMATION_LEVEL;
  else
    return SYSTEM_LEVEL;
}

static void *sst_logger_thread(void *a) {
  sst_logger_thread_arg *arg = (sst_logger_thread_arg *)a;

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_register_thread(key_THREAD_wsrep_sst_logger);
#endif /* HAVE_PSI_INTERFACE */

  int const out_len = 1024;
  char out_buf[out_len];

  /* Loops over the READ end of stderr.  Only stops
     when the pipe is in an error or EOF state.
   */
  while (!feof(arg->err_pipe) && !ferror(arg->err_pipe)) {
    char *p = my_fgets(out_buf, out_len, arg->err_pipe);
    if (!p) continue;

    enum loglevel level = string_to_loglevel(p);
    if (level != SYSTEM_LEVEL) {
      WSREP_SST_LOG(level, p + 4);
      if (level == ERROR_LEVEL) {
        flush_error_log_messages();
        pxc_force_flush_error_message = true;
      }
    } else if (strncmp(p, "FIL:", 4) == 0) {
      /* Expect a string with 3 components (separated by semi-colons)
          "FIL" marker
          Level marker : "ERR", "WRN", "INF", "DBG"
          Descriptive string
      */
      std::string s;
      std::string description;
      s.reserve(8002);

      level = string_to_loglevel(p + 4);
      if (level == SYSTEM_LEVEL) {
        // Unknown priority level
        WSREP_ERROR("Unexpected line formatting: %s", p);
        level = INFORMATION_LEVEL;
        description = "(Unknown)";
      } else
        description = p + 8;

      // Header line
      s = "------------ " + description + " (START) ------------\n";

      // Loop around until we get an "EOF:"
      while ((p = fgets(out_buf, out_len, arg->err_pipe)) != NULL) {
        if (strncmp(p, "EOF:", 4) == 0) break;

        // Output the data  if we have to (data max is 8K)
        if (s.length() + strlen(p) + 1 >= 8000) {
          // Ensure the string ends with a newline
          if (s.back() != '\n') s += "\n";
          WSREP_SST_LOG(level, s.c_str());
          s = "------------ " + description + " (cont) ------------\n";
        }
        s += "\t";  // Indent the file contents
        s += p;
      }

      // Either we've had an error, or we've gotten to the end
      // Either way, dump all of our contents
      WSREP_SST_LOG(level, s.c_str());

      s = "------------ " + description + " (END) ------------";
      WSREP_SST_LOG(level, s.c_str());
    } else {
      // Unexpected output
      WSREP_SST_LOG(INFORMATION_LEVEL, p);
    }
  }

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

  return NULL;
}

static int start_sst_logger_thread(sst_logger_thread_arg *arg, pthread_t *thd) {
  int ret;

  ret = pthread_create(thd, NULL, sst_logger_thread, arg);
  if (ret) {
    *thd = 0;
    WSREP_ERROR("start_sst_logger_thread(): pthread_create() failed: %d (%s)",
                ret, strerror(ret));
    return -ret;
  }
  return 0;
}

static int sst_scan_uuid_seqno(const char *str, wsrep_uuid_t *uuid,
                               wsrep_seqno_t *seqno) {
  int offt = wsrep_uuid_scan(str, strlen(str), uuid);
  if (offt > 0 && strlen(str) > (unsigned int)offt && ':' == str[offt]) {
    *seqno = strtoll(str + offt + 1, NULL, 10);
    if (*seqno != LLONG_MAX || errno != ERANGE) {
      return 0;
    }
  }

  WSREP_ERROR("Failed to parse uuid:seqno pair: '%s'", str);
  return -EINVAL;
}

/*
  Generate opt_binlog_opt_val for sst_donate_other(), sst_prepare_other().

  Returns zero on success, negative error code otherwise.

  String containing binlog name is stored in param ret if binlog is enabled
  and GTID mode is on, otherwise empty string. Returned string should be
  freed with my_free().
 */
static int generate_binlog_opt_val(char **ret) {
  assert(ret);
  *ret = NULL;
  if (opt_bin_log && global_gtid_mode.get() > Gtid_mode::OFF) {
    assert(opt_bin_logname);
    *ret = strcmp(opt_bin_logname, "0")
               ? my_strdup(key_memory_wsrep, opt_bin_logname, MYF(0))
               : my_strdup(key_memory_wsrep, "", MYF(0));
  } else {
    *ret = my_strdup(key_memory_wsrep, "", MYF(0));
  }
  if (!*ret) return -ENOMEM;
  return 0;
}

// Pointer to SST process object:

static wsp::process *sst_process = static_cast<wsp::process *>(NULL);

// SST cancellation flag:

static bool sst_cancelled = false;

void wsrep_sst_cancel(bool call_wsrep_cb) {
  if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
  if (!sst_cancelled) {
    WSREP_INFO("Initiating SST cancellation");
    sst_cancelled = true;
    /*
      When we launched the SST process, then we need
      to terminate it before exit from the parent (server)
      process:
    */
    if (sst_process) {
      WSREP_INFO("Terminating SST process");
      sst_process->terminate();
      sst_process = NULL;
    }
    /*
      If this is a normal shutdown, then we need to notify
      the wsrep provider about completion of the SST, to
      prevent infinite waitng in the wsrep provider after
      the SST process was canceled:
    */
    if (call_wsrep_cb && sst_awaiting_callback) {
      WSREP_INFO("Signalling cancellation of the SST request.");
      const wsrep::gtid state_id = wsrep::gtid::undefined();
      sst_awaiting_callback = false;
      Wsrep_server_state::instance().provider().sst_received(state_id,
                                                             -ECANCELED);
    }
  }
  mysql_mutex_unlock(&LOCK_wsrep_sst);
}

/*
  Handling of fatal signals: SIGABRT, SIGTERM, etc.
*/
extern "C" void wsrep_handle_fatal_signal(int) { wsrep_sst_cancel(false); }

/*
  This callback is invoked in the case of abnormal
  termination of the wsrep provider:
*/
void wsrep_abort_cb(void) { wsrep_sst_cancel(false); }

static void *sst_joiner_thread(void *a) {
  sst_thread_arg *arg = (sst_thread_arg *)a;
  int err = 1;

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_register_thread(key_THREAD_wsrep_sst_joiner);
#endif /* HAVE_PSI_INTERFACE */

  {
    THD *thd;
    const char magic[] = "ready";
    const size_t magic_len = sizeof(magic) - 1;
    const int out_len = 512;
    char out[out_len];

    WSREP_INFO("Initiating SST/IST transfer on JOINER side (%s)", arg->cmd);

    // Launch the SST script and save pointer to its process:

    if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
    wsp::process proc(arg->cmd, "rw", arg->env);
    sst_process = &proc;
    mysql_mutex_unlock(&LOCK_wsrep_sst);

    pthread_t logger_thd = 0;
    sst_logger_thread_arg logger_arg(proc.err_pipe());
    proc.clear_err_pipe();

    err = start_sst_logger_thread(&logger_arg, &logger_thd);

    if (!err && !proc.error()) {
      // Write out to the stdin pipe any parameters (if needed)

      // Close the pipe, so that the other side gets an EOF
      proc.close_write_pipe();
    }
    if (!err && proc.pipe() && !proc.error()) {
      const char *tmp = my_fgets(out, out_len, proc.pipe());
      if (!tmp || strlen(tmp) < (magic_len + 2) ||
          strncasecmp(tmp, magic, magic_len)) {
        if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
        // Print error message if SST is not cancelled:
        if (!sst_cancelled) {
          // The process has exited, so the logger thread should
          // also have exited
          if (logger_thd) {
            pthread_join(logger_thd, NULL);
            logger_thd = 0;
          }
          // Null-pointer is not valid argument for %s formatting (even
          // though it is supported by many compilers):
          WSREP_ERROR("Failed to read '%s <addr>' from: %s\n\tRead: '%s'",
                      magic, arg->cmd, tmp ? tmp : "(null)");
        }
        // Clear the pointer to SST process:
        sst_process = NULL;
        mysql_mutex_unlock(&LOCK_wsrep_sst);
        proc.wait();
        if (proc.error()) err = proc.error();
      } else {
        // Do not clear the pointer to SST process. It will be useful if
        // we decide to interrupt it.
        err = 0;
      }
    } else {
      // Clear the pointer to SST process:
      if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
      sst_process = NULL;
      mysql_mutex_unlock(&LOCK_wsrep_sst);
      err = proc.error();
      WSREP_ERROR("Failed to execute: %s : %d (%s)", arg->cmd, err,
                  strerror(err));
    }

    // signal sst_prepare thread with ret code,
    // it will go on sending SST request
    mysql_mutex_lock(&arg->LOCK_wsrep_sst_thread);
    if (!err) {
      arg->ret_str = strdup(out + magic_len + 1);
      if (!arg->ret_str) err = ENOMEM;
    }
    arg->err = -err;
    mysql_cond_signal(&arg->COND_wsrep_sst_thread);
    mysql_mutex_unlock(&arg->LOCK_wsrep_sst_thread);

    if (err) {
      // The process has exited, so the logger thread should
      // also have exited
      if (logger_thd) pthread_join(logger_thd, NULL);

      return NULL; /* lp:808417 - return immediately, don't signal
                    * initializer thread to ensure single thread of
                    * shutdown. */
    }

    wsrep_uuid_t ret_uuid = WSREP_UUID_UNDEFINED;
    wsrep_seqno_t ret_seqno = WSREP_SEQNO_UNDEFINED;

    // in case of successfull receiver start, wait for SST completion/end
    char *tmp = my_fgets(out, out_len, proc.pipe());

    proc.wait();
    err = EINVAL;

    // The process has exited, so the logger thread should
    // also have exited
    if (logger_thd) pthread_join(logger_thd, NULL);

    if (!tmp || proc.error()) {
      WSREP_ERROR("Failed to read uuid:seqno from joiner script.");
      if (proc.error()) {
        err = proc.error();
        char errbuf[MYSYS_STRERROR_SIZE];
        WSREP_ERROR("SST script aborted with error %d (%s)", err,
                    my_strerror(errbuf, sizeof(errbuf), err));
      }
    } else {
      err = sst_scan_uuid_seqno(out, &ret_uuid, &ret_seqno);
    }

    wsrep::gtid ret_gtid;

    if (err) {
      ret_gtid = wsrep::gtid::undefined();
    } else {
      ret_gtid = wsrep::gtid(wsrep::id(ret_uuid.data, sizeof(ret_uuid.data)),
                             wsrep::seqno(ret_seqno));
    }

    // Tell initializer thread that SST is complete
    if (my_thread_init()) {
      WSREP_ERROR(
          "my_thread_init() failed, can't signal end of SST. "
          "Aborting.");
      unireg_abort(1);
    }

    thd = new THD;
    thd->set_new_thread_id();

    if (!thd) {
      WSREP_ERROR(
          "Failed to allocate THD to restore view from local state, "
          "can't signal end of SST. Aborting.");
      unireg_abort(1);
    }

    thd->thread_stack = (char *)&thd;
    thd->security_context()->skip_grants();
    thd->system_thread = SYSTEM_THREAD_BACKGROUND;
    thd->real_id = pthread_self();
    wsrep_assign_from_threadvars(thd);
    wsrep_store_threadvars(thd);

    /* */
    thd->variables.wsrep_on = 0;
    /* No binlogging */
    thd->variables.sql_log_bin = 0;
    thd->variables.option_bits &= ~OPTION_BIN_LOG;
    thd->variables.option_bits |= OPTION_BIN_LOG_INTERNAL_OFF;
    /* No general log */
    thd->variables.option_bits |= OPTION_LOG_OFF;
    /* Read committed isolation to avoid gap locking */
    thd->variables.transaction_isolation = ISO_READ_COMMITTED;

    if (wsrep_sst_complete(thd, -err)) {
      WSREP_WARN("Failure while signalling the completion of SST.");
    }

    WSREP_SYSTEM("SST completed");
    delete thd;
    my_thread_end();
  }

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

  if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
  sst_process = NULL;
  mysql_mutex_unlock(&LOCK_wsrep_sst);

  return NULL;
}

#if defined(HAVE_ASAN)
/*
 * When MySQL is built with ASAN, libasan will be propagated to child processes
 * via LD_PRELOAD. This can be annoying because unix utils including bash can
 * have memory errors itself. Let's unset LD_PRELOAD on ASAN builds.
 */
static void reset_ld_preload(wsp::env &env) { env.append("LD_PRELOAD="); }
#endif

static ssize_t sst_prepare_other(const char *method, const char *addr_in,
                                 const char **addr_out) {
  int const cmd_len = 4096;
  wsp::string cmd_str(cmd_len);

  if (!cmd_str()) {
    WSREP_ERROR(
        "sst_prepare_other(): could not allocate cmd buffer of %d bytes",
        cmd_len);
    return -ENOMEM;
  }

  const char *binlog_opt = "";
  char *binlog_opt_val = NULL;

  int ret;
  if ((ret = generate_binlog_opt_val(&binlog_opt_val))) {
    WSREP_ERROR("sst_prepare_other(): generate_binlog_opt_val() failed: %d",
                ret);
    return ret;
  }
  if (strlen(binlog_opt_val)) binlog_opt = WSREP_SST_OPT_BINLOG;

  ret = snprintf(cmd_str(), cmd_len,
                 "wsrep_sst_%s " WSREP_SST_OPT_ROLE
                 " 'joiner' " WSREP_SST_OPT_ADDR " '%s' " WSREP_SST_OPT_DATA
                 " '%s' " WSREP_SST_OPT_BASEDIR " '%s' " WSREP_SST_OPT_PLUGINDIR
                 " '%s' " WSREP_SST_OPT_CONF " '%s' " WSREP_SST_OPT_CONF_SUFFIX
                 " '%s' " WSREP_SST_OPT_PARENT " '%d' " WSREP_SST_OPT_VERSION
                 " '%s' "
                 " %s '%s' ",
                 method, addr_in, mysql_real_data_home,
                 mysql_home_ptr ? mysql_home_ptr : "",
                 opt_plugin_dir_ptr ? opt_plugin_dir_ptr : "",
                 wsrep_defaults_file, wsrep_defaults_group_suffix,
                 (int)getpid(), MYSQL_SERVER_VERSION MYSQL_SERVER_SUFFIX_DEF,
                 binlog_opt, binlog_opt_val);
  my_free(binlog_opt_val);

  if (ret < 0 || ret >= cmd_len) {
    WSREP_ERROR("sst_prepare_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  wsp::env env(NULL);
  if (env.error()) {
    WSREP_ERROR("sst_prepare_other(): env. var ctor failed: %d", -env.error());
    return -env.error();
  }

#if defined(HAVE_ASAN)
  reset_ld_preload(env);
#endif

  pthread_t tmp;
  sst_thread_arg arg(cmd_str(), env());
  mysql_mutex_lock(&arg.LOCK_wsrep_sst_thread);
  ret = pthread_create(&tmp, NULL, sst_joiner_thread, &arg);
  if (ret) {
    WSREP_ERROR("sst_prepare_other(): pthread_create() failed: %d (%s)", ret,
                strerror(ret));
    return -ret;
  }
  mysql_cond_wait(&arg.COND_wsrep_sst_thread, &arg.LOCK_wsrep_sst_thread);

  *addr_out = arg.ret_str;

  if (!arg.err)
    ret = strlen(*addr_out);
  else {
    assert(arg.err < 0);
    ret = arg.err;
  }

  pthread_detach(tmp);

  return ret;
}

extern uint mysqld_port;

std::string wsrep_sst_prepare() {
  const ssize_t ip_max = 256;
  char ip_buf[ip_max];
  const char *addr_in = NULL;
  const char *addr_out = NULL;
  const char *method;

  if (!strcmp(wsrep_sst_method, WSREP_SST_ONLY_IST)) {
    /* Inform Galera that we are done with SST and it can proceed with IST.
    In fact the following call will wait until the server is fully initialized
    and then inform Galera about two things:
    1. call sst_received() - so from Galera's point of view it looks like we are done with
                             sst
    2. return empty string from this function - informs Galera that only IST should
       be processed.*/
    WSREP_WARN("State Transfer via SST was prohibited by setting wsrep_sst_method=ist_only. "
               "The node will try to join the cluster using only IST.");
    wsrep_sst_complete(current_thd, 0);
    return WSREP_STATE_TRANSFER_NO_SST;
  }

  // Figure out SST address. Common for all SST methods
  if (wsrep_sst_receive_address &&
      strcmp(wsrep_sst_receive_address, WSREP_SST_ADDRESS_AUTO)) {
    addr_in = wsrep_sst_receive_address;
  } else if (wsrep_node_address && strlen(wsrep_node_address)) {
    size_t const addr_len = strlen(wsrep_node_address);
    size_t const host_len = wsrep_host_len(wsrep_node_address, addr_len);

    /* ip_buf is 256 bytes, wsrep_node_address is <ip_address>[:port]
       so for ipv6 host part is at most 45 characters (IPv4-mapped IPv6).
       Host part will entirely fit into ip_buf leaving space for trailing zero.
     */
    if (unlikely(host_len >= ip_max)) {
      WSREP_ERROR(
          "Could not prepare state transfer request: "
          "Host IP does not fit into temporary buffer. "
          "wsrep_node_address: %s, host_len: %lu",
          wsrep_node_address, host_len);
      throw wsrep::runtime_error("Could not prepare state transfer request");
    }

    if (host_len < addr_len) {
      strncpy(ip_buf, wsrep_node_address, host_len);
      ip_buf[host_len] = '\0';
      addr_in = ip_buf;
    } else {
      addr_in = wsrep_node_address;
    }
  } else {
    ssize_t ret = wsrep_guess_ip(ip_buf, ip_max);

    if (ret && ret < ip_max) {
      addr_in = ip_buf;
    } else {
      WSREP_ERROR(
          "Could not prepare state transfer request: "
          "failed to guess address to accept state transfer at. "
          "wsrep_sst_receive_address must be set manually.");
      throw wsrep::runtime_error("Could not prepare state transfer request");
    }
  }

  ssize_t addr_len = -ENOSYS;
  method = wsrep_sst_method;
  if (Wsrep_server_state::instance().is_initialized() &&
      Wsrep_server_state::instance().state() == Wsrep_server_state::s_joiner) {
    // we already did SST at initializaiton, now engines are running
    WSREP_INFO(
        "WSREP: "
        "You have configured '%s' state snapshot transfer method "
        "which cannot be performed on a running server. "
        "Wsrep provider won't be able to fall back to it "
        "if other means of state transfer are unavailable. "
        "In that case you will need to restart the server.",
        wsrep_sst_method);
    return "";
  }

  addr_len = sst_prepare_other(wsrep_sst_method, addr_in, &addr_out);
  if (addr_len < 0) {
    WSREP_ERROR("Failed to prepare for '%s' SST. Unrecoverable.",
                wsrep_sst_method);
    throw wsrep::runtime_error("Failed to prepare for SST. Unrecoverable");
  }

  std::string ret;
  ret += method;
  ret.push_back('\0');
  ret += addr_out;

  const char *method_ptr(ret.data());
  const char *addr_ptr(ret.data() + strlen(method_ptr) + 1);
  WSREP_INFO("Prepared SST request: %s|%s", method_ptr, addr_ptr);

  if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
  sst_awaiting_callback = true;
  mysql_mutex_unlock(&LOCK_wsrep_sst);

  if (addr_out != addr_in) /* malloc'ed */
    free(const_cast<char *>(addr_out));

  return ret;
}

#if 0
// helper method for donors
static int sst_run_shell(const char *cmd_str, char **env, int max_tries) {
  int ret = 0;

  for (int tries = 1; tries <= max_tries; tries++) {
    // Launch the SST script and save pointer to its process:

    if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
    wsp::process proc(cmd_str, "r", env);
    sst_process = &proc;
    mysql_mutex_unlock(&LOCK_wsrep_sst);

    if (proc.pipe() && !proc.error()) {
      proc.wait();
    }

    // Clear the pointer to SST process:

    if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock(&LOCK_wsrep_sst);

    if ((ret = proc.error())) {
      // Try again if SST is not cancelled:
      if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
      if (!sst_cancelled) {
        mysql_mutex_unlock(&LOCK_wsrep_sst);
        WSREP_ERROR("Try %d/%d: '%s' failed: %d (%s)", tries, max_tries,
                    proc.cmd(), ret, strerror(ret));
        sleep(1);
      } else {
        mysql_mutex_unlock(&LOCK_wsrep_sst);
      }
    } else {
      WSREP_DEBUG("SST script successfully completed.");
      break;
    }
  }

  return -ret;
}
#endif

static void sst_reject_queries(bool close_conn) {
  WSREP_INFO("Rejecting client queries for the duration of SST.");
  if (true == close_conn) wsrep_close_client_connections(false, false);
}

wsrep_seqno_t wsrep_locked_seqno = WSREP_SEQNO_UNDEFINED;

/*
  Executes a query.  The second parameter, safe_query, if not NULL, will be used
  instead of the real query when logging an error (for security-senstive
queries).
**/
static int run_sql_command(THD *thd, const char *query,
                           const char *safe_query) {
  thd->set_query(query, strlen(query));

  Parser_state ps;
  if (ps.init(thd, thd->query().str, thd->query().length)) {
    WSREP_ERROR("SST query: %s failed", (safe_query ? safe_query : query));
    return -1;
  }

  dispatch_sql_command(thd, &ps, false);
  if (thd->is_error()) {
    int const err = thd->get_stmt_da()->mysql_errno();
    if (safe_query) {
      WSREP_WARN("error executing '%s' : %d", safe_query, err);
    } else {
      WSREP_WARN("error executing '%s' : %d (%s)", query, err,
                 thd->get_stmt_da()->message_text());
    }
    thd->clear_error();
    return err;
  }
  return 0;
}

static int sst_flush_tables(THD *thd) {
  WSREP_INFO("Flushing tables for SST...");

  int err;
  int not_used;
  err = run_sql_command(thd, "FLUSH TABLES WITH READ LOCK", NULL);
  if (err) {
    if (err == ER_UNKNOWN_SYSTEM_VARIABLE)
      WSREP_WARN("Was mysqld built with --with-innodb-disallow-writes ?");
    WSREP_ERROR("Failed to flush and lock tables");
    err = ECANCELED;
  } else {
    /* make sure logs are flushed after global read lock acquired */
    err = handle_reload_request(thd, REFRESH_ENGINE_LOG | REFRESH_BINARY_LOG,
                                (Table_ref *)0, &not_used);
  }

  if (err) {
    WSREP_ERROR("Failed to flush tables: %d (%s)", err, strerror(err));
  } else {
    WSREP_INFO("Table flushing completed.");
    const char base_name[] = "tables_flushed";
    ssize_t const full_len =
        strlen(mysql_real_data_home) + strlen(base_name) + 2;
    char *real_name = static_cast<char *>(alloca(full_len));
    sprintf(real_name, "%s/%s", mysql_real_data_home, base_name);
    char *tmp_name = static_cast<char *>(alloca(full_len + 4));
    sprintf(tmp_name, "%s.tmp", real_name);

    FILE *file = fopen(tmp_name, "w+");
    if (0 == file) {
      err = errno;
      WSREP_ERROR("Failed to open '%s': %d (%s)", tmp_name, err, strerror(err));
    } else {
      Wsrep_server_state &server_state = Wsrep_server_state::instance();
      std::ostringstream uuid_oss;

      uuid_oss << server_state.current_view().state_id().id();

      fprintf(file, "%s:%lld\n", uuid_oss.str().c_str(),
              server_state.pause_seqno().get());
      fsync(fileno(file));
      fclose(file);
      if (rename(tmp_name, real_name) == -1) {
        err = errno;
        WSREP_ERROR("Failed to rename '%s' to '%s': %d (%s)", tmp_name,
                    real_name, err, strerror(err));
      }
    }
  }

  return err;
}

static void sst_disallow_writes(THD *thd, bool yes) {
  /* rsync takes a snapshot. All objects of snapshot has to be consistent
  at given point in time. If say n objects are being backed up and if rsync
  reads state = at-timestamp(X) of object-1 and state = at-timestamp(Y) of
  object-2 then rsync will not work. This co-ordination is achieved by blocking
  writes in InnoDB. Note: FLUSH TABLE WITH READ LOCK will block external action
  but internal ongoing action needs to be blocked too. */
  char query_str[128] = {
      0,
  };
  ssize_t const query_max = sizeof(query_str) - 1;
  snprintf(query_str, query_max, "SET GLOBAL innodb_disallow_writes=%s",
           yes ? "true" : "false");

  if (run_sql_command(thd, query_str, NULL)) {
    WSREP_ERROR("Failed to disallow InnoDB writes");
  }
  return;
}

// Password characters are limited, because we are using these passwords
// within a bash script.
const std::string g_allowed_pwd_chars(
    "qwertyuiopasdfghjklzxcvbnm1234567890"
    "QWERTYUIOPASDFGHJKLZXCVBNM");
const std::string get_allowed_pwd_chars() { return g_allowed_pwd_chars; }

static void generate_password(std::string *password, int size) {
  std::stringstream ss;
  bool srnd;
  constexpr const char *prefix = "yx9!A-";
  ss << prefix;
  size -= strlen(prefix);
  while (size > 0) {
    int ch = ((int)(my_rnd_ssl(&srnd) * 100)) % get_allowed_pwd_chars().size();
    ss << get_allowed_pwd_chars()[ch];
    --size;
  }
  password->assign(ss.str());
}

static void srv_session_error_handler(void *ctx MY_ATTRIBUTE((unused)),
                                      unsigned int sql_errno,
                                      const char *err_msg) {
  switch (sql_errno) {
    case ER_CON_COUNT_ERROR:
      WSREP_ERROR(
          "Can't establish an internal server connection to "
          "execute operations since the server "
          "does not have available connections, please "
          "increase @@GLOBAL.MAX_CONNECTIONS. Server error: %i.",
          sql_errno);
      break;
    default:
      WSREP_ERROR(
          "Can't establish an internal server connection to "
          "execute operations. Server error: %i. "
          "Server error message: %s",
          sql_errno, err_msg);
  }
}

static void cleanup_server_session(MYSQL_SESSION session,
                                   bool initialize_thread) {
  srv_session_close(session);
  if (initialize_thread) srv_session_deinit_thread();
}

static MYSQL_SESSION setup_server_session(bool initialize_thread) {
  MYSQL_SESSION session = NULL;

  if (initialize_thread) {
    if (srv_session_init_thread(NULL)) {
      WSREP_ERROR("Failed to initialize the server session thread.");
      return NULL;
    }
  }

  session = srv_session_open(srv_session_error_handler, NULL);

  if (session == NULL) {
    if (initialize_thread) srv_session_deinit_thread();
    WSREP_ERROR("Failed to open a session for the SST commands");
    return NULL;
  }

  MYSQL_SECURITY_CONTEXT sc;
  if (thd_get_security_context(srv_session_info_get_thd(session), &sc)) {
    cleanup_server_session(session, initialize_thread);
    WSREP_ERROR(
        "Failed to fetch the security context when contacting the server");
    return NULL;
  }
  if (security_context_lookup(sc, PXC_INTERNAL_SESSION_USER.str,
                              PXC_INTERNAL_SESSION_HOST.str, NULL, NULL)) {
    cleanup_server_session(session, initialize_thread);
    WSREP_ERROR("Error accessing server with user:%s@%s",
                PXC_INTERNAL_SESSION_USER.str, PXC_INTERNAL_SESSION_HOST.str);
    return NULL;
  }
  // Turn wsrep off here (because the server session has it's own THD object)
  session->get_thd()->variables.wsrep_on = false;
  session->get_thd()->wsrep_sst_donor = true;
  return session;
}

static uint server_session_execute(MYSQL_SESSION session, std::string query,
                                   const char *safe_query,
                                   bool ignore_error = false) {
  COM_DATA cmd;
  wsp::Sql_resultset rset;

  memset(&cmd, 0, sizeof(cmd));
  cmd.com_query.query = query.c_str();
  cmd.com_query.length = static_cast<unsigned int>(query.length());
  wsp::Sql_service_context_base *ctx = new wsp::Sql_service_context(&rset);
  uint err(0);

  /* execute sql command */
  command_service_run_command(
      session, COM_QUERY, &cmd, &my_charset_utf8mb4_general_ci,
      &wsp::Sql_service_context_base::sql_service_callbacks,
      CS_TEXT_REPRESENTATION, ctx);
  delete ctx;

  err = rset.sql_errno();
  if (err && !ignore_error) {
    // an error occurred, retrieve the status/message
    if (safe_query) {
      WSREP_ERROR("Command execution failed (%d) : %s", err, safe_query);
    } else {
      WSREP_ERROR("Command execution failed (%d) : %s : %s", err,
                  rset.err_msg().c_str(), query.c_str());
    }
  }
  return err;
}

static int wsrep_create_sst_user(bool initialize_thread, const char *password) {
  int err = 0;
  int const auth_len = 512;
  char auth_buf[auth_len];
  MYSQL_SESSION session = NULL;

  // This array is filled with pairs of entries
  // The first entry is the actual query to be run
  // The second entry is the string to be displayed if the query fails
  //  (this can be NULL, in which case the actual query will be used)
  const char *cmds[] = {
    "SET SESSION sql_log_bin = OFF;",
    nullptr,
    "DROP USER IF EXISTS 'mysql.pxc.sst.user'@localhost;",
    nullptr,
    "CREATE USER 'mysql.pxc.sst.user'@localhost "
    " IDENTIFIED BY '%s' ACCOUNT LOCK;",
    "CREATE USER mysql.pxc.sst.user IDENTIFIED WITH * BY * ACCOUNT LOCK",
    "GRANT 'mysql.pxc.sst.role'@localhost TO 'mysql.pxc.sst.user'@localhost;", nullptr,
    "SET DEFAULT ROLE 'mysql.pxc.sst.role'@localhost to 'mysql.pxc.sst.user'@localhost;", nullptr,
    "ALTER USER 'mysql.pxc.sst.user'@localhost ACCOUNT UNLOCK;",
    nullptr,
    nullptr,
    nullptr
  };

  wsrep_allow_server_session = true;
  session = setup_server_session(initialize_thread);
  if (!session) {
    wsrep_allow_server_session = false;
    return ECANCELED;
  }

  for (int index = 0; !err && cmds[index]; index += 2) {
    int ret;
    ret = snprintf(auth_buf, auth_len, cmds[index], password);
    if (ret < 0 || ret >= auth_len) {
      WSREP_ERROR("wsrep_create_sst_user() : snprintf() failed: %d", ret);
      err = (ret < 0 ? ret : -EMSGSIZE);
      break;
    }
    // Ignore errors: also ignored in the boostrap code
    err = server_session_execute(session, auth_buf, cmds[index + 1], true);
  }

  // Overwrite query (clear out any sensitive data)
  ::memset(auth_buf, 0, auth_len);

  cleanup_server_session(session, initialize_thread);
  wsrep_allow_server_session = false;
  return err;
}

int wsrep_remove_sst_user(bool initialize_thread) {
  int err = 0;
  MYSQL_SESSION session = NULL;

  // Skip the attempt to  mysql.pxc.sst.user in case the server was started with
  // --skip-grant-tables option. It would fail enyway with error.
  // This will prevent writing out error to error log.
  if (skip_grant_tables()) {
    return ECANCELED;
  }
  // This array is filled with pairs of entries
  // The first entry is the actual query to be run
  // The second entry is the string to be displayed if the query fails
  //  (this can be NULL, in which case the actual query will be used)
  const char *cmds[] = {"SET SESSION sql_log_bin = OFF;",
                        nullptr,
                        "SET SESSION lock_wait_timeout = 1;",
                        nullptr,
                        "DROP USER IF EXISTS 'mysql.pxc.sst.user'@localhost;",
                        nullptr,
                        nullptr,
                        nullptr};

  wsrep_allow_server_session = true;
  session = setup_server_session(initialize_thread);
  if (!session) {
    wsrep_allow_server_session = false;
    return ECANCELED;
  }

  for (int index = 0; !err && cmds[index]; index += 2) {
    // Ignore errors, as those are also ignored during user creation
    err = server_session_execute(session, cmds[index], cmds[index + 1], true);
  }

  cleanup_server_session(session, initialize_thread);
  wsrep_allow_server_session = false;
  return err;
}

static void *sst_donor_thread(void *a) {
  sst_thread_arg *arg = (sst_thread_arg *)a;

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_register_thread(key_THREAD_wsrep_sst_donor);
#endif /* HAVE_PSI_INTERFACE */

  WSREP_INFO("Initiating SST/IST transfer on DONOR side (%s)", arg->cmd);

  int err = 1;
  bool locked = false;

  const char *out = NULL;
  int const out_len = 1024;
  char out_buf[out_len];

  // Generate the random password
  std::string password;
  generate_password(&password, 32);

  wsrep_uuid_t ret_uuid = WSREP_UUID_UNDEFINED;
  wsrep_seqno_t ret_seqno = WSREP_SEQNO_UNDEFINED;  // seqno of complete SST

  wsp::thd thd(false);  // we turn off wsrep_on for this THD so that it can
                        // operate with wsrep_ready == OFF

  thd.ptr->wsrep_sst_donor = true;
  // Create the SST auth user
  err = wsrep_create_sst_user(true, password.c_str());

  if (err) {
#ifdef HAVE_PSI_INTERFACE
    wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

    /* Inform server about SST script startup and release TO isolation */
    mysql_mutex_lock(&arg->LOCK_wsrep_sst_thread);
    arg->err = -err;
    mysql_cond_signal(&arg->COND_wsrep_sst_thread);
    mysql_mutex_unlock(
        &arg->LOCK_wsrep_sst_thread);  //! @note arg is unusable after that.

    return NULL;
  }

  // Launch the SST script and save pointer to its process:

  if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
  wsp::process proc(arg->cmd, "rw", arg->env, !err /* execute_immediately */);

  pthread_t logger_thd = 0;
  sst_logger_thread_arg logger_arg(proc.err_pipe());
  proc.clear_err_pipe();
  err = start_sst_logger_thread(&logger_arg, &logger_thd);

  if (!err) {
    int ret;
    ret = fprintf(proc.write_pipe(),
                  "sst_user=mysql.pxc.sst.user\n"
                  "sst_password=%s\n",
                  password.c_str());
    if (ret < 0) {
      WSREP_ERROR("sst_donor_thread(): fprintf() failed: %d", ret);
      err = (ret < 0 ? ret : -EMSGSIZE);
    }

    // Close the pipe, so that the other side gets an EOF
    proc.close_write_pipe();
  }
  password.assign(password.length(), 0);  // overwrite password value

  sst_process = &proc;
  mysql_mutex_unlock(&LOCK_wsrep_sst);

  if (!err) err = proc.error();

  /* Inform server about SST script startup and release TO isolation */
  mysql_mutex_lock(&arg->LOCK_wsrep_sst_thread);
  arg->err = -err;
  mysql_cond_signal(&arg->COND_wsrep_sst_thread);
  mysql_mutex_unlock(
      &arg->LOCK_wsrep_sst_thread);  //! @note arg is unusable after that.

  if (proc.pipe() && !err) {
  wait_signal:
    out = my_fgets(out_buf, out_len, proc.pipe());

    if (out) {
      const char magic_flush[] = "flush tables";
      const char magic_cont[] = "continue";
      const char magic_done[] = "done";

      if (!strcasecmp(out, magic_flush)) {
        err = sst_flush_tables(thd.ptr);
        if (!err) {
          sst_disallow_writes(thd.ptr, true);
          locked = true;
          goto wait_signal;
        }
      } else if (!strcasecmp(out, magic_cont)) {
        if (locked) {
          sst_disallow_writes(thd.ptr, false);
          thd.ptr->global_read_lock.unlock_global_read_lock(thd.ptr);
          locked = false;
        }
        err = 0;
        goto wait_signal;
      } else if (!strncasecmp(out, magic_done, strlen(magic_done))) {
        err = sst_scan_uuid_seqno(out + strlen(magic_done) + 1, &ret_uuid,
                                  &ret_seqno);
      } else {
        WSREP_WARN("Received unknown signal: '%s'", out);
      }
    } else {
      if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
        // Print error message if SST is not cancelled:
#if 0
      if (!sst_cancelled)
      {
         WSREP_ERROR("Failed to read from: %s", proc.cmd());
      }
#endif
      // Clear the pointer to SST process:
      sst_process = NULL;
      mysql_mutex_unlock(&LOCK_wsrep_sst);
      proc.wait();
      goto skip_clear_pointer;
    }

    // Clear the pointer to SST process:
    if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock(&LOCK_wsrep_sst);

  skip_clear_pointer:
    if (!err && proc.error()) err = proc.error();
  } else {
    // Clear the pointer to SST process:
    if (mysql_mutex_lock(&LOCK_wsrep_sst)) abort();
    sst_process = NULL;
    mysql_mutex_unlock(&LOCK_wsrep_sst);
    WSREP_ERROR("Failed to execute: %s : %d (%s)", proc.cmd(), err,
                strerror(err));
  }

  wsrep_remove_sst_user(true);

  if (locked)  // don't forget to unlock server before return
  {
    sst_disallow_writes(thd.ptr, false);
    thd.ptr->global_read_lock.unlock_global_read_lock(thd.ptr);
  }

  // signal to donor that SST is over
  wsrep::gtid gtid(
      wsrep::id(ret_uuid.data, sizeof(ret_uuid.data)),
      wsrep::seqno(err ? wsrep::seqno::undefined() : wsrep::seqno(ret_seqno)));
  Wsrep_server_state::instance().sst_sent(gtid, -err);
  proc.wait();

  // The process has exited, so the logger thread should
  // also have exited
  if (logger_thd) pthread_join(logger_thd, NULL);

#ifdef HAVE_PSI_INTERFACE
  wsrep_pfs_delete_thread();
#endif /* HAVE_PSI_INTERFACE */

  return NULL;
}

static int sst_donate_other(const char *method, const char *addr,
                            const wsrep::gtid &gtid, bool bypass,
                            char **env)  // carries auth info
{
  int const cmd_len = 4096;
  wsp::string cmd_str(cmd_len);

  if (!cmd_str()) {
    WSREP_ERROR(
        "sst_donate_other(): "
        "could not allocate cmd buffer of %d bytes",
        cmd_len);
    return -ENOMEM;
  }

  const char *binlog_opt = "";
  char *binlog_opt_val = NULL;

  int ret;
  if ((ret = generate_binlog_opt_val(&binlog_opt_val))) {
    WSREP_ERROR("sst_donate_other(): generate_binlog_opt_val() failed: %d",
                ret);
    return ret;
  }
  if (strlen(binlog_opt_val)) binlog_opt = WSREP_SST_OPT_BINLOG;

  std::ostringstream uuid_oss;
  uuid_oss << gtid.id();

  ret = snprintf(
      cmd_str(), cmd_len,
      "wsrep_sst_%s " WSREP_SST_OPT_ROLE " 'donor' " WSREP_SST_OPT_ADDR
      " '%s' " WSREP_SST_OPT_SOCKET " '%s' " WSREP_SST_OPT_DATA
      " '%s' " WSREP_SST_OPT_BASEDIR " '%s' " WSREP_SST_OPT_PLUGINDIR
      " '%s' " WSREP_SST_OPT_CONF " '%s' " WSREP_SST_OPT_CONF_SUFFIX
      " '%s' " WSREP_SST_OPT_VERSION
      " '%s' "
      " %s '%s' " WSREP_SST_OPT_GTID
      " '%s:%lld' "
      "%s",
      method, addr, mysqld_unix_port, mysql_real_data_home,
      mysql_home_ptr ? mysql_home_ptr : "",
      opt_plugin_dir_ptr ? opt_plugin_dir_ptr : "", wsrep_defaults_file,
      wsrep_defaults_group_suffix, MYSQL_SERVER_VERSION MYSQL_SERVER_SUFFIX_DEF,
      binlog_opt, binlog_opt_val, uuid_oss.str().c_str(), gtid.seqno().get(),
      bypass ? " " WSREP_SST_OPT_BYPASS : "");
  DBUG_EXECUTE_IF("wsrep_sst_donor_skip", {
    ret = snprintf(cmd_str() + strlen(cmd_str()), cmd_len - strlen(cmd_str()),
                   WSREP_SST_OPT_DEBUG " '%s'", "wsrep_sst_donor_skip");
  });
  my_free(binlog_opt_val);

  if (ret < 0 || ret >= cmd_len) {
    WSREP_ERROR("sst_donate_other(): snprintf() failed: %d", ret);
    return (ret < 0 ? ret : -EMSGSIZE);
  }

  if (!bypass && wsrep_sst_donor_rejects_queries) sst_reject_queries(false);

  pthread_t tmp;
  sst_thread_arg arg(cmd_str(), env);
  mysql_mutex_lock(&arg.LOCK_wsrep_sst_thread);
  ret = pthread_create(&tmp, NULL, sst_donor_thread, &arg);
  if (ret) {
    WSREP_ERROR("sst_donate_other(): pthread_create() failed: %d (%s)", ret,
                strerror(ret));
    return -ret;
  }
  mysql_cond_wait(&arg.COND_wsrep_sst_thread, &arg.LOCK_wsrep_sst_thread);

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
static bool is_sst_request_valid(const std::string &msg) {
  size_t method_len = strlen(msg.c_str());

  if (method_len == 0) {
    return false;
  }

  std::string method = msg.substr(0, method_len);

  // Is this method allowed?
  auto res = std::find(std::begin(allowed_sst_methods),
                       std::end(allowed_sst_methods), method);
  if (res == std::end(allowed_sst_methods)) {
    return false;
  }

  const char *data_ptr = msg.c_str() + method_len + 1;
  size_t data_len = strlen(data_ptr);

  // method + null + data + null
  if (method_len + 1 + data_len + 1 != msg.length()) {
    // Someone tries to piggyback after 2nd null
    return false;
  }

  if (data_len > 0) {
    /* We allow custom sst scripts, so data can be anything.

      We could create and maintain the list of forbidden
      characters and the ways they could be used to inject the command to
      the OS. However this approach seems to be too error prone.
      Instead of this we will just allow alpha-num + a few special characters
      (colon, slash, dot, underscore, square brackets, hyphen). */
    std::string data = msg.substr(method_len + 1, data_len);
    static const std::regex allowed_chars_regex("[\\w:/.[\\]@-]+");
    if (!std::regex_match(data, allowed_chars_regex)) {
      return false;
    }
  }
  return true;
}

int wsrep_sst_donate(const std::string &msg, const wsrep::gtid &current_gtid,
                     const bool bypass) {
  /* This will be reset when sync callback is called.
   * Should we set wsrep_ready to false here too? */
  local_status.set(wsrep::server_state::s_donor);

  if (!is_sst_request_valid(msg)) {
    std::ostringstream ss;
    std::for_each(std::begin(msg), std::end(msg), [&ss](char ch) {
      if (ch != 0)
        ss << ch;
      else
        ss << "<nullptr>";
    });
    WSREP_ERROR("Invalid sst_request: %s", ss.str().c_str());
    return WSREP_CB_FAILURE;
  }

  const char *method = msg.data();
  size_t method_len = strlen(method);
  const char *data = method + method_len + 1;

  DBUG_EXECUTE_IF("wsrep_sst_donate_cb_fails", { return WSREP_CB_FAILURE; });

  wsp::env env(NULL);
  if (env.error()) {
    WSREP_ERROR("wsrep_sst_donate_cb(): env var ctor failed: %d", -env.error());
    return WSREP_CB_FAILURE;
  }

#if defined(HAVE_ASAN)
  reset_ld_preload(env);
#endif

#if 0
  /* Wait for wsrep-SE to initialize that also signals
  completion of init_server_component which is important before we initiate
  any meangiful action especially DONOR action from this node. */
  while (!wsrep_is_SE_initialized()) {
    sleep(1);
    THD *applier_thd = static_cast<THD *>(recv_ctx);
    if (applier_thd->killed == THD::KILL_CONNECTION) return WSREP_CB_FAILURE;
  }
#endif
  DBUG_EXECUTE_IF("halt_before_sst_donate", {
    const char action[] =
        "now SIGNAL stopped_before_sst_donate WAIT_FOR continue";
    assert(!debug_sync_set_action(current_thd, STRING_WITH_LEN(action)));
  };);

  int ret;
  ret = sst_donate_other(method, data, current_gtid, bypass, env());

  /* Above methods should return 0 in case of success and negative value
   * in case of failure. If we have any positive value here it means that we
   * handle errors in above functions in the wrong way */
  assert(ret <= 0);
  return (ret >= 0 ? WSREP_CB_SUCCESS : WSREP_CB_FAILURE);
}
