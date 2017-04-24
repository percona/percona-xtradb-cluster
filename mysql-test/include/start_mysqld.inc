if (!$restart_parameters)
{
  let $restart_parameters = restart;
}

if ($galera_wsrep_start_position != '') {
   --echo Using --wsrep-start-position when starting mysqld ...
   --let $restart_parameters = restart:$start_mysqld_params --wsrep-start-position=$galera_wsrep_start_position
   --let $galera_wsrep_start_position = 0
}

if ($galera_wsrep_start_position == '') {
   if ($start_mysqld_params != '') {
      --let $restart_parameters = restart:$start_mysqld_params
   }
}

# We use --exec echo instead of simple "--echo" because
# replace_result works on --exec
--replace_regex /--wsrep-start-position=[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}:[0-9]*/--wsrep-start-position=<WSREP_START_POSITION>/ /127\.0\.0\.1:[0-9]{1,5}/127.0.0.1:<PORT>/
--exec echo "# $restart_parameters"

# Include this script only after using shutdown_mysqld.inc
# or kill_mysqld.inc or expect_crash.inc
# where $_expect_file_name was initialized.
# Write file to make mysql-test-run.pl start up the server again
--exec echo "$restart_parameters" > $_expect_file_name

# Turn on reconnect
--enable_reconnect

# Call script that will poll the server waiting for it to be back online again
--source include/wait_until_connected_again.inc

# Wait for wsrep
--source include/wait_wsrep_ready.inc

# Turn off reconnect again
--disable_reconnect
