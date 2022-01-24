# Start server with provided value for --wsrep-sst-allowed-methods or with defaults.
# Then make sst request using garbd
# Validate the log accordingly.
# Expected server state before execution is started.
# Server state after execution is started.
#
# Usage:
# --let $sst_allowed_methods = my_sst_method1,my_sst_method2
# --let $sst_request_method = my_sst_method1
# --let $sst_request_data = /request/data/string
# --let $expect_rejection = no  <-- we expect the server will accept the request
# source galera_sst_allowed_methods_garbd.inc


--let $error_log= $MYSQLTEST_VARDIR/tmp/test_error_log.err

if ($sst_allowed_methods != '')
{
  --let $wsrep_sst_allowed_methods_param = --wsrep-sst-allowed-methods='$sst_allowed_methods'
}

--let $do_not_echo_parameters = 1
--let $restart_parameters = "restart:--wsrep-provider-options='base_port=$NODE_GALERAPORT_1' --log-error=$error_log $wsrep_sst_allowed_methods_param"

--source include/restart_mysqld.inc

--echo Starting garbd with --sst $sst_request_method:$sst_request_data

--error 0, 1
--exec `dirname $WSREP_PROVIDER`/../bin/garbd --address "gcomm://127.0.0.1:$NODE_GALERAPORT_1" --group my_wsrep_cluster --options 'base_port=$NODE_GALERAPORT_2' --sst "$sst_request_method:$sst_request_data"

--let SEARCH_FILE = $error_log

if ($expect_rejection == 'yes')
{
  --let SEARCH_PATTERN = \[ERROR\] WSREP: Invalid sst_request: $sst_request_method
}

if ($expect_rejection != 'yes')
{
  --let SEARCH_PATTERN = WSREP: Running: 'wsrep_sst_$sst_request_method
}

--source include/search_pattern_in_file.inc

--remove_file $error_log
