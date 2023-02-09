-- Copyright (c) 2007, 2022, Oracle and/or its affiliates.
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
-- The system tables of MySQL Server
--

set @have_innodb= (select count(engine) from information_schema.engines where engine='INNODB' and support != 'NO');
set @is_mysql_encrypted = (select ENCRYPTION from information_schema.INNODB_TABLESPACES where NAME='mysql');

-- Tables below are NOT treated as DD tables by MySQL server yet.

SET FOREIGN_KEY_CHECKS= 1;

# Added sql_mode elements and making it as SET, instead of ENUM

set default_storage_engine=InnoDB;

SET @cmd = "CREATE TABLE IF NOT EXISTS db
(
Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
Db char(64) binary DEFAULT '' NOT NULL,
User char(32) binary DEFAULT '' NOT NULL,
Select_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Insert_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Update_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Delete_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Drop_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Grant_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
References_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Index_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Alter_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_tmp_table_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Lock_tables_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_view_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Show_view_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_routine_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Alter_routine_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Execute_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Event_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Trigger_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
PRIMARY KEY Host (Host,User,Db), KEY User (User)
)
engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Database privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


-- Remember for later if db table already existed
set @had_db_table= @@warning_count != 0;

SET @cmd = "CREATE TABLE IF NOT EXISTS user
(
Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
User char(32) binary DEFAULT '' NOT NULL,
Select_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Insert_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Update_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Delete_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Drop_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Reload_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Shutdown_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Process_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
File_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Grant_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
References_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Index_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Alter_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Show_db_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Super_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_tmp_table_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Lock_tables_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Execute_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Repl_slave_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Repl_client_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_view_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Show_view_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_routine_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Alter_routine_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_user_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Event_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Trigger_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_tablespace_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
ssl_type enum('','ANY','X509', 'SPECIFIED') COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
ssl_cipher BLOB NOT NULL,
x509_issuer BLOB NOT NULL,
x509_subject BLOB NOT NULL,
max_questions int unsigned DEFAULT 0  NOT NULL,
max_updates int unsigned DEFAULT 0  NOT NULL,
max_connections int unsigned DEFAULT 0  NOT NULL,
max_user_connections int unsigned DEFAULT 0  NOT NULL,
plugin char(64) DEFAULT 'caching_sha2_password' NOT NULL,
authentication_string TEXT,
password_expired ENUM('N', 'Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
password_last_changed timestamp NULL DEFAULT NULL,
password_lifetime smallint unsigned NULL DEFAULT NULL,
account_locked ENUM('N', 'Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Create_role_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Drop_role_priv enum('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
Password_reuse_history smallint unsigned NULL DEFAULT NULL,
Password_reuse_time smallint unsigned NULL DEFAULT NULL,
Password_require_current enum('N', 'Y') COLLATE utf8mb3_general_ci DEFAULT NULL,
User_attributes JSON DEFAULT NULL,
PRIMARY KEY Host (Host,User)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Users and global privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


SET @cmd = "CREATE TABLE IF NOT EXISTS default_roles
(
HOST CHAR(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
USER CHAR(32) BINARY DEFAULT '' NOT NULL,
DEFAULT_ROLE_HOST CHAR(255) CHARACTER SET ASCII DEFAULT '%' NOT NULL,
DEFAULT_ROLE_USER CHAR(32) BINARY DEFAULT '' NOT NULL,
PRIMARY KEY (HOST, USER, DEFAULT_ROLE_HOST, DEFAULT_ROLE_USER)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Default roles' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


SET @cmd = "CREATE TABLE IF NOT EXISTS role_edges
(
FROM_HOST CHAR(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
FROM_USER CHAR(32) BINARY DEFAULT '' NOT NULL,
TO_HOST CHAR(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
TO_USER CHAR(32) BINARY DEFAULT '' NOT NULL,
WITH_ADMIN_OPTION ENUM('N', 'Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
PRIMARY KEY (FROM_HOST,FROM_USER,TO_HOST,TO_USER)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Role hierarchy and role grants' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


SET @cmd = "CREATE TABLE IF NOT EXISTS global_grants
(
USER CHAR(32) BINARY DEFAULT '' NOT NULL,
HOST CHAR(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
PRIV CHAR(32) COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
WITH_GRANT_OPTION ENUM('N','Y') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL,
PRIMARY KEY (USER,HOST,PRIV)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Extended global grants' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


-- Remember for later if user table already existed
set @had_user_table= @@warning_count != 0;

SET @cmd = "CREATE TABLE IF NOT EXISTS password_history
(
  Host CHAR(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
  User CHAR(32) BINARY DEFAULT '' NOT NULL,
  Password_timestamp TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  Password TEXT,
  PRIMARY KEY(Host, User, Password_timestamp DESC)
 ) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin
 comment='Password history for user accounts' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS func (  name char(64) binary DEFAULT '' NOT NULL, ret tinyint DEFAULT '0' NOT NULL, dl char(128) DEFAULT '' NOT NULL, type enum ('function','aggregate') COLLATE utf8mb3_general_ci NOT NULL, PRIMARY KEY (name) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin   comment='User defined functions' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS plugin ( name varchar(64) DEFAULT '' NOT NULL, dl varchar(128) DEFAULT '' NOT NULL, PRIMARY KEY (name) ) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci comment='MySQL plugins' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS help_topic ( help_topic_id int unsigned not null, name char(64) not null, help_category_id smallint unsigned not null, description text not null, example text not null, url text not null, primary key (help_topic_id), unique index (name) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 comment='help topics' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS help_category ( help_category_id smallint unsigned not null, name  char(64) not null, parent_category_id smallint unsigned null, url text not null, primary key (help_category_id), unique index (name) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 comment='help categories' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS help_relation ( help_topic_id int unsigned not null, help_keyword_id  int unsigned not null, primary key (help_keyword_id, help_topic_id) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 comment='keyword-topic relation' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS servers ( Server_name char(64) NOT NULL DEFAULT '', Host char(255) CHARACTER SET ASCII NOT NULL DEFAULT '', Db char(64) NOT NULL DEFAULT '', Username char(64) NOT NULL DEFAULT '', Password char(64) NOT NULL DEFAULT '', Port INT NOT NULL DEFAULT '0', Socket char(64) NOT NULL DEFAULT '', Wrapper char(64) NOT NULL DEFAULT '', Owner char(64) NOT NULL DEFAULT '', PRIMARY KEY (Server_name)) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 comment='MySQL Foreign Servers table' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS tables_priv
(
  Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
  Db char(64) binary DEFAULT '' NOT NULL,
  User char(32) binary DEFAULT '' NOT NULL,
  Table_name char(64) binary DEFAULT '' NOT NULL,
  Grantor varchar(288) DEFAULT '' NOT NULL,
  Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  Table_priv set('Select','Insert','Update','Delete','Create','Drop','Grant','References','Index','Alter','Create View','Show view','Trigger') COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
  Column_priv set('Select','Insert','Update','References') COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
  PRIMARY KEY (Host,User,Db,Table_name),
  KEY Grantor (Grantor)
) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='Table privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS columns_priv
(
  Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
  Db char(64) binary DEFAULT '' NOT NULL,
  User char(32) binary DEFAULT '' NOT NULL,
  Table_name char(64) binary DEFAULT '' NOT NULL,
  Column_name char(64) binary DEFAULT '' NOT NULL,
  Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  Column_priv set('Select','Insert','Update','References') COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
  PRIMARY KEY (Host,User,Db,Table_name,Column_name)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin   comment='Column privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS help_keyword (   help_keyword_id  int unsigned not null, name char(64) not null, primary key (help_keyword_id), unique index (name) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 comment='help keywords' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS time_zone_name (   Name char(64) NOT NULL, Time_zone_id int unsigned NOT NULL, PRIMARY KEY Name (Name) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3   comment='Time zone names' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS time_zone (   Time_zone_id int unsigned NOT NULL auto_increment, Use_leap_seconds enum('Y','N') COLLATE utf8mb3_general_ci DEFAULT 'N' NOT NULL, PRIMARY KEY TzId (Time_zone_id) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3   comment='Time zones' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS time_zone_transition (   Time_zone_id int unsigned NOT NULL, Transition_time bigint signed NOT NULL, Transition_type_id int unsigned NOT NULL, PRIMARY KEY TzIdTranTime (Time_zone_id, Transition_time) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3   comment='Time zone transitions' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS time_zone_transition_type (   Time_zone_id int unsigned NOT NULL, Transition_type_id int unsigned NOT NULL, Offset int signed DEFAULT 0 NOT NULL, Is_DST tinyint unsigned DEFAULT 0 NOT NULL, Abbreviation char(8) DEFAULT '' NOT NULL, PRIMARY KEY TzIdTrTId (Time_zone_id, Transition_type_id) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3   comment='Time zone transition types' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS time_zone_leap_second (   Transition_time bigint signed NOT NULL, Correction int signed NOT NULL, PRIMARY KEY TranTime (Transition_time) ) engine=INNODB STATS_PERSISTENT=0 CHARACTER SET utf8mb3   comment='Leap seconds information for time zones' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;



SET @cmd = "CREATE TABLE IF NOT EXISTS procs_priv
(
  Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL,
  Db char(64) binary DEFAULT '' NOT NULL,
  User char(32) binary DEFAULT '' NOT NULL,
  Routine_name char(64) COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
  Routine_type enum('FUNCTION','PROCEDURE') NOT NULL,
  Grantor varchar(288) DEFAULT '' NOT NULL,
  Proc_priv set('Execute','Alter Routine','Grant') COLLATE utf8mb3_general_ci DEFAULT '' NOT NULL,
  Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (Host,User,Db,Routine_name,Routine_type),
  KEY Grantor (Grantor)
) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin   comment='Procedure privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


-- bug#92988: Temporarily turn off sql_require_primary_key for the
-- next 2 tables as this is not necessary as they do not replicate.
SET @old_sql_require_primary_key = @@session.sql_require_primary_key;
SET @@session.sql_require_primary_key = 0;

-- Create general_log
CREATE TABLE IF NOT EXISTS general_log (event_time TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6), user_host MEDIUMTEXT NOT NULL, thread_id BIGINT UNSIGNED NOT NULL, server_id INTEGER UNSIGNED NOT NULL, command_type VARCHAR(64) NOT NULL, argument MEDIUMBLOB NOT NULL) engine=CSV CHARACTER SET utf8mb3 comment="General log";

-- Create slow_log
CREATE TABLE IF NOT EXISTS slow_log (start_time TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6), user_host MEDIUMTEXT NOT NULL, query_time TIME(6) NOT NULL, lock_time TIME(6) NOT NULL, rows_sent INTEGER NOT NULL, rows_examined INTEGER NOT NULL, db VARCHAR(512) NOT NULL, last_insert_id INTEGER NOT NULL, insert_id INTEGER NOT NULL, server_id INTEGER UNSIGNED NOT NULL, sql_text MEDIUMBLOB NOT NULL, thread_id BIGINT UNSIGNED NOT NULL) engine=CSV CHARACTER SET utf8mb3 comment="Slow log";

SET @@session.sql_require_primary_key = @old_sql_require_primary_key;

SET @cmd = "CREATE TABLE IF NOT EXISTS component ( component_id int unsigned NOT NULL AUTO_INCREMENT, component_group_id int unsigned NOT NULL, component_urn text NOT NULL, PRIMARY KEY (component_id)) engine=INNODB DEFAULT CHARSET=utf8mb3 COMMENT 'Components' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


SET @cmd="CREATE TABLE IF NOT EXISTS slave_relay_log_info (
  Number_of_lines INTEGER UNSIGNED NOT NULL COMMENT 'Number of lines in the file or rows in the table. Used to version table definitions.',
  Relay_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The name of the current relay log file.',
  Relay_log_pos BIGINT UNSIGNED COMMENT 'The relay log position of the last executed event.',
  Master_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The name of the master binary log file from which the events in the relay log file were read.',
  Master_log_pos BIGINT UNSIGNED COMMENT 'The master log position of the last executed event.',
  Sql_delay INTEGER COMMENT 'The number of seconds that the slave must lag behind the master.',
  Number_of_workers INTEGER UNSIGNED,
  Id INTEGER UNSIGNED COMMENT 'Internal Id that uniquely identifies this record.',
  Channel_name VARCHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'The channel on which the replica is connected to a source. Used in Multisource Replication',
  Privilege_checks_username VARCHAR(32) COLLATE utf8mb3_bin DEFAULT NULL COMMENT 'Username part of PRIVILEGE_CHECKS_USER.',
  Privilege_checks_hostname VARCHAR(255) CHARACTER SET ascii COLLATE ascii_general_ci DEFAULT NULL COMMENT 'Hostname part of PRIVILEGE_CHECKS_USER.',
  Require_row_format BOOLEAN NOT NULL COMMENT 'Indicates whether the channel shall only accept row based events.',
  Require_table_primary_key_check ENUM('STREAM','ON','OFF') NOT NULL DEFAULT 'STREAM' COMMENT 'Indicates what is the channel policy regarding tables having primary keys on create and alter table queries',
  Assign_gtids_to_anonymous_transactions_type ENUM('OFF', 'LOCAL', 'UUID')  NOT NULL DEFAULT 'OFF' COMMENT 'Indicates whether the channel will generate a new GTID for anonymous transactions. OFF means that anonymous transactions will remain anonymous. LOCAL means that anonymous transactions will be assigned a newly generated GTID based on server_uuid. UUID indicates that anonymous transactions will be assigned a newly generated GTID based on Assign_gtids_to_anonymous_transactions_value',
  Assign_gtids_to_anonymous_transactions_value TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin  COMMENT 'Indicates the UUID used while generating GTIDs for anonymous transactions',
  PRIMARY KEY(Channel_name)) DEFAULT CHARSET=utf8mb3 STATS_PERSISTENT=0 COMMENT 'Relay Log Information'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd= "CREATE TABLE IF NOT EXISTS slave_master_info (
  Number_of_lines INTEGER UNSIGNED NOT NULL COMMENT 'Number of lines in the file.',
  Master_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL COMMENT 'The name of the master binary log currently being read from the master.',
  Master_log_pos BIGINT UNSIGNED NOT NULL COMMENT 'The master log position of the last read event.',
  Host VARCHAR(255) CHARACTER SET ASCII COMMENT 'The host name of the source.',
  User_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The user name used to connect to the master.',
  User_password TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The password used to connect to the master.',
  Port INTEGER UNSIGNED NOT NULL COMMENT 'The network port used to connect to the master.',
  Connect_retry INTEGER UNSIGNED NOT NULL COMMENT 'The period (in seconds) that the slave will wait before trying to reconnect to the master.',
  Enabled_ssl BOOLEAN NOT NULL COMMENT 'Indicates whether the server supports SSL connections.',
  Ssl_ca TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The file used for the Certificate Authority (CA) certificate.',
  Ssl_capath TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The path to the Certificate Authority (CA) certificates.',
  Ssl_cert TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The name of the SSL certificate file.',
  Ssl_cipher TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The name of the cipher in use for the SSL connection.',
  Ssl_key TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The name of the SSL key file.',
  Ssl_verify_server_cert BOOLEAN NOT NULL COMMENT 'Whether to verify the server certificate.',
  Heartbeat FLOAT NOT NULL COMMENT '',
  Bind TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'Displays which interface is employed when connecting to the MySQL server',
  Ignored_server_ids TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The number of server IDs to be ignored, followed by the actual server IDs',
  Uuid TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The master server uuid.',
  Retry_count BIGINT UNSIGNED NOT NULL COMMENT 'Number of reconnect attempts, to the master, before giving up.',
  Ssl_crl TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The file used for the Certificate Revocation List (CRL)',
  Ssl_crlpath TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The path used for Certificate Revocation List (CRL) files',
  Enabled_auto_position BOOLEAN NOT NULL COMMENT 'Indicates whether GTIDs will be used to retrieve events from the master.',
  Channel_name VARCHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'The channel on which the replica is connected to a source. Used in Multisource Replication',
  Tls_version TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'Tls version',
  Public_key_path TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'The file containing public key of master server.',
  Get_public_key BOOLEAN NOT NULL COMMENT 'Preference to get public key from master.',
  Network_namespace TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin COMMENT 'Network namespace used for communication with the master server.',
  Master_compression_algorithm VARCHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL COMMENT 'Compression algorithm supported for data transfer between source and replica.',
  Master_zstd_compression_level INTEGER UNSIGNED NOT NULL COMMENT 'Compression level associated with zstd compression algorithm.',
  Tls_ciphersuites TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin DEFAULT NULL COMMENT 'Ciphersuites used for TLS 1.3 communication with the master server.',
  Source_connection_auto_failover BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'Indicates whether the channel connection failover is enabled.',
  Gtid_only BOOLEAN NOT NULL DEFAULT FALSE COMMENT 'Indicates if this channel only uses GTIDs and does not persist positions.',
  PRIMARY KEY(Channel_name)) DEFAULT CHARSET=utf8mb3 STATS_PERSISTENT=0 COMMENT 'Master Information'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd= "CREATE TABLE IF NOT EXISTS slave_worker_info (
  Id INTEGER UNSIGNED NOT NULL,
  Relay_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL,
  Relay_log_pos BIGINT UNSIGNED NOT NULL,
  Master_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL,
  Master_log_pos BIGINT UNSIGNED NOT NULL,
  Checkpoint_relay_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL,
  Checkpoint_relay_log_pos BIGINT UNSIGNED NOT NULL,
  Checkpoint_master_log_name TEXT CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL,
  Checkpoint_master_log_pos BIGINT UNSIGNED NOT NULL,
  Checkpoint_seqno INT UNSIGNED NOT NULL,
  Checkpoint_group_size INTEGER UNSIGNED NOT NULL,
  Checkpoint_group_bitmap BLOB NOT NULL,
  Channel_name VARCHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'The channel on which the replica is connected to a source. Used in Multisource Replication',
  PRIMARY KEY(Channel_name, Id)) DEFAULT CHARSET=utf8mb3 STATS_PERSISTENT=0 COMMENT 'Worker Information'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd= "CREATE TABLE IF NOT EXISTS gtid_executed (
    source_uuid CHAR(36) NOT NULL COMMENT 'uuid of the source where the transaction was originally executed.',
    interval_start BIGINT NOT NULL COMMENT 'First number of interval.',
    interval_end BIGINT NOT NULL COMMENT 'Last number of interval.',
    PRIMARY KEY(source_uuid, interval_start)) STATS_PERSISTENT=0";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

-- replication_asynchronous_connection_failover table
SET @cmd= "CREATE TABLE IF NOT EXISTS replication_asynchronous_connection_failover (
    Channel_name CHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'The replication channel name that connects source and replica.',
    Host CHAR(255) CHARACTER SET ASCII NOT NULL COMMENT 'The source hostname that the replica will attempt to switch over the replication connection to in case of a failure.',
    Port INTEGER UNSIGNED NOT NULL COMMENT 'The source port that the replica will attempt to switch over the replication connection to in case of a failure.',
    Network_namespace CHAR(64) COMMENT 'The source network namespace that the replica will attempt to switch over the replication connection to in case of a failure. If its value is empty, connections use the default (global) namespace.',
    Weight TINYINT UNSIGNED NOT NULL COMMENT 'The order in which the replica shall try to switch the connection over to when there are failures. Weight can be set to a number between 1 and 100, where 100 is the highest weight and 1 the lowest.',
    Managed_name CHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL DEFAULT '' COMMENT 'The name of the group which this server belongs to.',
    PRIMARY KEY(Channel_name, Host, Port, Network_namespace, Managed_name), KEY(Channel_name, Managed_name)) DEFAULT CHARSET=utf8mb3 STATS_PERSISTENT=0 COMMENT 'The source configuration details'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

-- replication_asynchronous_connection_failover_managed table
SET @cmd= "CREATE TABLE IF NOT EXISTS replication_asynchronous_connection_failover_managed (
    Channel_name CHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT 'The replication channel name that connects source and replica.',
    Managed_name CHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL DEFAULT '' COMMENT 'The name of the source which needs to be managed.',
    Managed_type CHAR(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL DEFAULT '' COMMENT 'Determines the managed type.',
    Configuration JSON DEFAULT NULL COMMENT 'The data to help manage group. For Managed_type = GroupReplication, Configuration value should contain {\"Primary_weight\": 80, \"Secondary_weight\": 60}, so that it assigns weight=80 to PRIMARY of the group, and weight=60 for rest of the members in mysql.replication_asynchronous_connection_failover table.',
    PRIMARY KEY(Channel_name, Managed_name)) DEFAULT CHARSET=utf8mb3 STATS_PERSISTENT=0 COMMENT 'The managed source configuration details'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

-- replication_group_member_actions
SET @cmd= "CREATE TABLE IF NOT EXISTS replication_group_member_actions (
    name CHAR(255) CHARACTER SET ASCII NOT NULL COMMENT 'The action name.',
    event CHAR(64) CHARACTER SET ASCII NOT NULL COMMENT 'The event that will trigger the action.',
    enabled BOOLEAN NOT NULL COMMENT 'Whether the action is enabled.',
    type CHAR(64) CHARACTER SET ASCII NOT NULL COMMENT 'The action type.',
    priority TINYINT UNSIGNED NOT NULL COMMENT 'The order on which the action will be run, value between 1 and 100, lower values first.',
    error_handling CHAR(64) CHARACTER SET ASCII NOT NULL COMMENT 'On errors during the action will be handled: IGNORE, CRITICAL.',
    PRIMARY KEY(name, event), KEY(event)) DEFAULT CHARSET=utf8mb4 STATS_PERSISTENT=0 COMMENT 'The member actions configuration.'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

-- replication_group_configuration_version
SET @cmd= "CREATE TABLE IF NOT EXISTS replication_group_configuration_version (
    name CHAR(255) CHARACTER SET ASCII NOT NULL COMMENT 'The configuration name.',
    version BIGINT UNSIGNED NOT NULL COMMENT 'The version of the configuration name.',
    PRIMARY KEY(name)) DEFAULT CHARSET=utf8mb4 STATS_PERSISTENT=0 COMMENT 'The group configuration version.'";

SET @str=IF(@have_innodb <> 0, CONCAT(@cmd, ' ENGINE= INNODB ROW_FORMAT=DYNAMIC TABLESPACE=mysql ENCRYPTION=\'', @is_mysql_encrypted,'\''), CONCAT(@cmd, ' ENGINE= MYISAM'));
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- Optimizer Cost Model configuration
-- (Note: Column definition for default_value needs to be updated when a
--        default value is changed).
--

-- Server cost constants

SET @cmd = "CREATE TABLE IF NOT EXISTS server_cost (
  cost_name   VARCHAR(64) NOT NULL,
  cost_value  FLOAT DEFAULT NULL,
  last_update TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  comment     VARCHAR(1024) DEFAULT NULL,
  default_value FLOAT GENERATED ALWAYS AS
    (CASE cost_name
       WHEN 'disk_temptable_create_cost' THEN 20.0
       WHEN 'disk_temptable_row_cost' THEN 0.5
       WHEN 'key_compare_cost' THEN 0.05
       WHEN 'memory_temptable_create_cost' THEN 1.0
       WHEN 'memory_temptable_row_cost' THEN 0.1
       WHEN 'row_evaluate_cost' THEN 0.1
       ELSE NULL
     END) VIRTUAL,
  PRIMARY KEY (cost_name)
) ENGINE=InnoDB CHARACTER SET=utf8mb3 COLLATE=utf8mb3_general_ci STATS_PERSISTENT=0 ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


INSERT IGNORE INTO server_cost(cost_name) VALUES
  ("row_evaluate_cost"), ("key_compare_cost"),
  ("memory_temptable_create_cost"), ("memory_temptable_row_cost"),
  ("disk_temptable_create_cost"), ("disk_temptable_row_cost");

-- Engine cost constants

SET @cmd = "CREATE TABLE IF NOT EXISTS engine_cost (
  engine_name VARCHAR(64) NOT NULL,
  device_type INTEGER NOT NULL,
  cost_name   VARCHAR(64) NOT NULL,
  cost_value  FLOAT DEFAULT NULL,
  last_update TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  comment     VARCHAR(1024) DEFAULT NULL,
  default_value FLOAT GENERATED ALWAYS AS
    (CASE cost_name
       WHEN 'io_block_read_cost' THEN 1.0
       WHEN 'memory_block_read_cost' THEN 0.25
       ELSE NULL
     END) VIRTUAL,
  PRIMARY KEY (cost_name, engine_name, device_type)
) ENGINE=InnoDB CHARACTER SET=utf8mb3 COLLATE=utf8mb3_general_ci STATS_PERSISTENT=0 ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


INSERT IGNORE INTO engine_cost(engine_name, device_type, cost_name) VALUES
  ("default", 0, "memory_block_read_cost"),
  ("default", 0, "io_block_read_cost");


<<<<<<< HEAD
SET @cmd = "CREATE TABLE IF NOT EXISTS proxies_priv (Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, User char(32) binary DEFAULT '' NOT NULL, Proxied_host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, Proxied_user char(32) binary DEFAULT '' NOT NULL, With_grant BOOL DEFAULT 0 NOT NULL, Grantor varchar(288) DEFAULT '' NOT NULL, Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, PRIMARY KEY Host (Host,User,Proxied_host,Proxied_user), KEY Grantor (Grantor) ) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8 COLLATE utf8_bin comment='User proxy privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
||||||| merged common ancestors
<<<<<<<<< Temporary merge branch 1
set @have_old_pfs= (select count(*) from information_schema.schemata where schema_name='performance_schema');

SET @cmd="SET @broken_tables = (select count(*) from information_schema.tables"
  " where engine != \'PERFORMANCE_SCHEMA\' and table_schema=\'performance_schema\')";

-- Work around for bug#49542
SET @str = IF(@have_old_pfs = 1, @cmd, 'SET @broken_tables = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd="SET @broken_views = (select count(*) from information_schema.views"
  " where table_schema='performance_schema')";

-- Work around for bug#49542
SET @str = IF(@have_old_pfs = 1, @cmd, 'SET @broken_views = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @broken_routines = (select count(*) from mysql.proc where db='performance_schema');

SET @broken_events = (select count(*) from mysql.event where db='performance_schema');

SET @broken_pfs= (select @broken_tables + @broken_views + @broken_routines + @broken_events);

--
-- The performance schema database.
-- Only drop and create the database if this is safe (no broken_pfs).
-- This database is created, even in --without-perfschema builds,
-- so that the database name is always reserved by the MySQL implementation.
--

SET @cmd= "DROP DATABASE IF EXISTS performance_schema";

SET @str = IF(@broken_pfs = 0, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd= "CREATE DATABASE performance_schema character set utf8";

SET @str = IF(@broken_pfs = 0, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- From this point, only create the performance schema tables
-- if the server is built with performance schema
--

set @have_pfs= (select count(engine) from information_schema.engines where engine='PERFORMANCE_SCHEMA');

--
-- TABLE COND_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.cond_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_waits_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_waits_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_waits_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_instance("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.file_instances("
  "FILE_NAME VARCHAR(512) not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "OPEN_COUNT INTEGER unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_SUMMARY_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.file_summary_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.file_summary_by_instance("
  "FILE_NAME VARCHAR(512) not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE SOCKET_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.socket_instances("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "THREAD_ID BIGINT unsigned,"
  "SOCKET_ID INTEGER not null,"
  "IP VARCHAR(64) not null,"
  "PORT INTEGER not null,"
  "STATE ENUM('IDLE','ACTIVE') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SOCKET_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.socket_summary_by_instance("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT unsigned not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SOCKET_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.socket_summary_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT unsigned not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE HOST_CACHE
--

SET @cmd="CREATE TABLE performance_schema.host_cache("
  "IP VARCHAR(64) not null,"
  "HOST VARCHAR(255) collate utf8_bin,"
  "HOST_VALIDATED ENUM ('YES', 'NO') not null,"
  "SUM_CONNECT_ERRORS BIGINT not null,"
  "COUNT_HOST_BLOCKED_ERRORS BIGINT not null,"
  "COUNT_NAMEINFO_TRANSIENT_ERRORS BIGINT not null,"
  "COUNT_NAMEINFO_PERMANENT_ERRORS BIGINT not null,"
  "COUNT_FORMAT_ERRORS BIGINT not null,"
  "COUNT_ADDRINFO_TRANSIENT_ERRORS BIGINT not null,"
  "COUNT_ADDRINFO_PERMANENT_ERRORS BIGINT not null,"
  "COUNT_FCRDNS_ERRORS BIGINT not null,"
  "COUNT_HOST_ACL_ERRORS BIGINT not null,"
  "COUNT_NO_AUTH_PLUGIN_ERRORS BIGINT not null,"
  "COUNT_AUTH_PLUGIN_ERRORS BIGINT not null,"
  "COUNT_HANDSHAKE_ERRORS BIGINT not null,"
  "COUNT_PROXY_USER_ERRORS BIGINT not null,"
  "COUNT_PROXY_USER_ACL_ERRORS BIGINT not null,"
  "COUNT_AUTHENTICATION_ERRORS BIGINT not null,"
  "COUNT_SSL_ERRORS BIGINT not null,"
  "COUNT_MAX_USER_CONNECTIONS_ERRORS BIGINT not null,"
  "COUNT_MAX_USER_CONNECTIONS_PER_HOUR_ERRORS BIGINT not null,"
  "COUNT_DEFAULT_DATABASE_ERRORS BIGINT not null,"
  "COUNT_INIT_CONNECT_ERRORS BIGINT not null,"
  "COUNT_LOCAL_ERRORS BIGINT not null,"
  "COUNT_UNKNOWN_ERRORS BIGINT not null,"
  "FIRST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "LAST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "FIRST_ERROR_SEEN TIMESTAMP(0) null default 0,"
  "LAST_ERROR_SEEN TIMESTAMP(0) null default 0"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MUTEX_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.mutex_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "LOCKED_BY_THREAD_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE OBJECTS_SUMMARY_GLOBAL_BY_TYPE
--

SET @cmd="CREATE TABLE performance_schema.objects_summary_global_by_type("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PERFORMANCE_TIMERS
--

SET @cmd="CREATE TABLE performance_schema.performance_timers("
  "TIMER_NAME ENUM ('CYCLE', 'NANOSECOND', 'MICROSECOND', 'MILLISECOND', 'TICK') not null,"
  "TIMER_FREQUENCY BIGINT,"
  "TIMER_RESOLUTION BIGINT,"
  "TIMER_OVERHEAD BIGINT"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE RWLOCK_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.rwlock_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "WRITE_LOCKED_BY_THREAD_ID BIGINT unsigned,"
  "READ_LOCKED_BY_COUNT INTEGER unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_ACTORS
--

SET @cmd="CREATE TABLE performance_schema.setup_actors("
  "HOST CHAR(60) collate utf8_bin default '%' not null,"
  "USER CHAR(32) collate utf8_bin default '%' not null,"
  "ROLE CHAR(16) collate utf8_bin default '%' not null,"
  "ENABLED ENUM ('YES', 'NO') not null default 'YES',"
  "HISTORY ENUM ('YES', 'NO') not null default 'YES'"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_CONSUMERS
--

SET @cmd="CREATE TABLE performance_schema.setup_consumers("
  "NAME VARCHAR(64) not null,"
  "ENABLED ENUM ('YES', 'NO') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_INSTRUMENTS
--

SET @cmd="CREATE TABLE performance_schema.setup_instruments("
  "NAME VARCHAR(128) not null,"
  "ENABLED ENUM ('YES', 'NO') not null,"
  "TIMED ENUM ('YES', 'NO') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_OBJECTS
--

SET @cmd="CREATE TABLE performance_schema.setup_objects("
  "OBJECT_TYPE ENUM ('EVENT', 'FUNCTION', 'PROCEDURE', 'TABLE', 'TRIGGER') not null default 'TABLE',"
  "OBJECT_SCHEMA VARCHAR(64) default '%',"
  "OBJECT_NAME VARCHAR(64) not null default '%',"
  "ENABLED ENUM ('YES', 'NO') not null default 'YES',"
  "TIMED ENUM ('YES', 'NO') not null default 'YES'"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_TIMERS
--

SET @cmd="CREATE TABLE performance_schema.setup_timers("
  "NAME VARCHAR(64) not null,"
  "TIMER_NAME ENUM ('CYCLE', 'NANOSECOND', 'MICROSECOND', 'MILLISECOND', 'TICK') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_IO_WAITS_SUMMARY_BY_INDEX_USAGE
--

SET @cmd="CREATE TABLE performance_schema.table_io_waits_summary_by_index_usage("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "INDEX_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_FETCH BIGINT unsigned not null,"
  "SUM_TIMER_FETCH BIGINT unsigned not null,"
  "MIN_TIMER_FETCH BIGINT unsigned not null,"
  "AVG_TIMER_FETCH BIGINT unsigned not null,"
  "MAX_TIMER_FETCH BIGINT unsigned not null,"
  "COUNT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_INSERT BIGINT unsigned not null,"
  "COUNT_UPDATE BIGINT unsigned not null,"
  "SUM_TIMER_UPDATE BIGINT unsigned not null,"
  "MIN_TIMER_UPDATE BIGINT unsigned not null,"
  "AVG_TIMER_UPDATE BIGINT unsigned not null,"
  "MAX_TIMER_UPDATE BIGINT unsigned not null,"
  "COUNT_DELETE BIGINT unsigned not null,"
  "SUM_TIMER_DELETE BIGINT unsigned not null,"
  "MIN_TIMER_DELETE BIGINT unsigned not null,"
  "AVG_TIMER_DELETE BIGINT unsigned not null,"
  "MAX_TIMER_DELETE BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_IO_WAITS_SUMMARY_BY_TABLE
--

SET @cmd="CREATE TABLE performance_schema.table_io_waits_summary_by_table("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_FETCH BIGINT unsigned not null,"
  "SUM_TIMER_FETCH BIGINT unsigned not null,"
  "MIN_TIMER_FETCH BIGINT unsigned not null,"
  "AVG_TIMER_FETCH BIGINT unsigned not null,"
  "MAX_TIMER_FETCH BIGINT unsigned not null,"
  "COUNT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_INSERT BIGINT unsigned not null,"
  "COUNT_UPDATE BIGINT unsigned not null,"
  "SUM_TIMER_UPDATE BIGINT unsigned not null,"
  "MIN_TIMER_UPDATE BIGINT unsigned not null,"
  "AVG_TIMER_UPDATE BIGINT unsigned not null,"
  "MAX_TIMER_UPDATE BIGINT unsigned not null,"
  "COUNT_DELETE BIGINT unsigned not null,"
  "SUM_TIMER_DELETE BIGINT unsigned not null,"
  "MIN_TIMER_DELETE BIGINT unsigned not null,"
  "AVG_TIMER_DELETE BIGINT unsigned not null,"
  "MAX_TIMER_DELETE BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_LOCK_WAITS_SUMMARY_BY_TABLE
--

SET @cmd="CREATE TABLE performance_schema.table_lock_waits_summary_by_table("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_READ_NORMAL BIGINT unsigned not null,"
  "SUM_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "MIN_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "AVG_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "MAX_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "COUNT_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "SUM_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "MIN_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "AVG_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "MAX_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "COUNT_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "SUM_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "MIN_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "AVG_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "MAX_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "COUNT_READ_NO_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "COUNT_READ_EXTERNAL BIGINT unsigned not null,"
  "SUM_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "MIN_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "AVG_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "MAX_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "COUNT_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "COUNT_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "COUNT_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "COUNT_WRITE_NORMAL BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "COUNT_WRITE_EXTERNAL BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_EXTERNAL BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE THREADS
--

SET @cmd="CREATE TABLE performance_schema.threads("
  "THREAD_ID BIGINT unsigned not null,"
  "NAME VARCHAR(128) not null,"
  "TYPE VARCHAR(10) not null,"
  "PROCESSLIST_ID BIGINT unsigned,"
  "PROCESSLIST_USER VARCHAR(32),"
  "PROCESSLIST_HOST VARCHAR(60),"
  "PROCESSLIST_DB VARCHAR(64),"
  "PROCESSLIST_COMMAND VARCHAR(16),"
  "PROCESSLIST_TIME BIGINT,"
  "PROCESSLIST_STATE VARCHAR(64),"
  "PROCESSLIST_INFO LONGTEXT,"
  "PARENT_THREAD_ID BIGINT unsigned,"
  "ROLE VARCHAR(64),"
  "INSTRUMENTED ENUM ('YES', 'NO') not null,"
  "HISTORY ENUM ('YES', 'NO') not null,"
  "CONNECTION_TYPE VARCHAR(16),"
  "THREAD_OS_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PROCESSLIST
--

SET @cmd="CREATE TABLE performance_schema.processlist("
  "ID BIGINT unsigned not null,"
  "USER VARCHAR(32),"
  "HOST VARCHAR(66),"
  "DB VARCHAR(64),"
  "COMMAND VARCHAR(16),"
  "TIME BIGINT,"
  "STATE VARCHAR(64),"
  "INFO LONGTEXT,"
  "TIME_MS BIGINT(20) unsigned not null,"
  "ROWS_SENT BIGINT(20) unsigned not null,"
  "ROWS_EXAMINED BIGINT(20) unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_stages_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_stages_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_stages_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_statements_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_statements_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_statements_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE HOSTS
--

SET @cmd="CREATE TABLE performance_schema.hosts("
  "HOST CHAR(60) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE USERS
--

SET @cmd="CREATE TABLE performance_schema.users("
  "USER CHAR(32) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE ACCOUNTS
--

SET @cmd="CREATE TABLE performance_schema.accounts("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_DIGEST
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_digest("
  "SCHEMA_NAME VARCHAR(64),"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "FIRST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "LAST_SEEN TIMESTAMP(0) NOT NULL default 0"
  ")ENGINE=PERFORMANCE_SCHEMA;";


SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_PROGRAM
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_program("
  "OBJECT_TYPE enum('EVENT', 'FUNCTION', 'PROCEDURE', 'TABLE', 'TRIGGER'),"
  "OBJECT_SCHEMA varchar(64) NOT NULL,"
  "OBJECT_NAME varchar(64) NOT NULL,"
  "COUNT_STAR bigint(20) unsigned NOT NULL,"
  "SUM_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "MIN_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "AVG_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "MAX_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "COUNT_STATEMENTS bigint(20) unsigned NOT NULL,"
  "SUM_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "MIN_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "AVG_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "MAX_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "SUM_LOCK_TIME bigint(20) unsigned NOT NULL,"
  "SUM_ERRORS bigint(20) unsigned NOT NULL,"
  "SUM_WARNINGS bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_AFFECTED bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_SENT bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_EXAMINED bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_DISK_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_RANGE_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE_CHECK bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_SORT_MERGE_PASSES bigint(20) unsigned NOT NULL,"
  "SUM_SORT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SORT_ROWS bigint(20) unsigned NOT NULL,"
  "SUM_SORT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_NO_INDEX_USED bigint(20) unsigned NOT NULL,"
  "SUM_NO_GOOD_INDEX_USED bigint(20) unsigned NOT NULL"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PREPARED_STATEMENT_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.prepared_statements_instances("
  "OBJECT_INSTANCE_BEGIN bigint(20) unsigned NOT NULL,"
  "STATEMENT_ID bigint(20) unsigned NOT NULL,"
  "STATEMENT_NAME varchar(64) default NULL,"
  "SQL_TEXT longtext NOT NULL,"
  "OWNER_THREAD_ID bigint(20) unsigned NOT NULL,"
  "OWNER_EVENT_ID bigint(20) unsigned NOT NULL,"
  "OWNER_OBJECT_TYPE enum('EVENT','FUNCTION','PROCEDURE','TABLE','TRIGGER') DEFAULT NULL,"
  "OWNER_OBJECT_SCHEMA varchar(64) DEFAULT NULL,"
  "OWNER_OBJECT_NAME varchar(64) DEFAULT NULL,"
  "TIMER_PREPARE bigint(20) unsigned NOT NULL,"
  "COUNT_REPREPARE bigint(20) unsigned NOT NULL,"
  "COUNT_EXECUTE bigint(20) unsigned NOT NULL,"
  "SUM_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "MIN_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "AVG_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "MAX_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "SUM_LOCK_TIME bigint(20) unsigned NOT NULL,"
  "SUM_ERRORS bigint(20) unsigned NOT NULL,"
  "SUM_WARNINGS bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_AFFECTED bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_SENT bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_EXAMINED bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_DISK_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_RANGE_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE_CHECK bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_SORT_MERGE_PASSES bigint(20) unsigned NOT NULL,"
  "SUM_SORT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SORT_ROWS bigint(20) unsigned NOT NULL,"
  "SUM_SORT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_NO_INDEX_USED bigint(20) unsigned NOT NULL,"
  "SUM_NO_GOOD_INDEX_USED bigint(20) unsigned NOT NULL"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_connection_configuration
--

SET @cmd="CREATE TABLE performance_schema.replication_connection_configuration("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "HOST CHAR(60) collate utf8_bin not null,"
  "PORT INTEGER not null,"
  "USER CHAR(32) collate utf8_bin not null,"
  "NETWORK_INTERFACE CHAR(60) collate utf8_bin not null,"
  "AUTO_POSITION ENUM('1','0') not null,"
  "SSL_ALLOWED ENUM('YES','NO','IGNORED') not null,"
  "SSL_CA_FILE VARCHAR(512) not null,"
  "SSL_CA_PATH VARCHAR(512) not null,"
  "SSL_CERTIFICATE VARCHAR(512) not null,"
  "SSL_CIPHER VARCHAR(512) not null,"
  "SSL_KEY VARCHAR(512) not null,"
  "SSL_VERIFY_SERVER_CERTIFICATE ENUM('YES','NO') not null,"
  "SSL_CRL_FILE VARCHAR(255) not null,"
  "SSL_CRL_PATH VARCHAR(255) not null,"
  "CONNECTION_RETRY_INTERVAL INTEGER not null,"
  "CONNECTION_RETRY_COUNT BIGINT unsigned not null,"
  "HEARTBEAT_INTERVAL DOUBLE(10,3) unsigned not null COMMENT 'Number of seconds after which a heartbeat will be sent .',"
  "TLS_VERSION VARCHAR(255) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE replication_group_member_stats
--

SET @cmd="CREATE TABLE performance_schema.replication_group_member_stats("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "VIEW_ID CHAR(60) collate utf8_bin not null,"
  "MEMBER_ID CHAR(36) collate utf8_bin not null,"
  "COUNT_TRANSACTIONS_IN_QUEUE BIGINT unsigned not null,"
  "COUNT_TRANSACTIONS_CHECKED BIGINT unsigned not null,"
  "COUNT_CONFLICTS_DETECTED BIGINT unsigned not null,"
  "COUNT_TRANSACTIONS_ROWS_VALIDATING BIGINT unsigned not null,"
  "TRANSACTIONS_COMMITTED_ALL_MEMBERS LONGTEXT not null,"
  "LAST_CONFLICT_FREE_TRANSACTION TEXT not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE replication_group_members
--

SET @cmd="CREATE TABLE performance_schema.replication_group_members("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "MEMBER_ID CHAR(36) collate utf8_bin not null,"
  "MEMBER_HOST CHAR(60) collate utf8_bin not null,"
  "MEMBER_PORT INTEGER,"
  "MEMBER_STATE CHAR(64) collate utf8_bin not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

 SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
 PREPARE stmt FROM @str;
 EXECUTE stmt;
 DROP PREPARE stmt;

--
-- TABLE replication_connection_status
--

SET @cmd="CREATE TABLE performance_schema.replication_connection_status("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "GROUP_NAME CHAR(36) collate utf8_bin not null,"
  "SOURCE_UUID CHAR(36) collate utf8_bin not null,"
  "THREAD_ID BIGINT unsigned,"
  "SERVICE_STATE ENUM('ON','OFF','CONNECTING') not null,"
  "COUNT_RECEIVED_HEARTBEATS bigint unsigned NOT NULL DEFAULT 0,"
  "LAST_HEARTBEAT_TIMESTAMP TIMESTAMP(0) not null COMMENT 'Shows when the most recent heartbeat signal was received.',"
  "RECEIVED_TRANSACTION_SET LONGTEXT not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_configuration
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_configuration("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "DESIRED_DELAY INTEGER not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "REMAINING_DELAY INTEGER unsigned,"
  "COUNT_TRANSACTIONS_RETRIES BIGINT unsigned not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status_by_coordinator
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status_by_coordinator("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "THREAD_ID BIGINT UNSIGNED,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status_by_worker
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status_by_worker("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "WORKER_ID BIGINT UNSIGNED not null,"
  "THREAD_ID BIGINT UNSIGNED,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "LAST_SEEN_TRANSACTION CHAR(57) not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_CONNECT_ATTRS
--

SET @cmd="CREATE TABLE performance_schema.session_connect_attrs("
  "PROCESSLIST_ID INT NOT NULL,"
  "ATTR_NAME VARCHAR(32) NOT NULL,"
  "ATTR_VALUE VARCHAR(1024),"
  "ORDINAL_POSITION INT"
  ")ENGINE=PERFORMANCE_SCHEMA CHARACTER SET utf8 COLLATE utf8_bin;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_ACCOUNT_CONNECT_ATTRS
--

SET @cmd="CREATE TABLE performance_schema.session_account_connect_attrs "
         " LIKE performance_schema.session_connect_attrs;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_HANDLES
--

SET @cmd="CREATE TABLE performance_schema.table_handles("
  "OBJECT_TYPE VARCHAR(64) not null,"
  "OBJECT_SCHEMA VARCHAR(64) not null,"
  "OBJECT_NAME VARCHAR(64) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "OWNER_THREAD_ID BIGINT unsigned,"
  "OWNER_EVENT_ID BIGINT unsigned,"
  "INTERNAL_LOCK VARCHAR(64),"
  "EXTERNAL_LOCK VARCHAR(64)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE METADATA_LOCKS
--

SET @cmd="CREATE TABLE performance_schema.metadata_locks("
  "OBJECT_TYPE VARCHAR(64) not null,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "LOCK_TYPE VARCHAR(32) not null,"
  "LOCK_DURATION VARCHAR(32) not null,"
  "LOCK_STATUS VARCHAR(32) not null,"
  "SOURCE VARCHAR(64),"
  "OWNER_THREAD_ID BIGINT unsigned,"
  "OWNER_EVENT_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE USER_VARIABLES_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.user_variables_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE LONGBLOB"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE VARIABLES_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.variables_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE GLOBAL_VARIABLES
--

SET @cmd="CREATE TABLE performance_schema.global_variables("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_VARIABLES
--

SET @cmd="CREATE TABLE performance_schema.session_variables("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.status_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_USER
--

SET @cmd="CREATE TABLE performance_schema.status_by_user("
  "USER CHAR(32) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_HOST
--

SET @cmd="CREATE TABLE performance_schema.status_by_host("
  "HOST CHAR(60) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_ACCOUNT
--

SET @cmd="CREATE TABLE performance_schema.status_by_account("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE GLOBAL_STATUS
--

SET @cmd="CREATE TABLE performance_schema.global_status("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_STATUS
--

SET @cmd="CREATE TABLE performance_schema.session_status("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
||||||||| merged common ancestors
set @have_old_pfs= (select count(*) from information_schema.schemata where schema_name='performance_schema');

SET @cmd="SET @broken_tables = (select count(*) from information_schema.tables"
  " where engine != \'PERFORMANCE_SCHEMA\' and table_schema=\'performance_schema\')";

-- Work around for bug#49542
SET @str = IF(@have_old_pfs = 1, @cmd, 'SET @broken_tables = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd="SET @broken_views = (select count(*) from information_schema.views"
  " where table_schema='performance_schema')";

-- Work around for bug#49542
SET @str = IF(@have_old_pfs = 1, @cmd, 'SET @broken_views = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @broken_routines = (select count(*) from mysql.proc where db='performance_schema');

SET @broken_events = (select count(*) from mysql.event where db='performance_schema');

SET @broken_pfs= (select @broken_tables + @broken_views + @broken_routines + @broken_events);

--
-- The performance schema database.
-- Only drop and create the database if this is safe (no broken_pfs).
-- This database is created, even in --without-perfschema builds,
-- so that the database name is always reserved by the MySQL implementation.
--

SET @cmd= "DROP DATABASE IF EXISTS performance_schema";

SET @str = IF(@broken_pfs = 0, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

SET @cmd= "CREATE DATABASE performance_schema character set utf8";

SET @str = IF(@broken_pfs = 0, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- From this point, only create the performance schema tables
-- if the server is built with performance schema
--

set @have_pfs= (select count(engine) from information_schema.engines where engine='PERFORMANCE_SCHEMA');

--
-- TABLE COND_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.cond_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_waits_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_waits_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_waits_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "SPINS INTEGER unsigned,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(512),"
  "INDEX_NAME VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "OPERATION VARCHAR(32) not null,"
  "NUMBER_OF_BYTES BIGINT,"
  "FLAGS INTEGER unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_instance("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_WAITS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_waits_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.file_instances("
  "FILE_NAME VARCHAR(512) not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "OPEN_COUNT INTEGER unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_SUMMARY_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.file_summary_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE FILE_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.file_summary_by_instance("
  "FILE_NAME VARCHAR(512) not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE SOCKET_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.socket_instances("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "THREAD_ID BIGINT unsigned,"
  "SOCKET_ID INTEGER not null,"
  "IP VARCHAR(64) not null,"
  "PORT INTEGER not null,"
  "STATE ENUM('IDLE','ACTIVE') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SOCKET_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.socket_summary_by_instance("
  "EVENT_NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT unsigned not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SOCKET_SUMMARY_BY_INSTANCE
--

SET @cmd="CREATE TABLE performance_schema.socket_summary_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_WRITE BIGINT unsigned not null,"
  "COUNT_MISC BIGINT unsigned not null,"
  "SUM_TIMER_MISC BIGINT unsigned not null,"
  "MIN_TIMER_MISC BIGINT unsigned not null,"
  "AVG_TIMER_MISC BIGINT unsigned not null,"
  "MAX_TIMER_MISC BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE HOST_CACHE
--

SET @cmd="CREATE TABLE performance_schema.host_cache("
  "IP VARCHAR(64) not null,"
  "HOST VARCHAR(255) collate utf8_bin,"
  "HOST_VALIDATED ENUM ('YES', 'NO') not null,"
  "SUM_CONNECT_ERRORS BIGINT not null,"
  "COUNT_HOST_BLOCKED_ERRORS BIGINT not null,"
  "COUNT_NAMEINFO_TRANSIENT_ERRORS BIGINT not null,"
  "COUNT_NAMEINFO_PERMANENT_ERRORS BIGINT not null,"
  "COUNT_FORMAT_ERRORS BIGINT not null,"
  "COUNT_ADDRINFO_TRANSIENT_ERRORS BIGINT not null,"
  "COUNT_ADDRINFO_PERMANENT_ERRORS BIGINT not null,"
  "COUNT_FCRDNS_ERRORS BIGINT not null,"
  "COUNT_HOST_ACL_ERRORS BIGINT not null,"
  "COUNT_NO_AUTH_PLUGIN_ERRORS BIGINT not null,"
  "COUNT_AUTH_PLUGIN_ERRORS BIGINT not null,"
  "COUNT_HANDSHAKE_ERRORS BIGINT not null,"
  "COUNT_PROXY_USER_ERRORS BIGINT not null,"
  "COUNT_PROXY_USER_ACL_ERRORS BIGINT not null,"
  "COUNT_AUTHENTICATION_ERRORS BIGINT not null,"
  "COUNT_SSL_ERRORS BIGINT not null,"
  "COUNT_MAX_USER_CONNECTIONS_ERRORS BIGINT not null,"
  "COUNT_MAX_USER_CONNECTIONS_PER_HOUR_ERRORS BIGINT not null,"
  "COUNT_DEFAULT_DATABASE_ERRORS BIGINT not null,"
  "COUNT_INIT_CONNECT_ERRORS BIGINT not null,"
  "COUNT_LOCAL_ERRORS BIGINT not null,"
  "COUNT_UNKNOWN_ERRORS BIGINT not null,"
  "FIRST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "LAST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "FIRST_ERROR_SEEN TIMESTAMP(0) null default 0,"
  "LAST_ERROR_SEEN TIMESTAMP(0) null default 0"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MUTEX_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.mutex_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "LOCKED_BY_THREAD_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE OBJECTS_SUMMARY_GLOBAL_BY_TYPE
--

SET @cmd="CREATE TABLE performance_schema.objects_summary_global_by_type("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PERFORMANCE_TIMERS
--

SET @cmd="CREATE TABLE performance_schema.performance_timers("
  "TIMER_NAME ENUM ('CYCLE', 'NANOSECOND', 'MICROSECOND', 'MILLISECOND', 'TICK') not null,"
  "TIMER_FREQUENCY BIGINT,"
  "TIMER_RESOLUTION BIGINT,"
  "TIMER_OVERHEAD BIGINT"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE RWLOCK_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.rwlock_instances("
  "NAME VARCHAR(128) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "WRITE_LOCKED_BY_THREAD_ID BIGINT unsigned,"
  "READ_LOCKED_BY_COUNT INTEGER unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_ACTORS
--

SET @cmd="CREATE TABLE performance_schema.setup_actors("
  "HOST CHAR(60) collate utf8_bin default '%' not null,"
  "USER CHAR(32) collate utf8_bin default '%' not null,"
  "ROLE CHAR(16) collate utf8_bin default '%' not null,"
  "ENABLED ENUM ('YES', 'NO') not null default 'YES',"
  "HISTORY ENUM ('YES', 'NO') not null default 'YES'"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_CONSUMERS
--

SET @cmd="CREATE TABLE performance_schema.setup_consumers("
  "NAME VARCHAR(64) not null,"
  "ENABLED ENUM ('YES', 'NO') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_INSTRUMENTS
--

SET @cmd="CREATE TABLE performance_schema.setup_instruments("
  "NAME VARCHAR(128) not null,"
  "ENABLED ENUM ('YES', 'NO') not null,"
  "TIMED ENUM ('YES', 'NO') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_OBJECTS
--

SET @cmd="CREATE TABLE performance_schema.setup_objects("
  "OBJECT_TYPE ENUM ('EVENT', 'FUNCTION', 'PROCEDURE', 'TABLE', 'TRIGGER') not null default 'TABLE',"
  "OBJECT_SCHEMA VARCHAR(64) default '%',"
  "OBJECT_NAME VARCHAR(64) not null default '%',"
  "ENABLED ENUM ('YES', 'NO') not null default 'YES',"
  "TIMED ENUM ('YES', 'NO') not null default 'YES'"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SETUP_TIMERS
--

SET @cmd="CREATE TABLE performance_schema.setup_timers("
  "NAME VARCHAR(64) not null,"
  "TIMER_NAME ENUM ('CYCLE', 'NANOSECOND', 'MICROSECOND', 'MILLISECOND', 'TICK') not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_IO_WAITS_SUMMARY_BY_INDEX_USAGE
--

SET @cmd="CREATE TABLE performance_schema.table_io_waits_summary_by_index_usage("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "INDEX_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_FETCH BIGINT unsigned not null,"
  "SUM_TIMER_FETCH BIGINT unsigned not null,"
  "MIN_TIMER_FETCH BIGINT unsigned not null,"
  "AVG_TIMER_FETCH BIGINT unsigned not null,"
  "MAX_TIMER_FETCH BIGINT unsigned not null,"
  "COUNT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_INSERT BIGINT unsigned not null,"
  "COUNT_UPDATE BIGINT unsigned not null,"
  "SUM_TIMER_UPDATE BIGINT unsigned not null,"
  "MIN_TIMER_UPDATE BIGINT unsigned not null,"
  "AVG_TIMER_UPDATE BIGINT unsigned not null,"
  "MAX_TIMER_UPDATE BIGINT unsigned not null,"
  "COUNT_DELETE BIGINT unsigned not null,"
  "SUM_TIMER_DELETE BIGINT unsigned not null,"
  "MIN_TIMER_DELETE BIGINT unsigned not null,"
  "AVG_TIMER_DELETE BIGINT unsigned not null,"
  "MAX_TIMER_DELETE BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_IO_WAITS_SUMMARY_BY_TABLE
--

SET @cmd="CREATE TABLE performance_schema.table_io_waits_summary_by_table("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_FETCH BIGINT unsigned not null,"
  "SUM_TIMER_FETCH BIGINT unsigned not null,"
  "MIN_TIMER_FETCH BIGINT unsigned not null,"
  "AVG_TIMER_FETCH BIGINT unsigned not null,"
  "MAX_TIMER_FETCH BIGINT unsigned not null,"
  "COUNT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_INSERT BIGINT unsigned not null,"
  "COUNT_UPDATE BIGINT unsigned not null,"
  "SUM_TIMER_UPDATE BIGINT unsigned not null,"
  "MIN_TIMER_UPDATE BIGINT unsigned not null,"
  "AVG_TIMER_UPDATE BIGINT unsigned not null,"
  "MAX_TIMER_UPDATE BIGINT unsigned not null,"
  "COUNT_DELETE BIGINT unsigned not null,"
  "SUM_TIMER_DELETE BIGINT unsigned not null,"
  "MIN_TIMER_DELETE BIGINT unsigned not null,"
  "AVG_TIMER_DELETE BIGINT unsigned not null,"
  "MAX_TIMER_DELETE BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_LOCK_WAITS_SUMMARY_BY_TABLE
--

SET @cmd="CREATE TABLE performance_schema.table_lock_waits_summary_by_table("
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ BIGINT unsigned not null,"
  "SUM_TIMER_READ BIGINT unsigned not null,"
  "MIN_TIMER_READ BIGINT unsigned not null,"
  "AVG_TIMER_READ BIGINT unsigned not null,"
  "MAX_TIMER_READ BIGINT unsigned not null,"
  "COUNT_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE BIGINT unsigned not null,"
  "COUNT_READ_NORMAL BIGINT unsigned not null,"
  "SUM_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "MIN_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "AVG_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "MAX_TIMER_READ_NORMAL BIGINT unsigned not null,"
  "COUNT_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "SUM_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "MIN_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "AVG_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "MAX_TIMER_READ_WITH_SHARED_LOCKS BIGINT unsigned not null,"
  "COUNT_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "SUM_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "MIN_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "AVG_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "MAX_TIMER_READ_HIGH_PRIORITY BIGINT unsigned not null,"
  "COUNT_READ_NO_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_READ_NO_INSERT BIGINT unsigned not null,"
  "COUNT_READ_EXTERNAL BIGINT unsigned not null,"
  "SUM_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "MIN_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "AVG_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "MAX_TIMER_READ_EXTERNAL BIGINT unsigned not null,"
  "COUNT_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_ALLOW_WRITE BIGINT unsigned not null,"
  "COUNT_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_CONCURRENT_INSERT BIGINT unsigned not null,"
  "COUNT_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_LOW_PRIORITY BIGINT unsigned not null,"
  "COUNT_WRITE_NORMAL BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_NORMAL BIGINT unsigned not null,"
  "COUNT_WRITE_EXTERNAL BIGINT unsigned not null,"
  "SUM_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "MIN_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "AVG_TIMER_WRITE_EXTERNAL BIGINT unsigned not null,"
  "MAX_TIMER_WRITE_EXTERNAL BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE THREADS
--

SET @cmd="CREATE TABLE performance_schema.threads("
  "THREAD_ID BIGINT unsigned not null,"
  "NAME VARCHAR(128) not null,"
  "TYPE VARCHAR(10) not null,"
  "PROCESSLIST_ID BIGINT unsigned,"
  "PROCESSLIST_USER VARCHAR(32),"
  "PROCESSLIST_HOST VARCHAR(60),"
  "PROCESSLIST_DB VARCHAR(64),"
  "PROCESSLIST_COMMAND VARCHAR(16),"
  "PROCESSLIST_TIME BIGINT,"
  "PROCESSLIST_STATE VARCHAR(64),"
  "PROCESSLIST_INFO LONGTEXT,"
  "PARENT_THREAD_ID BIGINT unsigned,"
  "ROLE VARCHAR(64),"
  "INSTRUMENTED ENUM ('YES', 'NO') not null,"
  "HISTORY ENUM ('YES', 'NO') not null,"
  "CONNECTION_TYPE VARCHAR(16),"
  "THREAD_OS_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PROCESSLIST
--

SET @cmd="CREATE TABLE performance_schema.processlist("
  "ID BIGINT unsigned not null,"
  "USER VARCHAR(32),"
  "HOST VARCHAR(66),"
  "DB VARCHAR(64),"
  "COMMAND VARCHAR(16),"
  "TIME BIGINT,"
  "STATE VARCHAR(64),"
  "INFO LONGTEXT"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_stages_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_stages_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_stages_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "WORK_COMPLETED BIGINT unsigned,"
  "WORK_ESTIMATED BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STAGES_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_stages_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_statements_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_statements_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_statements_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "LOCK_TIME bigint unsigned not null,"
  "SQL_TEXT LONGTEXT,"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "CURRENT_SCHEMA VARCHAR(64),"
  "OBJECT_TYPE VARCHAR(64),"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "MYSQL_ERRNO INTEGER,"
  "RETURNED_SQLSTATE VARCHAR(5),"
  "MESSAGE_TEXT VARCHAR(128),"
  "ERRORS BIGINT unsigned not null,"
  "WARNINGS BIGINT unsigned not null,"
  "ROWS_AFFECTED BIGINT unsigned not null,"
  "ROWS_SENT BIGINT unsigned not null,"
  "ROWS_EXAMINED BIGINT unsigned not null,"
  "CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SELECT_RANGE BIGINT unsigned not null,"
  "SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SELECT_SCAN BIGINT unsigned not null,"
  "SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SORT_RANGE BIGINT unsigned not null,"
  "SORT_ROWS BIGINT unsigned not null,"
  "SORT_SCAN BIGINT unsigned not null,"
  "NO_INDEX_USED BIGINT unsigned not null,"
  "NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT'),"
  "NESTING_EVENT_LEVEL INTEGER"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_CURRENT
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_current("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_HISTORY
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_history("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_HISTORY_LONG
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_history_long("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_ID BIGINT unsigned not null,"
  "END_EVENT_ID BIGINT unsigned,"
  "EVENT_NAME VARCHAR(128) not null,"
  "STATE ENUM('ACTIVE', 'COMMITTED', 'ROLLED BACK'),"
  "TRX_ID BIGINT unsigned,"
  "GTID VARCHAR(64),"
  "XID_FORMAT_ID INTEGER,"
  "XID_GTRID VARCHAR(130),"
  "XID_BQUAL VARCHAR(130),"
  "XA_STATE VARCHAR(64),"
  "SOURCE VARCHAR(64),"
  "TIMER_START BIGINT unsigned,"
  "TIMER_END BIGINT unsigned,"
  "TIMER_WAIT BIGINT unsigned,"
  "ACCESS_MODE ENUM('READ ONLY', 'READ WRITE'),"
  "ISOLATION_LEVEL VARCHAR(64),"
  "AUTOCOMMIT ENUM('YES','NO') not null,"
  "NUMBER_OF_SAVEPOINTS BIGINT unsigned,"
  "NUMBER_OF_ROLLBACK_TO_SAVEPOINT BIGINT unsigned,"
  "NUMBER_OF_RELEASE_SAVEPOINT BIGINT unsigned,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned,"
  "NESTING_EVENT_ID BIGINT unsigned,"
  "NESTING_EVENT_TYPE ENUM('TRANSACTION', 'STATEMENT', 'STAGE', 'WAIT')"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_TRANSACTIONS_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.events_transactions_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "COUNT_READ_WRITE BIGINT unsigned not null,"
  "SUM_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MIN_TIMER_READ_WRITE BIGINT unsigned not null,"
  "AVG_TIMER_READ_WRITE BIGINT unsigned not null,"
  "MAX_TIMER_READ_WRITE BIGINT unsigned not null,"
  "COUNT_READ_ONLY BIGINT unsigned not null,"
  "SUM_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MIN_TIMER_READ_ONLY BIGINT unsigned not null,"
  "AVG_TIMER_READ_ONLY BIGINT unsigned not null,"
  "MAX_TIMER_READ_ONLY BIGINT unsigned not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE HOSTS
--

SET @cmd="CREATE TABLE performance_schema.hosts("
  "HOST CHAR(60) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE USERS
--

SET @cmd="CREATE TABLE performance_schema.users("
  "USER CHAR(32) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE ACCOUNTS
--

SET @cmd="CREATE TABLE performance_schema.accounts("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "CURRENT_CONNECTIONS bigint not null,"
  "TOTAL_CONNECTIONS bigint not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_GLOBAL_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_global_by_event_name("
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_THREAD_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_thread_by_event_name("
  "THREAD_ID BIGINT unsigned not null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_ACCOUNT_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_account_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_HOST_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_host_by_event_name("
  "HOST CHAR(60) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE MEMORY_SUMMARY_BY_USER_BY_EVENT_NAME
--

SET @cmd="CREATE TABLE performance_schema.memory_summary_by_user_by_event_name("
  "USER CHAR(32) collate utf8_bin default null,"
  "EVENT_NAME VARCHAR(128) not null,"
  "COUNT_ALLOC BIGINT unsigned not null,"
  "COUNT_FREE BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_ALLOC BIGINT unsigned not null,"
  "SUM_NUMBER_OF_BYTES_FREE BIGINT unsigned not null,"
  "LOW_COUNT_USED BIGINT not null,"
  "CURRENT_COUNT_USED BIGINT not null,"
  "HIGH_COUNT_USED BIGINT not null,"
  "LOW_NUMBER_OF_BYTES_USED BIGINT not null,"
  "CURRENT_NUMBER_OF_BYTES_USED BIGINT not null,"
  "HIGH_NUMBER_OF_BYTES_USED BIGINT not null"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_DIGEST
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_digest("
  "SCHEMA_NAME VARCHAR(64),"
  "DIGEST VARCHAR(32),"
  "DIGEST_TEXT LONGTEXT,"
  "COUNT_STAR BIGINT unsigned not null,"
  "SUM_TIMER_WAIT BIGINT unsigned not null,"
  "MIN_TIMER_WAIT BIGINT unsigned not null,"
  "AVG_TIMER_WAIT BIGINT unsigned not null,"
  "MAX_TIMER_WAIT BIGINT unsigned not null,"
  "SUM_LOCK_TIME BIGINT unsigned not null,"
  "SUM_ERRORS BIGINT unsigned not null,"
  "SUM_WARNINGS BIGINT unsigned not null,"
  "SUM_ROWS_AFFECTED BIGINT unsigned not null,"
  "SUM_ROWS_SENT BIGINT unsigned not null,"
  "SUM_ROWS_EXAMINED BIGINT unsigned not null,"
  "SUM_CREATED_TMP_DISK_TABLES BIGINT unsigned not null,"
  "SUM_CREATED_TMP_TABLES BIGINT unsigned not null,"
  "SUM_SELECT_FULL_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_FULL_RANGE_JOIN BIGINT unsigned not null,"
  "SUM_SELECT_RANGE BIGINT unsigned not null,"
  "SUM_SELECT_RANGE_CHECK BIGINT unsigned not null,"
  "SUM_SELECT_SCAN BIGINT unsigned not null,"
  "SUM_SORT_MERGE_PASSES BIGINT unsigned not null,"
  "SUM_SORT_RANGE BIGINT unsigned not null,"
  "SUM_SORT_ROWS BIGINT unsigned not null,"
  "SUM_SORT_SCAN BIGINT unsigned not null,"
  "SUM_NO_INDEX_USED BIGINT unsigned not null,"
  "SUM_NO_GOOD_INDEX_USED BIGINT unsigned not null,"
  "FIRST_SEEN TIMESTAMP(0) NOT NULL default 0,"
  "LAST_SEEN TIMESTAMP(0) NOT NULL default 0"
  ")ENGINE=PERFORMANCE_SCHEMA;";


SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE EVENTS_STATEMENTS_SUMMARY_BY_PROGRAM
--

SET @cmd="CREATE TABLE performance_schema.events_statements_summary_by_program("
  "OBJECT_TYPE enum('EVENT', 'FUNCTION', 'PROCEDURE', 'TABLE', 'TRIGGER'),"
  "OBJECT_SCHEMA varchar(64) NOT NULL,"
  "OBJECT_NAME varchar(64) NOT NULL,"
  "COUNT_STAR bigint(20) unsigned NOT NULL,"
  "SUM_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "MIN_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "AVG_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "MAX_TIMER_WAIT bigint(20) unsigned NOT NULL,"
  "COUNT_STATEMENTS bigint(20) unsigned NOT NULL,"
  "SUM_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "MIN_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "AVG_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "MAX_STATEMENTS_WAIT bigint(20) unsigned NOT NULL,"
  "SUM_LOCK_TIME bigint(20) unsigned NOT NULL,"
  "SUM_ERRORS bigint(20) unsigned NOT NULL,"
  "SUM_WARNINGS bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_AFFECTED bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_SENT bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_EXAMINED bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_DISK_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_RANGE_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE_CHECK bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_SORT_MERGE_PASSES bigint(20) unsigned NOT NULL,"
  "SUM_SORT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SORT_ROWS bigint(20) unsigned NOT NULL,"
  "SUM_SORT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_NO_INDEX_USED bigint(20) unsigned NOT NULL,"
  "SUM_NO_GOOD_INDEX_USED bigint(20) unsigned NOT NULL"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE PREPARED_STATEMENT_INSTANCES
--

SET @cmd="CREATE TABLE performance_schema.prepared_statements_instances("
  "OBJECT_INSTANCE_BEGIN bigint(20) unsigned NOT NULL,"
  "STATEMENT_ID bigint(20) unsigned NOT NULL,"
  "STATEMENT_NAME varchar(64) default NULL,"
  "SQL_TEXT longtext NOT NULL,"
  "OWNER_THREAD_ID bigint(20) unsigned NOT NULL,"
  "OWNER_EVENT_ID bigint(20) unsigned NOT NULL,"
  "OWNER_OBJECT_TYPE enum('EVENT','FUNCTION','PROCEDURE','TABLE','TRIGGER') DEFAULT NULL,"
  "OWNER_OBJECT_SCHEMA varchar(64) DEFAULT NULL,"
  "OWNER_OBJECT_NAME varchar(64) DEFAULT NULL,"
  "TIMER_PREPARE bigint(20) unsigned NOT NULL,"
  "COUNT_REPREPARE bigint(20) unsigned NOT NULL,"
  "COUNT_EXECUTE bigint(20) unsigned NOT NULL,"
  "SUM_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "MIN_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "AVG_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "MAX_TIMER_EXECUTE bigint(20) unsigned NOT NULL,"
  "SUM_LOCK_TIME bigint(20) unsigned NOT NULL,"
  "SUM_ERRORS bigint(20) unsigned NOT NULL,"
  "SUM_WARNINGS bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_AFFECTED bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_SENT bigint(20) unsigned NOT NULL,"
  "SUM_ROWS_EXAMINED bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_DISK_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_CREATED_TMP_TABLES bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_FULL_RANGE_JOIN bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_RANGE_CHECK bigint(20) unsigned NOT NULL,"
  "SUM_SELECT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_SORT_MERGE_PASSES bigint(20) unsigned NOT NULL,"
  "SUM_SORT_RANGE bigint(20) unsigned NOT NULL,"
  "SUM_SORT_ROWS bigint(20) unsigned NOT NULL,"
  "SUM_SORT_SCAN bigint(20) unsigned NOT NULL,"
  "SUM_NO_INDEX_USED bigint(20) unsigned NOT NULL,"
  "SUM_NO_GOOD_INDEX_USED bigint(20) unsigned NOT NULL"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_connection_configuration
--

SET @cmd="CREATE TABLE performance_schema.replication_connection_configuration("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "HOST CHAR(60) collate utf8_bin not null,"
  "PORT INTEGER not null,"
  "USER CHAR(32) collate utf8_bin not null,"
  "NETWORK_INTERFACE CHAR(60) collate utf8_bin not null,"
  "AUTO_POSITION ENUM('1','0') not null,"
  "SSL_ALLOWED ENUM('YES','NO','IGNORED') not null,"
  "SSL_CA_FILE VARCHAR(512) not null,"
  "SSL_CA_PATH VARCHAR(512) not null,"
  "SSL_CERTIFICATE VARCHAR(512) not null,"
  "SSL_CIPHER VARCHAR(512) not null,"
  "SSL_KEY VARCHAR(512) not null,"
  "SSL_VERIFY_SERVER_CERTIFICATE ENUM('YES','NO') not null,"
  "SSL_CRL_FILE VARCHAR(255) not null,"
  "SSL_CRL_PATH VARCHAR(255) not null,"
  "CONNECTION_RETRY_INTERVAL INTEGER not null,"
  "CONNECTION_RETRY_COUNT BIGINT unsigned not null,"
  "HEARTBEAT_INTERVAL DOUBLE(10,3) unsigned not null COMMENT 'Number of seconds after which a heartbeat will be sent .',"
  "TLS_VERSION VARCHAR(255) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE replication_group_member_stats
--

SET @cmd="CREATE TABLE performance_schema.replication_group_member_stats("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "VIEW_ID CHAR(60) collate utf8_bin not null,"
  "MEMBER_ID CHAR(36) collate utf8_bin not null,"
  "COUNT_TRANSACTIONS_IN_QUEUE BIGINT unsigned not null,"
  "COUNT_TRANSACTIONS_CHECKED BIGINT unsigned not null,"
  "COUNT_CONFLICTS_DETECTED BIGINT unsigned not null,"
  "COUNT_TRANSACTIONS_ROWS_VALIDATING BIGINT unsigned not null,"
  "TRANSACTIONS_COMMITTED_ALL_MEMBERS LONGTEXT not null,"
  "LAST_CONFLICT_FREE_TRANSACTION TEXT not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


--
-- TABLE replication_group_members
--

SET @cmd="CREATE TABLE performance_schema.replication_group_members("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "MEMBER_ID CHAR(36) collate utf8_bin not null,"
  "MEMBER_HOST CHAR(60) collate utf8_bin not null,"
  "MEMBER_PORT INTEGER,"
  "MEMBER_STATE CHAR(64) collate utf8_bin not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

 SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
 PREPARE stmt FROM @str;
 EXECUTE stmt;
 DROP PREPARE stmt;

--
-- TABLE replication_connection_status
--

SET @cmd="CREATE TABLE performance_schema.replication_connection_status("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "GROUP_NAME CHAR(36) collate utf8_bin not null,"
  "SOURCE_UUID CHAR(36) collate utf8_bin not null,"
  "THREAD_ID BIGINT unsigned,"
  "SERVICE_STATE ENUM('ON','OFF','CONNECTING') not null,"
  "COUNT_RECEIVED_HEARTBEATS bigint unsigned NOT NULL DEFAULT 0,"
  "LAST_HEARTBEAT_TIMESTAMP TIMESTAMP(0) not null COMMENT 'Shows when the most recent heartbeat signal was received.',"
  "RECEIVED_TRANSACTION_SET LONGTEXT not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_configuration
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_configuration("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "DESIRED_DELAY INTEGER not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "REMAINING_DELAY INTEGER unsigned,"
  "COUNT_TRANSACTIONS_RETRIES BIGINT unsigned not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status_by_coordinator
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status_by_coordinator("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "THREAD_ID BIGINT UNSIGNED,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE replication_applier_status_by_worker
--

SET @cmd="CREATE TABLE performance_schema.replication_applier_status_by_worker("
  "CHANNEL_NAME CHAR(64) collate utf8_general_ci not null,"
  "WORKER_ID BIGINT UNSIGNED not null,"
  "THREAD_ID BIGINT UNSIGNED,"
  "SERVICE_STATE ENUM('ON','OFF') not null,"
  "LAST_SEEN_TRANSACTION CHAR(57) not null,"
  "LAST_ERROR_NUMBER INTEGER not null,"
  "LAST_ERROR_MESSAGE VARCHAR(1024) not null,"
  "LAST_ERROR_TIMESTAMP TIMESTAMP(0) not null"
  ") ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_CONNECT_ATTRS
--

SET @cmd="CREATE TABLE performance_schema.session_connect_attrs("
  "PROCESSLIST_ID INT NOT NULL,"
  "ATTR_NAME VARCHAR(32) NOT NULL,"
  "ATTR_VALUE VARCHAR(1024),"
  "ORDINAL_POSITION INT"
  ")ENGINE=PERFORMANCE_SCHEMA CHARACTER SET utf8 COLLATE utf8_bin;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_ACCOUNT_CONNECT_ATTRS
--

SET @cmd="CREATE TABLE performance_schema.session_account_connect_attrs "
         " LIKE performance_schema.session_connect_attrs;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE TABLE_HANDLES
--

SET @cmd="CREATE TABLE performance_schema.table_handles("
  "OBJECT_TYPE VARCHAR(64) not null,"
  "OBJECT_SCHEMA VARCHAR(64) not null,"
  "OBJECT_NAME VARCHAR(64) not null,"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "OWNER_THREAD_ID BIGINT unsigned,"
  "OWNER_EVENT_ID BIGINT unsigned,"
  "INTERNAL_LOCK VARCHAR(64),"
  "EXTERNAL_LOCK VARCHAR(64)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE METADATA_LOCKS
--

SET @cmd="CREATE TABLE performance_schema.metadata_locks("
  "OBJECT_TYPE VARCHAR(64) not null,"
  "OBJECT_SCHEMA VARCHAR(64),"
  "OBJECT_NAME VARCHAR(64),"
  "OBJECT_INSTANCE_BEGIN BIGINT unsigned not null,"
  "LOCK_TYPE VARCHAR(32) not null,"
  "LOCK_DURATION VARCHAR(32) not null,"
  "LOCK_STATUS VARCHAR(32) not null,"
  "SOURCE VARCHAR(64),"
  "OWNER_THREAD_ID BIGINT unsigned,"
  "OWNER_EVENT_ID BIGINT unsigned"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE USER_VARIABLES_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.user_variables_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE LONGBLOB"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE VARIABLES_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.variables_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE GLOBAL_VARIABLES
--

SET @cmd="CREATE TABLE performance_schema.global_variables("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_VARIABLES
--

SET @cmd="CREATE TABLE performance_schema.session_variables("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_THREAD
--

SET @cmd="CREATE TABLE performance_schema.status_by_thread("
  "THREAD_ID BIGINT unsigned not null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_USER
--

SET @cmd="CREATE TABLE performance_schema.status_by_user("
  "USER CHAR(32) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_HOST
--

SET @cmd="CREATE TABLE performance_schema.status_by_host("
  "HOST CHAR(60) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE STATUS_BY_ACCOUNT
--

SET @cmd="CREATE TABLE performance_schema.status_by_account("
  "USER CHAR(32) collate utf8_bin default null,"
  "HOST CHAR(60) collate utf8_bin default null,"
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE GLOBAL_STATUS
--

SET @cmd="CREATE TABLE performance_schema.global_status("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;

--
-- TABLE SESSION_STATUS
--

SET @cmd="CREATE TABLE performance_schema.session_status("
  "VARIABLE_NAME VARCHAR(64) not null,"
  "VARIABLE_VALUE VARCHAR(1024)"
  ")ENGINE=PERFORMANCE_SCHEMA;";

SET @str = IF(@have_pfs = 1, @cmd, 'SET @dummy = 0');
=========
SET @cmd = "CREATE TABLE IF NOT EXISTS proxies_priv (Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, User char(32) binary DEFAULT '' NOT NULL, Proxied_host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, Proxied_user char(32) binary DEFAULT '' NOT NULL, With_grant BOOL DEFAULT 0 NOT NULL, Grantor varchar(288) DEFAULT '' NOT NULL, Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, PRIMARY KEY Host (Host,User,Proxied_host,Proxied_user), KEY Grantor (Grantor) ) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8 COLLATE utf8_bin comment='User proxy privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
=======
SET @cmd = "CREATE TABLE IF NOT EXISTS proxies_priv (Host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, User char(32) binary DEFAULT '' NOT NULL, Proxied_host char(255) CHARACTER SET ASCII DEFAULT '' NOT NULL, Proxied_user char(32) binary DEFAULT '' NOT NULL, With_grant BOOL DEFAULT 0 NOT NULL, Grantor varchar(288) DEFAULT '' NOT NULL, Timestamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, PRIMARY KEY Host (Host,User,Proxied_host,Proxied_user), KEY Grantor (Grantor) ) engine=InnoDB STATS_PERSISTENT=0 CHARACTER SET utf8mb3 COLLATE utf8mb3_bin comment='User proxy privileges' ROW_FORMAT=DYNAMIC TABLESPACE=mysql";
>>>>>>> Percona-Server-8.0.31-23
SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;


-- Remember for later if proxies_priv table already existed
set @had_proxies_priv_table= @@warning_count != 0;


--
-- Only create the ndb_binlog_index table if the server is built with ndb.
-- Create this table last among the tables in the mysql schema to make it
-- easier to keep tests agnostic wrt. the existence of this table.
--
SET @have_ndb= (select count(engine) from information_schema.engines where engine='ndbcluster');
SET @cmd="CREATE TABLE IF NOT EXISTS ndb_binlog_index (
  Position BIGINT UNSIGNED NOT NULL,
  File VARCHAR(255) NOT NULL,
  epoch BIGINT UNSIGNED NOT NULL,
  inserts INT UNSIGNED NOT NULL,
  updates INT UNSIGNED NOT NULL,
  deletes INT UNSIGNED NOT NULL,
  schemaops INT UNSIGNED NOT NULL,
  orig_server_id INT UNSIGNED NOT NULL,
  orig_epoch BIGINT UNSIGNED NOT NULL,
  gci INT UNSIGNED NOT NULL,
  next_position BIGINT UNSIGNED NOT NULL,
  next_file VARCHAR(255) NOT NULL,
  PRIMARY KEY(epoch, orig_server_id, orig_epoch)
  ) ENGINE=INNODB CHARACTER SET latin1 STATS_PERSISTENT=0 ROW_FORMAT=DYNAMIC TABLESPACE=mysql";

SET @str = CONCAT(@cmd, " ENCRYPTION='", @is_mysql_encrypted, "'");
SET @str = IF(@have_ndb = 1, @str, 'SET @dummy = 0');
PREPARE stmt FROM @str;
EXECUTE stmt;
DROP PREPARE stmt;
