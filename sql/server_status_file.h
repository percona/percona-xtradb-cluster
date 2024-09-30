#ifndef SERVER_STATUS_FILE_INCLUDED
#define SERVER_STATUS_FILE_INCLUDED

namespace Server_status_file {
enum Status {
  INITIALIZING,
  DOWNGRADING,
  UPGRADING,
  READY,
  STOPPING,
  STOPPED,
  UNKNOWN
};

int init(int *argc, char ***argv);
void set_status(Status status);
}  // namespace Server_status_file

#endif /* SERVER_STATUS_FILE_INCLUDED */