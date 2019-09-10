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

#ifndef WSREP_BINLOG_H
#define WSREP_BINLOG_H

#include "sql_class.h"  // THD, IO_CACHE
#include "sql/binlog_ostream.h"

#define HEAP_PAGE_SIZE 65536         /* 64K */
#define WSREP_MAX_WS_SIZE 2147483647 /* 2GB */

/*
  Write the contents of a cache to a memory buffer.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.
 */
int wsrep_write_cache_buf(IO_CACHE_binlog_cache_storage *cache, uchar **buf,
                          size_t *buf_len);

/*
  Write the contents of a cache to wsrep provider.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.

  @param len  total amount of data written
  @return     wsrep error status
 */
int wsrep_write_cache(THD *thd,
                      IO_CACHE_binlog_cache_storage *cache, size_t *len);

/* Dump replication buffer to disk */
void wsrep_dump_rbr_buf(THD *thd, const void *rbr_buf, size_t buf_len);

/* Dump replication buffer along with header to a file */
void wsrep_dump_rbr_buf_with_header(THD *thd, const void *rbr_buf,
                                    size_t buf_len);

int wsrep_binlog_close_connection(THD *thd);
int wsrep_binlog_savepoint_set(THD *thd, void *sv);
int wsrep_binlog_savepoint_rollback(THD *thd, void *sv);

/**
   Write a skip event into binlog.

   @param thd Thread object pointer
   @return Zero in case of success, non-zero on failure.
*/
int wsrep_write_skip_event(THD *thd);

/*
  Write dummy event into binlog in place of unused GTID.
  The binlog write is done in thd context.
*/
int wsrep_write_dummy_event_low(THD *thd, const char *msg);
/*
  Write dummy event to binlog in place of unused GTID and
  commit. The binlog write and commit are done in temporary
  thd context, the original thd state is not altered.
*/
int wsrep_write_dummy_event(THD *thd, const char *msg);

void wsrep_register_binlog_handler(THD *thd, bool trx);

/**
   Return true if committing THD will write to binlog during commit.
   This is the case for:
   - Local THD, binlog is open
   - Replaying THD, binlog is open
   - Applier THD, log-slave-updates is enabled
*/
bool wsrep_commit_will_write_binlog(THD *thd);

void wsrep_register_for_group_commit(THD *thd);
void wsrep_wait_for_turn_in_group_commit(THD* thd);
void wsrep_unregister_from_group_commit(THD *thd);

#endif /* WSREP_BINLOG_H */
