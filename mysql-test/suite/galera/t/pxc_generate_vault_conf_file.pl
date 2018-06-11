#!/usr/bin/perl
#
# Generates a vault conf file that can be used by the PXC mtr test cases.
#
# Script parameters:
#  Parameter 1: The template file that is the basis for the generated config file
#  Parameter 2: The location of the generated file
#  Parameter 3: The secret mount point
#

use strict;
use warnings;

my ($keyring_conf_template_file, $keyring_conf_file_to_generate, $keyring_conf_secret_mount_point) = @ARGV;

if (not defined $keyring_conf_template_file)
{
  die("Need to specify the keyring_conf_template_file as the first parameter\n");
}

if (not defined $keyring_conf_file_to_generate)
{
  die("Need to specify the keyring_conf_file_to_generate as the second parameter\n");
}

if (not defined $keyring_conf_secret_mount_point)
{
  die("Need to specify the keyring_conf_secret_mount_point as the third parameter\n");
}

my $mysql_test_dir= $ENV{MYSQL_TEST_DIR} or die "Need MYSQL_TEST_DIR";

open CONF_FILE, ">", "$keyring_conf_file_to_generate" or die "Could not open configuration file: ${keyring_conf_file_to_generate}.\n";
open CONF_TEMPLATE_FILE, "<", "$keyring_conf_template_file" or die "Could not open configuration template file: ${keyring_conf_template_file}.\n";

while (my $row = <CONF_TEMPLATE_FILE>)
{
  $row =~ s/MYSQL_TEST_DIR/$mysql_test_dir/g;
  $row =~ s/SECRET_MOUNT_POINT_TAG/$keyring_conf_secret_mount_point/g;
  print CONF_FILE $row;
}
close(CONF_FILE);
close(CONF_TEMPLATE_FILE);
