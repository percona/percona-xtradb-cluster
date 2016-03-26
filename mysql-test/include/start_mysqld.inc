
--let $restart_param= 1;

if (!$restart_parameters)
{
  let $restart_parameters = restart;
  --let $restart_param = 0;
}

# We use --exec echo instead of simple "--echo" because
# replace_result works on --exec
# we avoid printing restart argument as galera uses files specific names
# that then needs to be either replaced so that result file is compatible
# on all distro or we use generic restart keyword.
--exec echo "# restart"
#if (!$restart_hide_args)
#{
#  --exec echo "# $restart_parameters"
#}
#if ($restart_hide_args)
#{
#  --echo # restart:<hidden args>
#  let $restart_hide_args=;
#}

# Include this script only after using shutdown_mysqld.inc
# or kill_mysqld.inc or expect_crash.inc
# where $_expect_file_name was initialized.
# Write file to make mysql-test-run.pl start up the server again

if ($galera_wsrep_start_position != '') {
	--echo Using --wsrep-start-position when starting mysqld ...

	# starting 5.7 ":" is important otherwise the restart params are not passed.
	if (!$restart_param)
	{
		--exec echo "restart: --wsrep-start-position=$galera_wsrep_start_position" > $_expect_file_name
	}

	if ($restart_param)
	{
		--exec echo "$restart_parameters --wsrep-start-position=$galera_wsrep_start_position" > $_expect_file_name
	}

	--let $galera_wsrep_start_position = 0
}

if ($galera_wsrep_start_position == '') {
	--exec echo "$restart_parameters" > $_expect_file_name
}


# Turn on reconnect
--enable_reconnect

# Call script that will poll the server waiting for it to be back online again
--source include/wait_until_connected_again.inc

# Turn off reconnect again
--disable_reconnect

