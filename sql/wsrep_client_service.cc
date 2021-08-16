/* Copyright 2018 Codership Oy <info@codership.com>

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

#include "wsrep_client_service.h"
#include "debug_sync.h"
#include "item_func.h"
#include "mysql/components/services/log_builtins.h"
#include "wsrep_applier.h" /* wsrep_apply_events() */
#include "wsrep_binlog.h"  /* wsrep_dump_rbr_buf() */
#include "wsrep_high_priority_service.h"
#include "wsrep_schema.h" /* remove_fragments() */
#include "wsrep_thd.h"
#include "wsrep_trans_observer.h"
#include "wsrep_xid.h"

#include "log.h"         /* stmt_has_updated_trans_table() */
#include "rpl_filter.h"  /* binlog_filter */
#include "rpl_rli.h"     /* Relay_log_info */
#include "sql_base.h"    /* close_temporary_table() */
#include "sql_class.h"   /* THD */
#include "sql_parse.h"   /* stmt_causes_implicit_commit() */
#include "transaction.h" /* trans_commit()... */
//#include "debug_sync.h"
#include "mysql/psi/mysql_thread.h" /* mysql_mutex_assert_owner() */

#if 0
namespace {
void debug_sync_caller(THD *thd, const char *sync_point) {
#ifdef ENABLED_DEBUG_SYNC_OUT
  debug_sync_set_action(thd, sync_point, strlen(sync_point));
#endif
#ifdef ENABLED_DEBUG_SYNC
  if (debug_sync_service)
    debug_sync_service(thd, sync_point, strlen(sync_point));
#endif
}
}  // namespace
#endif

Wsrep_client_service::Wsrep_client_service(THD *thd,
                                           Wsrep_client_state &client_state)
    : wsrep::client_service(), m_thd(thd), m_client_state(client_state) {}

void Wsrep_client_service::store_globals() { wsrep_store_threadvars(m_thd); }

void Wsrep_client_service::reset_globals() { wsrep_reset_threadvars(m_thd); }

bool Wsrep_client_service::interrupted(
    wsrep::unique_lock<wsrep::mutex> &lock WSREP_UNUSED) const {
  assert(m_thd == current_thd);
  /* Underlying mutex in lock object points to LOCK_wsrep_thd, which
     protects m_thd->wsrep_trx()
  */
  mysql_mutex_assert_owner(static_cast<mysql_mutex_t *>(lock.mutex().native()));
  bool ret = (m_thd->killed != THD::NOT_KILLED);
  if (ret) {
    WSREP_DEBUG("wsrep state is interrupted, THD::killed %d trx state %d",
                m_thd->is_killed(), m_thd->wsrep_trx().state());
  }
  return ret;
}

int Wsrep_client_service::prepare_data_for_replication() {
  assert(m_thd == current_thd);
  DBUG_ENTER("Wsrep_client_service::prepare_data_for_replication");
  size_t data_len = 0;
  IO_CACHE_binlog_cache_storage *cache = wsrep_get_trans_cache(m_thd, true);

  if (cache) {
    m_thd->binlog_flush_pending_rows_event(true);
    if (wsrep_write_cache(m_thd, cache, &data_len)) {
      WSREP_ERROR("Failed to create write-set from binlog, data_len: %zu",
                  data_len);
      // wsrep_override_error(m_thd, ER_ERROR_DURING_COMMIT);
      DBUG_RETURN(1);
    }
  }

  if (data_len == 0) {
    if (m_thd->get_stmt_da()->is_ok() &&
        m_thd->get_stmt_da()->affected_rows() > 0 && !binlog_filter->is_on() &&
        !m_thd->wsrep_trx().is_streaming()) {
      WSREP_DEBUG(
          "empty rbr buffer, query: %s, "
          "affected rows: %llu, "
          "changed tables: %d, "
          "sql_log_bin: %d",
          WSREP_QUERY(m_thd), m_thd->get_stmt_da()->affected_rows(),
          stmt_has_updated_trans_table(
              m_thd->get_transaction()->ha_trx_info(Transaction_ctx::STMT)),
          m_thd->variables.sql_log_bin);
    } else {
      WSREP_DEBUG("empty rbr buffer, query: %s", WSREP_QUERY(m_thd));
    }
  }
  DBUG_RETURN(0);
}

