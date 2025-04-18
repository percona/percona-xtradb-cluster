#
# The scenario is:
# Setup parent-child tables
# node2: Setup DDL applier trap before commit
# node1: Block DDL and DML after cert_and_catch()
# node1: DDL: RENAME TABLE child to child_new
# node2: Wait for DDL applier to reach trap
# node2: Setup DML applier trap before applying the writeset
# node1: DML INSERT INTO parent VALUES (0)
# node2: Wait for DDL applier to reach trap
# node2: DDL will block before commit (acquires locks on child and parent tables), DML will block before applying
# node2: Unblock DML. Without the fix it will try to acquire locks on parent table, which will end up with BF-BF
# node2: Unblock DDL
# node1: Unblock DDL and DML. Collect results
#

--source include/count_sessions.inc

--connect node_1_ddl, 127.0.0.1, root, , test, $NODE_MYPORT_1
--connection node_1_ddl
SET SESSION wsrep_sync_wait = 0;

--connect node_1_dml, 127.0.0.1, root, , test, $NODE_MYPORT_1
--connection node_1_dml
SET SESSION wsrep_sync_wait = 0;

--connection node_1
SET SESSION wsrep_sync_wait = 0;
CREATE TABLE parent (id INT PRIMARY KEY);
CREATE TABLE child (id INT PRIMARY KEY, parent_id INT, FOREIGN KEY(parent_id) REFERENCES parent(id) ON DELETE CASCADE ON UPDATE CASCADE);
INSERT INTO parent values (0);
INSERT INTO child values (0, 0);
--let $galera_sync_point = after_cert_and_catch
--source include/galera_set_sync_point.inc

--connection node_2
SET SESSION wsrep_sync_wait = 0;
--let $wsrep_applier_threads_orig = `SELECT @@wsrep_applier_threads`
SET GLOBAL wsrep_applier_threads = 2;

--let $wait_condition = SELECT COUNT(*) = 1 FROM parent
--source include/wait_condition.inc
--let $wait_condition = SELECT COUNT(*) = 1 FROM child
--source include/wait_condition.inc

# DDL applier should stop here
SET GLOBAL debug = "+d,ordered_commit_blocked";


--connection node_1_ddl
--send_eval $ddl_query
--connection node_1
--let $galera_sync_point = after_cert_and_catch
--source include/galera_wait_sync_point.inc

--connection node_2
# Wait for DDL applier. It will block before commit.
SET DEBUG_SYNC = "now WAIT_FOR signal.ordered_commit_waiting";
SET GLOBAL debug = "-d,ordered_commit_blocked";

# Setup DML applier trap. It should stop before acquiring ApplyMonitor
--let $galera_sync_point = sync.apply_trx.start_of_apply_trx
--source include/galera_set_sync_point.inc

--connection node_1_dml
--send_eval $dml_query
--connection node_1
--let $galera_sync_point = after_cert_and_catch after_cert_and_catch
--source include/galera_wait_sync_point.inc

--connection node_2
# Wait for DML applier
--let $galera_sync_point = sync.apply_trx.start_of_apply_trx
--source include/galera_wait_sync_point.inc

#
# Let the DML applier proceed. As DDL applier already acquired all MDL locks
# it will cause BF-BF abort if the dependency between transactions is not set
# properly
#
--source include/galera_clear_sync_point.inc
--source include/galera_signal_sync_point.inc

SET DEBUG_SYNC = "now SIGNAL signal.ordered_commit_continue";

--let $wait_condition = SELECT COUNT(*) = 2 FROM $dml_table
--source include/wait_condition.inc

#
# Reap results on node_1
#
--connection node_1
--source include/galera_clear_sync_point.inc
--let $galera_sync_point = after_cert_and_catch
--source include/galera_signal_sync_point.inc

--connection node_1_ddl
--reap
--connection node_1_dml
--reap

#
# cleanup
#
--connection node_2
--disable_query_log
--eval SET GLOBAL wsrep_applier_threads = $wsrep_applier_threads_orig;
--enable_query_log

--connection node_1
--eval $drop_query

--disconnect node_1_ddl
--disconnect node_1_dml
--source include/wait_until_count_sessions.inc

