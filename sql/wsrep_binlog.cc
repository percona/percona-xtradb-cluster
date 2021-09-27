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
#include "wsrep_priv.h"
#include "log_event.h"
#include "sql_base.h"  // TEMP_PREFIX

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
int wsrep_write_cache_buf(IO_CACHE *cache, uchar **buf, size_t *buf_len)
{
  my_off_t const saved_pos(my_b_tell(cache));

  if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
  {
    WSREP_ERROR("Failed to initialize io-cache");
    return ER_ERROR_ON_WRITE;
  }

  uint length = my_b_bytes_in_cache(cache);
  if (unlikely(0 == length)) length = my_b_fill(cache);

  size_t total_length = *buf_len;

  if (likely(length > 0)) do
  {
      total_length += length;
      /*
        Bail out if buffer grows too large.
        A temporary fix to avoid allocating indefinitely large buffer,
        not a real limit on a writeset size which includes other things
        like header and keys.
      */
      if (total_length > wsrep_max_ws_size)
      {
          WSREP_WARN("Transaction/Write-set size limit (%lu) exceeded: %zu",
                     wsrep_max_ws_size, total_length);
          goto error;
      }

      uchar* tmp = (uchar *)my_realloc(key_memory_wsrep, *buf, total_length, MYF(0));
      if (!tmp)
      {
          WSREP_ERROR("Fail to allocate/reallocate memory to hold"
                      " write-set for replication."
                      " Existing Size: %zu, Requested Size: %lu",
                      *buf_len, (long unsigned) total_length);
          goto error;
      }
      *buf = tmp;

      memcpy(*buf + *buf_len, cache->read_pos, length);
      *buf_len = total_length;
      cache->read_pos = cache->read_end;
  } while ((cache->file >= 0) && (length = my_b_fill(cache)));

  if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
  {
    WSREP_WARN("Failed to initialize io-cache");
    goto cleanup;
  }

  if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
  {
    WSREP_ERROR("Failed to initialize io-cache");
    goto cleanup;
  }

  return 0;

error:
  if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
  {
    WSREP_WARN("Failed to initialize io-cache");
  }
cleanup:
  my_free(*buf);
  *buf= NULL;
  *buf_len= 0;
  return ER_ERROR_ON_WRITE;
}

#define STACK_SIZE 4096 /* 4K - for buffer preallocated on the stack:
                         * many transactions would fit in there
                         * so there is no need to reach for the heap */

/* Returns minimum multiple of HEAP_PAGE_SIZE that is >= length */
static inline size_t
heap_size(size_t length)
{
    return (length + HEAP_PAGE_SIZE - 1)/HEAP_PAGE_SIZE*HEAP_PAGE_SIZE;
}

/* append data to writeset */
static inline wsrep_status_t
wsrep_append_data(wsrep_t*           const wsrep,
                  wsrep_ws_handle_t* const ws,
                  const void*        const data,
                  size_t             const len)
{
    struct wsrep_buf const buff = { data, len };
    wsrep_status_t const rc(wsrep->append_data(wsrep, ws, &buff, 1,
                                               WSREP_DATA_ORDERED, true));
    if (rc != WSREP_OK)
    {
        WSREP_WARN("append_data() returned %d", rc);
    }

    return rc;
}

/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.

  This version reads all of cache into single buffer and then appends to a
  writeset at once.
 */
