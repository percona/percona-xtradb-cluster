# Teardown

--echo # ----------------------------------------------------------------------
--echo # Teardown

if ($global_manifest) {
  # Restore the manifest file
  --source include/keyring_tests/helper/binary_restore_manifest.inc
}

# Remove local manifest file on the donor
if ($donor_local_manifest) {
  --let CURRENT_DATADIR = $DONOR_DATADIR
  --source include/keyring_tests/helper/instance_remove_manifest.inc
}

if ($joiner_local_manifest) {
  --let CURRENT_DATADIR = $JOINER_DATADIR
  --source include/keyring_tests/helper/instance_remove_manifest.inc
}

# Remove keyring file
--let KEYRING_FILE_PATH = $DONOR_KEYRING_FILE_PATH
--source include/keyring_tests/helper/local_keyring_file_remove.inc
--let KEYRING_FILE_PATH = $JOINER_KEYRING_FILE_PATH
--source include/keyring_tests/helper/local_keyring_file_remove.inc

# Remove local keyring config
--let CURRENT_DATADIR = $DONOR_DATADIR
--source include/keyring_tests/helper/local_keyring_remove_config.inc
--let CURRENT_DATADIR = $JOINER_DATADIR
--source include/keyring_tests/helper/local_keyring_remove_config.inc

# Restart server without manifest file
--source include/keyring_tests/helper/cleanup_server_with_manifest.inc
--echo # ----------------------------------------------------------------------
