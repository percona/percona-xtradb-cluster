###
### NOTE: For audit log filter impact can only be seen in new connection.
###       Expected is to restart server but we are opening new connection for testing.
###

--let $_ds_servers= $diff_servers
--let $_save_assert_command= $CURRENT_CONNECTION

while ($_ds_servers != '') {
  --connection $_save_assert_command
  --let $_ds_server_number= `SELECT SUBSTRING_INDEX('$_ds_servers', ' ', 1)`
  --let $_ds_servers= `SELECT TRIM(SUBSTRING('$_ds_servers', 1 + LENGTH('_$ds_server_number')))`

  --let $port_for_connection=$NODE_MYPORT_1
  if ($_ds_server_number == 2) {
    --let $port_for_connection=$NODE_MYPORT_2
  }
  if ($_ds_server_number == 3) {
    --let $port_for_connection=$NODE_MYPORT_3
  }
  --connect _node_, 127.0.0.1, user1, 111, test, $port_for_connection
  --connection _node_
  SELECT audit_log_filter_flush();
  --echo
  --echo Executing statement on: node_$_ds_server_number
  if ($expected_error) {
    --error $expected_error
  }
  --eval $statement
  --disconnect _node_
  
}

--connection $_save_assert_command
--let $expected_error=
--let $statement=
