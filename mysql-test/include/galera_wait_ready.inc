let $wait_condition = SELECT 1 FROM performance_schema.global_status WHERE VARIABLE_NAME = 'wsrep_ready' AND VARIABLE_VALUE = 'ON';
--source include/wait_condition.inc
