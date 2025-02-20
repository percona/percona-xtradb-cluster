if (!$restart_parameters)
{
  let $restart_parameters = restart;
}

--let $_server_id= `SELECT @@server_id`
--let $_expect_file_name= $MYSQLTEST_VARDIR/tmp/mysqld.$_server_id.expect

if (!$do_not_echo_parameters)
{
  --exec echo "# Kill and $restart_parameters"
}
if ($do_not_echo_parameters)
{
  --exec echo "# Kill and restart:<hidden args>"
  let $do_not_echo_parameters=;
}
--exec echo "$restart_parameters" > $_expect_file_name
--shutdown_server 0
--source include/wait_until_disconnected.inc
<<<<<<< HEAD
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
||||||| merged common ancestors
<<<<<<<<< Temporary merge branch 1
||||||||| merged common ancestors
--enable_reconnect
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
=========
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
>>>>>>>>> Temporary merge branch 2
=======
>>>>>>> Percona-Server-9.1.0-1

--let $_xplugin_active= `SELECT plugin_status='ACTIVE' FROM information_schema.plugins WHERE plugin_name LIKE 'mysqlx'`
if ($_xplugin_active)
{
<<<<<<< HEAD
  --source include/xplugin_wait_for_interfaces.inc
||||||| merged common ancestors
    --enable_reconnect
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
    --disable_reconnect
=======
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
>>>>>>> Percona-Server-9.1.0-1
}
<<<<<<< HEAD
||||||| merged common ancestors
<<<<<<<<< Temporary merge branch 1
--let $no_wait_for_reconnect = 0
||||||||| merged common ancestors
--disable_reconnect
=========
>>>>>>>>> Temporary merge branch 2
=======
--let $no_wait_for_reconnect = 0
>>>>>>> Percona-Server-9.1.0-1