void Wsrep_client_service::cleanup_transaction() {
  assert(m_thd == current_thd);
  if (WSREP_EMULATE_BINLOG(m_thd)) wsrep_thd_binlog_trx_reset(m_thd);
  m_thd->wsrep_affected_rows = 0;

  m_thd->wsrep_skip_wsrep_GTID = false;
  m_thd->run_wsrep_commit_hooks = false;
  m_thd->run_wsrep_ordered_commit = false;
  m_thd->wsrep_enforce_group_commit = false;
  m_thd->wsrep_post_insert_error = false;
  m_thd->wsrep_stmt_transaction_rolled_back = false;
  m_thd->wsrep_force_savept_rollback = false;

  if (m_thd->wsrep_non_replicating_atomic_ddl) {
    /* Restore the sql_log_bin mode back to original value
    on completion of the non-replicating atomic DDL statement. */
    if (m_thd->variables.wsrep_saved_binlog_state ==
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_ENABLED) {
      m_thd->variables.option_bits |= OPTION_BIN_LOG;
    } else {
      m_thd->variables.option_bits &= ~OPTION_BIN_LOG;
    }
    if (m_thd->variables.wsrep_saved_binlog_internal_off_state) {
      m_thd->variables.option_bits |= OPTION_BIN_LOG_INTERNAL_OFF;
    } else {
      m_thd->variables.option_bits &= ~OPTION_BIN_LOG_INTERNAL_OFF;
    }
    m_thd->variables.wsrep_saved_binlog_internal_off_state = false;

    m_thd->variables.wsrep_saved_binlog_state =
        System_variables::wsrep_binlog_state_t::WSREP_BINLOG_NOTSET;
  }
  m_thd->wsrep_non_replicating_atomic_ddl = false;
}

int Wsrep_client_service::prepare_fragment_for_replication(
    wsrep::mutable_buffer &buffer, size_t& log_position) {
  assert(m_thd == current_thd);
  THD *thd = m_thd;
  DBUG_ENTER("Wsrep_client_service::prepare_fragment_for_replication");
  IO_CACHE_binlog_cache_storage *cache = wsrep_get_trans_cache(thd, true);
  thd->binlog_flush_pending_rows_event(true);

  if (!cache) {
    DBUG_RETURN(0);
  }

  const my_off_t saved_pos(cache->position());

  int ret = 0;
  size_t total_length = 0;
  unsigned char *read_pos = NULL;
  my_off_t read_len = 0;

  if (cache->begin(&read_pos, &read_len, thd->wsrep_sr().log_position())) {
    DBUG_RETURN(1);
  }

  if (read_len == 0 && cache->next(&read_pos, &read_len)) {
    DBUG_RETURN(1);
  }

  while (read_len > 0) {
    {
      total_length += read_len;
      if (total_length > wsrep_max_ws_size) {
        WSREP_WARN("transaction size limit (%lu) exceeded: %zu",
                   wsrep_max_ws_size, total_length);
        ret = 1;
        goto cleanup;
      }

      buffer.push_back(reinterpret_cast<const char *>(read_pos),
                       reinterpret_cast<const char *>(read_pos + read_len));
      cache->next(&read_pos, &read_len);
    }
  }
  assert(total_length == buffer.size());
  log_position = cache->length();
cleanup:
  if (cache->truncate(saved_pos)) {
    WSREP_WARN("Failed to reinitialize IO cache");
    ret = 1;
  }
  DBUG_RETURN(ret);
}

int Wsrep_client_service::remove_fragments(
    wsrep::unique_lock<wsrep::mutex> &lock) {
  DBUG_ENTER("Wsrep_client_service::remove_fragments");

  mysql_mutex_assert_owner(static_cast<mysql_mutex_t *>(lock.mutex().native()));

  // Here we are going to iterate over SR fragments vector and remove them.
  // At the same time the SR fragments vector can be modified by wsrep applier
  // thread. So we need to protect it.
  // remove_fragments() acquires trx->mutex inside InnoDB (lock_table() =>
  // lock_table_has()).
  // At the same time trx->mutex may be already acquired by applier thread
  // during BF abort of trx (row_mysql_handle_errors() => trx_kill_blocking() =>
  // lock_make_trx_hit_list()). So local transaction thread blocks. Then applier
  // thread tries to acquire wsrep_thd_LOCK (wsrep_kill_victim() =>
  // wsrep_innobase_kill_one_trx()), but it is already acquired by local trx
  // thread.
  // The solution is to get the snapshot of SR fragments vector under the lock
  // and then process fragments removal without lock.
  std::vector<wsrep::seqno> fragments = m_thd->wsrep_sr().fragments();
  lock.unlock();
  int res =
      wsrep_schema->remove_fragments(m_thd, Wsrep_server_state::instance().id(),
                                     m_thd->wsrep_trx().id(), fragments);
  lock.lock();
  if (res) {
    WSREP_DEBUG(
        "Failed to remove fragments from SR storage for transaction "
        "%u, %llu",
        m_thd->thread_id(), m_thd->wsrep_trx().id().get());
    DBUG_RETURN(1);
  }
  DBUG_RETURN(0);
}

bool Wsrep_client_service::statement_allowed_for_streaming() const {
  /*
    Todo: Decide if implicit commit is allowed with streaming
    replication.
    !stmt_causes_implicit_commit(m_thd, CF_IMPLICIT_COMMIT_BEGIN);
  */
  return true;
}

size_t Wsrep_client_service::bytes_generated() const {
  IO_CACHE_binlog_cache_storage *cache = wsrep_get_trans_cache(m_thd, true);
  if (cache) {
    size_t pending_rows_event_length = 0;
    if (Rows_log_event *ev = m_thd->binlog_get_pending_rows_event(true)) {
      pending_rows_event_length = ev->get_data_size();
    }
    return cache->position() + pending_rows_event_length;
  }
  return 0;
}

