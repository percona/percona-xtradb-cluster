#
# ==== Purpose ====
#
# Assert that tables in given databases have the same number of rows
# across the cluster and then select all rows from a specific table on
# each node.
#
# ==== Usage ====
#
# --let $test_details= <informational message>
# --let $replicate= <1 for replication check>
# --let $statement_executing_server= <server where statement needed to be executed>
#
# --let $statement= <optional statement to execute>
#
# --let $databases= <db_name1> [<db_name2> ...]
# --let $diff_servers= <node_1> <node_2> ...
# --let $table= <table_name for executing SELECT * FROM db.table ORDER BY order_by_column>
# --let $order_by_column= <column_name for executing SELECT * FROM db.table ORDER BY order_by_column, ensures row sequence consistency>
# --let $table1= <table_name for executing SELECT * FROM db.table1 ORDER BY order_by_column1>
# --let $order_by_column1= <column_name for executing SELECT * FROM db.table1 ORDER BY order_by_column1, ensures row sequence consistency>
# --let $outside_component_transactions=1 <During non-replication,if transactions have been executed outside component that replicated, set this flag to avoid no replication check>


--echo
--echo ##### START #####
--let $include_filename= assert_table_are_same_in_cluster.inc [$test_details] [node_$statement_executing_server: $statement]
--source include/begin_include_file.inc

if ($statement) {
  --connection node_$statement_executing_server
  --eval $statement
}
if (!$wait_timeout) {
  --let $wait_timeout= 60
}
# Replication can take some time, so below loop makes sure replication is done.
if ($replicate) {
  # Below takes lot of time so commented but it works
  # Anyhow we make sure all tables in database have same number of rows
  # Also we print replicated rows from each node, so result file makes sure data replicated is same.
  #--source ../include/diff_servers_galera.inc

  --let $_db_list= $databases
  while ($_db_list != '') {
    --let $_db= `SELECT SUBSTRING_INDEX('$_db_list', ' ', 1)`
    --let $_db_list= `SELECT TRIM(SUBSTRING('$_db_list', 1 + LENGTH('$_db')))`

    --connection node_$statement_executing_server
    --let $_table_list= `SELECT GROUP_CONCAT(TABLE_NAME SEPARATOR ' ') FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='$_db'`

    while ($_table_list != '') {
      --let $_table= `SELECT SUBSTRING_INDEX('$_table_list', ' ', 1)`
      --let $_table_list= `SELECT TRIM(SUBSTRING('$_table_list', 1 + LENGTH('$_table')))`

      --connection node_$statement_executing_server
      --let $_row_count= `SELECT COUNT(*) FROM $_db.$_table`

      --let $_ds_servers= $diff_servers
      while ($_ds_servers != '') {
        --let $_ds_server_number= `SELECT SUBSTRING_INDEX('$_ds_servers', ' ', 1)`
        --let $_ds_servers= `SELECT TRIM(SUBSTRING('$_ds_servers', 1 + LENGTH('$_ds_server_number')))`
        --connection node_$_ds_server_number

        if($statement_executing_server != $_ds_server_number) {
          if ($_table == $table) {
            --let $wait_condition= SELECT COUNT(*) = $_row_count FROM $_db.$_table;
            --source include/wait_condition.inc
          }
          if ($_table == $table1) {
            --let $wait_condition= SELECT COUNT(*) = $_row_count FROM $_db.$_table;
            --source include/wait_condition.inc
          }
        }
      }
    }
  }
}

if (!$replicate) {
  --let $_db_list= $databases
  while ($_db_list != '') {
    --let $_db= `SELECT SUBSTRING_INDEX('$_db_list', ' ', 1)`
    --let $_db_list= `SELECT TRIM(SUBSTRING('$_db_list', 1 + LENGTH('$_db')))`

    --connection node_$statement_executing_server
    --let $_table_list= `SELECT GROUP_CONCAT(TABLE_NAME SEPARATOR ' ') FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='$_db'`

    while ($_table_list != '') {
      --let $_table= `SELECT SUBSTRING_INDEX('$_table_list', ' ', 1)`
      --let $_table_list= `SELECT TRIM(SUBSTRING('$_table_list', 1 + LENGTH('$_table')))`

      --connection node_$statement_executing_server
      --let $_row_count= `SELECT COUNT(*) FROM $_db.$_table`

      --let $_ds_servers= $diff_servers
      while ($_ds_servers != '') {
        --let $_ds_server_number= `SELECT SUBSTRING_INDEX('$_ds_servers', ' ', 1)`
        --let $_ds_servers= `SELECT TRIM(SUBSTRING('$_ds_servers', 1 + LENGTH('$_ds_server_number')))`
        if ($statement_executing_server != $_ds_server_number) {
          --connection node_$_ds_server_number
          --let $_current_row_count= `SELECT COUNT(*) FROM $_db.$_table`
          if ($outside_component_transactions) {
            --echo "Verify manually due to outside transaction for $_db.$_table."
            --echo "Primary server has $_row_count rows, secondary server row count: $_current_row_count
          }
          if (!$outside_component_transactions) {
            --let $assert_cond= "[SELECT COUNT(*) AS count FROM $_db.$_table, count, 1]" = "0"
            --let $assert_text= Table $_db.$_table has different number of rows on node $_ds_server_number. Expected 0(no replication), found $_current_row_count. Remote server has $_row_count rows.
            --source include/assert.inc
          }
        }
      }
    }
  }
}

--let $_ds_servers= $diff_servers
while ($_ds_servers != '')
{
  --let $_ds_server_number= `SELECT SUBSTRING_INDEX('$_ds_servers', ' ', 1)`
  --let $_ds_servers= `SELECT TRIM(SUBSTRING('$_ds_servers', 1 + LENGTH('_$ds_server_number')))`
  --connection node_$_ds_server_number
  --echo Connected to: node_$_ds_server_number
  --eval SELECT * FROM $databases.$table ORDER BY $order_by_column
  if ($table1)
  {
    --eval SELECT * FROM $databases.$table1 ORDER BY $order_by_column1
  }
}

--let $include_filename= assert_table_are_same_in_cluster.inc [$test_details]
--source include/end_include_file.inc

--echo ##### END #####
--echo

--connection node_1
--let $statement=
--let $test_details=
