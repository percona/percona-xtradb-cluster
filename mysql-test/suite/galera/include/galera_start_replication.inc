--echo Loading wsrep provider ...

--disable_query_log

# Changing wsrep_cluster_address will cause the client connection to be shutdown
# Depending on timing, it may trigger 2013 (CR_SERVER_LOST) on the client side
--error 0,2013
--eval SET GLOBAL wsrep_cluster_address = '$wsrep_cluster_address_orig';

--enable_query_log

--enable_reconnect
--source include/wait_until_connected_again.inc
--source include/galera_wait_ready.inc
