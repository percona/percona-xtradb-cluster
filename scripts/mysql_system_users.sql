-- Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License, version 2.0,
-- as published by the Free Software Foundation.
--
-- This program is also distributed with certain software (including
-- but not limited to OpenSSL) that is licensed under separate terms,
-- as designated in a particular file or component or in included license
-- documentation.  The authors of MySQL hereby grant you an additional
-- permission to link the program and your derivative works with the
-- separately licensed software that they have included with MySQL.
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
GRANT PERSIST_RO_VARIABLES_ADMIN ON *.* TO 'mysql.session'@localhost;
GRANT SYSTEM_USER ON *.* TO 'mysql.session'@localhost;

-- Create an user that is definer for information_schema view
CREATE USER 'mysql.infoschema'@localhost IDENTIFIED WITH caching_sha2_password
 AS '$A$005$THISISACOMBINATIONOFINVALIDSALTANDPASSWORDTHATMUSTNEVERBRBEUSED'
 ACCOUNT LOCK;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.infoschema'@localhost;
GRANT SELECT ON *.* TO 'mysql.infoschema'@localhost;

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
GRANT ALL PRIVILEGES ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
GRANT BACKUP_ADMIN, LOCK TABLES, PROCESS, RELOAD, REPLICATION CLIENT, SUPER ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
--GRANT CREATE USER ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
--GRANT SUPER ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;
--GRANT RELOAD ON *.* TO 'mysql.pxc.internal.session'@localhost WITH GRANT OPTION;


-- Create the PXC SST role
-- This role is used by the SST user during an SST (on the donor)
-- These are the permissions needed to backup the database (using Percona XtraBackup)
-- See https://www.percona.com/doc/percona-xtrabackup/8.0/using_xtrabackup/privileges.html
CREATE ROLE 'mysql.pxc.sst.role'@localhost;
REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'mysql.pxc.sst.role'@localhost;
GRANT BACKUP_ADMIN, LOCK TABLES, PROCESS, RELOAD, REPLICATION CLIENT, SUPER ON *.*
 TO 'mysql.pxc.sst.role'@localhost;
GRANT CREATE, SELECT, INSERT ON PERCONA_SCHEMA.xtrabackup_history
 TO 'mysql.pxc.sst.role'@localhost;
-- For some reason this is also needed, although the docs say BACKUP_ADMIN is enough
GRANT SELECT ON performance_schema.* TO 'mysql.pxc.sst.role'@localhost;
-- Need this to create the PERCONA_SCHEMA database if needed
GRANT CREATE ON PERCONA_SCHEMA.* to 'mysql.pxc.sst.role'@localhost;
