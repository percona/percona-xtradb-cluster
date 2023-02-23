#include <stddef.h>
#include "my_inttypes.h"
#include "my_thread_local.h"
#include "mysql/components/services/log_builtins.h"
#include "sql/sql_class.h"
#include "sql/system_variables.h"

class THD;
class MDL_context;
class MDL_ticket;
struct MDL_key;

ulong wsrep_debug = 0;
uint wsrep_min_log_verbosity = 3;
bool pxc_force_flush_error_message = false;
bool wsrep_log_conflicts = false;
struct System_variables global_system_variables;
bool wsrep_provider_set = false;

extern "C" bool wsrep_thd_is_BF(const THD *thd [[maybe_unused]],
                                bool sync [[maybe_unused]]) {
  return false;
}

extern "C" const char *wsrep_thd_query(const THD *thd [[maybe_unused]]) {
  return nullptr;
}

extern "C" my_thread_id wsrep_thd_thread_id(THD *thd [[maybe_unused]]) {
  return 0;
}

extern bool wsrep_handle_mdl_conflict(const MDL_context *requestor_ctx
                                      [[maybe_unused]],
                                      MDL_ticket *ticket [[maybe_unused]],
                                      const MDL_key *key [[maybe_unused]]) {
  return false;
}

extern bool debug_sync_set_action(THD *thd [[maybe_unused]],
                                  const char *action_str [[maybe_unused]],
                                  size_t len [[maybe_unused]]) {
  return false;
}

void flush_error_log_messages() {}

bool log_item_set_cstring(log_item_data *, char const *) { return false; }
bool log_item_set_int(log_item_data *, longlong) { return false; }
bool log_item_set_lexstring(log_item_data *, char const *, size_t) {
  return false;
}
void log_line_exit(log_line *) {}
log_line *log_line_init() { return nullptr; }
log_item_data *log_line_item_set(log_line *, enum_log_item_type) {
  return nullptr;
}
log_item_type_mask log_line_item_types_seen(log_line *, log_item_type_mask) {
  return 0;
}
int log_line_submit(log_line *) { return 0; }
const char *error_message_for_error_log(int) { return nullptr; }

log_item_data *log_line_item_set_with_key(log_line *, log_item_type,
                                          const char *, uint32) {
  return nullptr;
}

void THD::enter_stage(const PSI_stage_info *new_stage [[maybe_unused]],
                      PSI_stage_info *old_stage [[maybe_unused]],
                      const char *calling_func [[maybe_unused]],
                      const char *calling_file [[maybe_unused]],
                      const unsigned int calling_line [[maybe_unused]]) {}