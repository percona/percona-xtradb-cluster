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

if ($suppress_print_params == '') {
  # We use --exec echo instead of simple "--echo" because
  # replace_result works on --exec
  if (!$do_not_echo_parameters) {
    --replace_regex /--wsrep-start-position=[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}:[0-9]*/--wsrep-start-position=<WSREP_START_POSITION>/ /127\.0\.0\.1:[0-9]{1,5}/127.0.0.1:<PORT>/ /socket\.ssl_ca=\/[-_\/[:alnum:]\.]*/socket.ssl_ca=<SSL_CA>/ /socket\.ssl_cert=\/[-_\/[:alnum:]\.]*/socket.ssl_cert=<SSL_CERT>/ /socket\.ssl_key=\/[-_\/[:alnum:]\.]*/socket.ssl_key=<SSL_KEY>/ /base_port=[0-9]{5}/base_port=<PORT>/
    --exec echo "# $restart_parameters"
  }
  if ($do_not_echo_parameters) {
    --echo # restart:<hidden args>
    let $do_not_echo_parameters=;
  }
}

# Include this script only after using shutdown_mysqld.inc
# or kill_mysqld.inc or expect_crash.inc
# where $_expect_file_name was initialized.
# Write file to make mysql-test-run.pl start up the server again
--exec echo "$restart_parameters" > $_expect_file_name

# Turn on reconnect
--enable_reconnect

# Call script that will poll the server waiting for it to be back online again
# Allow for slow machines like Sparc
--let $wait_counter= 6000
--source include/wait_until_connected_again.inc

## Script xplugin_wait_for_interfaces.inc
#
#  $do_not_wait_for_x_interface -
#    set for which interfaces the script shouldn't wait
#    In default the script wait for both socket and tcp
#    on *NIX OS-es and only for TCP on windows
#
## Usage
#
# --let $wait_for_tcpsocket_status = valid_value|undefined_value|no_wait
# --let $wait_for_unixsocket_status = valid_value|undefined_value|no_wait
# --source include/xplugin_wait_for_interfaces.inc
#

--let $_xplugin_active= `SELECT plugin_status='ACTIVE' FROM information_schema.plugins WHERE plugin_name LIKE 'mysqlx'`
if ($_xplugin_active)
{
  --source include/xplugin_wait_for_interfaces.inc
}

# Turn off reconnect again
--disable_reconnect

