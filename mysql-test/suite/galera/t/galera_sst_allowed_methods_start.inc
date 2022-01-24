# Start server with provided value for --wsrep-sst-allowed-methods.
# This may result with startup abort, or proper startup
# Validate the log accordingly.
# Expected server state before execution is stopped.
# Server state after execution is stopped.
#
# Usage:
# --let $sst_allowed_methods = test_method
# --let $expect_abort = no  <-- we expect the server will start correctly
# source galera_sst_allowed_methods_start.inc


--echo Starting server with --wsrep-sst-allowed-methods='$sst_allowed_methods'

--let $error_log= $MYSQLTEST_VARDIR/tmp/test_error_log.err
--let $start_params = --wsrep-provider-options='base_port=$NODE_GALERAPORT_1' --wsrep-sst-allowed-methods='$sst_allowed_methods'

if ($expect_abort == 'yes')
{
  --let $mysqld=$MYSQLD_CMD --core-file --console > $error_log 2>&1 $start_params
  --error 1
  --exec $mysqld
  --let SEARCH_FILE = $error_log
  --let SEARCH_PATTERN = \[ERROR\] WSREP: Wrong format of --wsrep-sst-allowed-methods parameter value: $sst_allowed_methods
  --source include/search_pattern_in_file.inc
}

if ($expect_abort != 'yes')
{
  --let $do_not_echo_parameters = 1
  --let $restart_parameters = "restart:$start_params --log-error=$error_log"
  --source include/start_mysqld.inc
  SHOW VARIABLES LIKE 'wsrep_sst_allowed_methods';
  --source include/shutdown_mysqld.inc
}

--remove_file $error_log
