# Parameters:
# $option_name
# $command_opt_value

# ------------------------------------------------------------------------------------------------------
--echo
--echo # Testing with invalid option: $option_name
--let $PID_FILE= $MYSQLTEST_VARDIR/tmp/garbd.pid
--let $GARBD_LOG_FILE= $MYSQLTEST_VARDIR/tmp/garbd_$option_name.log

--let $command = "$GALERA_GARBD"
--let $command_opt = $command_opt_value > $GARBD_LOG_FILE 2>&1
--source include/start_proc_in_background.inc

--source include/wait_proc_to_finish.inc

# Verify that the reason for the shutdown is logged in the garbd log.
--let $assert_select= '$option_name' option contains non-printable ASCII characters
--let $assert_text= Appropriate message has been written to the garbd log explaining the reason for the shutdown.
--let $assert_count= 1
--let $assert_file= $GARBD_LOG_FILE
--source include/assert_grep.inc

--remove_file $GARBD_LOG_FILE
--remove_file $PID_FILE

