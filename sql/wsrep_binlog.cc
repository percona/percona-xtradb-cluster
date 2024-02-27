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

#include "wsrep_binlog.h"
#include "log_event.h"
#include "mysql/components/services/log_builtins.h"
#include "mysql/plugin.h"
#include "wsrep_priv.h"

#include "binlog.h"
#include "log.h"
#include "log_event.h"  // Log_event_writer
#include "mutex_lock.h"
#include "mysql/psi/mysql_file.h"
#include "service_wsrep.h"
#include "sql/sql_base.h"  // TEMP_PREFIX
#include "transaction.h"
#include "wsrep_applier.h"

static bool wsrep_dump_cache_common(
    IO_CACHE_binlog_cache_storage *cache,
    std::function<bool(unsigned char *, my_off_t)> dumpFn) {
  my_off_t const saved_pos(cache->position());

  unsigned char *read_pos = nullptr;
  my_off_t read_len = 0;

  if (cache->begin(&read_pos, &read_len)) {
    WSREP_ERROR("Failed to initialize io-cache");
    return true;
  }

  if (read_len == 0 && cache->next(&read_pos, &read_len)) {
    WSREP_ERROR("Failed to read from io-cache");
    return true;
  }

  bool res = false;
  while (read_len > 0) {
    if (dumpFn(read_pos, read_len)) {
      res = true;
      break;
    }
    cache->next(&read_pos, &read_len);
  }

  if (cache->truncate(saved_pos)) {
    WSREP_WARN("Failed to reinitialize io-cache");
    res = true;
  }

  return res;
}

bool wsrep_write_cache_file(IO_CACHE_binlog_cache_storage *cache, File file) {
  return wsrep_dump_cache_common(
      cache, [file](unsigned char *data, my_off_t size) {
        size_t written = mysql_file_write(file, data, size, MYF(0));
        if (written != size) return true;
        return false;
      });
}

/*
  Write the contents of a cache to a memory buffer.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.

  @params
    cache   - IO cahce to read events from
    buf     - buffer where to write the events, may contain some events
              at call time, this function appends events in the end
    buf_len - in input, this tells the position where to append events
              (0 is in the begin)
              in output, function sets the actual length of buffer after
              all appends
 */
int wsrep_write_cache_buf(IO_CACHE_binlog_cache_storage *cache, uchar **buf,
                          size_t *buf_len) {
  size_t total_length = *buf_len;

  bool res = wsrep_dump_cache_common(cache, [&total_length, buf, buf_len](
                                                unsigned char *data,
                                                my_off_t size) {
    total_length += size;

    /*
      Bail out if buffer grows too large.
      A temporary fix to avoid allocating indefinitely large buffer,
      not a real limit on a writeset size which includes other things
      like header and keys.
    */
    if (total_length > wsrep_max_ws_size) {
      WSREP_WARN("Transaction/Write-set size limit (%lu) exceeded: %zu",
                 wsrep_max_ws_size, total_length);
      return true;
    }

    uchar *tmp =
        (uchar *)my_realloc(key_memory_wsrep, *buf, total_length, MYF(0));
    if (!tmp) {
      WSREP_ERROR(
          "Fail to allocate/reallocate memory to hold"
          " write-set for replication."
          " Existing Size: %zu, Requested Size: %lu",
          *buf_len, (long unsigned)total_length);
      return true;
    }
    *buf = tmp;

    memcpy(*buf + *buf_len, data, size);
    *buf_len = total_length;
    return false;
  });

  if (res) {
    my_free(*buf);
    *buf = nullptr;
    *buf_len = 0;
    return ER_ERROR_ON_WRITE;
  }
  return 0;
}

#define STACK_SIZE                                   \
  4096 /* 4K - for buffer preallocated on the stack: \
        * many transactions would fit in there       \
        * so there is no need to reach for the heap */

/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.

  This version uses incremental data appending as it reads it from cache.
 */
