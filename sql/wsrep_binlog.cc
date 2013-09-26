/* Copyright 2013 Codership Oy <http://www.codership.com>

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

#include "wsrep_priv.h"

/*
  Write the contents of a cache to memory buffer.

  This function quite the same as MYSQL_BIN_LOG::write_cache(),
  with the exception that here we write in buffer instead of log file.
 */
int wsrep_write_cache(IO_CACHE *cache, uchar **buf, size_t *buf_len)
{
  *buf= NULL;
  *buf_len= 0;

  if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
  {
    WSREP_ERROR("failed to initialize io-cache");
    return ER_ERROR_ON_WRITE;
  }

  uint length= my_b_bytes_in_cache(cache);
  long long total_length= 0;

  do
  {
    total_length += length;
    /* bail out if buffer grows too large
       This is a temporary fix to avoid allocating indefinitely large buffer,
       not a real limit on a writeset size which includes other things like
       header and keys.
     */
    if (total_length > wsrep_max_ws_size)
    {
      WSREP_WARN("transaction size limit (%lld) exceeded: %lld",
                 wsrep_max_ws_size, total_length);
      goto error;
    }

#ifndef DBUG_OFF /* my_realloc() asserts on zero length */
    if (total_length) {
#endif
    uchar* tmp= (uchar *)my_realloc(*buf, total_length, MYF(0));
    if (!tmp)
    {
      WSREP_ERROR("could not (re)allocate buffer: %zu + %u", *buf_len, length);
      goto error;
    }
    *buf= tmp;
#ifndef DBUG_OFF
    }
#endif

    memcpy(*buf + *buf_len, cache->read_pos, length);
    *buf_len= total_length;
    cache->read_pos= cache->read_end;
  } while ((cache->file >= 0) && (length= my_b_fill(cache)));

  return 0;

error:
  my_free(*buf);
  *buf= NULL;
  *buf_len= 0;
  if (reinit_io_cache(cache, WRITE_CACHE, 0, 0, 0))
  {
    WSREP_WARN("failed to initialize io-cache");
  }
  return ER_ERROR_ON_WRITE;
}

