
# Setup the test on node 1
--connection node_1
--echo # Node 1
--let $wsrep_local_bf_aborts_before = `SELECT VARIABLE_VALUE FROM information_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_bf_aborts'`

USE database1;
CREATE TABLE t1 (f1 INTEGER PRIMARY KEY, f2 TEXT) ENGINE=InnoDB;
INSERT INTO t1 VALUES (1, 'initial');
INSERT INTO t1 VALUES (2, 'initial');
INSERT INTO t1 VALUES (3, 'initial');
INSERT INTO t1 VALUES (4, 'initial');

# Invoke the SP
--connection node_1a
--echo # Node 1a
USE database1;

SET DEBUG_SYNC = "ha_innobase_end_of_write_row SIGNAL entered WAIT_FOR continue";
--send_eval CALL $galera_sp_bf_abort_proc

# Ensure that we have reached the sync point
--connection node_1
--echo # Node 1
SET SESSION wsrep_sync_wait = 0;
SET DEBUG_SYNC = "now WAIT_FOR entered";

# Force the thread on node1 to be aborted (by making a change on node2)
--connection node_2
--echo # Node 2
UPDATE database1.t1 SET f2 = "from node2" WHERE f1 < 4;


--connection node_1
--echo # Node 1
# Ensure that the update from node2 has reached node1
--let $wait_condition = SELECT f2 = 'from node2' FROM database1.t1 WHERE f1 = 1;
--source include/wait_condition.inc

# Signal the thread to continue
# The update from node2 should have bf aborted the op on node1
SET DEBUG_SYNC = "now SIGNAL continue";

--connection node_1a
--echo # Node 1a

if ($galera_sp_bf_abort2_expect_error)
{
	--error ER_LOCK_DEADLOCK
	--reap
}
if (!$galera_sp_bf_abort2_expect_error)
{
	--reap	
}

--connection node_1
--echo # Node 1

--let $wsrep_local_bf_aborts_after = `SELECT VARIABLE_VALUE FROM information_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_bf_aborts'`
--let $assert_text = wsrep_local_bf_aborts has been incremented once
--let $assert_cond = $wsrep_local_bf_aborts_after - $wsrep_local_bf_aborts_before = 1 AS wsrep_local_bf_aborts_increment;
--source include/assert.inc


SELECT * FROM database1.t1;

SET DEBUG_SYNC = "RESET";

--connection node_2
--echo # Node 2
SELECT * FROM database1.t1;

--connection node_1
DROP TABLE t1;

