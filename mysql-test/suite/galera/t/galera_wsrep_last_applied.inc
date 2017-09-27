#
# galera_wsrep_last_applied : test the operation of wsrep_last_applied (no binlog)
#
# Run a transaction and check that wsrep_last_applied is being set differently
# from wsrep_last_committed
#

CREATE TABLE t1 (f1 INTEGER PRIMARY KEY, f2 CHAR(1));

SET AUTOCOMMIT=ON;

#
# DML test (insert)
# Test that the values are set separately locally.
#
--echo #
--echo # DML test (local)
--echo #

--echo # Node 1 : before insert
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log

--echo # Node 1 : block the commit, perform the insert
--let $galera_sync_point = $galera_dml_local_sync_point
--source include/galera_set_sync_point.inc

--send INSERT INTO t1 VALUES (1, 'a');

--echo # Node 1a : wait until the commit is blocked
--connect node_1a, 127.0.0.1, root, , test, $NODE_MYPORT_1
--connection node_1a
SET SESSION wsrep_sync_wait = 0;
--source include/galera_wait_sync_point.inc

--echo # Node 1 : after insert (but stuck in commit)
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied+1 = $wsrep_last_committed AS 'applied+1 = committed';
--enable_query_log

--echo # Node 1a : unblock the commit
--connection node_1a
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc

# Commit succeeds
--connection node_1
--reap

SELECT * FROM t1;

--echo # Node 1 : after insert (after commit)
--disable_query_log
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--let $wait_condition = SELECT VARIABLE_VALUE = $wsrep_last_committed FROM performance_schema.global_status where VARIABLE_NAME = 'wsrep_last_applied'
--let $wait_condition_on_error_output = SELECT VARIABLE_NAME, VARIABLE_VALUE FROM performance_schema.global_status where VARIABLE_NAME IN ('wsrep_last_committed', 'wsrep_last_applied')
--source include/wait_condition_with_debug.inc
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log

#
# DML test (insert)
# Test that the values are set separately remotely.
#
--echo #
--echo # DML test (remote)
--echo #

--echo # Node 2 : before insert
--connection node_2
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log

--echo # Node 2 : block the commit
--let $galera_sync_point = $galera_dml_remote_sync_point
--source include/galera_set_sync_point.inc

--echo # Node 1 : perform the insert
--connection node_1

INSERT INTO t1 VALUES (2, 'b');

--echo # Node 2a : wait until the commit is blocked
--connect node_2a, 127.0.0.1, root, , test, $NODE_MYPORT_2
--connection node_2a
SET SESSION wsrep_sync_wait = 0;
--source include/galera_wait_sync_point.inc

--echo # Node 2a : after insert (but stuck in commit)
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied+1 = $wsrep_last_committed AS 'applied+1 = committed';
--enable_query_log

--echo # Node 2a : unblock the commit
--connection node_2a
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc

--let $wait_condition = SELECT COUNT(*) = 2 FROM t1;
--source include/wait_condition.inc

SELECT * FROM t1;

--echo # Node 2 : after insert (after commit)
--disable_query_log
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--let $wait_condition = SELECT VARIABLE_VALUE = $wsrep_last_committed FROM performance_schema.global_status where VARIABLE_NAME = 'wsrep_last_applied'
--let $wait_condition_on_error_output = SELECT VARIABLE_NAME, VARIABLE_VALUE FROM performance_schema.global_status where VARIABLE_NAME IN ('wsrep_last_committed', 'wsrep_last_applied')
--source include/wait_condition_with_debug.inc
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log


#
# DDL test
# Test that the values are set separately locally.
#
--echo #
--echo # DDL test (local)
--echo #

--echo # Node 1 : before alter
--connection node_1
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log

--echo # Node 1 : block the commit, perform the alter
--let $galera_sync_point = $galera_ddl_local_sync_point
--source include/galera_set_sync_point.inc

--send ALTER TABLE t1 ADD COLUMN f3 LONGBLOB;

--echo # Node 1a : wait until the commit is blocked
--connection node_1a
SET SESSION wsrep_sync_wait = 0;
--source include/galera_wait_sync_point.inc

--echo # Node 1 : after alter (but stuck in commit)
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied+1 = $wsrep_last_committed AS 'applied+1 = committed';
--enable_query_log

--echo # Node 1a : unblock the commit
--connection node_1a
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc

# Commit succeeds
--connection node_1
--reap

SELECT * FROM t1;

--echo # Node 1 : after alter (after commit)
--disable_query_log
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--let $wait_condition = SELECT VARIABLE_VALUE = $wsrep_last_committed FROM performance_schema.global_status where VARIABLE_NAME = 'wsrep_last_applied'
--let $wait_condition_on_error_output = SELECT VARIABLE_NAME, VARIABLE_VALUE FROM performance_schema.global_status where VARIABLE_NAME IN ('wsrep_last_committed', 'wsrep_last_applied')
--source include/wait_condition_with_debug.inc
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log


#
# DDL test (alter)
# Test that the values are set separately remotely.
#
--echo #
--echo # DDL test (remote)
--echo #

--echo # Node 2 : before alter
--connection node_2
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log

--echo # Node 2 : block the commit
--let $galera_sync_point = $galera_ddl_remote_sync_point
--source include/galera_set_sync_point.inc

--echo # Node 1 : perform the alter
--connection node_1

ALTER TABLE t1 ADD COLUMN f4 LONGBLOB;

--echo # Node 2a : wait until the commit is blocked
--connection node_2a
SET SESSION wsrep_sync_wait = 0;
--source include/galera_wait_sync_point.inc

--echo # Node 2a : after insert (but stuck in commit)
--disable_query_log
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--eval SELECT $wsrep_last_applied+1 = $wsrep_last_committed AS 'applied+1 = committed';
--enable_query_log

--echo # Node 2a : unblock the commit
--connection node_2a
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc

--let $wait_condition = SELECT COUNT(*) = 4 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 't1';
--source include/wait_condition.inc

SELECT * FROM t1;

--echo # Node 2 : after alter (after commit)
--disable_query_log
--let $wsrep_last_committed = query_get_value(SHOW STATUS LIKE 'wsrep_last_committed', Value, 1)
--let $wait_condition = SELECT VARIABLE_VALUE = $wsrep_last_committed FROM performance_schema.global_status where VARIABLE_NAME = 'wsrep_last_applied'
--let $wait_condition_on_error_output = SELECT VARIABLE_NAME, VARIABLE_VALUE FROM performance_schema.global_status where VARIABLE_NAME IN ('wsrep_last_committed', 'wsrep_last_applied')
--source include/wait_condition_with_debug.inc
--let $wsrep_last_applied = query_get_value(SHOW STATUS LIKE 'wsrep_last_applied', Value, 1)
--eval SELECT $wsrep_last_applied = $wsrep_last_committed AS 'applied = committed';
--enable_query_log


#
# Test cleanup
#
--connection node_2
DROP TABLE t1;
