
# Test the sst receiver-function capability of garbd.
# sst_garbd_test.sh parameters:
# sst_garbd_test.sh <port> <ssl_opts> <before_exit_sleep_sec> <exit_code>

--source include/not_parallel.inc
--source include/galera_cluster.inc

SHOW STATUS LIKE 'wsrep_flow_control_interval';

--connection node_1
CALL mtr.add_suppression("Protocol violation\. JOIN message sender 1\.0 \(.*\) is not in state transfer \(SYNCED\)");
CALL mtr.add_suppression("State transfer to .*");
CALL mtr.add_suppression("Process completed with error:.*");
CALL mtr.add_suppression("Command did not run:.*");
CALL mtr.add_suppression("WSREP-SST.*");
CALL mtr.add_suppression("SST preparation failed");
CALL mtr.add_suppression("SST failed");
CALL mtr.add_suppression("Invalid sst_request");
# Socat streaming might fail with the port 9997 test. This will result in xbstream errors in the log. That's ok.
CALL mtr.add_suppression("Xtrabackup");
# Random xtrabackup diagnostic, shouldn't be an issue
CALL mtr.add_suppression("Allocated tablespace ID");

--connection node_2
CALL mtr.add_suppression("Protocol violation\. JOIN message sender 1\.0 \(.*\) is not in state transfer \(SYNCED\)");
CALL mtr.add_suppression("State transfer to .*");
CALL mtr.add_suppression("Process completed with error:.*");
CALL mtr.add_suppression("Command did not run:.*");
CALL mtr.add_suppression("WSREP-SST.*");
CALL mtr.add_suppression("SST preparation failed");
CALL mtr.add_suppression("SST failed");
CALL mtr.add_suppression("Invalid sst_request");
# Socat streaming might fail with the port 9997 test. This will result in xbstream errors in the log. That's ok.
CALL mtr.add_suppression("Xtrabackup");
# Random xtrabackup diagnostic, shouldn't be an issue
CALL mtr.add_suppression("Allocated tablespace ID");

--exec `chmod +x "$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh"`
--exec `chmod +x "$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh"`

--let $ssl_opts=cafile=$MYSQLTEST_VARDIR/std_data/cacert.pem,cert=$MYSQLTEST_VARDIR/std_data/server-cert.pem,key=$MYSQLTEST_VARDIR/std_data/server-key.pem

#
# Success scenario
#
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 0 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd1.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

#
# Success scenario with post-recv-script
#
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 0 0" --post-recv-script="$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh $MYSQLTEST_VARDIR/tmp/post_recv_script_output1 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd1.1.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

# Check that post_recv_script_output1 was created by post-recv-script
--file_exists $MYSQLTEST_VARDIR/tmp/post_recv_script_output1

#
# Error: sst script does not finish within 5 secs after Donor finishes serving SST. SST script will be terminated and garbd should return SST_SCRIPT_TERMINATED
#
--error $SST_SCRIPT_TERMINATED
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 10 0" --post-recv-script="$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh $MYSQLTEST_VARDIR/tmp/post_recv_script_output2 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd1.2.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

# Check that post-recv-script was not executed (post_recv_script_output2 was NOT created)
--let $file_does_not_exist = $MYSQLTEST_VARDIR/tmp/post_recv_script_output2
--source include/file_does_not_exist.inc

#
# Error: recv-script finishes, but returns non-zero error code. garbd should return the error returned by the script.
#
--error 100
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 0 100" --post-recv-script="$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh $MYSQLTEST_VARDIR/tmp/post_recv_script_output3 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd1.3.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

# Check that post-recv-script was not executed (post_recv_script_output3 was NOT created)
--let $file_does_not_exist = $MYSQLTEST_VARDIR/tmp/post_recv_script_output3
--source include/file_does_not_exist.inc

#
# Error: recv-script finishes and returns 0, but post-recv-script returns non-zero error code.
# Garbd should return the error returned by the post-recv-script.
#
--error 101
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 0 0" --post-recv-script="$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh $MYSQLTEST_VARDIR/tmp/post_recv_script_output4 101" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd1.4.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

# Check that post-recv-script was executed (post_recv_script_output4 was created)
--file_exists $MYSQLTEST_VARDIR/tmp/post_recv_script_output4

#
# Success scenario with post-recv-script not finishing in 5 sec, but garbd started with --wait-for-recv-script-exit. Garbd should return exit code returned by post-recv-script
#
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9999/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9999 $ssl_opts 15 0" --post-recv-script="$MYSQLTEST_VARDIR/std_data/post_sst_garbd_test.sh $MYSQLTEST_VARDIR/tmp/post_recv_script_output5 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' --wait-for-recv-script-exit > $MYSQLTEST_VARDIR/tmp/garbd1.5.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

# Check that post_recv_script_output5 was created by post-recv-script
--file_exists $MYSQLTEST_VARDIR/tmp/post_recv_script_output5

#
# Error: incorrect donor sst script. Garbd should return SST_REQUEST_FAILURE
#
--error $SST_REQUEST_FAILURE
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="nonexistent-script:127.0.0.1:9998/xtrabackup_sst//1" --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9998 $ssl_opts 0 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd2.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

#
# Error: incorrect receiver script (garbd will return standard ENOENT error)
#
--error 2
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9997/xtrabackup_sst//1" --recv-script="nonexistent-script" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd3.log 2>&1 
SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

#
# Simulate two connection error 
#
--let $command = "$GALERA_GARBD"
--let $command_opt = $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9996/xtrabackup_sst//1" --donor node1 --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9996 $ssl_opts 0 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3'
--let $output_file = $MYSQLTEST_VARDIR/tmp/garbd3.log
--let $pid_file = $MYSQLTEST_VARDIR/tmp/garbd3.pid
--source include/start_proc_in_background.inc

# Depending on how much time it passes since the previous garbd start, this either fails or succeeds.
# More than a few ms: the second garbd process knows about the SST transfer for the first, and waits until it is done
# Minimal time: both garbd processes think that the donor sends data to them.
# Second garbd process only notices the issue when the SST process finishes yet it didn't receive any data, and it exists with a failure.
--exec $GALERA_GARBD $ext_exit_codes_opt --sst="xtrabackup-v2:127.0.0.1:9998/xtrabackup_sst//1" --donor node1 --recv-script="$MYSQLTEST_VARDIR/std_data/sst_garbd_test.sh 9998 $ssl_opts 0 0" --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'evs.inactive_timeout=PT6S;base_port=$NODE_GALERAPORT_3' > $MYSQLTEST_VARDIR/tmp/garbd4.log 2>&1  || exit 0

--connection node_1

# Wait until the background garbd process completes to avoid shutdown assertion
--source include/wait_proc_to_finish.inc

--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.1.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.2.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.3.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.4.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd1.5.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd2.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd3.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd4.log
--remove_file $MYSQLTEST_VARDIR/tmp/garbd3.pid
--remove_file $MYSQLTEST_VARDIR/tmp/post_recv_script_output1
--remove_file $MYSQLTEST_VARDIR/tmp/post_recv_script_output4
--remove_file $MYSQLTEST_VARDIR/tmp/post_recv_script_output5
