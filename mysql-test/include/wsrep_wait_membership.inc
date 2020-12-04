# Waits for N members in the cluster
#
# Usage:
# --let $members=1
# --source wsrep_wait_membership.inc
#

let $wait_condition = SELECT VARIABLE_VALUE = $members FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_cluster_size';

--source include/wsrep_wait_condition.inc
