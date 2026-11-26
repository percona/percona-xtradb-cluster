#
# Scenario of galera.galera_replay_no_op_trx for a single statement form.
#
# The victim on node_1 runs $odku_stmt, which matches the existing row
# (k = 20, v = 100) without changing it, so its write set has certification
# keys but no row events. A conflicting INSERT from node_2 BF-aborts the
# victim after certification and the victim is replayed.
#
# Parameters:
#   $odku_stmt  no-op INSERT ... ON DUPLICATE KEY UPDATE for k = 20, v = 100
#
# Requires connections node_1, node_1a (on node #1, wsrep_sync_wait = 0)
# and node_2. Creates and drops table t1.
#

--connection node_1
--let $wsrep_local_replays_old = `SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_replays'`

CREATE TABLE t1 (id BIGINT AUTO_INCREMENT PRIMARY KEY, k INT NOT NULL, v INT NOT NULL, UNIQUE KEY uk(k));
INSERT INTO t1 (k, v) VALUES (10, 100), (20, 100);

# Victim: no-op ODKU, locks uk(k=20) and the gap (10, 20)
START TRANSACTION;
--eval $odku_stmt

# Block the applier on node #1 before it applies the conflicting write set
--connection node_1a
--let $galera_sync_point = apply_monitor_slave_enter_sync
--source include/galera_set_sync_point.inc

# Conflicting transaction: inserts into the gap locked by the victim
--connection node_2
INSERT INTO t1 (k, v) VALUES (15, 1);

--connection node_1a
--source include/galera_wait_sync_point.inc
--source include/galera_clear_sync_point.inc

# The victim gets a higher seqno; block it before the commit monitor
--let $galera_sync_point = commit_monitor_master_enter_sync
--source include/galera_set_sync_point.inc

--connection node_1
--send COMMIT

--connection node_1a
--let $galera_sync_point = apply_monitor_slave_enter_sync commit_monitor_master_enter_sync
--source include/galera_wait_sync_point.inc
--source include/galera_clear_sync_point.inc

# Let the applier proceed until it BF-aborts the victim
--let $galera_sync_point = abort_trx_end
--source include/galera_set_sync_point.inc
--let $galera_sync_point = apply_monitor_slave_enter_sync
--source include/galera_signal_sync_point.inc
--let $galera_sync_point = abort_trx_end commit_monitor_master_enter_sync
--source include/galera_wait_sync_point.inc

# Let both transactions proceed; the victim is replayed
--source include/galera_clear_sync_point.inc
--let $galera_sync_point = abort_trx_end
--source include/galera_signal_sync_point.inc
--let $galera_sync_point = commit_monitor_master_enter_sync
--source include/galera_signal_sync_point.inc

# The replayed commit succeeds
--connection node_1
--reap

--let $wsrep_local_replays_new = `SELECT VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_local_replays'`
--disable_query_log
--eval SELECT $wsrep_local_replays_new - $wsrep_local_replays_old = 1 AS wsrep_local_replays
--enable_query_log

SELECT k, v FROM t1 ORDER BY k;

# The node must still be able to commit, apply and replicate
INSERT INTO t1 (k, v) VALUES (30, 1);

--connection node_2
--let $wait_condition = SELECT COUNT(*) = 4 FROM t1
--source include/wait_condition.inc
SELECT k, v FROM t1 ORDER BY k;
INSERT INTO t1 (k, v) VALUES (40, 1);

--connection node_1
--let $wait_condition = SELECT COUNT(*) = 5 FROM t1
--source include/wait_condition.inc
SELECT k, v FROM t1 ORDER BY k;

DROP TABLE t1;
