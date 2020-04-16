#
# Include this script to wait until the connection to the
# server has been restored or timeout occurs
--disable_result_log
--disable_query_log
# Wsrep change: The native $counter initial value was 500, but this
# may not be enough for all tests which restart a node which joins
# back with SST.
let $counter= 1000;
if ($VALGRIND_TEST) {
  let $counter= 3000;
}
let $mysql_errno= 9999;
while ($mysql_errno)
{

  # Strangely enough, the server might return "Too many connections"
  # while being shutdown, thus 1040 is an "allowed" error
  # See BUG#36228

  # WSREP: allow error 1205, as SHOW STATUS may fail if
  #        wsrep_sync_wait is enabled
  --error 0,1040,1053,1205,2002,2003,2006,2013,1045,ER_SECURE_TRANSPORT_REQUIRED
  show status;
  if ($mysql_errno == 1045){
    let mysql_errno=0;
  }
  if ($mysql_errname == ER_SECURE_TRANSPORT_REQUIRED){
    let mysql_errno=0;
  }
  dec $counter;
  if (!$counter)
  {
    --die Server failed to restart
  }
  --sleep 0.1
}
--enable_query_log
--enable_result_log

# WSREP: SHOW STATUS queries are allowed even if wsrep
#        is not ready. Make sure wsrep is ready before
#        returning from this script

--source include/wait_wsrep_ready.inc
