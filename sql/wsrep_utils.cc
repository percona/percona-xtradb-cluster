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
#define _GNU_SOURCE  // POSIX_SPAWN_USEVFORK flag
#endif

#include "wsrep_utils.h"
#include "wsrep_mysqld.h"
#include "mysql/components/services/log_builtins.h"

#include "sql_class.h"
#include "wsrep_api.h"

#include <errno.h>   // errno
#include <netdb.h>   // getaddrinfo()
#include <signal.h>  // sigemptyset(), sigaddset()
#include <signal.h>
#include <string.h>  // strerror()
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>  // waitpid()
#include <unistd.h>    // pipe()

#include "sql/conn_handler/socket_connection.h"  // MY_BIND_ALL_ADDRESSES
#include "sql/sql_lex.h"                         // lex_start/lex_end

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef _POSIX_SPAWN
#include <spawn.h>
#endif

#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#include <net/if.h>
#endif  // HAVE_GETIFADDRS

#include "mysql/psi/mysql_file.h"

extern char **environ;  // environment variables

static wsp::string wsrep_PATH;

void wsrep_prepend_PATH(const char *path) {
  int count = 0;

  while (environ[count]) {
    if (strncmp(environ[count], "PATH=", 5)) {
      count++;
      continue;
    }

    char *const old_path(environ[count]);

    if (strstr(old_path, path)) return;  // path already there

    size_t const new_path_len(strlen(old_path) + strlen(":") + strlen(path) +
                              1);

    char *const new_path(static_cast<char *>(malloc(new_path_len)));

    if (new_path) {
      snprintf(new_path, new_path_len, "PATH=%s:%s", path,
               old_path + strlen("PATH="));

      wsrep_PATH.set(new_path);
      environ[count] = new_path;
    } else {
      WSREP_ERROR(
          "Failed to allocate 'PATH' environment variable "
          "buffer of size %zu.",
          new_path_len);
    }

    return;
  }

  WSREP_ERROR(
      "Failed to find 'PATH' environment variable. "
      "State snapshot transfer may not be working.");
}

/**
 * This is a wrapper around the WSREP_ON macro.  This allows this check
 * to be called from plugins (like memcached, keyring, etc....).
 */
extern "C" bool wsrep_is_wsrep_on(void) { return WSREP_ON; }

