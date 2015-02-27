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

//! @file some utility functions and classes not directly related to replication

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // POSIX_SPAWN_USEVFORK flag
#endif

#include "wsrep_utils.h"
#include "wsrep_mysqld.h"

#include <sql_class.h>

#include <unistd.h>   // pipe()
#include <errno.h>    // errno
#include <string.h>   // strerror()
#include <sys/wait.h> // waitpid()
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>    // getaddrinfo()
#include <signal.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

extern char** environ; // environment variables

static wsp::string wsrep_PATH;

void
wsrep_prepend_PATH (const char* path)
{
    int count = 0;

    while (environ[count])
    {
        if (strncmp (environ[count], "PATH=", 5))
        {
            count++;
            continue;
        }

        char* const old_path (environ[count]);

        if (strstr (old_path, path)) return; // path already there

        size_t const new_path_len(strlen(old_path) + strlen(":") +
                                  strlen(path) + 1);

        char* const new_path (reinterpret_cast<char*>(malloc(new_path_len)));

        if (new_path)
        {
            snprintf (new_path, new_path_len, "PATH=%s:%s", path,
                      old_path + strlen("PATH="));

            wsrep_PATH.set (new_path);
            environ[count] = new_path;
        }
        else
        {
            WSREP_ERROR ("Failed to allocate 'PATH' environment variable "
                         "buffer of size %zu.", new_path_len);
        }

        return;
    }

    WSREP_ERROR ("Failed to find 'PATH' environment variable. "
                 "State snapshot transfer may not be working.");
}

namespace wsp
{

#define PIPE_READ  0
#define PIPE_WRITE 1
#define STDIN_FD   0
#define STDOUT_FD  1

process::process (const char* cmd, const char* type)
    : str_(cmd ? strdup(cmd) : strdup("")), io_(NULL), err_(0), pid_(0)
{
    int sig;
    struct sigaction sa;

    if (0 == str_)
    {
        WSREP_ERROR ("Can't allocate command line of size: %zu", strlen(cmd));
        err_ = ENOMEM;
        return;
    }

    if (0 == strlen(str_))
    {
        WSREP_ERROR ("Can't start a process: null or empty command line.");
        return;
    }

    if (NULL == type || (strcmp (type, "w") && strcmp(type, "r")))
    {
        WSREP_ERROR ("type argument should be either \"r\" or \"w\".");
        return;
    }

    int pipe_fds[2] = { -1, };
    if (::pipe(pipe_fds))
    {
        err_ = errno;
        WSREP_ERROR ("pipe() failed: %d (%s)", err_, strerror(err_));
        return;
    }

    // which end of pipe will be returned to parent
    int const parent_end (strcmp(type,"w") ? PIPE_READ : PIPE_WRITE);
    int const child_end  (parent_end == PIPE_READ ? PIPE_WRITE : PIPE_READ);
    int const close_fd   (parent_end == PIPE_READ ? STDOUT_FD : STDIN_FD);

    pid_ = fork();

    if (pid_ == -1)
    {
      err_= errno;
      WSREP_ERROR ("fork() failed: %d (%s)", err_, strerror(err_));
      pid_ = 0;
      goto cleanup;
    }
    else if (pid_ > 0)
    {
      /* Parent */

      io_ = fdopen (pipe_fds[parent_end], type);

      if (io_)
      {
        pipe_fds[parent_end] = -1; // skip close on cleanup
      }
      else
      {
        err_ = errno;
        WSREP_ERROR ("fdopen() failed: %d (%s)", err_, strerror(err_));
      }

      goto cleanup;
    }

    /* Child */

#ifdef PR_SET_PDEATHSIG
    /*
      Ensure this process gets SIGTERM when the server is terminated for
      whatever reasons.
    */

    if (prctl(PR_SET_PDEATHSIG, SIGTERM))
    {
      sql_perror("prctl() failed");
      _exit(EXIT_FAILURE);
    }
#endif

    /* Reset the process signal mask to unblock signals blocked by the server */

    sigset_t set;
    (void) sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL))
    {
      sql_perror("sigprocmask() failed");
      _exit(EXIT_FAILURE);
    }

    /* Reset all ignored signals to SIG_DFL */

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;

    for (sig = 1; sig < NSIG; sig++)
    {
      /*
        For some signals sigaction() even when SIG_DFL handler is set, so don't
        check for return value here.
      */
      sigaction(sig, &sa, NULL);
    }

    /*
      Make the child process a session/group leader. This is required to
      simplify cleanup procedure for SST process, so that all processes spawned
      by the SST script could be killed by killing the entire process group.
    */

    if (setsid() < 0)
    {
      sql_perror("setsid() failed");
      _exit(EXIT_FAILURE);
    }

    /* close child's stdout|stdin depending on what we returning */

    if (close(close_fd) < 0)
    {
      sql_perror("close() failed");
      _exit(EXIT_FAILURE);
    }

    /* substitute our pipe descriptor in place of the closed one */

    if (dup2(pipe_fds[child_end], close_fd) < 0)
    {
      sql_perror("dup2() failed");
      _exit(EXIT_FAILURE);
    }

    execlp("sh", "sh", "-c", str_, NULL);

    sql_perror("execlp() failed");
    _exit(EXIT_FAILURE);

