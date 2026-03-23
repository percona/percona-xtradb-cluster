# For a given tuple of (user, password), test the three scenarios:
# 1. Fixed source + fixed replica
# 2. Fixed source + old replica
# 3. Old source + fixed replica
#
# We expect all combinations to work.

--source include/count_sessions.inc

# The test will use this TOI as a simple synchronization point, to know that
# all previous queries were replicated and executed on remote nodes.
--let $syncpoint = CREATE TABLE checkpoint(id INT PRIMARY KEY); DROP TABLE checkpoint;

--echo #
--echo # Testing for: user: $username, password: $password
--echo #

--echo # Scenario: Fixed source, fixed replica

# Create the user on the source node
--connection node_1
--eval CREATE USER "$username"@"%" IDENTIFIED BY "secret_original_1";
--eval GRANT ALL ON *.* TO "$username"@"%";
--eval $syncpoint

# Check that it is possible to use this user on replica
--connect node_2_testuser, 127.0.0.1, $username, secret_original_1,, $NODE_MYPORT_2
--connection node_2_testuser
SELECT 1;
--disconnect node_2_testuser

# Change the password on the source node
--connect node_1_testuser, 127.0.0.1, $username, secret_original_1,, $NODE_MYPORT_1
--eval SET PASSWORD = "$password";
--eval $syncpoint
--disconnect node_1_testuser

# Check that it is possible to connect with the new password on the replica
--connect node_2_testuser, 127.0.0.1, $username, $password,, $NODE_MYPORT_2
--connection node_2_testuser
SELECT 1;
--disconnect node_2_testuser

# cleanup
--connection node_1
--eval DROP USER "$username"@"%";
--eval $syncpoint


--echo # Scenario: Fixed source, old replica

--connection node_2
SET GLOBAL debug = 'd,set_password_simulate_no_pxc_4965';

--connection node_1
--eval CREATE USER "$username"@"%" IDENTIFIED BY "secret_original_2";
--eval GRANT ALL ON *.* TO "$username"@"%";

--connect node_1_testuser, 127.0.0.1, $username, secret_original_2,, $NODE_MYPORT_1
--eval SET PASSWORD = "$password";
--eval $syncpoint
--disconnect node_1_testuser

--connect node_2_testuser, 127.0.0.1, $username, $password,, $NODE_MYPORT_2
--connection node_2_testuser
SELECT 1;
--disconnect node_2_testuser

--connection node_1
--eval DROP USER "$username"@"%";
--connection node_2
SET GLOBAL debug = '';
--eval $syncpoint

--echo # Scenario: Old source, fixed replica

--connection node_1
SET GLOBAL debug = 'd,set_password_simulate_no_pxc_4965';

--connection node_1
--eval CREATE USER "$username"@"%" IDENTIFIED BY "secret_original_3";
--eval GRANT ALL ON *.* TO "$username"@"%";

--connect node_1_testuser, 127.0.0.1, $username, secret_original_3,, $NODE_MYPORT_1
--eval SET PASSWORD = "$password";
--eval $syncpoint
--disconnect node_1_testuser

--connect node_2_testuser, 127.0.0.1, $username, $password,, $NODE_MYPORT_2
--connection node_2_testuser
SELECT 1;
--disconnect node_2_testuser

--connection node_1
--eval DROP USER "$username"@"%";
SET GLOBAL debug = '';
--eval $syncpoint

--source include/wait_until_count_sessions.inc
