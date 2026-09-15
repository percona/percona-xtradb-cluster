#
# This macro verifies that two statements conflict in certification
# and checks that the returned error code for the failed one is correct.
#
# The caller is responsible for setting control connection node_1_ctrl on
# the first node and checking the final result in the database contents.
#
# Parameters
#
# $certification_query_node_1 - Query which is run on node_1
# $certification_query_node_2 - Query which is run on node_2, is ordered first
# $certification_toi          - Is the first action TOI
#

--let $certification_sync_signal = wsrep_apply_cb
if ($certification_toi == 1)
{
  --let $certification_sync_signal = wsrep_apply_toi
}
--let $certification_sync_point = `SELECT CONCAT("$certification_sync_signal", "_reached")`

--connection node_1
SET SESSION wsrep_sync_wait = 0;
SET SESSION wsrep_retry_autocommit = 0;

--connection node_1_ctrl
--eval SET GLOBAL DEBUG = "+d,sync.$certification_sync_signal"

--connection node_2
--eval $certification_query_node_2

--connection node_1_ctrl
--eval SET DEBUG_SYNC = "now WAIT_FOR sync.$certification_sync_point"

--let $galera_sync_point = commit_monitor_master_enter_sync
--source include/galera_set_sync_point.inc

--connection node_1
--send_eval $certification_query_node_1

--connection node_1_ctrl
--source include/galera_wait_sync_point.inc
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc
--eval SET GLOBAL DEBUG = "-d,sync.$certification_sync_signal"
--eval SET DEBUG_SYNC = "now SIGNAL signal.$certification_sync_signal"

--connection node_1
--error ER_LOCK_DEADLOCK
--reap
SET SESSION wsrep_retry_autocommit = DEFAULT;
SET SESSION wsrep_sync_wait = DEFAULT;