cleanup:
    if (pipe_fds[0] >= 0) close (pipe_fds[0]);
    if (pipe_fds[1] >= 0) close (pipe_fds[1]);
}

process::~process ()
{
    if (io_)
    {
        assert (pid_);
        assert (str_);

        WSREP_WARN("Closing pipe to child process: %s, PID(%ld) "
                   "which might still be running.", str_, (long)pid_);

        if (fclose (io_) == -1)
        {
            err_ = errno;
            WSREP_ERROR("fclose() failed: %d (%s)", err_, strerror(err_));
        }
    }

    if (str_) free (const_cast<char*>(str_));
}

int
process::wait ()
{
  if (pid_)
  {
      int status;
      if (-1 == waitpid(pid_, &status, 0))
      {
          err_ = errno; assert (err_);
          WSREP_ERROR("Waiting for process failed: %s, PID(%ld): %d (%s)",
                      str_, (long)pid_, err_, strerror (err_));
      }
      else
      {                // command completed, check exit status
          if (WIFEXITED (status)) {
              err_ = WEXITSTATUS (status);
          }
          else {       // command didn't complete with exit()
              WSREP_ERROR("Process was aborted.");
              err_ = errno ? errno : ECHILD;
          }

          if (err_) {
              switch (err_) /* Translate error codes to more meaningful */
              {
              case 126: err_ = EACCES; break; /* Permission denied */
              case 127: err_ = ENOENT; break; /* No such file or directory */
              }
              WSREP_ERROR("Process completed with error: %s: %d (%s)",
                          str_, err_, strerror(err_));
          }

          pid_ = 0;
          if (io_) fclose (io_);
          io_ = NULL;
      }
  }
  else {
      assert (NULL == io_);
      WSREP_ERROR("Command did not run: %s", str_);
  }

  return err_;
}

thd::thd (my_bool won) : init(), ptr(new THD)
{
  if (ptr)
  {
    ptr->thread_stack= (char*) &ptr;
    ptr->store_globals();
    ptr->variables.option_bits&= ~OPTION_BIN_LOG; // disable binlog
    ptr->variables.wsrep_on = won;
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

/* Returns INADDR_NONE, INADDR_ANY, INADDR_LOOPBACK or something else */
unsigned int wsrep_check_ip (const char* const addr)
{
  if (addr && 0 == strcasecmp(addr, MY_BIND_ALL_ADDRESSES)) return INADDR_ANY;

  unsigned int ret = INADDR_NONE;
  struct addrinfo *res, hints;

  memset (&hints, 0, sizeof(hints));
  hints.ai_flags= AI_PASSIVE/*|AI_ADDRCONFIG*/;
  hints.ai_socktype= SOCK_STREAM;
  hints.ai_family= AF_UNSPEC;

  if (strcasecmp(my_bind_addr_str, MY_BIND_ALL_ADDRESSES) != 0)
  {
    int gai_ret = getaddrinfo(addr, NULL, &hints, &res);
    if (0 == gai_ret)
    {
        if (AF_INET == res->ai_family) /* IPv4 */
        {
            struct sockaddr_in* a= (struct sockaddr_in*)res->ai_addr;
            ret= htonl(a->sin_addr.s_addr);
        }
        else /* IPv6 */
        {
            struct sockaddr_in6* a= (struct sockaddr_in6*)res->ai_addr;
            if (IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr))
                ret= INADDR_ANY;
            else if (IN6_IS_ADDR_LOOPBACK(&a->sin6_addr))
                ret= INADDR_LOOPBACK;
            else
                ret= 0xdeadbeef;
        }
        freeaddrinfo (res);
    } else {
        WSREP_ERROR ("getaddrinfo() failed on '%s': %d (%s)",
                    addr, gai_ret, gai_strerror(gai_ret));
    }
  }

  return ret;
}

size_t wsrep_guess_ip (char* buf, size_t buf_len)
{
  size_t ip_len = 0;

  if (my_bind_addr_str && strlen(my_bind_addr_str))
  {
    unsigned int const ip_type= wsrep_check_ip(my_bind_addr_str);

    if (INADDR_NONE == ip_type) {
      WSREP_ERROR("Node IP address not obtained from bind_address, trying alternate methods");
    } else if (INADDR_ANY != ip_type) {
      strncpy (buf, my_bind_addr_str, buf_len);
      return strlen(buf);
    }
  }

  // mysqld binds to all interfaces - try IP from wsrep_node_address
  if (wsrep_node_address && wsrep_node_address[0] != '\0') {
    const char* const colon_ptr = strchr(wsrep_node_address, ':');

    if (colon_ptr)
      ip_len = colon_ptr - wsrep_node_address;
    else
      ip_len = strlen(wsrep_node_address);

    if (ip_len >= buf_len) {
      WSREP_WARN("default_ip(): buffer too short: %zu <= %zd", buf_len, ip_len);
      return 0;
    }

    memcpy (buf, wsrep_node_address, ip_len);
    buf[ip_len] = '\0';
    return ip_len;
  }

  // try to find the address of the first one
#if (TARGET_OS_LINUX == 1)
  const char cmd[] = "ip addr show | grep -E '^[ ]*inet' | grep -m1 global | "
                     "awk '{ print $2 }' | sed -e 's/\\/.*//'";
#elif defined(__sun__)
  const char cmd[] = "/sbin/ifconfig -a | "
      "/usr/gnu/bin/grep -m1 -1 -E 'net[0-9]:' | tail -n 1 | awk '{ print $2 }'";
#elif defined(__APPLE__) || defined(__FreeBSD__)
  const char cmd[] = "/sbin/route -nv get 8.8.8.8 | tail -n1 | awk '{print $(NF)}'";
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
      WSREP_ERROR("Failed to read output of: '%s'", cmd);
      return 0;
    }
  }
  else {
    WSREP_ERROR("Failed to execute: '%s'", cmd);
    return 0;
  }

  // clear possible \n at the end of ip string left by fgets()
  ip_len = strlen (buf);
  if (ip_len > 0 && '\n' == buf[ip_len - 1]) {
    ip_len--;
    buf[ip_len] = '\0';
  }

  if (INADDR_NONE == inet_addr(buf)) {
    if (strlen(buf) != 0) {
      WSREP_WARN("Shell command returned invalid address: '%s'", buf);
    }
    return 0;
  }

  return ip_len;
}

