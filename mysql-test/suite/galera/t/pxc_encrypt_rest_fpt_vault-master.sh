#!/bin/bash
#
# The purpose of this shell script is to create the vault config files
# before starting the server.
#

#
# Take the std_data/keyring_vault_confs/keyring_vault_mtr_template*.conf files
# and create the appropriate vault config files for the given test
#

if ! which curl >/dev/null
then
    echo "ERROR: Running $0"
    echo "ERROR: 'curl' not found in PATH"
    return 2 # no such file or directory
fi

test_name1="pxc_encrypt_rest_fpt_vault_57_1"
test_name2="pxc_encrypt_rest_fpt_vault_57_2"

# Configure the vault config files
$(dirname $0)/pxc_generate_vault_conf_file.pl $MYSQL_TEST_DIR/std_data/keyring_vault_confs/keyring_vault_mtr_template1.conf $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name1.conf $test_name1
$(dirname $0)/pxc_generate_vault_conf_file.pl $MYSQL_TEST_DIR/std_data/keyring_vault_confs/keyring_vault_mtr_template2.conf $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name2.conf $test_name2

# Invoke perl script that sends the commands to the vault server
# by pointing it at the appropriate config file
MOUNT_SERVICE_SCRIPT=$(dirname $0)/pxc_mount_point_service.pl

# create the mount point for node1
$MOUNT_SERVICE_SCRIPT DELETE $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name1.conf
$MOUNT_SERVICE_SCRIPT CREATE $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name1.conf

# create the mount point for node2
$MOUNT_SERVICE_SCRIPT DELETE $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name2.conf
$MOUNT_SERVICE_SCRIPT CREATE $MYSQLTEST_VARDIR/std_data/keyring_vault_confs/$test_name2.conf