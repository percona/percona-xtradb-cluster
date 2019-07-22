/* Copyright (C) 2013 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef WSREP_THD_H
#define WSREP_THD_H

#include "sql_class.h"
#include "service_wsrep.h"
#include "wsrep/client_state.hpp"
#include "wsrep_utils.h"
#include <deque>
#include "sql/sql_lex.h"

#include "mysql/psi/mysql_stage.h"
#include "mysql/psi/psi_file.h"
#include "mysql/psi/psi_thread.h"

#ifdef HAVE_PSI_THREAD_INTERFACE
#define wsrep_pfs_register_thread(key)                                  \
  do {                                                                  \
    struct PSI_thread *psi = PSI_THREAD_CALL(new_thread)(key, NULL, 0); \
    PSI_THREAD_CALL(set_thread_os_id)(psi);                             \
    PSI_THREAD_CALL(set_thread)(psi);                                   \
  } while (0)

/* This macro delist the current thread from performance schema */
#define wsrep_pfs_delete_thread()             \
  do {                                        \
    PSI_THREAD_CALL(delete_current_thread)(); \
  } while (0)

#define wsrep_register_pfs_file_open_begin(state, locker, key, op, name,      \
                                           src_file, src_line)                \
  do {                                                                        \
    locker = PSI_FILE_CALL(get_thread_file_name_locker)(state, key, op, name, \
                                                        &locker);             \
    if (locker != NULL) {                                                     \
      PSI_FILE_CALL(start_file_open_wait)(locker, src_file, src_line);        \
    }                                                                         \
  } while (0)

#define wsrep_register_pfs_file_open_end(locker, file)                        \
  do {                                                                        \
    if (locker != NULL) {                                                     \
      PSI_FILE_CALL(end_file_open_wait_and_bind_to_descriptor)(locker, file); \
    }                                                                         \
  } while (0)

#define wsrep_register_pfs_file_close_begin(state, locker, key, op, name,     \
                                            src_file, src_line)               \
  do {                                                                        \
    locker = PSI_FILE_CALL(get_thread_file_name_locker)(state, key, op, name, \
                                                        &locker);             \
    if (locker != NULL) {                                                     \
      PSI_FILE_CALL(start_file_close_wait)(locker, src_file, src_line);       \
    }                                                                         \
  } while (0)

#define wsrep_register_pfs_file_close_end(locker, result) \
  do {                                                    \
    if (locker != NULL) {                                 \
      PSI_FILE_CALL(end_file_close_wait)(locker, result); \
    }                                                     \
  } while (0)

#define wsrep_register_pfs_file_io_begin(state, locker, file, count, op,   \
                                         src_file, src_line)               \
  do {                                                                     \
    locker =                                                               \
        PSI_FILE_CALL(get_thread_file_descriptor_locker)(state, file, op); \
    if (locker != NULL) {                                                  \
      PSI_FILE_CALL(start_file_wait)(locker, count, src_file, src_line);   \
    }                                                                      \
  } while (0)

#define wsrep_register_pfs_file_io_end(locker, count) \
  do {                                                \
    if (locker != NULL) {                             \
      PSI_FILE_CALL(end_file_wait)(locker, count);    \
    }                                                 \
  } while (0)

#endif /* HAVE_PSI_THREAD_INTERFACE */

class Wsrep_thd_queue {
 public:
  Wsrep_thd_queue(THD *t) : thd(t) {
    mysql_mutex_init(key_LOCK_wsrep_thd_queue, &LOCK_wsrep_thd_queue,
                     MY_MUTEX_INIT_FAST);
    mysql_cond_init(key_COND_wsrep_thd_queue, &COND_wsrep_thd_queue);
  }
  ~Wsrep_thd_queue() {
    mysql_mutex_destroy(&LOCK_wsrep_thd_queue);
    mysql_cond_destroy(&COND_wsrep_thd_queue);
  }
  bool push_back(THD *thd) {
    DBUG_ASSERT(thd);
    wsp::auto_lock lock(&LOCK_wsrep_thd_queue);
    std::deque<THD *>::iterator it = queue.begin();
    while (it != queue.end()) {
      if (*it == thd) {
        return true;
      }
      it++;
    }
    queue.push_back(thd);
    mysql_cond_signal(&COND_wsrep_thd_queue);
    return false;
  }
  THD *pop_front() {
    wsp::auto_lock lock(&LOCK_wsrep_thd_queue);
    while (queue.empty()) {
      if (thd->killed != THD::NOT_KILLED) return NULL;

      thd->current_mutex = &LOCK_wsrep_thd_queue;
      thd->current_cond = &COND_wsrep_thd_queue;

      mysql_cond_wait(&COND_wsrep_thd_queue, &LOCK_wsrep_thd_queue);

      thd->current_mutex = 0;
      thd->current_cond = 0;
    }
    THD *ret = queue.front();
    queue.pop_front();
    return ret;
  }

 private:
  THD *thd;
  std::deque<THD *> queue;
  mysql_mutex_t LOCK_wsrep_thd_queue;
  mysql_cond_t COND_wsrep_thd_queue;
};

void wsrep_prepare_bf_thd(THD *, struct wsrep_thd_shadow *);
void wsrep_return_from_bf_mode(THD *, struct wsrep_thd_shadow *);