size_t wsrep_guess_address(char* buf, size_t buf_len)
{
  size_t addr_len = wsrep_guess_ip (buf, buf_len);

  if (addr_len && addr_len < buf_len) {
    addr_len += snprintf (buf + addr_len, buf_len - addr_len,
                          ":%u", mysqld_port);
  }

  return addr_len;
}

/*
 * WSREPXid
 */

#define WSREP_XID_PREFIX "WSREPXid"
#define WSREP_XID_PREFIX_LEN MYSQL_XID_PREFIX_LEN
#define WSREP_XID_UUID_OFFSET 8
#define WSREP_XID_SEQNO_OFFSET (WSREP_XID_UUID_OFFSET + sizeof(wsrep_uuid_t))
#define WSREP_XID_GTRID_LEN (WSREP_XID_SEQNO_OFFSET + sizeof(wsrep_seqno_t))

void wsrep_xid_init(XID* xid, const wsrep_uuid_t* uuid, wsrep_seqno_t seqno)
{
  xid->formatID= 1;
  xid->gtrid_length= WSREP_XID_GTRID_LEN;
  xid->bqual_length= 0;
  memset(xid->data, 0, sizeof(xid->data));
  memcpy(xid->data, WSREP_XID_PREFIX, WSREP_XID_PREFIX_LEN);
  memcpy(xid->data + WSREP_XID_UUID_OFFSET, uuid, sizeof(wsrep_uuid_t));
  memcpy(xid->data + WSREP_XID_SEQNO_OFFSET, &seqno, sizeof(wsrep_seqno_t));
}

const wsrep_uuid_t* wsrep_xid_uuid(const XID* xid)
{
  if (wsrep_is_wsrep_xid(xid))
    return reinterpret_cast<const wsrep_uuid_t*>(xid->data
                                                 + WSREP_XID_UUID_OFFSET);
  else
    return &WSREP_UUID_UNDEFINED;
}

wsrep_seqno_t wsrep_xid_seqno(const XID* xid)
{

  if (wsrep_is_wsrep_xid(xid))
  {
    wsrep_seqno_t seqno;
    memcpy(&seqno, xid->data + WSREP_XID_SEQNO_OFFSET, sizeof(wsrep_seqno_t));
    return seqno;
  }
  else
  {
    return WSREP_SEQNO_UNDEFINED;
  }
}

extern
int wsrep_is_wsrep_xid(const void* xid_ptr)
{
  const XID* xid= reinterpret_cast<const XID*>(xid_ptr);
  return (xid->formatID      == 1                   &&
          xid->gtrid_length  == WSREP_XID_GTRID_LEN &&
          xid->bqual_length  == 0                   &&
          !memcmp(xid->data, WSREP_XID_PREFIX, WSREP_XID_PREFIX_LEN));
}
