#include "server_status_file.h"

#include <assert.h>
#include <sys/file.h>
#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <thread>

#include "my_alloc.h"
#include "my_getopt.h"
#include "my_io.h"

#include "mysqld_error.h"

#include <mysql/components/services/log_builtins.h>
#include <sql/server_component/log_builtins_internal.h>
#include "sql/mysqld.h"
#include "sql/raii/sentry.h"  // raii::Sentry

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace Server_status_file {

static constexpr auto path_separator =
    std::filesystem::path::preferred_separator;
static const std::string m_server_state_filename("server.state");
static bool m_create_server_state_file = false;
static std::string m_server_state_file_path;

static std::map<Status, std::string> status2string{
    {Status::INITIALIZING, "INITIALIZING"},
    {Status::DOWNGRADING, "DOWNGRADING"},
    {Status::UPGRADING, "UPGRADING"},
    {Status::READY, "READY"},
    {Status::STOPPING, "STOPPING"},
    {Status::STOPPED, "STOPPED"},
    {Status::UNKNOWN, "UNKNOWN"}};

void log(const char *msg) {
  /* It is not nice to call from internals, but if we are just
     deinitializing the server, log may be disabled already. */

  if (log_builtins_started()) {
    LogErr(WARNING_LEVEL, ER_LOG_PRINTF_MSG, msg);
  }
}

static int delete_status_file() {
  return unlink(m_server_state_file_path.c_str());
}

/* Try to acquire desired lock on file within a specified time.
   Returns 0 in case of success, -1 otherwise. */
static int flockt(int fd, int operation, std::chrono::milliseconds timeout) {
  static const std::chrono::milliseconds loop_wait_time = 500ms;
  std::chrono::milliseconds wait_total = 0ms;

  while (flock(fd, operation | LOCK_NB) == -1) {
    if (errno == EWOULDBLOCK) {
      if (wait_total >= timeout) {
        // failed to lock
        return -1;
      }
      std::this_thread::sleep_for(loop_wait_time);
      wait_total += loop_wait_time;
    } else {
      return -1;
    }
  }
  return 0;
}

static const std::string &get_status_string(Status status) {
  if (status2string.find(status) != status2string.end()) {
    return status2string[status];
  }
  return status2string[Status::UNKNOWN];
}

/* Store string representation of status to the server.status file.
   If the file does not exist, it is created.
   If the file exists, try to acquire exclusive lock with 5s timeout.
   Regardless of the lock being acquired or not store the status in file. */
void set_status(Status status) {
  if (!m_create_server_state_file) {
    return;
  }

  int fd = open(m_server_state_file_path.c_str(), O_WRONLY | O_CREAT, 0644);
  if (fd == -1) {
    log("Cannot open server.status file");
    return;
  }
  // close() will release the file lock if acquired
  raii::Sentry file_close{[fd] { close(fd); }};

  /* Let's try acquire exclusive lock on server.status file.
     We will give up after a short time. It is to avoid the case when someone
     acquires a shared lock on this file without releasing it. That would block
     the server forever.
     We will update the status file anyway. In worst case the reader will
     receive garbage if it reads in the middle of the write, but it is up to
     reader to not lock for a long time. */
  int res = flockt(fd, LOCK_EX, 5000ms);
  if (res) {
    log("Failed to acquire exclusive lock on server.status file. Reporting "
        "status anyway.");
  }

  // truncate only after acquiring the lock
  res = ftruncate(fd, 0);
  if (res) {
    log("Failed to truncate server.status file. Trying to delete.");
    if (delete_status_file()) {
      log("Failed to delete server.status file");
      return;
    }
  }

  const std::string &status_str = get_status_string(status);
  size_t s = write(fd, status_str.c_str(), status_str.length());
  if (s != status_str.length()) {
    log("Error writing data to server.status file");
  }

  // file is closed by file_close Sentry
}

int init(int *argc, char ***argv) {
  int temp_argc = *argc;
  MEM_ROOT alloc{PSI_NOT_INSTRUMENTED, 512};
  char *ptr, **res, *datadir = nullptr;
  char local_datadir_buffer[FN_REFLEN] = {0};

  /* mysql_real_data_home_ptr is initialized in init_common_variables().
  We want to store the status file much earlier. Similar code exists in
  Persisted_variables_cache::init() */
  my_option persist_options[] = {
      {"create_server_state_file", 0, "", &m_create_server_state_file,
       &m_create_server_state_file, nullptr, GET_BOOL, OPT_ARG, 1, 0, 0,
       nullptr, 0, nullptr},
      {"datadir", 0, "", &datadir, nullptr, nullptr, GET_STR, OPT_ARG, 0, 0, 0,
       nullptr, 0, nullptr},
      {nullptr, 0, nullptr, nullptr, nullptr, nullptr, GET_NO_ARG, NO_ARG, 0, 0,
       0, nullptr, 0, nullptr}};

  /* create temporary args list and pass it to handle_options */
  if (!(ptr =
            (char *)alloc.Alloc(sizeof(alloc) + (*argc + 1) * sizeof(char *))))
    return 1;
  memset(ptr, 0, (sizeof(char *) * (*argc + 1)));
  res = (char **)(ptr);
  memcpy((uchar *)res, (char *)(*argv), (*argc) * sizeof(char *));

  my_getopt_skip_unknown = true;
  if (my_handle_options(&temp_argc, &res, persist_options, nullptr, nullptr,
                        true)) {
    alloc.Clear();
    return 1;
  }
  my_getopt_skip_unknown = false;
  alloc.Clear();

  if (!datadir) {
    // mysql_real_data_home must be initialized at this point
    assert(mysql_real_data_home[0]);
    /*
    mysql_home_ptr should also be initialized at this point.
    See calculate_mysql_home_from_my_progname() for details
    */
    assert(mysql_home_ptr && mysql_home_ptr[0]);
    convert_dirname(local_datadir_buffer, mysql_real_data_home, NullS);
    (void)my_load_path(local_datadir_buffer, local_datadir_buffer,
                       mysql_home_ptr);
    datadir = local_datadir_buffer;
  }

  m_server_state_file_path =
      std::string(datadir) + path_separator + m_server_state_filename;

  // Ignore the error. Server starts and it is possible there is no
  // server.status file
  delete_status_file();

  return 0;
}

}  // namespace Server_status_file