#
# Helper macro to run the BF abort sequence.
#
# It is assumed that the caller:
# - Creates a table t1 which is operated by the DDL
# - Creates connections con_dml and con_ddl
# - Sets bf_ddl which operates on table t1
# - Sets expect_wsrep_local_replays to check if replay happens
#
# The test starts a transaction which operates on table t2 to generate
# a write set and then does an operation to table t1 which takes a
# MDL lock but does not generate a key (statement does not have any effect).
#
# The ordering of the DDL will be such that the DDL will be ordered
# and certified first, the transaction after that.
#
# As there are no certification conflicts because DDL and transaction
# have keys for different tables, the DDL will BF abort the transaction
# which is waiting for commit.
#
# The transaction will be aborted and replayed.
#

--connection node_1
--let $wsrep_local_replays_orig = `SELECT VARIABLE_VALUE FROM information_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_replays'`
CREATE TABLE t2 (f1 INT PRIMARY KEY, f2 INT) ENGINE=InnoDB;

# Start transaction which gets a shared MDL lock on table t1 but
# does not generate any changes on it.
--connection con_dml
START TRANSACTION;
INSERT INTO t2 VALUES (1, 1);
UPDATE IGNORE t1 SET f2 = 2 WHERE f1 = 2;
SET DEBUG_SYNC = "wsrep_before_certification SIGNAL wbc_reached WAIT_FOR wbc_continue";
--send COMMIT

--connection node_1
SET DEBUG_SYNC = "now WAIT_FOR wbc_reached";

--connection con_ddl
SET DEBUG_SYNC = "wsrep_after_toi_begin SIGNAL watb_reached WAIT_FOR watb_continue";
--send_eval $bf_ddl

--connection node_1
SET SESSION wsrep_sync_wait = 0;
SET DEBUG_SYNC = "now WAIT_FOR watb_reached";

--let $galera_sync_point = commit_monitor_enter_sync
--source include/galera_set_sync_point.inc
SET DEBUG_SYNC = "now SIGNAL wbc_continue";
--source include/galera_wait_sync_point.inc
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc
SET DEBUG_SYNC = "now SIGNAL watb_continue";

--connection con_dml
--reap

--connection con_ddl
--error 0,ER_TABLE_EXISTS_ERROR
--reap

--connection node_1
SET DEBUG_SYNC = "RESET";
DROP TABLE t2;

--let $wsrep_local_replays_curr = `SELECT VARIABLE_VALUE FROM information_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_replays'`
--disable_query_log
--eval SELECT $wsrep_local_replays_curr - $wsrep_local_replays_orig = $expect_wsrep_local_replays AS wsrep_local_replays
--enable_query_log