static int wsrep_write_cache_inc(THD *const thd,
                                 IO_CACHE_binlog_cache_storage *const cache,
                                 size_t *const len) {
  DBUG_ENTER("wsrep_write_cache_inc");
  my_off_t const saved_pos(cache->position());

  unsigned char *read_pos = NULL;
  my_off_t read_len = 0;

  if (cache->begin(&read_pos, &read_len, thd->wsrep_sr().log_position())) {
    WSREP_ERROR("Failed to initialize io-cache");
    DBUG_RETURN(ER_ERROR_ON_WRITE);
  }

  int ret = 0;
  size_t total_length(*len);

  if (thd->wsrep_gtid_event_buf) {
    if (thd->wsrep_cs().append_data(wsrep::const_buffer(
            thd->wsrep_gtid_event_buf, thd->wsrep_gtid_event_buf_len)))
      goto cleanup;
    total_length += thd->wsrep_gtid_event_buf_len;
  } else if (total_length == 0 &&
             thd->variables.gtid_next.type != AUTOMATIC_GTID) {
    /* Starting 5.7, MySQL delays appending GTID to binlog. It is done at
    commit time. pre-commit hook doesn't have the GTID information.
    If user has set explict GTID using gtid_next=UUID:seqno then such event
    should be appended to write-set. */
    Gtid_log_event gtid_event(thd, true, 0, 0, false, 0, 0,
                              UNKNOWN_SERVER_VERSION, UNKNOWN_SERVER_VERSION);
    int error = 0;
    StringBuffer_ostream<Gtid_log_event::MAX_EVENT_LENGTH> ostream;
    if ((error = gtid_event.write(&ostream))) {
      WSREP_ERROR("Failed to write the GTID event");
      ret = -1;
      goto cleanup;
    }

    if (thd->wsrep_cs().append_data(
            wsrep::const_buffer(ostream.c_ptr(), ostream.length())))
      goto cleanup;
    total_length += ostream.length();
  }

  if (read_len == 0 && cache->next(&read_pos, &read_len)) {
    WSREP_ERROR("Failed to read from io-cache");
    DBUG_RETURN(ER_ERROR_ON_WRITE);
  }

  while (read_len > 0) {
    total_length += read_len;

    /* bail out if buffer grows too large
       not a real limit on a writeset size which includes other things
       like header and keys.
    */
    if (unlikely(total_length > wsrep_max_ws_size)) {
      WSREP_WARN("Transaction/Write-set size limit (%lu) exceeded: %zu",
                 wsrep_max_ws_size, total_length);
      ret = 1;
      goto cleanup;
    }

    if (thd->wsrep_cs().append_data(wsrep::const_buffer(read_pos, read_len)))
      goto cleanup;

    cache->next(&read_pos, &read_len);
  }

  if (ret == 0) {
    *len = total_length;
  }

cleanup:

  if (cache->truncate(saved_pos)) {
    WSREP_WARN("Failed to re-initialize io-cache");
  }

  if (thd->wsrep_gtid_event_buf) my_free(thd->wsrep_gtid_event_buf);
  thd->wsrep_gtid_event_buf_len = 0;
  thd->wsrep_gtid_event_buf = NULL;

  DBUG_RETURN(ret);
}

/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.
 */
int wsrep_write_cache(THD *const thd,
                      IO_CACHE_binlog_cache_storage *const cache,
                      size_t *const len) {
  if (int res = prepend_binlog_control_event(thd)) {
    return res;
  }
  return wsrep_write_cache_inc(thd, cache, len);
}

void wsrep_dump_rbr_buf(THD *thd, const void *rbr_buf, size_t buf_len) {
  /* filename is derived from wsrep_data_home_dir (which in turn from
  mysql_real_data_home that has max length limit of FN_REFLEN */
  char filename[FN_REFLEN + 64] = {0};
  int len = snprintf(filename, FN_REFLEN + 64, "%s/GRA_%u_%lld.log",
                     wsrep_data_home_dir, thd->thread_id(),
                     (long long)wsrep_thd_trx_seqno(thd));
  if (len >= (FN_REFLEN + 64)) {
    WSREP_ERROR("RBR dump path too long: %d, skipping dump.", len);
    return;
  }

  FILE *of = fopen(filename, "wb");
  if (of) {
    fwrite(rbr_buf, buf_len, 1, of);
    fclose(of);
  } else {
    WSREP_ERROR("Failed to open file '%s': %d (%s)", filename, errno,
                strerror(errno));
  }
}

