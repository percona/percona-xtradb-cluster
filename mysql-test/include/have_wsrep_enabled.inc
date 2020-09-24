
# To be used in a test which requires wsrep plugin to be ACTIVE and enabled
# (i.e. wsrep_on=ON). It includes have_wsrep.inc.

--source include/have_wsrep.inc

--disable_query_log
--disable_result_log
SHOW VARIABLES LIKE 'wsrep_on';
--enable_result_log
--enable_query_log
