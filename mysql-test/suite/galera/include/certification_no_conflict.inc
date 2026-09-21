#
# This macro verifies that two statements do not conflict in certification.
#
# The caller is responsible for setting control connection node_1_ctrl on
# the first node and checking the final result in the database contents.
#
# Parameters
#
# $certification_query_node_1 - Query which is run on node_1
# $certification_query_node_2 - Query which is run on node_2
#

--connection node_1
SET SESSION wsrep_sync_wait = 0;

--connection node_1_ctrl
SET GLOBAL DEBUG = "+d,sync.wsrep_apply_cb";

--connection node_2
--eval $certification_query_node_2

--connection node_1_ctrl
SET DEBUG_SYNC = "now WAIT_FOR sync.wsrep_apply_cb_reached";

--let $galera_sync_point = commit_monitor_master_enter_sync
--source include/galera_set_sync_point.inc

--connection node_1
--send_eval $certification_query_node_1

--connection node_1_ctrl
--source include/galera_wait_sync_point.inc
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc
SET GLOBAL DEBUG = "-d,sync.wsrep_apply_cb";
SET DEBUG_SYNC = "now SIGNAL signal.wsrep_apply_cb";

--connection node_1
--reap
SET SESSION wsrep_sync_wait = DEFAULT;