namespace wsp {

bool env::ctor_common(char **e) {
  env_ = static_cast<char **>(malloc((len_ + 1) * sizeof(char *)));

  if (env_) {
    for (size_t i(0); i < len_; ++i) {
      assert(e[i]);  // caller should make sure about len_
      env_[i] = strdup(e[i]);
      if (!env_[i]) {
        errno_ = errno;
        WSREP_ERROR("Failed to allocate environment variable: %s", e[i]);
        return true;
      }
    }

    env_[len_] = NULL;
    return false;
  } else {
    errno_ = errno;
    WSREP_ERROR("Failed to allocate environment variable vector of length: %zu",
                len_);
    return true;
  }
}

void env::dtor() {
  if (env_) {
    /* don't need to go beyond the first NULL */
    for (size_t i(0); env_[i] != NULL; ++i) {
      free(env_[i]);
    }
    free(env_);
    env_ = NULL;
  }
  len_ = 0;
}

env::env(char **e) : len_(0), env_(NULL), errno_(0) {
  if (!e) {
    e = environ;
  }
  /* count the size of the vector */
  while (e[len_]) {
    ++len_;
  }

  if (ctor_common(e)) dtor();
}

env::env(const env &e) : len_(e.len_), env_(0), errno_(0) {
  if (ctor_common(e.env_)) dtor();
}

env::~env() { dtor(); }

int env::append(const char *val) {
  char **tmp = static_cast<char **>(realloc(env_, (len_ + 2) * sizeof(char *)));

  if (tmp) {
    env_ = tmp;
    env_[len_] = strdup(val);

    if (env_[len_]) {
      ++len_;
      env_[len_] = NULL;
    } else
      errno_ = errno;
  } else
    errno_ = errno;

  return errno_;
}

#if !(defined(_POSIX_SPAWN) || defined(HAVE_EXECVPE))

#include <alloca.h>

#define _PATH_BSHELL "/bin/sh"

/*
  The Unix standard contains a long explanation of the way to signal
  an error after the fork() was successful.  Since no new wait status
  was wanted there is no way to signal an error using one of the
  available methods.  The committee chose to signal an error by a
  normal program exit with the exit code 127.
*/

#define SPAWN_ERROR 127

/*
  The file is accessible but it is not an executable file. Invoke
  the shell to interpret it as a script.
*/
static void script_execute(const char *file, char *const argv[],
                           char *const envp[]) {
  /* Count the arguments. */
  int argc = 0;
  while (argv[argc++])
    ;

  /* Construct an argument list for the shell. */
  {
    char **new_argv = static_cast<char **>(alloca(sizeof(char *) * (argc + 1)));
    new_argv[0] = (char *)_PATH_BSHELL;
    new_argv[1] = (char *)file;
    while (argc > 1) {
      new_argv[argc] = argv[argc - 1];
      --argc;
    }
    /* Execute the shell. */
    execve(new_argv[0], new_argv, envp);
  }
}

/*
  execvpe() emulation for old versions of Linux, based
  on glibc implementation:
*/

static int execvpe(const char *file, char *const argv[], char *const envp[]) {
  char *path, *p, *name;
  size_t len;
  size_t pathlen;
  bool got_eacces = false;

  if (file == NULL || *file == '\0') {
    /* We check the simple case first. */
    errno = ENOENT;
    return -1;
  }

  if (strchr(file, '/') != NULL) {
    /* The FILE parameter is actually a path. */
    execve(file, argv, envp);

    if (errno == ENOEXEC) script_execute(file, argv, envp);

    /* Oh, oh. `execve' returns. This is bad. */
    _exit(SPAWN_ERROR);
  }

  /* We have to search for FILE on the path. */
  path = getenv("PATH");
  if (path == NULL) {
    /*
      There is no `PATH' in the environment.
      The default search path is the current directory
       followed by the path `confstr' returns for `_CS_PATH'.
    */
    len = confstr(_CS_PATH, (char *)NULL, 0);
    path = static_cast<char *>(alloca(1 + len));
    path[0] = ':';
    (void)confstr(_CS_PATH, path + 1, len);
  }

  len = strlen(file) + 1;
  pathlen = strlen(path);
  name = static_cast<char *>(alloca(pathlen + len + 1));
  /* Copy the file name at the top. */
  name = (char *)memcpy(name + pathlen + 1, file, len);
  /* And add the slash. */
  *--name = '/';

  p = path;
  do {
    char *startp;

    path = p;
    p = strchrnul(path, ':');

    if (p == path) {
      /*
        Two adjacent colons, or a colon at the beginning or the end
        of `PATH' means to search the current directory.
      */
      startp = name + 1;
    } else
      startp = (char *)memcpy(name - (p - path), path, p - path);

    /* Try to execute this name. If it works, execv will not return. */
    execve(startp, argv, envp);

    if (errno == ENOEXEC) script_execute(startp, argv, envp);

    switch (errno) {
      case EACCES:
        /* Record the we got a `Permission denied' error. If we end
           up finding no executable we can use, we want to diagnose
           that we did find one but were denied access. */
        got_eacces = true;
      case ENOENT:
      case ESTALE:
      case ENOTDIR:
        /* Those errors indicate the file is missing or not executable
           by us, in which case we want to just try the next path
           directory. */
      case ENODEV:
      case ETIMEDOUT:
        /* Some strange filesystems like AFS return even
           stranger error numbers.  They cannot reasonably mean
           anything else so ignore those, too. */
        break;
      default:
        /* Some other error means we found an executable file, but
           something went wrong executing it; return the error to our
           caller. */
        return -1;
    }
  } while (*p++ != '\0');

  /* We tried every element and none of them worked. */
  if (got_eacces) {
    /*
      At least one failure was due to permissions,
      so report that  error.
    */
    errno = EACCES;
  }

  /* Return with an error. */
  return -1;
}

#endif

#define PIPE_READ 0
#define PIPE_WRITE 1
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2

process::process(const char *cmd, const char *type, char **env, bool execute_immediately)
    : str_(cmd ? strdup(cmd) : strdup("")), io_(NULL), io_w_(NULL), io_err_(NULL), err_(0), pid_(0)
  {
    if (execute_immediately)
      execute(type, env);
  }

void process::execute(const char *type, char **env)
{
  int sig;
  struct sigaction sa;

  if (0 == str_) {
    WSREP_ERROR("Can't allocate command line of size: %zu", strlen(str_));
    err_ = ENOMEM;
    return;
  }

  if (0 == strlen(str_)) {
    WSREP_ERROR("Can't start a process: null or empty command line.");
    return;
  }

  if (NULL == type || (strcmp(type, "w") && strcmp(type, "r") && strcmp(type, "rw"))) {
    WSREP_ERROR("type argument should be either \"r\" or \"w\" or \"rw\".");
    return;
  }

  if (NULL == env) {
    env = environ;
  }  // default to global environment

  int pipe_fds[2] = { -1, -1 };
  int pipe2_fds[2] = { -1, -1 };
  int pipeerr_fds[2] = { -1, -1 };

  if (::pipe(pipe_fds)) {
    err_ = errno;
    WSREP_ERROR("pipe() failed: %d (%s)", err_, strerror(err_));
    return;
  }

  // which end of pipe will be returned to parent
  int const parent_end(strcmp(type, "w") ? PIPE_READ : PIPE_WRITE);
  int const child_end(parent_end == PIPE_READ ? PIPE_WRITE : PIPE_READ);
  int const close_fd(parent_end == PIPE_READ ? STDOUT_FD : STDIN_FD);

  char *const pargv[4] = {strdup("sh"), strdup("-c"), strdup(str_), NULL};

  // Create the second pipe (needed only if type = "rw")
  // One pipe for reading and one pipe for writing
  if (strcmp(type, "rw") == 0 && ::pipe(pipe2_fds))
  {
    err_ = errno;
    WSREP_ERROR ("pipe() failed to create the second pipe: %d (%s)", err_, strerror(err_));
    goto cleanup_pipe;
  }

  if (::pipe(pipeerr_fds)) {
    err_ = errno;
    WSREP_ERROR("pipe() failed to create the error pipe: %d (%s)", err_, strerror(err_));
    goto cleanup_pipe;
  }

  if (!(pargv[0] && pargv[1] && pargv[2])) {
    err_ = ENOMEM;
    WSREP_ERROR("Failed to allocate pargv[] array.");
    goto cleanup_pipe;
  }

#ifndef _POSIX_SPAWN

  pid_ = fork();

  if (pid_ == -1) {
    err_ = errno;
    WSREP_ERROR("fork() failed: %d (%s)", err_, strerror(err_));
    pid_ = 0;
    goto cleanup_pipe;
  } else if (pid_ > 0) {
    /* Parent */

    // Treat 'rw' as an 'r'
    io_ = fdopen(pipe_fds[parent_end],  (strcmp(type,"rw") == 0 ? "r" : type));

    if (io_) {
      pipe_fds[parent_end] = -1;  // skip close on cleanup
    } else {
      err_ = errno;
      WSREP_ERROR("fdopen() failed: %d (%s)", err_, strerror(err_));
    }

    if (strcmp(type, "rw") == 0)
    {
      // Need to open the write end of the pipe
      io_w_ = fdopen(pipe2_fds[PIPE_WRITE], "w");
      if (io_w_)
      {
        pipe2_fds[PIPE_WRITE] = -1; // skip close on cleanup
      }
      else
      {
        err_ = errno;
        WSREP_ERROR ("fdopen() failed: %d (%s)", err_, strerror(err_));
      }
    }

    io_err_ = fdopen(pipeerr_fds[PIPE_READ], "r");
    if (io_err_)
    {
      pipeerr_fds[PIPE_READ] = -1;  // skip close on cleanup
    }
    else
    {
      err = errno;
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

  if (prctl(PR_SET_PDEATHSIG, SIGTERM)) {
    WSREP_ERROR("prctl() failed");
    _exit(EXIT_FAILURE);
  }
#endif

  /* Reset the process signal mask to unblock signals blocked by the server */

  sigset_t set;
  (void)sigemptyset(&set);

  if (sigprocmask(SIG_SETMASK, &set, NULL)) {
    WSREP_ERROR("sigprocmask() failed");
    _exit(EXIT_FAILURE);
  }

  /* Reset all ignored signals to SIG_DFL */

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_DFL;

  for (sig = 1; sig < NSIG; sig++) {
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

  if (setsid() < 0) {
    WSREP_ERROR("setsid() failed");
    _exit(EXIT_FAILURE);
  }

  /* Close child's stdout|stdin depending on what we returning.    */
  /* PXC-502: commented out, because subsequent dup2() call closes */
  /* close_fd descriptor automatically.                            */
  /*
  if (close(close_fd) < 0)
  {
    sql_print_error("close() failed");
    _exit(EXIT_FAILURE);
  }
  */

  /* substitute our pipe descriptor in place of the closed one */

  if (dup2(pipe_fds[child_end], close_fd) < 0) {
    WSREP_ERROR("dup2() failed");
    _exit(EXIT_FAILURE);
  }
  if ((strcmp(type, "rw") == 0) && dup2(pipe2_fds[PIPE_READ], STDIN_FD) < 0)
  {
    WSREP_ERROR("dup2() failed");
    _exit(EXIT_FAILURE);
  }
  if (dup2(pipeerr_fds[PIPE_WRITE], STDERR_FD) < 0) {
    WSREP_ERROR("dup2() failed");
    _exit(EXIT_FAILURE);
  }

  /* Close child and parent pipe descriptors after redirection. */

  if (close(pipe_fds[child_end]) < 0) {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  if (close(pipe_fds[parent_end]) < 0) {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  if (close(pipe2_fds[child_end]) < 0)
  {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  if (close(pipe2_fds[parent_end]) < 0)
  {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  if (close(pipeerr_fds[PIPE_WRITE]) < 0)
  {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  if (close(pipeerr_fds[PIPE_READ]) < 0)
  {
    WSREP_ERROR("close() failed");
    _exit(EXIT_FAILURE);
  }

  execvpe(pargv[0], pargv, env);

  WSREP_ERROR("execlp() failed");
  _exit(EXIT_FAILURE);

#else  // _POSIX_SPAWN is defined:

  posix_spawnattr_t sattr;

  err_ = posix_spawnattr_init(&sattr);
  if (err_) {
    WSREP_ERROR("posix_spawnattr_init() failed: %d (%s)", err_, strerror(err_));
    goto cleanup_pipe;
  }

  /*
    Make the child process a session/group leader. This is required to
    simplify cleanup procedure for SST process, so that all processes spawned
    by the SST script could be killed by killing the entire process group:
  */

  err_ = posix_spawnattr_setpgroup(&sattr, 0);
  if (err_) {
    WSREP_ERROR("posix_spawnattr_setpgroup() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_attr;
  }

  /* Reset the process signal mask to unblock signals blocked by the server: */

  sigset_t set;

  err_ = sigemptyset(&set);
  if (err_) {
    err_ = errno;
    WSREP_ERROR("sigemptyset() failed: %d (%s)", err_, strerror(err_));
    goto cleanup_attr;
  }

  err_ = posix_spawnattr_setsigmask(&sattr, &set);
  if (err_) {
    WSREP_ERROR("posix_spawnattr_setsigmask() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_attr;
  }

  /* Reset all ignored signals to SIG_DFL: */

  int def_flag;

  def_flag = 0;

  for (sig = 1; sig < NSIG; sig++) {
    if (sigaction(sig, NULL, &sa) == 0) {
      if (sa.sa_handler == SIG_IGN) {
        err_ = sigaddset(&set, sig);
        if (err_) {
          err_ = errno;
          WSREP_ERROR("sigaddset() failed: %d (%s)", err_, strerror(err_));
          goto cleanup_attr;
        }
        def_flag = POSIX_SPAWN_SETSIGDEF;
      }
    }
  }

  if (def_flag) {
    err_ = posix_spawnattr_setsigdefault(&sattr, &set);
    if (err_) {
      WSREP_ERROR("posix_spawnattr_setsigdefault() failed: %d (%s)", err_,
                  strerror(err_));
      goto cleanup_attr;
    }
  }

  /* Set flags for all modified parameters: */

#ifdef POSIX_SPAWN_USEVFORK
  def_flag |= POSIX_SPAWN_USEVFORK;
#endif

  err_ = posix_spawnattr_setflags(
      &sattr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | def_flag);
  if (err_) {
    WSREP_ERROR("posix_spawnattr_setflags() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_attr;
  }

  /* Substitute our pipe descriptor in place of the stdin or stdout: */

  posix_spawn_file_actions_t sfa;

  err_ = posix_spawn_file_actions_init(&sfa);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_init() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_attr;
  }

  err_ = posix_spawn_file_actions_adddup2(&sfa, pipe_fds[child_end], close_fd);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_adddup2() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }

  err_ = posix_spawn_file_actions_addclose(&sfa, pipe_fds[child_end]);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_addclose() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }

  err_ = posix_spawn_file_actions_addclose(&sfa, pipe_fds[parent_end]);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_addclose() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }

  if (strcmp(type, "rw") == 0)
  {
    err_ = posix_spawn_file_actions_adddup2(&sfa, pipe2_fds[PIPE_READ], STDIN_FD);
    if (err_)
    {
      WSREP_ERROR ("posix_spawn_file_actions_adddup2() failed: %d (%s)", err_, strerror(err_));
      goto cleanup_actions;
    }

    err_ = posix_spawn_file_actions_addclose(&sfa, pipe2_fds[PIPE_READ]);
    if (err_)
    {
      WSREP_ERROR ("posix_spawn_file_actions_addclose() failed: %d (%s)", err_, strerror(err_));
      goto cleanup_actions;
    }

    err_ = posix_spawn_file_actions_addclose(&sfa, pipe2_fds[PIPE_WRITE]);
    if (err_)
    {
      WSREP_ERROR ("posix_spawn_file_actions_addclose() failed: %d (%s)", err_, strerror(err_));
      goto cleanup_actions;
    }
  }

  err_ = posix_spawn_file_actions_adddup2(&sfa, pipeerr_fds[PIPE_WRITE], STDERR_FD);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_adddup2() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }

  err_ = posix_spawn_file_actions_addclose(&sfa, pipeerr_fds[PIPE_WRITE]);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_addclose() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }

  err_ = posix_spawn_file_actions_addclose(&sfa, pipeerr_fds[PIPE_READ]);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_addclose() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_actions;
  }


  /* Launch the child process: */

  err_ = posix_spawnp(&pid_, pargv[0], &sfa, &sattr, pargv, env);
  if (err_) {
    WSREP_ERROR("posix_spawnp() failed: %d (%s)", err_, strerror(err_));
    pid_ = 0;
    goto cleanup_actions;
  }

  err_ = posix_spawn_file_actions_destroy(&sfa);
  if (err_) {
    WSREP_ERROR("posix_spawn_file_actions_destroy() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_attr;
  }

  err_ = posix_spawnattr_destroy(&sattr);
  if (err_) {
    WSREP_ERROR("posix_spawnattr_destroy() failed: %d (%s)", err_,
                strerror(err_));
    goto cleanup_pipe;
  }

  /* We are in the parent process: */

  io_ = fdopen(pipe_fds[parent_end], (strcmp(type, "rw") == 0 ? "r" : type));

  if (io_) {
    pipe_fds[parent_end] = -1;  // skip close on cleanup
  } else {
    err_ = errno;
    WSREP_ERROR("fdopen() failed: %d (%s)", err_, strerror(err_));
  }

  if (strcmp(type, "rw") == 0)
  {
    io_w_ = fdopen(pipe2_fds[PIPE_WRITE], "w");

    if (io_w_)
    {
      pipe2_fds[PIPE_WRITE] = -1; // skip close on cleanup
    }
    else
    {
      err_ = errno;
      WSREP_ERROR ("fdopen() failed: %d (%s)", err_, strerror(err_));
    }
  }

  io_err_ = fdopen(pipeerr_fds[PIPE_READ], "r");

  if (io_err_) {
    pipeerr_fds[PIPE_READ] = -1;  // skip close on cleanup
  } else {
    err_ = errno;
    WSREP_ERROR("fdopen() failed: %d (%s)", err_, strerror(err_));
  }


#endif

cleanup_pipe:
  if (pipe_fds[0] >= 0) close(pipe_fds[0]);
  if (pipe_fds[1] >= 0) close(pipe_fds[1]);
  if (pipe2_fds[0] >= 0) close (pipe2_fds[0]);
  if (pipe2_fds[1] >= 0) close (pipe2_fds[1]);
  if (pipeerr_fds[0] >= 0) close (pipeerr_fds[0]);
  if (pipeerr_fds[1] >= 0) close (pipeerr_fds[1]);

  free(pargv[0]);
  free(pargv[1]);
  free(pargv[2]);

#ifdef _POSIX_SPAWN
  return;

cleanup_actions:
  posix_spawn_file_actions_destroy(&sfa);

cleanup_attr:
  posix_spawnattr_destroy(&sattr);
  goto cleanup_pipe;

#endif
}

process::~process() {
  if (io_) {
    assert(pid_);
    assert(str_);

    WSREP_WARN(
        "Closing pipe to child process: %s, PID(%ld) "
        "which might still be running.",
        str_, (long)pid_);

    if (fclose(io_) == -1) {
      err_ = errno;
      WSREP_ERROR("fclose() failed: %d (%s)", err_, strerror(err_));
    }
  }

  if (io_w_)
  {
      assert (pid_);
      assert (str_);

      WSREP_WARN("Closing write pipe to child process: %s, PID(%ld) "
                 "which might still be running.", str_, (long)pid_);

      if (fclose (io_w_) == -1)
      {
          err_ = errno;
          WSREP_ERROR("fclose() failed: %d (%s)", err_, strerror(err_));
      }
  }

  if (io_err_)
  {
    WSREP_WARN(
        "Closing pipe to child process: %s, PID(%ld) "
        "which might still be running.",
        str_, (long)pid_);

    if (fclose(io_err_) == -1) {
      err_ = errno;
      WSREP_ERROR("fclose() failed: %d (%s)", err_, strerror(err_));
    }
  }

  if (str_) free(const_cast<char *>(str_));
}

void process::close_write_pipe()
{
    if (io_w_)
    {
        assert (pid_);
        assert (str_);

        if (fclose (io_w_) == -1)
        {
            err_ = errno;
            WSREP_ERROR("fclose() failed: %d (%s)", err_, strerror(err_));
        }
        io_w_ = NULL;
    }
}

int process::wait() {
  if (pid_) {
    int status;
    if (-1 == waitpid(pid_, &status, 0)) {
      err_ = errno;
      assert(err_);
      WSREP_ERROR("Waiting for process failed: %s, PID(%ld): %d (%s)", str_,
                  (long)pid_, err_, strerror(err_));
    } else {  // command completed, check exit status
      if (WIFEXITED(status)) {
        err_ = WEXITSTATUS(status);
      } else {  // command didn't complete with exit()
        WSREP_ERROR("Process was aborted.");
        err_ = errno ? errno : ECHILD;
      }

      if (err_) {
        switch (err_) /* Translate error codes to more meaningful */
        {
          case 126:
            err_ = EACCES;
            break; /* Permission denied */
          case 127:
            err_ = ENOENT;
            break; /* No such file or directory */
          case 143:
            err_ = EINTR;
            break; /* Subprocess killed */
        }
        WSREP_ERROR("Process completed with error: %s: %d (%s)", str_, err_,
                    strerror(err_));
      }

      pid_ = 0;
      if (io_) fclose(io_);
      io_ = NULL;
      if (io_w_) fclose(io_w_);
      io_w_ = NULL;
      if (io_err_) fclose(io_err_);
      io_err_ = NULL;
    }
  } else {
    assert(NULL == io_);
    WSREP_ERROR("Command did not run: %s", str_);
  }

  return err_;
}

void process::terminate() {
  if (pid_) {
    /*
      If we have an appropriated system call, then we try
      to terminate entire process group:
    */
#if _XOPEN_SOURCE >= 500 || _DEFAULT_SOURCE || _BSD_SOURCE
    if (killpg(pid_, SIGTERM))
#else
    if (kill(pid_, SIGTERM))
#endif
    {
      WSREP_WARN("Unable to terminate process: %s: %d (%s)", str_, errno,
                 strerror(errno));
    }
  }
}

thd::thd(bool won) : init(), ptr(new THD) {
  if (ptr) {
    ptr->thread_stack = (char *)&ptr;
    ptr->store_globals();
    ptr->variables.option_bits &= ~OPTION_BIN_LOG;  // disable binlog
    ptr->variables.wsrep_on = won;
    ptr->m_security_ctx->set_master_access(~(ulong)0);
    lex_start(ptr);
  }
}

thd::~thd() {
  if (ptr) {
    delete ptr;
  }
}


void WSREPState::parse_version(const char *str, uint& major, uint& minor, uint& revision)
{
  char * end;
  major = minor = revision = 0;

  if (!str || !*str)
    return;

  major = strtoul(str, &end, 10);
  if (*end)
    minor = strtoul(end+1, &end, 10);
  if (*end)
    revision = strtoul(end+1, &end, 10);

  /* Ensure that we aren't overflowing any values */
  if (major > 99) major = 99;
  if (minor > 99) minor = 99;
  if (revision > 99) revision = 99;
  return;
}

bool WSREPState::wsrep_schema_version_equals(const char *server_version)
{
  uint  wsrep_major, wsrep_minor, wsrep_revision;
  uint  server_major, server_minor, server_revision;

  parse_version(this->wsrep_schema_version.c_str(), wsrep_major, wsrep_minor, wsrep_revision);
  parse_version(server_version, server_major, server_minor, server_revision);

  return wsrep_major == server_major &&
         wsrep_minor == server_minor &&
         wsrep_revision == server_revision;
}


bool WSREPState::load_from(const char *dir, const char *filename)
{
  MYSQL_FILE *file;
  int         errcode = 0;
  const int   full_path_len = 1024;
  char        full_path[full_path_len];
  const int   buf_len = 256;
  char        buf[buf_len];

  if (snprintf(full_path, full_path_len,
               "%s/%s", dir ? dir : "", filename ? filename : "") <= 0)
  {
    WSREP_ERROR("WSREPState::load_from() snprintf failed");
    return false;
  }

  file = mysql_file_fopen(key_file_misc, full_path, O_RDONLY, MYF(0));
  if (!file)
  {
    WSREP_WARN("Could not open the wsrep state file : %s", full_path);
    return false;
  }

  /* Clear out all data */
  clear();

  /* Loop over the file and read it in */
  while (mysql_file_fgets(buf, buf_len, file) != NULL)
  {
    /* Examine the line */
    char *line = buf;
    while (*line && isspace(*line))
      line++;

    /* Are we at the end of the line? */
    if (*line == 0)
      continue;

    /* If the line starts with a '#', then it's a comment */
    if (line[0] == '#')
      continue;

    char *sep = strchr(line, '=');
    if (sep == NULL)
    {
      WSREP_ERROR("Unexpected line formatting (expected '=') : %s", buf);
      errcode = EINVAL;
      break;
    }

    /* Separate the name/value */
    *sep = 0;
    char * name = line;
    char * value = sep+1;

    if (strcmp(name, WSREP_SCHEMA_VERSION_NAME) == 0)
    {
      uint  major, minor, revision;
      const int buf_len=32;
      char buf[buf_len];
      this->parse_version(value, major, minor, revision);
      if (snprintf(buf, buf_len, "%d.%d.%d", major, minor, revision) <= 0)
      {
        WSREP_ERROR("WSREPState::load_from() snprintf failed to format the version");
        errcode = EINVAL;
        break;
      }
      this->wsrep_schema_version = buf;
    }
    else if (strcmp(name, WSREP_STATE_FILE_VERSION_NAME) == 0)
    {
      /* Only allow version 1.x */
      uint  major, minor, revision;
      this->parse_version(value, major, minor, revision);
      if (major != 1)
      {
        WSREP_ERROR("WSREPState::load_from() unsupported/unknown version: %s", value);
        errcode = EINVAL;
        break;
      }
    }
    else
    {
      /*
       Unknown name
       Issue a warning, but continue to process the entire file
      */
      WSREP_WARN("Unknown state file entry: %s=%s", name, value);
    }
  }

  if (this->wsrep_schema_version.empty())
  {
    WSREP_ERROR("Could not locate the wsrep schema version in the wsrep_state.dat file");
    errcode = EINVAL;
  }
  mysql_file_fclose(file, MYF(0));
  return errcode == 0;
}

bool WSREPState::save_to(const char *dir, const char *filename)
{
  MYSQL_FILE *file;
  const int   buf_len=1024;
  char        buf[buf_len];

  /* Normalize the wsrep_schema_version */
  uint        major, minor, revision;
  parse_version(this->wsrep_schema_version.c_str(), major, minor, revision);
  if (snprintf(buf, buf_len, "%d.%d.%d", major, minor, revision) <= 0)
  {
    /* Failure to format */
    WSREP_ERROR("WSREPState::save_to() snprintf failed to format the version");
    return false;
  }
  this->wsrep_schema_version = buf;

  if (snprintf(buf, buf_len, "%s/%s", dir ? dir : "", filename ? filename : "") <= 0)
  {
    WSREP_ERROR("WSREPState::save_to() snprintf failed");
    return false;
  }

  file = mysql_file_fopen(key_file_misc, buf, O_TRUNC | O_CREAT | O_WRONLY, MYF(0));
  if (!file)
  {
    WSREP_ERROR("Could not open the wsrep state file : %s", buf);
    return false;
  }
  int errcode;
  errcode = mysql_file_fprintf(file,
        "# WSREP state file (created on initialization)\n"
        "%s=%s\n"
        "%s=%s\n",
        WSREP_STATE_FILE_VERSION_NAME, WSREP_STATE_FILE_VERSION,
        WSREP_SCHEMA_VERSION_NAME, this->wsrep_schema_version.c_str()
      );
  mysql_file_fclose(file, MYF(0));

  if (errcode <= 0) {
    WSREP_ERROR("Error while writing to the wsrep state file");
    return false;
  }
  return true;
}

bool WSREPState::node_needs_upgrading()
{
    wsp::WSREPState  wsrep_state;

    /* If we can't load the data file, assume the node is out-of-date (i.e. from 5.7) */
    if (!wsrep_state.load_from(mysql_real_data_home_ptr, WSREP_STATE_FILENAME))
      return true;

    if (!wsrep_state.wsrep_schema_version_equals(WSREP_SCHEMA_VERSION))
    {
      WSREP_WARN("WSREP schema versions  server:%s  datadir:%s", MYSQL_SERVER_VERSION, wsrep_state.wsrep_schema_version.c_str());
      return true;
    }

    return false;
}

}  // namespace wsp

/* Returns INADDR_NONE, INADDR_ANY, INADDR_LOOPBACK or something else */
unsigned int wsrep_check_ip(const char *const addr, bool *is_ipv6) {
  if (addr && 0 == strcasecmp(addr, MY_BIND_ALL_ADDRESSES)) return INADDR_ANY;

  unsigned int ret = INADDR_NONE;
  struct addrinfo *res, hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE /*|AI_ADDRCONFIG*/;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int gai_ret = getaddrinfo(addr, NULL, &hints, &res);
  if (0 == gai_ret) {
    if (AF_INET == res->ai_family) /* IPv4 */
    {
      struct sockaddr_in *a = (struct sockaddr_in *)res->ai_addr;
      ret = htonl(a->sin_addr.s_addr);
    } else /* IPv6 */
    {
      struct sockaddr_in6 *a = (struct sockaddr_in6 *)res->ai_addr;
      if (IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr))
        ret = INADDR_ANY;
      else if (IN6_IS_ADDR_LOOPBACK(&a->sin6_addr))
        ret = INADDR_LOOPBACK;
      else
        ret = 0xdeadbeef;

      *is_ipv6 = true;
    }
    freeaddrinfo(res);
  } else {
    WSREP_ERROR("getaddrinfo() failed on '%s': %d (%s)", addr, gai_ret,
                gai_strerror(gai_ret));
  }

  // uint8_t* b= (uint8_t*)&ret;
  // fprintf (stderr, "########## wsrep_check_ip returning:
  // %hhu.%hhu.%hhu.%hhu\n",
  //          b[0], b[1], b[2], b[3]);

  return ret;
}

size_t wsrep_guess_ip(char *buf, size_t buf_len) {
  size_t ip_len = 0;

  if (my_bind_addr_str && my_bind_addr_str[0] != '\0') {
    bool unused;
    unsigned int const ip_type = wsrep_check_ip(my_bind_addr_str, &unused);

    if (INADDR_NONE == ip_type) {
      WSREP_ERROR("Networking not configured, cannot receive state transfer.");
      return 0;
    }

    if (INADDR_ANY != ip_type) {
      strncpy(buf, my_bind_addr_str, buf_len);
      return strlen(buf);
    }
  }

  // mysqld binds to all interfaces - try IP from wsrep_node_address
  if (wsrep_node_address && wsrep_node_address[0] != '\0') {
    const char *const colon_ptr = strchr(wsrep_node_address, ':');

    if (colon_ptr)
      ip_len = colon_ptr - wsrep_node_address;
    else
      ip_len = strlen(wsrep_node_address);

    if (ip_len >= buf_len) {
      WSREP_WARN("default_ip(): buffer too short: %zu <= %zd", buf_len, ip_len);
      return 0;
    }

    memcpy(buf, wsrep_node_address, ip_len);
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
  if (getifaddrs(&ifaddr) == 0) {
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if (!ifa->ifa_addr ||
          ifa->ifa_addr->sa_family != AF_INET)  // TODO AF_INET6
        continue;

      // Skip loopback interfaces (like lo:127.0.0.1)
      if (ifa->ifa_flags & IFF_LOOPBACK) continue;

      if (vio_getnameinfo(ifa->ifa_addr, buf, buf_len, NULL, 0, NI_NUMERICHOST))
        continue;

      freeifaddrs(ifaddr);
      return strlen(buf);
    }
    freeifaddrs(ifaddr);
  }
#endif  // HAVE_GETIFADDRS

  return 0;
}

/* returns the length of the host part of the address string */
size_t wsrep_host_len(const char *const addr, size_t const addr_len) {
  // check for IPv6 notation first
  const char *const bracket = ('[' == addr[0] ? strchr(addr, ']') : NULL);

  if (bracket) {  // IPv6
    return (bracket - addr + 1);
  } else {  // host part ends at ':' or end of string
    const char *const colon = strchr(addr, ':');
    return (colon ? colon - addr : addr_len);
  }
}
