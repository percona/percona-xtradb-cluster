--source include/have_debug_sync.inc
--source include/galera_cluster.inc
--source include/count_sessions.inc

#
# Test a transaction within session with wsrep_on set to on/off being aborted
# by TOI transaction
#

# common setup
--connect node_1a, 127.0.0.1, root, , test, $NODE_MYPORT_1
--connect node_1b, 127.0.0.1, root, , test, $NODE_MYPORT_1
--connection node_1
CREATE TABLE t1(id INT PRIMARY KEY AUTO_INCREMENT, k INT);
INSERT INTO t1(k) VALUES (1),(2),(3),(101),(102),(103);

# auxiliary table for easy synchronization between nodes
CREATE TABLE sync(id INT PRIMARY KEY);
--let $sync_cnt = 0

--eval SET SESSION wsrep_on = $wsrep_on_value;

#
# Case 1: Abort by local TOI when query in IDLE state
#
--connection node_1
BEGIN;
UPDATE t1 SET k=k+1 WHERE id<100;

--connection node_1a
ALTER TABLE t1 ADD KEY k1 (k);

--connection node_1
--error ER_LOCK_DEADLOCK
COMMIT;

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

#
# Case 2: Abort by local TOI when query in EXEC state
#
--connection node_1
SET DEBUG_SYNC="open_tables_after_open_and_process_table SIGNAL open_tables_after_open_and_process_table.reached WAIT_FOR open_tables_after_open_and_process_table.continue";
--send UPDATE t1 SET k=k+1 WHERE id<100

--connection node_1a
SET DEBUG_SYNC="NOW WAIT_FOR open_tables_after_open_and_process_table.reached";
ALTER TABLE t1 ADD KEY k1 (k);

--connection node_1
--error $expected_error
--reap

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

#
# Case 3: When wsrep_on is enabled, certification takes place.
# Stop just after certification, but before local commit.
# TOI will be ordered after the DML.
# When wsrep_on is disabled, the certification does not take place.
# However, in this case commit of DML transaction acquires explicit intention
# exclusive lock (aka MDL commit lock - see ha_commit_trans())
# TOI transaction needs to wait for other transactions owning MDL commit lock
# (see wsrep_grant_mdl_exception() logic)
#
--connection node_1
SET DEBUG_SYNC="wsrep_before_commit SIGNAL wsrep_before_commit.reached WAIT_FOR wsrep_before_commit.continue";
--send UPDATE t1 SET k=k+1 WHERE id<100

--connection node_1a
SET DEBUG_SYNC="now WAIT_FOR wsrep_before_commit.reached";
--send ALTER TABLE t1 ADD KEY k1 (k)

--connection node_1b
SET DEBUG_SYNC="now SIGNAL wsrep_before_commit.continue";

--connection node_1
--reap

--connection node_1a
--reap

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

#
# Case 4: Abort by remote TOI when query in IDLE state
#
--connection node_1
BEGIN;
UPDATE t1 SET k=k+1 WHERE id<100;

--connection node_2
ALTER TABLE t1 ADD KEY k1 (k);
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

--connection node_1
--error ER_LOCK_DEADLOCK
COMMIT;

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

#
# Case 5: Abort by remote TOI when query in EXEC state
#
--connection node_1
SET DEBUG_SYNC="open_tables_after_open_and_process_table SIGNAL open_tables_after_open_and_process_table.reached WAIT_FOR open_tables_after_open_and_process_table.continue";
--send UPDATE t1 SET k=k+1 WHERE id<100

--connection node_1a
SET DEBUG_SYNC="NOW WAIT_FOR open_tables_after_open_and_process_table.reached";

--connection node_2
ALTER TABLE t1 ADD KEY k1 (k);
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

--connection node_1
--error $expected_error
--reap

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

#
# Case 6: When wsrep_on is enabled, certification takes place.
# Stop just after certification, but before local commit.
# TOI will be ordered after the DML.
# When wsrep_on is disabled, the certification does not take place.
# However, in this case commit of DML transaction acquires explicit intention
# exclusive lock (aka MDL commit lock - see ha_commit_trans())
# TOI transaction needs to wait for other transactions owning MDL commit lock
# (see wsrep_grant_mdl_exception() logic)
#
--connection node_1
SET DEBUG_SYNC="wsrep_before_commit SIGNAL wsrep_before_commit.reached WAIT_FOR wsrep_before_commit.continue";
--send UPDATE t1 SET k=k+1 WHERE id<100

--connection node_1a
SET DEBUG_SYNC="now WAIT_FOR wsrep_before_commit.reached";

--connection node_2
ALTER TABLE t1 ADD KEY k1 (k);

--connection node_1a
SET DEBUG_SYNC="now SIGNAL wsrep_before_commit.continue";

--connection node_1
--reap

# partial cleanup
--connection node_2
ALTER TABLE t1 DROP KEY k1;
--disable_query_log
--let $sync_cnt = $sync_cnt + 1
--eval INSERT INTO sync VALUES($sync_cnt)
--connection node_1a
--let $wait_condition = SELECT COUNT(*) = $sync_cnt FROM sync
--source include/wait_condition.inc
--enable_query_log

# cleanup
--disconnect node_1a
--disconnect node_1b
--connection node_1
SET SESSION wsrep_on = ON;
DROP TABLE t1;
DROP TABLE sync;
SET DEBUG_SYNC="RESET";
--source include/wait_until_count_sessions.inc