void Wsrep_client_service::will_replay() {
  assert(m_thd == current_thd);
  mysql_mutex_lock(&LOCK_wsrep_replaying);
  ++wsrep_replaying;
  mysql_mutex_unlock(&LOCK_wsrep_replaying);
}

void Wsrep_client_service::signal_replayed()
{
  assert(m_thd == current_thd);
  mysql_mutex_lock(&LOCK_wsrep_replaying);
  --wsrep_replaying;
  assert(wsrep_replaying >= 0);
  mysql_cond_broadcast(&COND_wsrep_replaying);
  mysql_mutex_unlock(&LOCK_wsrep_replaying);
}

enum wsrep::provider::status Wsrep_client_service::replay() {
  assert(m_thd == current_thd);
  DBUG_ENTER("Wsrep_client_service::replay");

  /*
    Allocate separate THD for replaying to avoid tampering
    original THD state during replication event applying.
   */
  THD *replayer_thd = new THD(true, true);
  replayer_thd->thread_stack = m_thd->thread_stack;
  replayer_thd->real_id = pthread_self();
  replayer_thd->set_time();
  replayer_thd->set_command(COM_SLEEP);
  replayer_thd->reset_for_next_command();
  replayer_thd->set_new_thread_id();

  enum wsrep::provider::status ret;
  {
    Wsrep_replayer_service replayer_service(replayer_thd, m_thd);
    wsrep::provider &provider(replayer_thd->wsrep_cs().provider());
    ret = provider.replay(replayer_thd->wsrep_trx().ws_handle(),
                          &replayer_service);
    replayer_service.replay_status(ret);
  }

  delete replayer_thd;

  DBUG_RETURN(ret);
}

enum wsrep::provider::status Wsrep_client_service::replay_unordered()
{
  assert(0);
  return wsrep::provider::error_not_implemented;
}

void Wsrep_client_service::wait_for_replayers(
    wsrep::unique_lock<wsrep::mutex> &lock) {
  assert(m_thd == current_thd);
  lock.unlock();
  mysql_mutex_lock(&LOCK_wsrep_replaying);
  /* We need to check if the THD is BF aborted during condition wait.
     Because the aborter does not know which condition this thread is waiting,
     use timed wait and check if the THD is BF aborted in the loop. */
  while (wsrep_replaying > 0 && !wsrep_is_bf_aborted(m_thd)) {
    struct timespec wait_time;
    set_timespec_nsec(&wait_time, 10000000L);
    mysql_cond_timedwait(&COND_wsrep_replaying, &LOCK_wsrep_replaying,
                         &wait_time);
  }
  mysql_mutex_unlock(&LOCK_wsrep_replaying);
  lock.lock();
}

enum wsrep::provider::status Wsrep_client_service::commit_by_xid()
{
  assert(0);
  return wsrep::provider::error_not_implemented;
}

void Wsrep_client_service::debug_sync(const char *sync_point
                                      __attribute__((unused))) {
#if defined(ENABLED_DEBUG_SYNC)
  if (unlikely(opt_debug_sync_timeout))
    ::debug_sync(current_thd, sync_point, strlen(sync_point));
    // debug_sync_caller(m_thd, sync_point);
#endif /* defined(ENABLED_DEBUG_SYNC) */
}

void Wsrep_client_service::debug_crash(const char *crash_point
                                       __attribute__((unused))) {
  DBUG_EXECUTE_IF(crash_point, DBUG_SUICIDE(););
}

int Wsrep_client_service::bf_rollback() {
  assert(m_thd == current_thd);
  DBUG_ENTER("Wsrep_client_service::rollback");

  /* If local transaction is aborted while it is executing rollback
  to savepoint then bf_rollback sequence is invoked by PXC.
  For normal command it will rollback a transaction but due to
  special check it will skip rollback action if command is
  rollback to savepoint. Enforce rolling back for such case
  as complete transaction needs to be aborted.
  PXC-5.7, sets wsrep_conflict_state to ABORTING this helps to exercise
  the rollback hook inside binlog rollback. */
  if (m_thd->lex->sql_command == SQLCOM_ROLLBACK_TO_SAVEPOINT) {
    m_thd->wsrep_force_savept_rollback = true;
  }
  int ret = (trans_rollback_stmt(m_thd) || trans_rollback(m_thd));
  m_thd->wsrep_force_savept_rollback = false;

  if (m_thd->locked_tables_mode && m_thd->lock) {
    m_thd->locked_tables_list.unlock_locked_tables(m_thd);
    m_thd->variables.option_bits &= ~OPTION_TABLE_LOCK;
  }
  if (m_thd->backup_tables_lock.is_acquired()) {
    m_thd->backup_tables_lock.release(m_thd);
  }
  if (m_thd->global_read_lock.is_acquired()) {
    m_thd->global_read_lock.unlock_global_read_lock(m_thd);
  }

  /* Release transactional metadata locks. */
  m_thd->mdl_context.release_transactional_locks();

  /* Release all user-locks. */
  mysql_ull_cleanup(m_thd);

  /* release explicit MDL locks */
  m_thd->mdl_context.release_explicit_locks();

  DBUG_RETURN(ret);
}
