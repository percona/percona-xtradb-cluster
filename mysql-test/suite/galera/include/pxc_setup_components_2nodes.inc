# Setup test to use keyring component

--echo # ----------------------------------------------------------------------
--echo # Setup


--let COMPONENT_LIBRARY = `SELECT SUBSTRING_INDEX('$KEYRING_FILE_COMPONENT_LOAD', '=', -1)`
--let COMPONENT_DIR = $KEYRING_FILE_COMPONENT_DIR
--let COMPONENT_NAME = `SELECT SUBSTRING_INDEX('$COMPONENT_LIBRARY', '.', 1)`

#
# Create keyring manifest and config for donor node
#
--connection node_1
--let CURRENT_DATADIR = `SELECT @@datadir`

if ($global_manifest)
{
  # Backup the global manifest file
  --source include/keyring_tests/helper/binary_backup_manifest.inc

  # Create manifest file for mysqld binary
  --let GLOBAL_MANIFEST_CONTENT = `SELECT CONCAT('{ \"components\": \"file://', '$COMPONENT_NAME', '\" }')`
  --source include/keyring_tests/helper/binary_create_customized_manifest.inc
}

if ($donor_local_manifest)
{
  # Create global manifest file
  --source include/keyring_tests/helper/binary_create_manifest.inc

  # Create local manifest file for current server instance
  --let LOCAL_MANIFEST_CONTENT = `SELECT CONCAT('{ \"components\": \"file://', '$COMPONENT_NAME', '\" }')`
  --source include/keyring_tests/helper/instance_create_manifest.inc
}

# Create local keyring config as we cannot test global keyring config in MTR environment
--let KEYRING_FILE_PATH = `SELECT CONCAT( '$MYSQLTEST_VARDIR', '/keyring_file_donor')`
--let KEYRING_CONFIG_CONTENT = `SELECT CONCAT('{ \"path\": \"', '$KEYRING_FILE_PATH','\", \"read_only\": false }')`
--source include/keyring_tests/helper/local_keyring_create_config.inc

--let DONOR_DATADIR = $CURRENT_DATADIR
--let DONOR_KEYRING_FILE_PATH = $KEYRING_FILE_PATH

#
# Create keyring manifest and config for joiner node
#

--connection node_2
--let CURRENT_DATADIR = `SELECT @@datadir`

if ($joiner_local_manifest)
{
  # Create global manifest file
  --source include/keyring_tests/helper/binary_create_manifest.inc

  # Create local manifest file for current server instance
  --let LOCAL_MANIFEST_CONTENT = `SELECT CONCAT('{ \"components\": \"file://', '$COMPONENT_NAME', '\" }')`
  --source include/keyring_tests/helper/instance_create_manifest.inc
}

# Create local keyring config as we cannot test global keyring config in MTR environment
--let KEYRING_FILE_PATH = `SELECT CONCAT( '$MYSQLTEST_VARDIR', '/keyring_file_joiner')`
--let KEYRING_CONFIG_CONTENT = `SELECT CONCAT('{ \"path\": \"', '$KEYRING_FILE_PATH','\", \"read_only\": false }')`
--source include/keyring_tests/helper/local_keyring_create_config.inc

--let JOINER_DATADIR = $CURRENT_DATADIR
--let JOINER_KEYRING_FILE_PATH = $KEYRING_FILE_PATH

--connection node_1
--let CURRENT_DATADIR = `SELECT @@datadir`
--echo # ----------------------------------------------------------------------
