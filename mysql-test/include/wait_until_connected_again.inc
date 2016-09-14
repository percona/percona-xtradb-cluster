#
# Include this script to wait until the connection to the
# server has been restored or timeout occurs

#
# We require two consequtive connection successes in order to
# work around a race condition on Galera startup where the server
# can temporarily accept queries before starting to reject them again
#

--disable_result_log
--disable_query_log
let $counter= 5000;
if ($VALGRIND_TEST) {
  let $counter= 30000;
}
let $mysql_errno= 9999;
let $successes= 2;

while ($successes) {
  while ($mysql_errno) {
    # Strangely enough, the server might return "Too many connections"
    # while being shutdown, thus 1040 is an "allowed" error
    # See BUG#36228
    --error 0,1040,1047,1053,1205,2002,2003,2006,2013,1205
    show status;

    --dec $counter
    if (!$counter) {
      --die Server failed to restart
    }
     --sleep 0.1
  }
  --dec $successes
  --sleep 0.1
}
--enable_query_log
--enable_result_log
