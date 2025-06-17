-- Copyright (c) 2018, 2025, Oracle and/or its affiliates.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License, version 2.0,
-- as published by the Free Software Foundation.
--
-- This program is designed to work with certain software (including
-- but not limited to OpenSSL) that is licensed under separate terms,
-- as designated in a particular file or component or in included license
-- documentation.  The authors of MySQL hereby grant you an additional
-- permission to link the program and your derivative works with the
-- separately licensed software that they have either included with
-- the program or referenced in the documentation.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License, version 2.0, for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

--
-- Create the system users and grant them appropiate privileges.
-- These users are used for internally.
-- This script is called only while initializing the database.
--

-- Create an user that is used by plugins.
CREATE USER 'mysql.session'@localhost IDENTIFIED WITH caching_sha2_password
 AS '$A$005$THISISACOMBINATIONOFINVALIDSALTANDPASSWORDTHATMUSTNEVERBRBEUSED'
 ACCOUNT LOCK;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.session'@localhost;
GRANT SELECT ON mysql.user TO 'mysql.session'@localhost;
GRANT SELECT ON `performance_schema`.* TO 'mysql.session'@localhost;
GRANT SUPER ON *.* TO 'mysql.session'@localhost;
GRANT SYSTEM_VARIABLES_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT SESSION_VARIABLES_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT AUTHENTICATION_POLICY_ADMIN  ON *.* TO 'mysql.session'@localhost;
GRANT PERSIST_RO_VARIABLES_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT CLONE_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT BACKUP_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT SHUTDOWN ON *.* TO 'mysql.session'@localhost;
GRANT CONNECTION_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT SYSTEM_USER ON *.* TO 'mysql.session'@localhost;

-- this is a plugin priv that might not be registered
INSERT IGNORE INTO mysql.global_grants VALUES ('mysql.session', 'localhost', 'AUDIT_ABORT_EXEMPT', 'N');

-- this is a plugin priv that might not be registered
INSERT IGNORE INTO mysql.global_grants VALUES ('mysql.session', 'localhost', 'FIREWALL_EXEMPT', 'N');

-- Create an user that is definer for information_schema view
CREATE USER 'mysql.infoschema'@localhost IDENTIFIED WITH caching_sha2_password
 AS '$A$005$THISISACOMBINATIONOFINVALIDSALTANDPASSWORDTHATMUSTNEVERBRBEUSED'
 ACCOUNT LOCK;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.infoschema'@localhost;
GRANT SELECT ON *.* TO 'mysql.infoschema'@localhost;
GRANT SYSTEM_USER ON *.* TO 'mysql.infoschema'@localhost;

-- Create the PXC internal session user
-- This user is used by PXC to run commands needed by PXC.
-- Specifically, this user is used by the SST process to create the SST user and
-- run other commands needed before/after the SST.
-- If the privileges are changed, the mysql_system_tables_fix.sql script
-- will also need to be modified to match.
--
-- This user is disabled for login
-- This user has the following privileges:
--   CREATE USER (to assign roles, add/delete users)
--   RELOAD (flush privileges)
--   SUPER (if node is read-only we can still run)
CREATE USER 'mysql.pxc.internal.session'@localhost IDENTIFIED WITH caching_sha2_password
 AS '$A$005$THISISACOMBINATIONOFINVALIDSALTANDPASSWORDTHATMUSTNEVERBRBEUSED'
 ACCOUNT LOCK;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.pxc.internal.session'@localhost;

-- Due to bugs with roles, we need to grant superuser access here
-- If the new dynamic privilege is introduced, remember to include it in the corresponding
-- part in mysql_syste_tables_fix.sql
GRANT ALL PRIVILEGES ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;

-- TODO: Investigate why commit 7e467c54 claims that there is a "bug with roles"
-- and grants all privileges. It should be enough to use only the following grants
-- as per the comment above.
-- We need mysql.pxc.internal.session only for one purpose: create SST user
-- and grant mysql.pxc.sst.role role to it.
-- GRANT CREATE USER ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
-- GRANT SUPER ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
-- GRANT RELOAD ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;

