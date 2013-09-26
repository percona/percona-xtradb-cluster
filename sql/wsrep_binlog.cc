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

  if (reinit_io_cache(cache, READ_CACHE, 0, 0, 0))
    return ER_ERROR_ON_WRITE;
  uint length= my_b_bytes_in_cache(cache);
  long long total_length = 0;
  uchar *buf_ptr = NULL;

  do
  {
    /* Bail out if buffer grows too large.
       This is a temporary fix to avoid RAM abuse and not an accurate
       writeset size control, since there are keys and stuff.
     */
    if (total_length > wsrep_max_ws_size)
    {
      WSREP_WARN("transaction size limit (%lld) exceeded: %lld",
                 wsrep_max_ws_size, total_length);
      if (reinit_io_cache(cache, WRITE_CACHE, 0, 0, 0))
      {
        WSREP_WARN("failed to initialize io-cache");
      }
      if (buf_ptr) my_free(*buf);
      *buf_len = 0;
      return ER_ERROR_ON_WRITE;
    }
    if (total_length > 0)
    {
      *buf_len += length;
      *buf = (uchar *)my_realloc(*buf, total_length+length, MYF(0));
      if (!*buf)
      {
        WSREP_ERROR("io cache write problem: %zu %d", *buf_len, length);
        return ER_ERROR_ON_WRITE;
      }
      buf_ptr = *buf+total_length;
    }
    else
    {
      if (buf_ptr != NULL)
      {
        WSREP_ERROR("io cache alloc error: %zu %d", *buf_len, length);
        my_free(*buf);
      }
      if (length > 0)
      {
        *buf = (uchar *) my_malloc(length, MYF(0));
        buf_ptr = *buf;
        *buf_len = length;
      }
    }
    total_length += length;

    memcpy(buf_ptr, cache->read_pos, length);
    cache->read_pos=cache->read_end;
  } while ((cache->file >= 0) && (length= my_b_fill(cache)));

  return 0;
}

