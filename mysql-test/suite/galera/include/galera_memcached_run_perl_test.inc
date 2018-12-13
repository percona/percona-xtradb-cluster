#
# This tries to use the memcached api and returns the
# results of several operations.  The result of the
# operations depends on the specific tests.
#
# Usage:
#       --let $MEMCACHED_SERVER_ADDRESS=127.0.0.1:11211
#       --let $USE_MEMCACHED_BINARY_PROTOCOL=0
#       --source galera_memcached_run_perl_test.inc
#       if ($galera_memcached_stat_result != "success")
#       {
#           die("Test failure!")
#       }
#

--let $perl_test_result_file=$MYSQL_TMP_DIR/galera_memcached_run_perl_test.txt

--let PERL_RESULT_FILE=$perl_test_result_file
--let USE_MEMCACHED_BINARY_PROTOCOL=$USE_MEMCACHED_BINARY_PROTOCOL
--let SERVER_ADDRESS=$MEMCACHED_SERVER_ADDRESS

--perl
    use strict;
    use warnings;

    my $result_file = $ENV{"PERL_RESULT_FILE"} or die "Missing result_file parameter";
    my $server_address = $ENV{'SERVER_ADDRESS'} or die "Missing memcached server address parameter";
    my $use_binary_protocol = $ENV{'USE_MEMCACHED_BINARY_PROTOCOL'} or 0;

    open(FILE, '>', "$result_file") or die("Unable to open $result_file: $!\n");

    my $val;
    my %versions;
    my $versions_ref;

    # By using eval we can suppress warnings and continue after.
    eval {
        use Cache::Memcached::libmemcached;
        my $memd = Cache::Memcached::libmemcached->new({
            'servers' => [ "$server_address" ],
        });

        if ($use_binary_protocol == 1) {
            $memd->set_binary_protocol(1);
            print(FILE "--let \$galera_memcached_binary_protocol=1\n");
        }
        else {
            print(FILE "--let \$galera_memcached_binary_protocol=0\n");
        }

        #
        # This will issue the "stat" command
        # This should always succeed
        #
        $versions_ref = $memd->server_versions;
        if ($versions_ref) {
            %versions = %{ $versions_ref };
        }
        if (%versions) {
            $val = $versions { $server_address };
        }
        if ($val) {
            print(FILE "--let \$galera_memcached_stat_result=success\n");
            print(FILE "--let \$galera_memcached_stat_value=\"$val\"\n");
        }
        else {
            print(FILE "--let \$galera_memcached_stat_result=fail\n");
        }

        #
        # Try a get(), this should fail if WSREP is enabled
        # and succeed if WSREP is not enabled
        #
        $val = $memd->get("AA");
        if ($val) {
            print(FILE "--let \$galera_memcached_get_result=success\n");
            print(FILE "--let \$galera_memcached_get_value=\"$val\"\n");
        }
        else {
            print(FILE "--let \$galera_memcached_get_result=fail\n");
        }

        #
        # Try a set(), this should fail if WSREP is enabled
        # and succeed if WSREP is not enabled
        #
        $val = $memd->set("bb", "A somewhat longer string to use in the set command.");
        if ($val) {
            print(FILE "--let \$galera_memcached_set_result=success\n");
        }
        else {
            print(FILE "--let \$galera_memcached_set_result=fail\n");
        }

        $memd->disconnect_all;
    };

    close(FILE);
EOF

--source $perl_test_result_file
--remove_file $perl_test_result_file

