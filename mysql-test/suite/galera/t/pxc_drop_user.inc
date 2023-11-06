# Test that DROP USER executed on the user with SYSTEM_USER privilege
# is not replicated when executing user has no SYSTEM_USER

--source include/have_debug_sync.inc
--source include/galera_cluster.inc
--source include/count_sessions.inc

show variables like 'wsrep_trx_fragment_%';

CREATE USER 'x'@'localhost' IDENTIFIED BY 'x';
GRANT CREATE USER, EXECUTE, ALTER ROUTINE, DROP ROLE, ROLE_ADMIN, SELECT, UPDATE, SYSTEM_VARIABLES_ADMIN ON *.* TO 'x'@'localhost' WITH GRANT OPTION;

CREATE USER 'y'@'localhost' IDENTIFIED BY 'y';
GRANT ALL ON *.* TO 'y'@'localhost' WITH GRANT OPTION;

DELIMITER //;

CREATE PROCEDURE p1()
BEGIN
  INSERT INTO t1 (a) VALUES (1);
END//

DELIMITER ;//

CREATE ROLE 'test_role';
GRANT ALL ON *.* TO 'test_role';

CREATE TABLE t1 (a INT PRIMARY KEY);

--connect node_1a, 127.0.0.1, x, x, test, $NODE_MYPORT_1
--error ER_SPECIFIC_ACCESS_DENIED_ERROR
DROP USER 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
ALTER USER 'y'@'localhost' PASSWORD EXPIRE NEVER;

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
REVOKE EXECUTE ON *.* FROM 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
GRANT EXECUTE ON PROCEDURE test.p1 TO 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
REVOKE EXECUTE ON PROCEDURE test.p1 FROM 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
GRANT 'test_role' TO 'y'@'localhost';
--error ER_SPECIFIC_ACCESS_DENIED_ERROR
REVOKE 'test_role' FROM 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
GRANT SELECT ON test.t1 TO 'y'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
ALTER USER 'y'@'localhost' DEFAULT ROLE NONE;

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
DROP PROCEDURE test.p1;

#
# not affected, but let's test them anyway
#
--error ER_SPECIFIC_ACCESS_DENIED_ERROR
RENAME USER 'y'@'localhost' TO 'z'@'localhost';

--error ER_SPECIFIC_ACCESS_DENIED_ERROR
SET PASSWORD FOR 'y'@'localhost' TO RANDOM;

#
# utility user drop
#
--error ER_CANNOT_USER
DROP USER 'utilityuser'@'localhost';


CALL mtr.add_suppression(".*Operation DROP USER failed.*");
CALL mtr.add_suppression(".*Fail to replicate: DROP USER.*");

--connection node_2
CALL mtr.add_suppression(".*Operation DROP USER failed.*");
CALL mtr.add_suppression(".*Query apply failed.*");

--connection node_1

--disconnect node_1a
--connection node_1
DROP TABLE t1;
DROP USER 'x'@'localhost';
DROP USER 'y'@'localhost';
DROP PROCEDURE test.p1;
DROP ROLE 'test_role';
--source include/wait_until_count_sessions.inc
