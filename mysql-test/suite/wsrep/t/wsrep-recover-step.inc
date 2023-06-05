#
# Macro to run wsrep recovery step. This is adapted from
# suite/galera/include/galera_wsrep_recover.inc, with additional
# option to pass binlog argument to recovery command. The macro
# returns recovered position split in uuid and seqno parts.
#
# Arguments:
#
# wsrep_recover_binlog_opt - Binlog options to recovery command
#
# Return:
#
# wsrep_recover_start_position_uuid - UUID corresponding to recovered position
# wsrep_recover_start_position_seqno - seqno corresponding to recovered position
#

--exec $MYSQLD --defaults-group-suffix=.1 --defaults-file=$MYSQLTEST_VARDIR/my.cnf --log-error=$MYSQL_TMP_DIR/galera_wsrep_recover.log --innodb --wsrep-recover $wsrep_recover_binlog_opt --core-file > $MYSQL_TMP_DIR/galera_wsrep_recover.log 2>&1

--let wsrep_recover_log = $MYSQL_TMP_DIR/galera_wsrep_recover.log
--source wsrep-read-recover-position.inc
