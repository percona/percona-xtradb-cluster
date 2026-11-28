# Shared body for the "SST with innodb_temp_tablespaces_dir outside datadir"
# tests (one wrapper per SST method).
#
# Background: when innodb_temp_tablespaces_dir points outside the datadir, the
# InnoDB session temporary tablespaces (temp_*.ibt) live there. After a crash
# (no clean shutdown) these files are left behind. With wsrep_sst_method=clone
# the joiner then runs 'mysqld --initialize-insecure', which used to fail: during
# --initialize InnoDB creates temp_*.ibt with O_EXCL and, unlike a normal start,
# does NOT clean pre-existing ones (srv0tmp.cc delete_old_pool is skipped when
# create_new_db is true). The fix makes wsrep_sst_clone.sh clean
# innodb_temp_tablespaces_dir together with the other InnoDB directories.
#
# This body reproduces the on-disk precondition deterministically: a clean
# shutdown removes the temp tablespaces, then we plant one leftover temp_*.ibt by
# hand (its mere existence is what matters - the create is an existence check, so
# the content is irrelevant). xtrabackup-v2 SST never runs --initialize and is
# not affected; its wrapper exercises the same scenario as a regression guard.
#
# Requires: $node_2_temp_tbs_dir - node_2's innodb_temp_tablespaces_dir, created
# before the servers start by the wrapper's -master.sh.

--connection node_1
CREATE TABLE t1 (f1 INT PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO t1 VALUES (1), (2), (3);

# node_2 keeps its session temp tablespaces outside the datadir, so temp_1.ibt
# exists there while it is running.
--connection node_2
--let $node_2_datadir= `SELECT @@datadir`
--file_exists $node_2_temp_tbs_dir/temp_1.ibt

# Cleanly stop node_2. This removes its session temp tablespaces.
--source include/shutdown_mysqld.inc

# One-node cluster; add more data so node_2 has to catch up via a real SST.
--connection node_1
--let $wait_condition = SELECT VARIABLE_VALUE = 1 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';
--source include/wait_condition.inc
INSERT INTO t1 VALUES (4), (5), (6);

# Plant a leftover session temporary tablespace in the external temp dir - the
# on-disk state a crash would leave behind (content is irrelevant).
--write_file $node_2_temp_tbs_dir/temp_1.ibt
planted leftover session temporary tablespace
EOF

# Force a full SST on the next start of node_2.
--remove_file $node_2_datadir/grastate.dat

--echo # Restarting node_2 - it must rejoin via SST despite the leftover
--echo # temp tablespace in innodb_temp_tablespaces_dir.
--connection node_2
--source include/start_mysqld.inc
--source include/wait_until_connected_again.inc

# node_2 is back in the cluster ...
--connection node_1
--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';
--source include/wait_condition.inc

# ... with the full dataset received through SST, and its session temp
# tablespaces recreated in the configured directory.
--connection node_2
--let $assert_text= node_2 received all 6 rows of t1 via SST
--let $assert_cond= [SELECT COUNT(*) AS c FROM t1, c, 1] = 6
--source include/assert.inc
--file_exists $node_2_temp_tbs_dir/temp_1.ibt

--connection node_1
DROP TABLE t1;

# Suppressions are installed at the end: a full SST rewrites node_2's datadir
# (clone transfers only the InnoDB part, xtrabackup replaces everything), so
# entries added to the MyISAM mtr.test_suppressions table on node_2 beforehand
# would be lost. See mysql-test/include/mtr_warnings.sql.
--disable_query_log
--connection node_1
CALL mtr.add_suppression("Failed to remove page file");
--connection node_2
CALL mtr.add_suppression("Failed to remove page file");
--enable_query_log
