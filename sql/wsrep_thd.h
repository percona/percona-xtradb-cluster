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

#include "mysql/psi/mysql_stage.h"
#include "mysql/psi/psi.h"

#ifdef HAVE_PSI_THREAD_INTERFACE
#define wsrep_pfs_register_thread(key)                      \
do {                                                        \
        struct PSI_thread* psi = PSI_THREAD_CALL(new_thread)(key, NULL, 0);\
        PSI_THREAD_CALL(set_thread_os_id)(psi);             \
        PSI_THREAD_CALL(set_thread)(psi);                   \
} while (0)

/* This macro delist the current thread from performance schema */
#define wsrep_pfs_delete_thread()                           \
do {                                                        \
        PSI_THREAD_CALL(delete_current_thread)();           \
} while (0)


# define wsrep_register_pfs_file_open_begin(state, locker, key, op, name,     \
                                      src_file, src_line)               \
do {                                                                    \
        locker = PSI_FILE_CALL(get_thread_file_name_locker)(            \
                state, key, op, name, &locker);                         \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(start_file_open_wait)(                    \
                        locker, src_file, src_line);                    \
        }                                                               \
} while (0)

# define wsrep_register_pfs_file_open_end(locker, file)                       \
do {                                                                    \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(end_file_open_wait_and_bind_to_descriptor)(\
                        locker, file);                                  \
        }                                                               \
} while (0)

# define wsrep_register_pfs_file_close_begin(state, locker, key, op, name,    \
                                      src_file, src_line)               \
do {                                                                    \
        locker = PSI_FILE_CALL(get_thread_file_name_locker)(            \
                state, key, op, name, &locker);                         \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(start_file_close_wait)(                   \
                        locker, src_file, src_line);                    \
        }                                                               \
} while (0)

# define wsrep_register_pfs_file_close_end(locker, result)                    \
do {                                                                    \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(end_file_close_wait)(                     \
                        locker, result);                                \
        }                                                               \
} while (0)

# define wsrep_register_pfs_file_io_begin(state, locker, file, count, op,     \
                                    src_file, src_line)                 \
do {                                                                    \
        locker = PSI_FILE_CALL(get_thread_file_descriptor_locker)(      \
                state, file, op);                                       \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(start_file_wait)(                         \
                        locker, count, src_file, src_line);             \
        }                                                               \
} while (0)

# define wsrep_register_pfs_file_io_end(locker, count)                        \
do {                                                                    \
        if (locker != NULL) {                                           \
                PSI_FILE_CALL(end_file_wait)(locker, count);            \
        }                                                               \
} while (0)


#endif /* HAVE_PSI_THREAD_INTERFACE */

int wsrep_show_bf_aborts (THD *thd, SHOW_VAR *var, char *buff);
void wsrep_client_rollback(THD *thd);
void wsrep_replay_sp_transaction(THD* thd);
void wsrep_replay_transaction(THD *thd);
void wsrep_create_appliers(long threads);
void wsrep_create_rollbacker();

int  wsrep_abort_thd(void *bf_thd_ptr, void *victim_thd_ptr,
                                my_bool signal);

extern void  wsrep_thd_set_PA_safe(void *thd_ptr, my_bool safe);
extern my_bool  wsrep_thd_is_BF(void *thd_ptr, my_bool sync);
extern int wsrep_thd_conflict_state(void *thd_ptr, my_bool sync);
//extern "C" my_bool  wsrep_thd_is_BF(void *thd_ptr, my_bool sync);
extern "C" my_bool  wsrep_thd_is_BF_or_commit(void *thd_ptr, my_bool sync);
extern "C" my_bool  wsrep_thd_is_local(void *thd_ptr, my_bool sync);
int  wsrep_thd_in_locking_session(void *thd_ptr);

#endif /* WSREP_THD_H */