static int wsrep_write_cache_once(wsrep_t*  const wsrep,
                                  THD*      const thd,
                                  IO_CACHE* const cache,
                                  size_t*   const len)
{
    my_off_t const saved_pos(my_b_tell(cache));

    if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
    {
        WSREP_ERROR("Failed to initialize io-cache");
        return ER_ERROR_ON_WRITE;
    }

    int err(WSREP_OK);

    size_t total_length(0);
    uchar  stack_buf[STACK_SIZE]; /* to avoid dynamic allocations for few data*/
    uchar* heap_buf(NULL);
    uchar* buf(stack_buf);
    size_t allocated(sizeof(stack_buf));
    size_t used(0);

    uint length(my_b_bytes_in_cache(cache));

    /* If galera node is acting as independent slave then GTID event that is
    captured during processing of relay log should be cached and appended to
    replicating write-set to ensure all the nodes of cluster are using
    GTID sequence. */
    if (thd->wsrep_gtid_event_buf)
    {
      if (thd->wsrep_gtid_event_buf_len < (allocated - used))
      {
        memcpy(buf + used,
               thd->wsrep_gtid_event_buf,
               thd->wsrep_gtid_event_buf_len);
      }
      else
      {
        size_t const new_size(heap_size(thd->wsrep_gtid_event_buf_len + used));
        uchar* tmp = (uchar *)my_realloc(key_memory_wsrep, heap_buf,
                                         new_size, MYF(0));
        if (!tmp)
        {
          WSREP_ERROR("Failed to allocate/reallocate buffer to hold GTID event"
                      " Existing Size: %zu, Requested Size: %lu",
                      allocated, thd->wsrep_gtid_event_buf_len);
          err = WSREP_TRX_SIZE_EXCEEDED;
          goto cleanup;
        }

        /* Copy over existing buffer content. */
        heap_buf = tmp;
        memcpy(heap_buf, buf, used);
        buf = heap_buf;
        allocated = new_size;

        memcpy(buf + used,
               thd->wsrep_gtid_event_buf,
               thd->wsrep_gtid_event_buf_len);
      }

      total_length += thd->wsrep_gtid_event_buf_len;
      used = total_length;
    }
    else if (thd->variables.gtid_next.type != AUTOMATIC_GROUP)
    {
      /* Starting 5.7, MySQL delays appending GTID to binlog.
      It is done at commit time. pre-commit hook doesn't have the GTID
      information. If user has set explict GTID using gtid_next=UUID:seqno
      then such event should be appended to write-set. */
      Gtid_log_event gtid_event(thd, true, 0, 0, false);
      uchar gtid_buf[Gtid_log_event::MAX_EVENT_LENGTH];
      uint32 gtid_len = gtid_event.write_to_memory(gtid_buf);

      assert(Gtid_log_event::MAX_EVENT_LENGTH < STACK_SIZE);

      memcpy(buf + used, gtid_buf, gtid_len);
      used += gtid_len;
      total_length += gtid_len;
    }

    if (unlikely(0 == length)) length = my_b_fill(cache);

    if (likely(length > 0)) do
    {
        total_length += length;
        /*
          Bail out if buffer grows too large.
          A temporary fix to avoid allocating indefinitely large buffer,
          not a real limit on a writeset size which includes other things
          like header and keys.
        */
        if (unlikely(total_length > wsrep_max_ws_size))
        {
            WSREP_WARN("Transaction/Write-set size limit (%lu) exceeded: %zu",
                       wsrep_max_ws_size, total_length);
	    err = WSREP_TRX_SIZE_EXCEEDED;
            goto cleanup;
        }

        if (total_length > allocated)
        {
            size_t const new_size(heap_size(total_length));
            uchar* tmp = (uchar *)my_realloc(key_memory_wsrep, heap_buf, new_size, MYF(0));
            if (!tmp)
            {
                WSREP_ERROR("Failed to allocate/reallocate memory to hold"
                            " write-set for replication."
                            " Existing Size: %zu, Requested Size: %lu",
                            allocated, (long unsigned) new_size);
                err = WSREP_TRX_SIZE_EXCEEDED;
                goto cleanup;
            }

            heap_buf = tmp;
            buf = heap_buf;
            allocated = new_size;

            if (used <= STACK_SIZE && used > 0) // there's data in stack_buf
<<<<<<< HEAD
||||||| merged common ancestors
            {
                DBUG_ASSERT(buf == stack_buf);
=======
            {
                assert(buf == stack_buf);
>>>>>>> wsrep_5.7.34-25.26
                memcpy(heap_buf, stack_buf, used);
        }

        memcpy(buf + used, cache->read_pos, length);
        used = total_length;
        cache->read_pos = cache->read_end;
    } while ((cache->file >= 0) && (length = my_b_fill(cache)));

    if (used > 0)
        err = wsrep_append_data(wsrep, &thd->wsrep_ws_handle, buf, used);

    if (WSREP_OK == err) *len = total_length;

cleanup:
    if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
    {
        WSREP_ERROR("Failed to reinitialize io-cache");
    }

    if (unlikely(WSREP_OK != err)) wsrep_dump_rbr_buf(thd, buf, used);

    my_free(heap_buf);
    if (thd->wsrep_gtid_event_buf_len < STACK_SIZE) my_free(thd->wsrep_gtid_event_buf);
    thd->wsrep_gtid_event_buf     = NULL;
    thd->wsrep_gtid_event_buf_len = 0;
    return err;
}

/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.

  This version uses incremental data appending as it reads it from cache.
 */
static int wsrep_write_cache_inc(wsrep_t*  const wsrep,
                                 THD*      const thd,
                                 IO_CACHE* const cache,
                                 size_t*   const len)
{
    my_off_t const saved_pos(my_b_tell(cache));

    if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
    {
      WSREP_ERROR("Failed to initialize io-cache");
      return WSREP_TRX_ERROR;
    }

    int err(WSREP_OK);

    size_t total_length(*len);
    uint length(my_b_bytes_in_cache(cache));

    if (thd->wsrep_gtid_event_buf)
    {
      if (WSREP_OK != (err=wsrep_append_data(wsrep, &thd->wsrep_ws_handle,
                                             thd->wsrep_gtid_event_buf,
                                             thd->wsrep_gtid_event_buf_len)))
        goto cleanup;
      total_length += thd->wsrep_gtid_event_buf_len;
    }
    else if (total_length == 0 && thd->variables.gtid_next.type != AUTOMATIC_GROUP)
    {
      /* Starting 5.7, MySQL delays appending GTID to binlog. It is done at
      commit time. pre-commit hook doesn't have the GTID information.
      If user has set explict GTID using gtid_next=UUID:seqno then such event
      should be appended to write-set. */
      Gtid_log_event gtid_event(thd, true, 0, 0, false);
      uchar gtid_buf[Gtid_log_event::MAX_EVENT_LENGTH];
      uint32 gtid_len = gtid_event.write_to_memory(gtid_buf);

      if (WSREP_OK != (err=wsrep_append_data(wsrep, &thd->wsrep_ws_handle,
                                            gtid_buf, gtid_len)))
        goto cleanup;

      total_length += gtid_len;
    }

    if (unlikely(0 == length)) length = my_b_fill(cache);

    if (likely(length > 0)) do
    {
        total_length += length;
        /* bail out if buffer grows too large
           not a real limit on a writeset size which includes other things
           like header and keys.
        */
        if (unlikely(total_length > wsrep_max_ws_size))
        {
            WSREP_WARN("Transaction/Write-set size limit (%lu) exceeded: %zu",
                       wsrep_max_ws_size, total_length);
            err = WSREP_TRX_SIZE_EXCEEDED;
            goto cleanup;
        }

        if(WSREP_OK != (err=wsrep_append_data(wsrep, &thd->wsrep_ws_handle,
                                              cache->read_pos, length)))
                goto cleanup;

        cache->read_pos = cache->read_end;
    } while ((cache->file >= 0) && (length = my_b_fill(cache)));

    if (WSREP_OK == err) *len = total_length;

cleanup:
    if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
    {
        WSREP_ERROR("Failed to reinitialize io-cache");
    }

    if (thd->wsrep_gtid_event_buf) my_free(thd->wsrep_gtid_event_buf);
    thd->wsrep_gtid_event_buf_len = 0;
    thd->wsrep_gtid_event_buf     = NULL;

    return err;
}

static int prepend_binlog_control_event(wsrep_t* const wsrep, THD* const thd)
{
    if ((thd->variables.option_bits & OPTION_BIN_LOG) != 0)
      return 0;

    IO_CACHE tmp_io_cache = {};
    if (open_cached_file(&tmp_io_cache, mysql_tmpdir, TEMP_PREFIX,
                        128, MYF(MY_WME)))
        return ER_ERROR_ON_WRITE;

    size_t length= 0;
    int ret= 0;
    Intvar_log_event ev((uchar)binary_log::Intvar_event::BINLOG_CONTROL_EVENT, 0);
    if (ev.write(&tmp_io_cache))
    {
        ret= ER_ERROR_ON_WRITE;
        goto cleanup;
    }
    if (reinit_io_cache(&tmp_io_cache, READ_CACHE, 0, 0, 0))
    {
        WSREP_ERROR("Failed to initialize io-cache");
        ret= ER_ERROR_ON_WRITE;
        goto cleanup;
    }

    length= my_b_bytes_in_cache(&tmp_io_cache);
    if (unlikely(0 == length)) length = my_b_fill(&tmp_io_cache);
    assert(length != 0);
    ret= wsrep_append_data(wsrep, &thd->wsrep_ws_handle, tmp_io_cache.read_pos, length);

cleanup:
    close_cached_file(&tmp_io_cache);
    return ret;
}
/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.
 */
int wsrep_write_cache(wsrep_t*  const wsrep,
                      THD*      const thd,
                      IO_CACHE* const cache,
                      size_t*   const len)
{
    if (int res = prepend_binlog_control_event(wsrep, thd))
    {
        return res;
    }
    if (wsrep_incremental_data_collection)
    {
        return wsrep_write_cache_inc(wsrep, thd, cache, len);
    }
    else
    {
        return wsrep_write_cache_once(wsrep, thd, cache, len);
    }
}

void wsrep_dump_rbr_buf(THD *thd, const void* rbr_buf, size_t buf_len)
{
  /* filename is derived from wsrep_data_home_dir (which in turn from
  mysql_real_data_home that has max length limit of FN_REFLEN */
  char filename[FN_REFLEN + 64]= {0};
  int len= snprintf(filename, FN_REFLEN + 64, "%s/GRA_%u_%lld.log",
                    wsrep_data_home_dir, thd->thread_id(),
                    (long long)wsrep_thd_trx_seqno(thd));
  if (len >= (FN_REFLEN + 64))
  {
    WSREP_ERROR("RBR dump path too long: %d, skipping dump.", len);
    return;
  }

  FILE *of= fopen(filename, "wb");
  if (of)
  {
    fwrite (rbr_buf, buf_len, 1, of);
    fclose(of);
  }
  else
  {
    WSREP_ERROR("Failed to open file '%s': %d (%s)",
                filename, errno, strerror(errno));
  }
}

void wsrep_dump_rbr_direct(THD* thd, IO_CACHE* cache)
{
  /* filename is derived from wsrep_data_home_dir (which in turn from
  mysql_real_data_home that has max length limit of FN_REFLEN */
  char filename[FN_REFLEN + 64]= {0};
  int len= snprintf(filename, FN_REFLEN + 64, "%s/GRA_%u_%lld.log",
                    wsrep_data_home_dir, thd->thread_id(),
                    (long long)wsrep_thd_trx_seqno(thd));
  size_t bytes_in_cache = 0;
  // check path
  if (len >= (FN_REFLEN + 64))
  {
    WSREP_ERROR("RBR dump path too long: %d, skipping dump.", len);
    return ;
  }
  // init cache
  my_off_t const saved_pos(my_b_tell(cache));
  if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
  {
    WSREP_ERROR("Failed to initialize io-cache");
    return ;
  }
  // open file
  FILE* of = fopen(filename, "wb");
  if (!of)
  {
    WSREP_ERROR("Failed to open file '%s': %d (%s)",
                filename, errno, strerror(errno));
    goto cleanup;
  }
  // ready to write
  bytes_in_cache= my_b_bytes_in_cache(cache);
  if (unlikely(bytes_in_cache == 0)) bytes_in_cache = my_b_fill(cache);
  if (likely(bytes_in_cache > 0)) do
  {
    if (my_fwrite(of, cache->read_pos, bytes_in_cache,
                  MYF(MY_WME | MY_NABP)) == (size_t) -1)
    {
      WSREP_ERROR("Failed to write file '%s'", filename);
      goto cleanup;
    }
    cache->read_pos= cache->read_end;
  } while ((cache->file >= 0) && (bytes_in_cache= my_b_fill(cache)));
  if(cache->error == -1)
  {
    WSREP_ERROR("RBR inconsistent");
    goto cleanup;
  }
cleanup:
  // init back
  if (reinit_io_cache(cache, WRITE_CACHE, saved_pos, 0, 0))
  {
    WSREP_ERROR("Failed to reinitialize io-cache");
  }
  // close file
  if (of) fclose(of);
}

extern handlerton *binlog_hton;

/*
  wsrep exploits binlog's caches even if binlogging itself is not
  activated. In such case connection close needs calling
  actual binlog's method.
  Todo: split binlog hton from its caches to use ones by wsrep
  without referring to binlog's stuff.
*/
int wsrep_binlog_close_connection(THD* thd)
{
  DBUG_ENTER("wsrep_binlog_close_connection");
  if (thd_get_ha_data(thd, binlog_hton) != NULL)
    binlog_hton->close_connection (binlog_hton, thd);
  DBUG_RETURN(0);
}

int wsrep_binlog_savepoint_set(THD *thd,  void *sv)
{
  if (!wsrep_emulate_bin_log) return 0;
  int rcode = binlog_hton->savepoint_set(binlog_hton, thd, sv);
  return rcode;
}

int wsrep_binlog_savepoint_rollback(THD *thd, void *sv)
{
  if (!wsrep_emulate_bin_log) return 0;
  int rcode = binlog_hton->savepoint_rollback(binlog_hton, thd, sv);
  return rcode;
}
