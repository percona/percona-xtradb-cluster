#
# This helper restarts the running cluster by performing SST
#

# Shutdown the cluster
--echo # shutting down node2
--connection node_2
--echo [connection node_2]
SET SESSION wsrep_sync_wait = 0;
--source include/shutdown_mysqld.inc

--echo # shutting down node1
--connection node_1
--echo [connection node_1]
SET SESSION wsrep_sync_wait = 0;
--source include/shutdown_mysqld.inc

# Remove the grastate.dat files to restart the cluster.
--error 0,1 
--remove_file $MYSQLTEST_VARDIR/mysqld.1/data/grastate.dat
--error 0,1 
--remove_file $MYSQLTEST_VARDIR/mysqld.2/data/grastate.dat

--echo "Restarting node 1"
--connection node_1
--let $_expect_file_name= $MYSQLTEST_VARDIR/tmp/mysqld.1.expect
--source include/start_mysqld.inc
--source include/wait_until_connected_again.inc

--echo "Restarting node 2"
--connection node_2
--let $_expect_file_name= $MYSQLTEST_VARDIR/tmp/mysqld.2.expect
--source include/start_mysqld.inc
--source include/wait_until_connected_again.inc


# Wait for the cluster size to become 2
--connection node_1
--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

--connection node_2
--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

# Switch the connection back to node_1
--connection node_1
