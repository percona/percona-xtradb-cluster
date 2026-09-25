# Shared scenario: strict validate_password policy, two-node cluster,
# force SST on node 2, verify replication succeeds.
#
# SST method and method-specific [sst] options come from the test .cnf file.

--source include/have_validate_password_component.inc

--disable_query_log
CALL mtr.add_suppression("Effective value of validate_password.length is changed.");
--enable_query_log

INSTALL COMPONENT "file://component_validate_password";
SET @@global.validate_password.policy = MEDIUM;
SET @@global.validate_password.length = 40;
SET @@global.validate_password.mixed_case_count = 7;
SET @@global.validate_password.number_count = 3;
SET @@global.validate_password.special_char_count = 5;

--source include/galera_cluster.inc
--source include/force_restart.inc

CREATE TABLE t1 (f1 INTEGER PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO t1 VALUES(1);

--connection node_1
SET @@global.validate_password.policy = MEDIUM;
SET @@global.validate_password.length = 40;
SET @@global.validate_password.mixed_case_count = 7;
SET @@global.validate_password.number_count = 3;
SET @@global.validate_password.special_char_count = 5;

--connection node_2
SET @@global.validate_password.policy = MEDIUM;
SET @@global.validate_password.length = 40;
SET @@global.validate_password.mixed_case_count = 7;
SET @@global.validate_password.number_count = 3;
SET @@global.validate_password.special_char_count = 5;

# Remove the grastate.dat file to force an SST
--remove_file $MYSQLTEST_VARDIR/mysqld.2/data/grastate.dat
--echo # Restarting node 2 ...
--source include/shutdown_mysqld.inc
--source include/start_mysqld.inc

--source include/wait_until_connected_again.inc
--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

--connection node_2
SELECT COUNT(*) FROM t1;

# Test cleanup
--connection node_1
--echo # connection node_1
DROP TABLE t1;
UNINSTALL COMPONENT "file://component_validate_password";

--disable_query_log
--connection node_1
CALL mtr.add_suppression("Failed to remove page file");
--connection node_2
CALL mtr.add_suppression("Failed to remove page file");
--enable_query_log
