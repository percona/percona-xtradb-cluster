/* Copyright (c) 2015 MariaDB Corporation Ab
                 2018 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef MYSQL_SERVICE_WSREP_INCLUDED
#define MYSQL_SERVICE_WSREP_INCLUDED

#include "my_inttypes.h"
#include "my_thread_local.h"  // my_thread_id

enum Wsrep_service_key_type {
  WSREP_SERVICE_KEY_SHARED,
  WSREP_SERVICE_KEY_REFERENCE,
  WSREP_SERVICE_KEY_UPDATE,
  WSREP_SERVICE_KEY_EXCLUSIVE
};

extern ulong wsrep_debug;
extern bool wsrep_log_conflicts;
extern bool wsrep_certify_nonPK;
extern bool wsrep_load_data_splitting;
extern bool wsrep_recovery;
extern long wsrep_protocol_version;

/* Must match to definition in sql/mysqld.h */
typedef int64 query_id_t;

class THD;

/* Return true if wsrep is enabled for a thd. This means that
   wsrep is enabled globally and the thd has wsrep on */
extern "C" bool wsrep_on(const THD *thd);

/* Lock thd wsrep lock */
extern "C" void wsrep_thd_LOCK(const THD *thd);

/* Unlock thd wsrep lock */
extern "C" void wsrep_thd_UNLOCK(const THD *thd);

/* Return thd client state string */
extern "C" const char *wsrep_thd_client_state_str(const THD *thd);

/* Return thd client mode string */
extern "C" const char *wsrep_thd_client_mode_str(const THD *thd);

/* Return thd transaction state string */
extern "C" const char *wsrep_thd_transaction_state_str(const THD *thd);

extern "C" const char *wsrep_thd_query(const THD *thd);

/* Return current transaction id */
extern "C" query_id_t wsrep_thd_transaction_id(const THD *thd);

extern "C" long long wsrep_thd_trx_seqno(const THD *thd);

/* Mark thd own transaction as aborted */
extern "C" void wsrep_thd_self_abort(THD *thd);

extern const char *wsrep_sr_table_name_full;
extern "C" const char *wsrep_get_sr_table_name();

extern "C" ulong wsrep_get_debug();

/* Return true if thd is in replicating mode */
extern "C" bool wsrep_thd_is_local(const THD *thd);

/* Return true if thd is in high priority mode */
/* todo: rename to is_high_priority() */
extern "C" bool wsrep_thd_is_applying(const THD *thd);

/* Return true if thd is in TOI mode */
extern "C" bool wsrep_thd_is_toi(const THD *thd);

/* Return true if thd is in replicating TOI mode */
extern "C" bool wsrep_thd_is_local_toi(const THD *thd);

/* Return true if thd is in RSU mode */
extern "C" bool wsrep_thd_is_in_rsu(const THD *thd);

/* Return true if thd is in BF mode, either high_priority or TOI */
extern "C" bool wsrep_thd_is_BF(const THD *thd, bool sync);

/* Return true if thd is streaming */
extern "C" bool wsrep_thd_is_SR(const THD *thd);

extern "C" void wsrep_handle_SR_rollback(THD *BF_thd, THD *victim_thd);

/* BF abort victim_thd */
extern "C" bool wsrep_thd_bf_abort(const THD *bf_thd, THD *victim_thd,
                                   bool signal);

/* Return true if thd should skip locking. This means that the thd
   is operating on shared resource inside commit order critical section. */
extern "C" bool wsrep_thd_skip_locking(const THD *thd);

/* Return true if left thd is ordered before right thd */
extern "C" bool wsrep_thd_order_before(const THD *left, const THD *right);

/* Return true if thd is aborting */
extern "C" bool wsrep_thd_is_aborting(const THD *thd);

struct wsrep_key;
struct wsrep_key_array;
extern "C" int wsrep_thd_append_key(THD *thd, const struct wsrep_key *key,
                                    int n_keys, enum Wsrep_service_key_type);

extern "C" void wsrep_commit_ordered(THD *thd);

extern "C" bool wsrep_consistency_check(const THD *thd);

extern "C" void wsrep_thd_mark_split_trx(THD *thd, bool split_trx);

extern "C" my_thread_id wsrep_thd_thread_id(THD *thd);

extern "C" bool get_wsrep_recovery();
#endif /* MYSQL_SERVICE_WSREP_INCLUDED */
