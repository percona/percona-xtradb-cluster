#
# Test scenario:
# Joiner is able to join with IST only, but there is connection issue between 
# Joiner and Donor (Joiner is not able to receive sst-info file).
# In sucha case Joiner timeouts without deleteing data directory, so it
# should be able to join with IST next time. To be able to do so, grastate.dat
# file needs to contain valid uuid:seqno pair.
#

--source include/have_debug_sync.inc
--source include/have_debug.inc
--source include/galera_cluster.inc
--source include/force_restart.inc

#
# node_1 will be Donor, node_2 Joiner.
# Do initial setup to allow node_2 restarts.
#
--connection node_1
--let $wsrep_provider_options_orig1 = `SELECT @@global.wsrep_provider_options`
SET GLOBAL wsrep_provider_options="pc.weight=1";

#
# Donor will fail to donate during the test and complain in logs.
#
CALL mtr.add_suppression("\[Warning\].*Terminating IST AsyncSender");
CALL mtr.add_suppression("\[ERROR\].*\[WSREP-SST\]");
CALL mtr.add_suppression("\[Warning\].*\[WSREP-SST\] Found a stale sst_in_progress file");
CALL mtr.add_suppression("\[ERROR\].*Process completed with error");
CALL mtr.add_suppression("\[ERROR\].*Command did not run");

--connection node_2
--let $wsrep_provider_options_orig2 = `SELECT @@global.wsrep_provider_options`
SET GLOBAL wsrep_provider_options="pc.weight=0";

#
# Shutdown node 2, to get it out of sync with node 1.
#
--connection node_2
--source include/shutdown_mysqld.inc

#
# Wait until the node_2 is confirmed as being down.
#
--connection node_1
--let $wait_condition = SELECT VARIABLE_VALUE = 1 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

#
# Do some queries, so node_2 will have to catch up after restart.
#
CREATE TABLE t1 (a INT PRIMARY KEY);
INSERT INTO t1 VALUES (0);

#
# Add a debug point on node_1 to halt just before donating state transfer.
# This way we will simulate connection issue between Donor and Joiner.
#
--let $debug_point= halt_before_sst_donate
--source include/add_debug_point.inc

#
# Restart node_2. It should timeout on SST and shutdown.
#
--connection node_2

--let $ofile= $MYSQLTEST_VARDIR/tmp/node.2.err
--let $restart_parameters = "--log-error=$ofile"
--let $_expect_file_name = $MYSQLTEST_VARDIR/tmp/mysqld.2.expect
--let $do_not_echo_parameters = 1
--source include/start_mysqld_expecting_crash.inc

#
# sleep to avoid checking the error log file if it was not created yet.
#
--sleep 5

#
# Wait for node_2 to shutdown. Also check that node's state was written back to grastate.dat file.
# In cnf file we have sst-initial-timeout=10, so waiting for 20 secs is enough.
#
--let $grep_pattern = .*Saving node state to retry with IST instead of full SST after restart.*
--let $grep_file = $ofile
--let $wait_timeout = 20
--source include/wait_for_pattern_in_file.inc

#
# Wait for node_2 process to be finished. It exists by ReplicatorSMM::abort().
# In sucha case IST receiver thread is not joined gracefuly, and its networking architecutre
# is terminated immediately so it is random when it happens.
# If we don't wait here we can hit one of the following cases:
# 1. Joiner's IST receiver is still listening -> node_1 will send IST
# 2. Joiner's IST receiver is down -> node_1's IST sender will fail to connect to node_2's IST receiver
# While case 1 handled by SocketWatchdog introduced in commit fb7fe369,
# case 2 was hit during 8.0.43 merge. Let's force it here by waiting.
#
# 'Terminating SST process.' is the last log. wait_proc_to_finish.inc doesn't work, because
# node_2 is starting and is expecting to terminate immediately, so there are low chances we
# will be able to access pid file from wait_for_pattern_in_file.inc to get pid of the process.

--let $grep_pattern = .*Terminating SST process.*
--let $grep_file = $ofile
--let $wait_timeout = 20
--source include/wait_for_pattern_in_file.inc
--remove_file $ofile

#
# Now let node_1 to go on and fail to donate.
#
--connection node_1
--echo "continue with node 1"
SET SESSION wsrep_sync_wait = 0;
--source include/remove_debug_point.inc
SET DEBUG_SYNC = "now SIGNAL continue";

#
# Wait for node_1 to go back to Synced state. It takes ca. 30 secs for Donor to timeout.
#
--let $wait_condition = SELECT VARIABLE_VALUE = 'Synced' FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_state_comment';
--let $wait_timeout = 120
--source include/wait_condition.inc

#
# Check that node_2's grastate.dat contains valid uuid:seqno pair.
# Use node_1 for doing assertions because node_2 is still down.
#
--connection node_1

--let $assert_text = "Check for uuid validity"
--let $assert_file = $MYSQLTEST_VARDIR/mysqld.2/data/grastate.dat
# If uuid is not 00000000-0000-0000-0000-000000000000, it is OK.
--let $assert_select = .*uuid:.*[1-9]+.*
--let $assert_match = .*uuid:.*[1-9]+.*
--source include/assert_grep.inc

--let $assert_text = "Check for seqno validity"
--let $assert_file = $MYSQLTEST_VARDIR/mysqld.2/data/grastate.dat
# If it is not -1, it is OK.
--let $assert_select = .*seqno:.*(?!-1)[0-9]+.*
--let $assert_match = .*seqno:.*(?!-1)[0-9]+.*
--source include/assert_grep.inc

#
# Start node_2 and ensure it joined with IST
#
--connection node_2
--let $restart_parameters = "restart: --log-error=$ofile"
--source include/start_mysqld_no_echo.inc
--source include/wait_until_connected_again.inc
--source include/galera_wait_ready.inc

--let $assert_text = "Check that node_2 joined with IST"
--let $assert_file = $ofile
--let $assert_select = $ist_check_log_message
--let $assert_match = $ist_check_log_message
--source include/assert_grep.inc
--remove_file $ofile

#
# cleanup
#
--connection node_1
DROP TABLE t1;

