#
# Checks that the perl Cache::Memcached::libmemcached module is installed.
# This particular module is needed to test the binary memcached API.
#

--let $result_file= $MYSQL_TMP_DIR/mysql-perl-module-check.txt

--exec suite/galera/include/check_for_perl_module.pl $result_file Cache::Memcached::libmemcached

--source $result_file
--remove_file $result_file

if (!$all_perl_modules_found) {
    --skip Test requires Perl module : Cache::Memcached::libmemcached
}

