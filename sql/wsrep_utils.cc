/* Copyright 2010 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//! @file declares symbols private to wsrep integration layer

//#include <linux/in.h>
#include <linux/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <mysqld.h>
#include <sql_class.h>
#include "wsrep_priv.h"

namespace wsp
{
process::process (const char* cmd, const char* type)
    : str (strdup (cmd)), io (popen (str, type)), err(0)
{
    if (!io) {
        err = errno;
        if (!err) err = ENOMEM; // errno is not set when out of memory
        WSREP_ERROR("Failed to execute: %s: %d (%s)",
                         str, err, strerror (err));
    }
}

process::~process ()
{
    if (str) free   (const_cast<char*>(str));
    if (io)
    {
        if (pclose (io) == -1)
        {
            err = errno;
            WSREP_ERROR("pclose() failed: %d (%s)", err, strerror(err));
        }
    }
}

int
process::wait ()
{
  if (io) {

    err = pclose (io);
    io  = NULL;

    if (-1 == err) { // wait4() failed
      err = errno; assert (err);
      WSREP_ERROR("Waiting for process failed: %s: %d (%s)",
                  str, err, strerror (err));
    }
    else {           // command completed, check exit status
      if (WIFEXITED (err)) {
        err = WEXITSTATUS (err);
      }
      else {       // command didn't complete with exit()
        WSREP_ERROR("Process was aborted.");
        err = errno ? errno : ECHILD;
      }

      if (err) {
        WSREP_ERROR("Process completed with error: %s: %d (%s)",
                    str, err, strerror(err));
      }
    }
  }
  else {
    WSREP_ERROR("Command did not run: %s", str);
  }

  return err;
}

thd::thd () : init(), ptr(new THD)
{
  if (ptr)
  {
    ptr->thread_stack= (char*) &ptr;
    ptr->store_globals();
    ptr->variables.option_bits&= ~OPTION_BIN_LOG; // disable binlog
    ptr->security_ctx->master_access= ~(ulong)0;
    lex_start(ptr);
  }
}

thd::~thd ()
{
  if (ptr)
  {
    delete ptr;
    my_pthread_setspecific_ptr (THR_THD, 0);
  }
}

} // namespace wsp

extern ulong my_bind_addr;
extern uint  mysqld_port;

size_t default_ip (char* buf, size_t buf_len)
{
  size_t ip_len = 0;

  if (htonl(INADDR_NONE) == my_bind_addr) {
    WSREP_ERROR("Networking not configured, cannot receive state transfer.");
    return 0;
  }

  if (htonl(INADDR_ANY) == my_bind_addr) {
    // binds to all interfaces, try to find the address of the first one
#if (TARGET_OS_LINUX == 1)
    const char cmd[] = "/sbin/ifconfig | "
        "grep -m1 -1 -E '^[a-z]?eth[0-9]' | tail -n 1 | "
        "awk '{ print $2 }' | awk -F : '{ print $2 }'";
#else
    char *cmd;
#error "OS not supported"
#endif
    wsp::process proc (cmd, "r");

    if (NULL != proc.pipe()) {
      char* ret;

      ret = fgets (buf, buf_len, proc.pipe());

      if (proc.wait()) return 0;

      if (NULL == ret) {
        WSREP_ERROR("Failed to read output of: %s", cmd);
        return 0;
      }
    }
    else {
      WSREP_ERROR("Failed to execute: %s", cmd);
      return 0;
    }

    // clear possible \n at the end of ip string left by fgets()
    ip_len = strlen (buf);
    if (ip_len > 0 && '\n' == buf[ip_len - 1]) {
      ip_len--;
      buf[ip_len] = '\0';
    }

    if (INADDR_NONE == inet_addr(buf)) {
      WSREP_ERROR("Shell command returned invalid address: %s", buf);
      return 0;
    }
  }
  else {
    uint8_t* b = (uint8_t*)&my_bind_addr;
    ip_len = snprintf (buf, buf_len,
                       "%hhu.%hhu.%hhu.%hhu", b[0],b[1],b[2],b[3]);
  }

  return ip_len;
}

size_t default_address(char* buf, size_t buf_len)
{
  size_t addr_len = default_ip (buf, buf_len);

  if (addr_len && addr_len < buf_len) {
    addr_len += snprintf (buf + addr_len, buf_len - addr_len,
                          ":%u", mysqld_port);
  }

  return addr_len;
}
