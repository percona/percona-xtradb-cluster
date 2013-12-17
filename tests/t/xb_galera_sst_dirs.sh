############################################################################
# Bug#1098566  SST fails with innodb_data_home_dir/innodb_log_group_home_dir 
############################################################################

. inc/common.sh

node1=1
# node2 will be getting SST
node2=901
ADDR=127.0.0.1
SSTPASS="password"
SUSER="root"
SMDSUM="d35d212fdfe9452e711704e3dc3e93cf"



vlog "Running test for innodb_data_home_dir/innodb_log_group_home_dir"

if [[ -n ${EXTRAFILE:-} ]];then 
    EXTRAFILE1="$XB_TESTDIR/conf/${EXTRAFILE}.cnf-node1"
    EXTRAFILE2="$XB_TESTDIR/conf/${EXTRAFILE}.cnf-node2"
    vlog "Including $EXTRAFILE1 and $EXTRAFILE2 for $CONF"
else 
    EXTRAFILE1="$XB_TESTDIR/conf/bug1098566.cnf-node1"
    EXTRAFILE2="$XB_TESTDIR/conf/bug1098566.cnf-node2"
    vlog "Including $EXTRAFILE1 and $EXTRAFILE2"
fi

debug=""
pdebug=""
if [[ -n ${WSREP_DEBUG:-} ]];then 
    debug="--wsrep-debug=1"
    pdebug=";debug=1"
fi

recv_addr1="${ADDR}:$(get_free_port 2)"
recv_addr2="${ADDR}:$(get_free_port 3)"
listen_addr1="${ADDR}:$(get_free_port 4)"
listen_addr2="${ADDR}:$(get_free_port 5)"

data1="/tmp/var-test/data1"
log1="/tmp/var-test/log1"
data2="/tmp/var-test/data2"
log2="/tmp/var-test/log2"

vlog "Initial cleanup"
rm -rf $data1 $log1 $data2 $log2

mkdir -p $data1 $log1 $data2 $log2

vlog "Starting server $node1"
MYSQLD_EXTRA_MY_CNF_OPTS="!include $EXTRAFILE1"
start_server_with_id $node1 --innodb_flush_method=O_DIRECT --innodb_autoinc_lock_mode=2  --innodb_locks_unsafe_for_binlog=1 --wsrep-slave-threads=2 --innodb_file_per_table  --binlog-format=ROW --wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so --wsrep_cluster_address=gcomm:// --wsrep_sst_receive_address=$recv_addr1 --wsrep_node_incoming_address=$ADDR --wsrep_provider_options="gmcast.listen_addr=tcp://$listen_addr1${pdebug}" --wsrep_sst_method=xtrabackup-v2 --wsrep_sst_auth=$SUSER:$SSTPASS  --wsrep_node_address=$ADDR $debug 

vlog "Sleeping before loading data"
sleep 8

load_dbase_schema sbtest
load_dbase_data sbtest

vlog "Setting password to 'password'"
run_cmd ${MYSQL} ${MYSQL_ARGS} <<EOF
 SET PASSWORD FOR 'root'@'localhost' = PASSWORD('password');
EOF

vlog "Starting server $node2"
MYSQLD_EXTRA_MY_CNF_OPTS="!include $EXTRAFILE2"
start_server_with_id $node2  --innodb_flush_method=O_DIRECT --innodb_autoinc_lock_mode=2  --innodb_locks_unsafe_for_binlog=1 --wsrep-slave-threads=2 --innodb_file_per_table --binlog-format=ROW --wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so --wsrep_cluster_address=gcomm://$listen_addr1 --wsrep_sst_receive_address=$recv_addr2 --wsrep_node_incoming_address=$ADDR --wsrep_provider_options="gmcast.listen_addr=tcp://$listen_addr2${pdebug}" --wsrep_sst_method=xtrabackup-v2 --wsrep_sst_auth=$SUSER:$SSTPASS  --wsrep_node_address=$ADDR $debug
switch_server $node2

# The password propagates through SST
MYSQL_ARGS="${MYSQL_ARGS} -ppassword"

sleep 3

if [[ "`${MYSQL} ${MYSQL_ARGS} -Ns -e 'SHOW STATUS LIKE "wsrep_local_state_uuid"'|awk {'print $2'}`" == "`sed  -re 's/:.+$//' $MYSQLD_DATADIR/xtrabackup_galera_info`" && "`${MYSQL} ${MYSQL_ARGS} -Ns -e 'SHOW STATUS LIKE "wsrep_last_committed"'|awk {'print $2'}`" == "`sed  -re 's/^.+://' $MYSQLD_DATADIR/xtrabackup_galera_info`" ]]
then
	vlog "SST successful"
else
	vlog "SST failed"
	exit 1
fi

# Lightweight verification till lp:1199656 is fixed
mdsum=$(${MYSQL} ${MYSQL_ARGS} -e 'select * from sbtest.sbtest1;' | md5sum | cut -d" " -f1)

if [[ $mdsum != $SMDSUM ]];then 
    vlog "Integrity verification failed: found: $mdsum expected: $SMDSUM"
    exit 1
else 
    vlog "Integrity verification successful"
fi

stop_server_with_id $node2
stop_server_with_id $node1

vlog "Cleanup"
rm -rf $data1 $log1 $data2 $log2

free_reserved_port ${listen_addr1}
free_reserved_port ${listen_addr2}
free_reserved_port ${recv_addr1}
free_reserved_port ${recv_addr2}