/* Dump replication buffer along with header to a file. */
void wsrep_dump_rbr_buf_with_header(THD *thd, const void *rbr_buf,
                                    size_t buf_len) {
  DBUG_ENTER("wsrep_dump_rbr_buf_with_header");

  File file;
  IO_CACHE_binlog_cache_storage tmp_io_cache;
  Format_description_log_event *ev = 0;

  longlong thd_trx_seqno = (long long)wsrep_thd_trx_seqno(thd);
  int len = snprintf(NULL, 0, "%s/GRA_%lld_%lld_v2.log", wsrep_data_home_dir,
                     (longlong)thd->thread_id(), thd_trx_seqno);
  /*
    len doesn't count the \0 end-of-string. Use len+1 below
    to alloc and pass as an argument to snprintf.
  */
  char *filename;
  if (len < 0 || !(filename = (char *)malloc(len + 1))) {
    WSREP_ERROR("snprintf error: %d, skipping dump.", len);
    DBUG_VOID_RETURN;
  }

  int len1 =
      snprintf(filename, len + 1, "%s/GRA_%lld_%lld_v2.log",
               wsrep_data_home_dir, (longlong)thd->thread_id(), thd_trx_seqno);

  if (len > len1) {
    WSREP_ERROR("RBR dump path truncated: %d, skipping dump.", len);
    free(filename);
    DBUG_VOID_RETURN;
  }

  if ((file = mysql_file_open(key_file_wsrep_gra_log, filename,
                              O_RDWR | O_CREAT, MYF(MY_WME))) < 0) {
    WSREP_ERROR("Failed to open file '%s' : %d (%s)", filename, errno,
                strerror(errno));
    goto cleanup1;
  }

  if (tmp_io_cache.open(mysql_tmpdir, TEMP_PREFIX, 1024000, 1024000)) {
    goto cleanup2;
  }

  if (tmp_io_cache.write((const uchar *)BINLOG_MAGIC, BIN_LOG_HEADER_SIZE)) {
    goto cleanup2;
  }

  /*
    Instantiate an FDLE object for non-wsrep threads (to be written
    to the dump file).
  */
  ev = (thd->wsrep_applier) ? wsrep_get_apply_format(thd)
                            : (new Format_description_log_event());

  if (((ev->write(&tmp_io_cache) ||
        tmp_io_cache.write(static_cast<uchar *>(const_cast<void *>(rbr_buf)),
                           buf_len)))) {
    WSREP_ERROR("Failed to write to populate io cache");
    goto cleanup2;
  }

  if (wsrep_write_cache_file(&tmp_io_cache, file)) {
    WSREP_ERROR("Failed to write to '%s'.", filename);
    goto cleanup2;
  }

cleanup2:
  tmp_io_cache.close();

cleanup1:
  free(filename);
  mysql_file_close(file, MYF(MY_WME));

  if (!thd->wsrep_applier) delete ev;
  DBUG_VOID_RETURN;
}

extern handlerton *binlog_hton;

/*
  wsrep exploits binlog's caches even if binlogging itself is not
  activated. In such case connection close needs calling
  actual binlog's method.
  Todo: split binlog hton from its caches to use ones by wsrep
  without referring to binlog's stuff.
*/
int wsrep_binlog_close_connection(THD *thd) {
  DBUG_ENTER("wsrep_binlog_close_connection");
  if (thd_get_ha_data(thd, binlog_hton) != NULL)
    binlog_hton->close_connection(binlog_hton, thd);
  DBUG_RETURN(0);
}

int wsrep_binlog_savepoint_set(THD *thd, void *sv) {
  if (!wsrep_emulate_bin_log) return 0;
  int rcode = binlog_hton->savepoint_set(binlog_hton, thd, sv);
  return rcode;
}

int wsrep_binlog_savepoint_rollback(THD *thd, void *sv) {
  if (!wsrep_emulate_bin_log) return 0;
  int rcode = binlog_hton->savepoint_rollback(binlog_hton, thd, sv);
  return rcode;
}

