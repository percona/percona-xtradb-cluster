#!/usr/bin/perl
#
# Script parameters:
#  Parameter 1: MOUNT_POINT_SERVICE_OP
#  Parameter 2: KEYRING_CONF_FILE
#
# Creates or deletes secret mount point specified in keyring_vault configuration file
# Mount point is :
#  - created when MOUNT_POINT_SERVICE_OP is set to CREATE
#  - deleted when MOUNT_POINT_SERVICE_OP is set to DELETE

# The sourcing test needs to set $KEYRING_CONF_FILE variable to
# the location of keyring_vault configuration file and
# MOUNT_POINT_SERVICE_OP variable to CREATE or DELETE

use strict;
use warnings;

my ($mount_point_service_op, $keyring_conf_file) = @ARGV;

if (not defined $mount_point_service_op)
{
  die("Need to specify a mount point service op (create or delete) as the first parameter\n");
}

$mount_point_service_op = lc($mount_point_service_op);
if (($mount_point_service_op ne "create") && ($mount_point_service_op ne "delete"))
{
  die("Only create and delete are allowed as mount point service ops\n");
}

if (not defined $keyring_conf_file)
{
  die("Need to specify a keyring conf file name as the second parameter\n");
}


use MIME::Base64 qw( decode_base64 );
my $token;
my $vault_url;
my $secret_mount_point;
my $vault_ca;
my $CONF_FILE;
open(CONF_FILE, "$keyring_conf_file") or die("Could not open configuration file.\n");
while (my $row = <CONF_FILE>)
{
  if ($row =~ m/token[ ]*=[ ]*(.*)/)
  {
    $token=$1;
  }
  elsif ($row =~ m/vault_url[ ]*=[ ]*(.*)/)
  {
    $vault_url=$1;
  }
  elsif ($row =~ m/secret_mount_point[ ]*= [ ]*(.*)/)
  {
    $secret_mount_point=$1;
  }
  elsif ($row =~ m/vault_ca[ ]*= [ ]*(.*)/)
  {
    $vault_ca=$1;
  }
}
close(CONF_FILE);
if ($token eq "" || $vault_url eq "" || $secret_mount_point eq "")
{
  die("Could not read vault credentials from configuration file.\n");
}

my $vault_ca_cert_opt= "";
if ($vault_ca)
{
  $vault_ca_cert_opt= "--cacert $vault_ca";
}

if ($mount_point_service_op eq 'create')
{
  system(qq#curl -H "X-Vault-Token: $token" $vault_ca_cert_opt --data '{"type":"generic"}' --request POST $vault_url/v1/sys/mounts/$secret_mount_point#);
}
elsif ($mount_point_service_op eq 'delete')
{
  system(qq#curl -H "X-Vault-Token: $token" $vault_ca_cert_opt -X DELETE $vault_url/v1/sys/mounts/$secret_mount_point#);
}
else
{
  die("Mount point should be either created or deleted. The resulting operation is no-op");
}

