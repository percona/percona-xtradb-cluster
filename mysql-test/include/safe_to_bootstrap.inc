# Prerequisite: --source include/have_util_sed.inc

--exec $SED -i "s/safe_to_bootstrap:[ \t]*0/safe_to_bootstrap: 1/" $MYSQLTEST_VARDIR/mysqld.1/data/grastate.dat