#include "log_event.h"

int wsrep_write_skip_event(THD *thd) {
  DBUG_ENTER("wsrep_write_skip_event");
  Ignorable_log_event skip_event(thd);
  int ret = mysql_bin_log.write_event(&skip_event);
  if (ret) {
    WSREP_WARN("wsrep_write_skip_event: write to binlog failed: %d", ret);
  }
  if (!ret && (ret = trans_commit_stmt(thd))) {
    WSREP_WARN("wsrep_write_skip_event: statt commit failed");
  }
  DBUG_RETURN(ret);
}

int wsrep_write_dummy_event_low(THD *thd __attribute__((unused)),
                                const char *msg __attribute__((unused))) {
  ::abort();
  return 0;
}

int wsrep_write_dummy_event(THD *orig_thd __attribute__((unused)),
                            const char *msg __attribute__((unused))) {
  return 0;
}

bool wsrep_commit_will_write_binlog(THD *thd) {
  return (!wsrep_emulate_bin_log &&       /* binlog enabled*/
          (wsrep_thd_is_local(thd) ||     /* local thd*/
           (thd->wsrep_applier_service && /* applier and log-slave-updates */
            opt_log_replica_updates)));
}

#include <queue>

static std::queue<THD *> wsrep_group_commit_queue;

void wsrep_register_for_group_commit(THD *thd) {
  DBUG_TRACE;
  if (wsrep_emulate_bin_log) {
    /* Binlog is off, no need to maintain group commit queue */
    return;
  }

  assert(thd->wsrep_trx().state() == wsrep::transaction::s_committing);
  MUTEX_LOCK(guard, &LOCK_wsrep_group_commit);
  wsrep_group_commit_queue.push(thd);
  thd->wsrep_enforce_group_commit = true;
  WSREP_DEBUG("Registering thread with id (%d) in wsrep group commit queue",
              thd->thread_id());
  return;
}

bool wsrep_implicit_transaction(THD *thd) {
  if (!thd) {
    return false;
  }
  return thd->is_operating_substatement_implicitly ||
         thd->is_operating_gtid_table_implicitly;
}

void wsrep_wait_for_turn_in_group_commit(THD *thd) {
  DBUG_TRACE;
  if (wsrep_emulate_bin_log || thd == NULL) {
    /* Binlog is off, no need to maintain group commit queue */
    /* thd can be NULL if the transaction is being committed
       during recovery using XID. */
    return;

  }

  if (!thd->wsrep_enforce_group_commit) {
    /* Said handler was not register for wsrep group commit */
    return;
  }

  MUTEX_LOCK(guard, &LOCK_wsrep_group_commit);

  while (true) {
    if (thd == wsrep_group_commit_queue.front()) {
      WSREP_DEBUG("Thread with id (%d) granted turn to proceed",
                  thd->thread_id());
      break;
    } else {
      WSREP_DEBUG(
          "Thread with id (%d) waiting for its turns in wsrep group"
          " commit queue - waiting for (%d)",
          thd->thread_id(),
          wsrep_group_commit_queue.front()->thread_id());
      mysql_cond_wait(&COND_wsrep_group_commit, &LOCK_wsrep_group_commit);
    }
  }

  return;
}

void wsrep_unregister_from_group_commit(THD *thd) {
  DBUG_TRACE;
  if (wsrep_emulate_bin_log || thd == NULL) {
    /* Binlog is off, no need to maintain group commit queue */
    /* thd can be NULL if the transaction is being committed
       during recovery using XID. */
    return;
  }

  if (!thd->wsrep_enforce_group_commit) {
    /* Said handler was not register for wsrep group commit */
    return;
  }

  MUTEX_LOCK(guard, &LOCK_wsrep_group_commit);

  thd->wsrep_enforce_group_commit = false;
  assert(wsrep_group_commit_queue.front() == thd);
  wsrep_group_commit_queue.pop();
  WSREP_DEBUG(
      "Un-Registering thread with id (%d) from wsrep group commit"
      " queue",
      thd->thread_id());
  mysql_cond_broadcast(&COND_wsrep_group_commit);
  return;
}