-- Create the PXC SST role
-- This role is used by the SST user during an SST (on the donor)
-- These are the permissions needed to backup the database (using Percona XtraBackup)
-- See https://www.percona.com/doc/percona-xtrabackup/8.0/using_xtrabackup/privileges.html
CREATE ROLE 'mysql.pxc.sst.role'@localhost;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.pxc.sst.role'@localhost;

-- Needed by sst_xtrabackup-v2
GRANT LOCK TABLES, PROCESS, RELOAD, REPLICATION CLIENT, SUPER, INNODB_REDO_LOG_ARCHIVE ON *.* TO 'mysql.pxc.sst.role'@localhost;

DROP PROCEDURE IF EXISTS conditional_grants;
DELIMITER $$
CREATE PROCEDURE conditional_grants()
BEGIN
    DECLARE lctn INT DEFAULT 0;

    -- Get the value of lower_case_table_names
    SET lctn = @@lower_case_table_names;

    IF lctn = 1 THEN
        -- Case-insensitive system: use lowercase schema/table
        GRANT ALTER, CREATE, SELECT, INSERT ON percona_schema.xtrabackup_history TO 'mysql.pxc.sst.role'@'localhost';
        -- Need this to create the percona_schema database if needed
        GRANT CREATE ON percona_schema.* TO 'mysql.pxc.sst.role'@localhost;
    ELSE
        -- Case-sensitive system: match the exact case used when creating DB
        GRANT ALTER, CREATE, SELECT, INSERT ON PERCONA_SCHEMA.xtrabackup_history TO 'mysql.pxc.sst.role'@'localhost';
        -- Need this to create the PERCONA_SCHEMA database if needed
        GRANT CREATE ON PERCONA_SCHEMA.* TO 'mysql.pxc.sst.role'@localhost;
    END IF;
END $$
DELIMITER ;

CALL conditional_grants();

DROP PROCEDURE conditional_grants;

-- Common. WITH GRANT OPTION is needed by clone-sst script
GRANT BACKUP_ADMIN, EXECUTE ON *.* TO 'mysql.pxc.sst.role'@localhost WITH GRANT OPTION;

-- The reason why we need SELECT on performance_schema.* is because both the SST script
-- and PXB need to query keyring status and log_status PFS tables respectively.
GRANT SELECT ON performance_schema.* TO 'mysql.pxc.sst.role'@localhost WITH GRANT OPTION;

-- Additional grants needed by sst_clone
-- Together with CREATE USER, grant CREATE/DROP ROLE, because during the server upgrade
-- they will be set anyway (and it affects mtr_check)
-- See mysql_system_tables_fix.sql:
-- UPDATE user SET Create_role_priv= 'Y', Drop_role_priv= 'Y' WHERE Create_user_priv = 'Y';
GRANT CLONE_ADMIN, SYSTEM_USER, SHUTDOWN, CONNECTION_ADMIN, CREATE USER, CREATE ROLE, DROP ROLE, SYSTEM_VARIABLES_ADMIN ON *.* TO 'mysql.pxc.sst.role'@localhost;
GRANT INSERT, DELETE ON mysql.plugin TO 'mysql.pxc.sst.role'@localhost;
GRANT UPDATE, INSERT ON performance_schema.* TO 'mysql.pxc.sst.role'@localhost;

-- this is a plugin priv that might not be registered
INSERT IGNORE INTO mysql.global_grants VALUES ('mysql.infoschema', 'localhost', 'AUDIT_ABORT_EXEMPT', 'N');

-- this is a plugin priv that might not be registered
INSERT IGNORE INTO mysql.global_grants VALUES ('mysql.infoschema', 'localhost', 'FIREWALL_EXEMPT', 'N');
