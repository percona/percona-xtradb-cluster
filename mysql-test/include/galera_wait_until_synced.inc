# === Purpose ===
#
# Waits until the node comes to 'Synced' state, or until a timeout is
# reached.
#
# === Usage ===
#
# --source include/galera_wait_until_synced.inc

--let $wait_condition = SELECT VARIABLE_VALUE = 'Synced' FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_state_comment';
--source include/wait_condition.inc
