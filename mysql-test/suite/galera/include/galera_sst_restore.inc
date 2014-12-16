#
# Restore the various options used for SST to their original values
# so that MTR's end-of-test checks are happy.
#

--disable_query_log

--connection node_1
--eval SET GLOBAL wsrep_sst_auth = '$wsrep_sst_auth_orig';

--error 0,ER_CANNOT_USER
DROP USER sst;

--connection node_2
--eval SET GLOBAL wsrep_sst_method = '$wsrep_sst_method_orig';
--eval SET GLOBAL wsrep_sst_receive_address = '$wsrep_sst_receive_address_orig';
