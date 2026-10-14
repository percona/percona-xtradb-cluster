#
# ==== Purpose ====
#
# Assert, on EVERY cluster node, whether a table carries the generated
# invisible primary key, and that every node agrees on the table's definition.
#
# PXC-5140 was precisely a case where the nodes silently disagreed - the node
# that issued the CREATE TABLE had no primary key while the others had a
# generated my_row_id - so checking each node individually AND comparing them
# against each other is the point of this helper. A per-node check alone would
# not catch divergence; a comparison alone would not catch both nodes being
# wrong in the same way.
#
# For each node it asserts:
#   1. whether the my_row_id column exists,
#   2. whether my_row_id is the PRIMARY key,
#   3. whether my_row_id is AUTO_INCREMENT and INVISIBLE,
# and then that the node's full column definition matches node_1's.
#
# ==== Usage ====
#
#   --let $gipk_table = t1
#   --let $gipk_expected = 0          # 1 = my_row_id must be present
#                                     # 0 = my_row_id must NOT be present
#   [--let $gipk_nodes = 1 2]         # node numbers, defaults to "1 2"
#   --source suite/galera/include/galera_gipk_assert.inc
#

--let $_gipk_sig_sql = SELECT IFNULL(MD5(GROUP_CONCAT(CONCAT(column_name, ' ', column_type, ' ', extra) ORDER BY ordinal_position SEPARATOR ', ')), 'no-such-table') FROM information_schema.columns WHERE table_schema = 'test' AND table_name = '$gipk_table'

--let $_gipk_node_list = $gipk_nodes
if (!$_gipk_node_list) {
  --let $_gipk_node_list = 1 2
}

# Reference definition: node_1's view of the table.
--connection node_1
--let $_gipk_ref_sig = `$_gipk_sig_sql`

while ($_gipk_node_list != '')
{
  --let $_gipk_n = `SELECT SUBSTRING_INDEX('$_gipk_node_list', ' ', 1)`
  --let $_gipk_node_list = `SELECT TRIM(SUBSTRING('$_gipk_node_list', 1 + LENGTH('$_gipk_n')))`

  --connection node_$_gipk_n

  --let $_gipk_col = `SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = 'test' AND table_name = '$gipk_table' AND column_name = 'my_row_id'`
  --let $_gipk_pk = `SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = 'test' AND table_name = '$gipk_table' AND index_name = 'PRIMARY' AND column_name = 'my_row_id'`
  --let $_gipk_attr = `SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = 'test' AND table_name = '$gipk_table' AND column_name = 'my_row_id' AND extra LIKE '%auto_increment%' AND extra LIKE '%INVISIBLE%'`
  --let $_gipk_sig = `$_gipk_sig_sql`

  --let $assert_text = node_$_gipk_n: $gipk_table my_row_id column present = $gipk_expected
  --let $assert_cond = $_gipk_col = $gipk_expected
  --source include/assert.inc

  --let $assert_text = node_$_gipk_n: $gipk_table my_row_id is the PRIMARY key = $gipk_expected
  --let $assert_cond = $_gipk_pk = $gipk_expected
  --source include/assert.inc

  --let $assert_text = node_$_gipk_n: $gipk_table my_row_id is AUTO_INCREMENT and INVISIBLE = $gipk_expected
  --let $assert_cond = $_gipk_attr = $gipk_expected
  --source include/assert.inc

  # node_1 supplies the reference, so comparing it with itself proves nothing.
  if ($_gipk_n != 1) {
    --let $assert_text = node_$_gipk_n: $gipk_table definition is identical to node_1
    --let $assert_cond = "$_gipk_sig" = "$_gipk_ref_sig"
    --source include/assert.inc
  }
}

--let $gipk_nodes =