int wsrep_show_bf_aborts(THD *thd, SHOW_VAR *var, char *buff);
void wsrep_replay_transaction(THD *thd);
void wsrep_create_appliers(long threads);
void wsrep_create_rollbacker();

bool wsrep_bf_abort(const THD*, THD*);
int wsrep_abort_thd(const THD *bf_thd_ptr, THD *victim_thd_ptr, bool signal);

extern void wsrep_thd_set_PA_safe(void *thd_ptr, bool safe);
THD* wsrep_start_SR_THD(char *thread_stack);
void wsrep_end_SR_THD(THD* thd);

/**
   Helper functions to override error status

   In many contexts it is desirable to mask the original error status
   set for THD or it is necessary to change OK status to error.
   This function implements the common logic for the most
   of the cases.

   Rules:
   * If the diagnostics are has OK or EOF status, override it unconditionally
   * If the error is either ER_ERROR_DURING_COMMIT or ER_LOCK_DEADLOCK
     it is usually the correct error status to be returned to client,
     so don't override those by default
 */

static inline void wsrep_override_error(THD *thd, uint error)
{
  DBUG_ASSERT(error != ER_ERROR_DURING_COMMIT);
  Diagnostics_area *da= thd->get_stmt_da();
  if (da->is_ok() ||
      da->is_eof() ||
      !da->is_set() ||
      (da->is_error() &&
       da->mysql_errno() != error &&
       da->mysql_errno() != ER_ERROR_DURING_COMMIT &&
       da->mysql_errno() != ER_LOCK_DEADLOCK))
  {
    da->reset_diagnostics_area();
    my_error(error, MYF(0));
  }
}

/**
   Override error with additional wsrep status.
 */
static inline void wsrep_override_error(THD *thd, uint error,
                                        enum wsrep::provider::status status,
                                        const char* errorstring = "") {
  Diagnostics_area *da = thd->get_stmt_da();
  if (da->is_ok() || !da->is_set() ||
      (da->is_error() && da->mysql_errno() != error &&
       da->mysql_errno() != ER_ERROR_DURING_COMMIT &&
       da->mysql_errno() != ER_LOCK_DEADLOCK)) {
    da->reset_diagnostics_area();
    my_error(error, MYF(0), status, errorstring);
  }
}

static inline void wsrep_override_error(THD *thd, wsrep::client_error ce,
                                        enum wsrep::provider::status status) {
  DBUG_ASSERT(ce != wsrep::e_success);
  switch (ce) {
    case wsrep::e_error_during_commit:
      wsrep_override_error(thd, ER_ERROR_DURING_COMMIT, status);
      break;
    case wsrep::e_deadlock_error:
      wsrep_override_error(thd, ER_LOCK_DEADLOCK);
      break;
    case wsrep::e_interrupted_error:
      wsrep_override_error(thd, ER_QUERY_INTERRUPTED);
      break;
    case wsrep::e_size_exceeded_error:
      wsrep_override_error(thd, ER_ERROR_DURING_COMMIT, status,
                           "Transaction size exceed set threshold");
      break;
    case wsrep::e_append_fragment_error:
      /* TODO: Figure out better error number */
      wsrep_override_error(thd, ER_ERROR_DURING_COMMIT, status);
      break;
    case wsrep::e_not_supported_error:
      wsrep_override_error(thd, ER_NOT_SUPPORTED_YET);
      break;
    case wsrep::e_timeout_error:
      wsrep_override_error(thd, ER_LOCK_WAIT_TIMEOUT);
      break;
    default:
      wsrep_override_error(thd, ER_UNKNOWN_ERROR);
      break;
  }
}

/**
   Helper function to log THD wsrep context.

   @param thd Pointer to THD
   @param message Optional message
   @param function Function where the call was made from
 */
static inline void wsrep_log_thd(THD *thd, const char *message,
                                 const char *function) {
  WSREP_DEBUG(
      "%s %s\n"
      "    thd: %u thd_ptr: %p client_mode: %s client_state: %s trx_state: "
      "%s\n"
      "    next_trx_id: %lld trx_id: %lld seqno: %lld\n"
      "    is_streaming: %d fragments: %zu\n"
      "    mysql_errno: %u message: %s\n"
#define WSREP_THD_LOG_QUERIES
#ifdef WSREP_THD_LOG_QUERIES
      "    command: %d query: %.72s"
#endif /* WSREP_OBSERVER_LOG_QUERIES */
      ,
      function, message ? message : "", thd->thread_id(), thd,
      wsrep_thd_client_mode_str(thd), wsrep_thd_client_state_str(thd),
      wsrep_thd_transaction_state_str(thd), (long long)thd->wsrep_next_trx_id(),
      (long long)thd->wsrep_trx_id(), (long long)wsrep_thd_trx_seqno(thd),
      thd->wsrep_trx().is_streaming(), thd->wsrep_sr().fragments().size(),
      (thd->get_stmt_da()->is_error() ? thd->get_stmt_da()->mysql_errno() : 0),
      (thd->get_stmt_da()->is_error() ? thd->get_stmt_da()->message_text() : "")
#ifdef WSREP_THD_LOG_QUERIES
          ,
      thd->lex->sql_command, WSREP_QUERY(thd)
#endif /* WSREP_OBSERVER_LOG_QUERIES */
  );
}

#define WSREP_LOG_THD(thd_, message_) \
  wsrep_log_thd(thd_, message_, __FUNCTION__)

#endif /* WSREP_THD_H */
