#!/usr/bin/env perl

# Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#
# This script is based on checkDBI-DBD-mysql.pl
#
# The comments below were taken from checkDBI-DBD-mysql.pl but altered
# to match the usage of this script.
#

################################################################################
#
# This perl script checks for availability of the Perl module specified.
# using the "current" perl interpreter.
#
# Useful for test environment checking before testing executable perl scripts
# in the MySQL Server distribution.
#
# NOTE: The "shebang" on the first line of this script should always point to
#       /usr/bin/perl, so that we can use this script to check whether or not we
#       support running perl scripts with such a shebang without specifying the
#       perl interpreter on the command line. Such a script is mysqlhotcopy.
#
#       When run as "check_for_perl_module.pl" the shebang line will be evaluated
#       and used. When run as "perl check_for_perl_module.pl" the shebang line is
#       not used.
#
# NOTE: This script will create a temporary file in MTR's tmp dir.
#       If modules are found, a mysql-test statement which sets a special
#       variable is written to this file.
#       A test (or include file) which sources that file can then easily do
#       an if-check on the special variable to determine success or failure.
#
#       Example:
#
#	      --let $perlChecker= suite/galera/include/check_for_perl_module.pl
#         --let $resultFile= $MYSQL_TMP_DIR/mysql-perl-module-check.txt
#         --chmod 0755 $perlChecker
#         --exec $perlChecker $resultFile DBI DBD::mysql Cache::Memcached
#         --source $resultFile
#         if (!$all_perl_modules_found) {
#             --skip Test needs Perl modules : $perl_modules_not_found
#         }
#
#         Three variables will be created in the result file:
#           $perl_modules_found     : string contains names of modules found
#           $perl_modules_not_found : string contains names of modules not found
#           $all_perl_modules_found : =1 if all modules found, =0 otherwise
#
#       The calling script is also responsible for cleaning up after use:
#
#         --remove_file $resultFile
#
# Windows notes:
#   - shebangs may work differently - call this script with "perl " in front.
#
# See mysql-test/include/have_dbi_dbd-mysql.inc for example use of this script.
# This script should be executable for the user running MTR.
#
################################################################################

use strict;
use warnings;

my $result_file = $ARGV[0] or die("result_file not set\n");

my $MODULES_FOUND = "";
my $MODULES_NOT_FOUND = "";

# By using eval we can suppress warnings and continue after.
# We need to catch "Can't locate" as well as "Can't load" errors.

# Skip over the first parameter (the result file location)
for my $i (1 .. $#ARGV) {
    my $MODULE_NAME=$ARGV[$i];
    eval{
        my $FOUND_MODULE = 0;
        $FOUND_MODULE = 1 if eval "require $MODULE_NAME";

        if ($FOUND_MODULE) {
            $MODULES_FOUND .= (' '.$MODULE_NAME );
        }
        else {
            $MODULES_NOT_FOUND .= (' '.$MODULE_NAME);
        };
    };
};

# Trim the strings (remove left and right whitespace)
$MODULES_FOUND =~ s/^\s+|\s+$//g;
$MODULES_NOT_FOUND =~ s/^\s+|\s+$//g;

# Open a file to be used for transfer of result back to mysql-test.
# The file must be created whether we write to it or not, otherwise mysql-test
# will complain if trying to source it.
open(FILE, ">", $result_file);

print(FILE "let \$perl_modules_found= \"$MODULES_FOUND\";"."\n");
print(FILE "let \$perl_modules_not_found= \"$MODULES_NOT_FOUND\";"."\n");

if (length($MODULES_NOT_FOUND) == 0) {
    print(FILE "let \$all_perl_modules_found= 1;"."\n");
}
else {
    print(FILE "let \$all_perl_modules_found= 0;"."\n");
}

# close the file.
close(FILE);

1;

