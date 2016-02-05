/* Copyright 2010-2015 Codership Oy <http://www.codership.com>

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

#include "socket_connection.h"           // MY_BIND_ALL_ADDRESSES

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_GETIFADDRS
#include <net/if.h>
#include <ifaddrs.h>
#endif // HAVE_GETIFADDRS

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

        char* const new_path (static_cast<char*>(malloc(new_path_len)));

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

bool
env::ctor_common(char** e)
{
    env_ = static_cast<char**>(malloc((len_ + 1) * sizeof(char*)));

    if (env_)
    {
        for (size_t i(0); i < len_; ++i)
        {
            assert(e[i]); // caller should make sure about len_
            env_[i] = strdup(e[i]);
            if (!env_[i])
            {
                errno_ = errno;
                WSREP_ERROR("Failed to allocate env. var: %s", e[i]);
                return true;
            }
        }

        env_[len_] = NULL;
        return false;
    }
    else
    {
        errno_ = errno;
        WSREP_ERROR("Failed to allocate env. var vector of length: %zu", len_);
        return true;
    }
}

void
env::dtor()
{
    if (env_)
    {
        /* don't need to go beyond the first NULL */
        for (size_t i(0); env_[i] != NULL; ++i) { free(env_[i]); }
        free(env_);
        env_ = NULL;
    }
    len_ = 0;
}

env::env(char** e)
    : len_(0), env_(NULL), errno_(0)
{
    if (!e) { e = environ; }
    /* count the size of the vector */
    while (e[len_]) { ++len_; }

    if (ctor_common(e)) dtor();
}

env::env(const env& e)
    : len_(e.len_), env_(0), errno_(0)
{
    if (ctor_common(e.env_)) dtor();
}

env::~env() { dtor(); }

int
env::append(const char* val)
{
    char** tmp = static_cast<char**>(realloc(env_, (len_ + 2)*sizeof(char*)));

    if (tmp)
    {
        env_ = tmp;
        env_[len_] = strdup(val);

        if (env_[len_])
        {
            ++len_;
            env_[len_] = NULL;
        }
        else errno_ = errno;
    }
    else errno_ = errno;

    return errno_;
}


#define PIPE_READ  0
#define PIPE_WRITE 1
#define STDIN_FD   0
#define STDOUT_FD  1

process::process (const char* cmd, const char* type, char** env)
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

    if (NULL == env) { env = environ; } // default to global environment

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

#ifdef HAVE_EXECVPE
    char* const pargv[4] = { strdup("sh"), strdup("-c"), strdup(str_), NULL };
    if (!(pargv[0] && pargv[1] && pargv[2]))
    {
        err_ = ENOMEM;
        WSREP_ERROR ("Failed to allocate pargv[] array.");
        goto cleanup_pipe;
    }
#endif

    pid_ = fork();

    if (pid_ == -1)
    {
      err_= errno;
      WSREP_ERROR ("fork() failed: %d (%s)", err_, strerror(err_));
      pid_ = 0;
      goto cleanup_pipe;
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

      goto cleanup_pipe;
    }

    /* Child */

#ifdef PR_SET_PDEATHSIG
    /*
      Ensure this process gets SIGTERM when the server is terminated for
      whatever reasons.
    */

    if (prctl(PR_SET_PDEATHSIG, SIGTERM))
    {
      sql_print_error("prctl() failed");
      _exit(EXIT_FAILURE);
    }
#endif

    /* Reset the process signal mask to unblock signals blocked by the server */

    sigset_t set;
    (void) sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL))
    {
      sql_print_error("sigprocmask() failed");
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
      sql_print_error("setsid() failed");
      _exit(EXIT_FAILURE);
    }

    /* close child's stdout|stdin depending on what we returning */

    if (close(close_fd) < 0)
    {
      sql_print_error("close() failed");
      _exit(EXIT_FAILURE);
    }

    /* substitute our pipe descriptor in place of the closed one */

    if (dup2(pipe_fds[child_end], close_fd) < 0)
    {
      sql_print_error("dup2() failed");
      _exit(EXIT_FAILURE);
    }

#ifdef HAVE_EXECVPE
    execvpe(pargv[0], pargv, env);
#else
    execlp("sh", "sh", "-c", str_, NULL);
#endif

    sql_print_error("execlp() failed");
    _exit(EXIT_FAILURE);

cleanup_pipe:
    if (pipe_fds[0] >= 0) close (pipe_fds[0]);
    if (pipe_fds[1] >= 0) close (pipe_fds[1]);

#ifdef HAVE_EXECVPE
    free (pargv[0]);
    free (pargv[1]);
    free (pargv[2]);
#endif
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
              case 143: err_ = EINTR;  break; /* Subprocess killed */
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
    ptr->m_security_ctx->set_master_access(~(ulong)0);
    lex_start(ptr);
  }
}

thd::~thd ()
{
  if (ptr)
  {
    delete ptr;
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
  }
  else {
    WSREP_ERROR ("getaddrinfo() failed on '%s': %d (%s)",
                 addr, gai_ret, gai_strerror(gai_ret));
  }

  // uint8_t* b= (uint8_t*)&ret;
  // fprintf (stderr, "########## wsrep_check_ip returning: %hhu.%hhu.%hhu.%hhu\n",
  //          b[0], b[1], b[2], b[3]);

  return ret;
}

size_t wsrep_guess_ip (char* buf, size_t buf_len)
{
  size_t ip_len = 0;

  if (my_bind_addr_str && my_bind_addr_str[0] != '\0')
  {
    unsigned int const ip_type= wsrep_check_ip(my_bind_addr_str);

    if (INADDR_NONE == ip_type) {
      WSREP_ERROR("Networking not configured, cannot receive state transfer.");
      return 0;
    }

    if (INADDR_ANY != ip_type) {
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


//
// getifaddrs() is avaiable at least on Linux since glib 2.3, FreeBSD
// MAC OS X, opensolaris, Solaris.
//
// On platforms which do not support getifaddrs() this function returns
// a failure and user is prompted to do manual configuration.
//
#if HAVE_GETIFADDRS
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == 0)
  {
    for (ifa= ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) // TODO AF_INET6
        continue;

      // Skip loopback interfaces (like lo:127.0.0.1)
      if (ifa->ifa_flags & IFF_LOOPBACK)
        continue;

      if (vio_getnameinfo(ifa->ifa_addr, buf, buf_len, NULL, 0, NI_NUMERICHOST))
        continue;

      freeifaddrs(ifaddr);
      return strlen(buf);
    }
    freeifaddrs(ifaddr);
  }
#endif // HAVE_GETIFADDRS

  return 0;
}
