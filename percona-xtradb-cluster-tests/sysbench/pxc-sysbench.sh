#!/bin/bash -ue


# This script will exercise pxc functionality using sysbench.
# Script try to load data using parallel sysbench through multiple
# nodes and validates that loaded data is sane on all nodes of the cluster.

#-------------------------------------------------------------------------------
#
# Step-1: Clear pending backlog.
#
set +e
echo "Killing existing mysqld"
killall -9 mysqld
killall -9 garbd
sleep 1
set -e

#-------------------------------------------------------------------------------
#
# Step-2: Parse and set configuration variable.
#
DURATION=
SST_METHOD=
WORKDIR=
LUASCRIPTS=
NUMBEROFTABLES=
TABLESIZE=
NUMBEROFTABLES=
THREADS=
SLAVE_THREADS=
NODE1PID=
NODE2PID=
NODE3PID=
GARBDPID=

SYSBENCH=`which sysbench` || (echo "Can't locate sysbench" && exit 1)
echo "Using $SYSBENCH"
XB_VER=2.4.1

usage() {
    echo "-d <insecs> -s <rsync|mysqldump|xtrabackup-v2> -w <pxc/pxb tar directory> -l <sysbench lua script folder> -n <number-of-table> -x <each-table-size> -t <num-of-threads>"
}

optspec=":d:s:w:l:n:x:t:h:"
while getopts $optspec option; do
    case "$option" in
    d)
    	DURATION=${OPTARG}
    	;;
    s)
    	SST_METHOD=$OPTARG
    	;;
    w)
    	WORKDIR=$OPTARG
    	;;
    l)
    	LUASCRIPTS=$OPTARG
    	;;
    n)
    	NUMBEROFTABLES=$OPTARG
    	;;
    x)
    	TABLESIZE=$OPTARG
    	;;
    t)
    	THREADS=$OPTARG
	SLAVE_THREADS=`expr $THREADS / 4`
	if [ $SLAVE_THREADS -eq 0 ]; then
          SLAVE_THREADS=1
	fi
    	;;
    h)
	usage;
    	;;
    *)
    	echo "Unknown option $option"
    	usage;
	exit 1;
    	;;
    esac
done

if [ -z ${DURATION} ]; then
  echo "Duration not specified"
  usage;
  exit 1;
fi
if [ -z ${SST_METHOD} ]; then
  echo "SST_METHOD not specified"
  usage;
  exit 1;
fi
if [ -z ${WORKDIR} ]; then
  echo "Workdir not specified"
  usage;
  exit 1;
fi
if [ -z ${LUASCRIPTS} ]; then
  echo "Lua script directory not specified"
  usage;
  exit 1;
fi
if [ -z ${NUMBEROFTABLES} ]; then
  echo "Number of tables not specified"
  usage;
  exit 1;
fi
if [ -z ${TABLESIZE} ]; then
  echo "Table size not specified"
  usage;
  exit 1;
fi
if [ -z ${THREADS} ]; then
  echo "Threads not specified"
  usage;
  exit 1;
fi

sysbench_cmd(){
  TEST_TYPE="$1"
  DB="$2"
  if [ "$($SYSBENCH --version | cut -d ' ' -f2 | grep -oe '[0-9]\.[0-9]')" == "0.5" ]; then
    if [ "$TEST_TYPE" == "load_data" ];then
      SYSBENCH_OPTIONS="--test=$LUASCRIPTS/parallel_prepare.lua --oltp-auto-inc=off --mysql-engine-trx=yes --mysql-table-engine=innodb --oltp-table-size=$TABLESIZE --oltp_tables_count=$NUMBEROFTABLES --mysql-db=$DB --mysql-user=root  --num-threads=$THREADS --db-driver=mysql"
    elif [ "$TEST_TYPE" == "oltp" ];then
      SYSBENCH_OPTIONS="--test=$LUASCRIPTS/oltp.lua --oltp-auto-inc=off --mysql-table-engine=innodb --init-rng=on --oltp_index_updates=10 --oltp_non_index_updates=10 --oltp_distinct_ranges=15 --oltp_order_ranges=15 --oltp-table-size=$TABLESIZE --oltp_tables_count=$NUMBEROFTABLES --max-time=$howlong --report-interval=1 --max-requests=0 --mysql-db=$DB --mysql-user=root  --num-threads=$THREADS --db-driver=mysql"
    elif [ "$TEST_TYPE" == "clean" ];then
      SYSBENCH_OPTIONS="--test=$LUASCRIPTS/parallel_prepare.lua --oltp_tables_count=$NUMBEROFTABLES --mysql-db=$DB --mysql-user=root --db-driver=mysql"
    fi
  elif [ "$($SYSBENCH --version | cut -d ' ' -f2 | grep -oe '[0-9]\.[0-9]')" == "1.0" ]; then
    LUASCRIPTS="/usr/share/sysbench"
    if [ "$TEST_TYPE" == "load_data" ];then
      SYSBENCH_OPTIONS="$LUASCRIPTS/oltp_insert.lua --auto_inc=off --mysql_storage_engine=innodb --table-size=$TABLESIZE --tables=$NUMBEROFTABLES --mysql-db=$DB --mysql-user=root  --threads=$THREADS --db-driver=mysql"
    elif [ "$TEST_TYPE" == "oltp" ];then
      SYSBENCH_OPTIONS="$LUASCRIPTS/oltp_read_write.lua --auto_inc=off --mysql_storage_engine=innodb --index_updates=10 --non_index_updates=10 --distinct_ranges=15 --order_ranges=15 --table-size=$TABLESIZE --tables=$NUMBEROFTABLES --mysql-db=$DB --mysql-user=root  --threads=$THREADS --time=$howlong --report-interval=1 --events=0 --db-driver=mysql --db-ps-mode=disable"
    elif [ "$TEST_TYPE" == "clean" ];then
      SYSBENCH_OPTIONS="$LUASCRIPTS/oltp_insert.lua --tables=$NUMBEROFTABLES --mysql-db=$DB --mysql-user=root --db-driver=mysql"
    fi
  else
    echo "Please check sysbench version, currently supported versions : 0.5, 1.0.x"
    exit 1
  fi
}

cd $WORKDIR
mkdir ${BUILD_NUMBER}
cd ${BUILD_NUMBER}
#mv $WORKDIR/Percona-XtraDB-Cluster*.gz .
BUILDDIR=`pwd`

echo "Workdir: $WORKDIR"
echo "Builddir: $BUILDDIR"

# now onwards everything for this run will be hosted in build-directory only.

#
# configure and set path to reference xtrabackup binary.
# mysqldump binary is part of mysql installation and rsync is generally pre-installed
# on most unix system at default location.
#
if [[ $SST_METHOD == "xtrabackup" ]];then
   SST_METHOD="xtrabackup-v2"
fi

#if [[ $SST_METHOD == "xtrabackup-v2" ]];then
#    mv $WORKDIR/percona-xtrabackup*.gz .
#    XB_TAR=`ls -1ct percona-xtrabackup*.tar.gz | head -n1`
#    tar -xf $XB_TAR
#    XB_BASE=`echo $XB_TAR | sed 's/.tar.gz//g'`
#    export PATH="$BUILDDIR/$XB_BASE/bin:$PATH"
#fi

#PXC_TAR=`ls -1ct Percona-XtraDB-Cluster*.tar.gz | head -n1`
#PXC_BASE="$(tar tf $PXC_TAR | head -1 | tr -d '/')"
#tar -xf $PXC_TAR

#-------------------------------------------------------------------------------
#
# Step-3: Helper and main function that does all the work.
#

# archives on completion. backup logs for reference.
archives() {
    cd ../
    tar czf $WORKDIR/results-${BUILD_NUMBER}.tar.gz ./${BUILD_NUMBER}/logs || true
    rm -rf $BUILDDIR
}
mkdir -p $BUILDDIR/logs
trap archives EXIT KILL

# sysbench prepare
prepare() {
    local socket=$1
    local logfile=$2

    echo "Sysbench Run: Prepare stage"
    sysbench_cmd load_data test
    $SYSBENCH $SYSBENCH_OPTIONS --mysql-socket=$socket prepare 2>&1 | tee $logfile || exit 1;
    echo "Seed data created"
}

ver_and_row(){
    local socket=$1

    $MYSQL_BASEDIR/bin/mysql -S $socket -u root -e "show variables like 'log_bin'";
    ITR=1
    DELIM=";"
    #while [ $ITR -lt $NUMBEROFTABLES ]
    while [ $ITR -lt $ITR ]
    do
      QUERY="select count(*) from test.sbtest"$ITR$DELIM
      $MYSQL_BASEDIR/bin/mysql -S $socket -u root -e "$QUERY"
      ITR=`expr $ITR + 1`
    done
    QUERY="select count(*) from test.sbtest"$ITR$DELIM
    $MYSQL_BASEDIR/bin/mysql -S $socket -u root -e "$QUERY"
}

# Read-Write testing workload
rw_workload() {
    local socket=$1
    local logfile=$2
    local howlong=$3

    echo "Sysbench Run: OLTP RW testing"
    sysbench_cmd oltp test
    $SYSBENCH $SYSBENCH_OPTIONS --mysql-socket=$socket run  2>&1 | tee $logfile || exit 1;
}

# DDL workload
ddl_workload()
{
    set -x
    local socket=$1
    local logfile=$2

    if [[ ! -e $LUASCRIPTS/oltp_ddl.lua ]]; then
      if [[ ! -e $BUILDDIR/sysbench/sysbench/tests/db/oltp_ddl.lua ]]; then
        pushd $BUILDDIR
        git clone -n https://github.com/Percona-QA/sysbench.git --depth 1
        cd sysbench
        git checkout HEAD sysbench/tests/db/oltp_ddl.lua
        ln -s $LUASCRIPTS/* sysbench/tests/db/
        popd
      fi
    fi

    echo "Sysbench Run: OLTP DDL testing"
    $SYSBENCH --mysql-table-engine=innodb --num-threads=$THREADS \
              --report-interval=10 --oltp-auto-inc=off \
              --max-time=$DURATION --max-requests=0 \
              --test=$BUILDDIR/sysbench/sysbench/tests/db/oltp_ddl.lua \
              --init-rng=on --oltp_index_updates=10 --oltp_non_index_updates=10 \
              --oltp_distinct_ranges=15 --oltp_order_ranges=15 \
              --oltp_tables_count=$NUMBEROFTABLES --mysql-db=test \
              --oltp_table_size=$TABLESIZE \
              --mysql-user=root --db-driver=mysql --mysql-socket=$socket \
              run 2>&1 | tee $logfile  || exit 1;
   set +x
}

# Cleanup seed db created by sysbench
cleanup()
{
    local socket=$1
    local logfile=$2

    echo "Sysbench Run: Cleanup"
    sysbench_cmd clean test
    $SYSBENCH $SYSBENCH_OPTIONS --mysql-socket=$socket cleanup 2>&1 | tee $logfile || exit 1;
}

MYSQL_BASEDIR="$BUILDDIR/../../../../"
echo "MySQL base-dir $MYSQL_BASEDIR"
export MYSQL_VARDIR="$BUILDDIR/mysqlvardir"
mkdir -p $MYSQL_VARDIR
echo "MySQL data-dir $MYSQL_VARDIR"

ADDR="127.0.0.1"
RPORT=$(( RANDOM%21 + 10 ))

RBASE1="$(( RPORT*1000 ))"
echo "Setting base port for node-1 to $RBASE1"
RADDR1="$ADDR:$(( RBASE1 + 7 ))"
LADDR1="$ADDR:$(( RBASE1 + 8 ))"

RBASE2="$(( RBASE1 + 100 ))"
echo "Setting base port for node-2 to $RBASE2"
RADDR2="$ADDR:$(( RBASE2 + 7 ))"
LADDR2="$ADDR:$(( RBASE2 + 8 ))"

RBASE3="$(( RBASE2 + 100 ))"
echo "Setting base port for node-3 to $RBASE3"
RADDR3="$ADDR:$(( RBASE3 + 7 ))"
LADDR3="$ADDR:$(( RBASE3 + 8 ))"

GARBDBASE="$(( RBASE3 + 100 ))"
GARBDP="$ADDR:$GARBDBASE"
echo "Setting base port for garbd to $GARBDBASE"

SUSER=root
SPASS=

node1="${MYSQL_VARDIR}/node1"
mkdir -p $node1
node2="${MYSQL_VARDIR}/node2"
mkdir -p $node2
node3="${MYSQL_VARDIR}/node3"
mkdir -p $node3

start_node_1()
{
  set +e
  echo "Starting node-1 ...."
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.1 \
        --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node1 \
	--loose-debug-sync-timeout=600 --skip-performance-schema \
	--innodb_file_per_table $1 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
	--wsrep_cluster_address=gcomm:// \
	--wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR1 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--log-error=$BUILDDIR/logs/node1.err \
	--pxc_maint_transition_period=0 \
	--socket=/tmp/n1.sock --log-output=none \
	--port=$RBASE1 --server-id=1 --wsrep_slave_threads=$SLAVE_THREADS \
	--core-file > $BUILDDIR/logs/node1.err 2>&1 &

  echo "Waiting for node-1 to start ...."
  NODE1PID=$!
  while true ; do
    sleep 1

    if egrep -qi  "Synchronized with group, ready for connections" $BUILDDIR/logs/node1.err ; then
     break
    fi

    if egrep -qi "gcomm: closed" $BUILDDIR/logs/node1.err; then
       echo "Failed to start node-1"
       exit 1
    fi

    if [ "${NODE1PID}" == "" ]; then
      echoit "Error! server not started.. Terminating!"
      egrep -i "ERROR|ASSERTION" $BUILDDIR/logs/node1.err
      exit 1
    fi
  done

  echo "Node-1 started"
  set -e
}

start_node_2()
{
  set +e
  echo "Starting node-2 ...."
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.2 \
      --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node2 \
	--loose-debug-sync-timeout=600 --skip-performance-schema \
	--innodb_file_per_table $1 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
        --wsrep_cluster_address=gcomm://$LADDR1,$LADDR3 \
	--wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR2 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--log-error=$BUILDDIR/logs/node2.err \
	--pxc_maint_transition_period=0 \
	--socket=/tmp/n2.sock --log-output=none \
	--port=$RBASE2 --server-id=2 --wsrep_slave_threads=$SLAVE_THREADS \
	--core-file > $BUILDDIR/logs/node2.err 2>&1 &

  echo "Waiting for node-2 to start ...."
  NODE2PID=$!
  while true ; do
    sleep 1

    if egrep -qi  "Synchronized with group, ready for connections" $BUILDDIR/logs/node2.err ; then
     break
    fi

    if egrep -qi "gcomm: closed" $BUILDDIR/logs/node2.err; then
       echo "Failed to start node-2"
       $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n3.sock -u root shutdown
       $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n1.sock -u root shutdown
       exit 1
    fi

    if [ "${NODE2PID}" == "" ]; then
      echoit "Error! server not started.. Terminating!"
      egrep -i "ERROR|ASSERTION" $BUILDDIR/logs/node2.err
      exit 1
    fi
  done

  echo "Node-2 started"
  set -e
}

start_node_3()
{
  set +e
  echo "Starting node-3 ...."
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.3 \
        --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node3 \
	--loose-debug-sync-timeout=600 --skip-performance-schema \
	--innodb_file_per_table $1 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
        --wsrep_cluster_address=gcomm://$LADDR1,$LADDR2 \
	--wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR3 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--log-error=$BUILDDIR/logs/node3.err \
	--pxc_maint_transition_period=0 \
	--socket=/tmp/n3.sock --log-output=none \
	--port=$RBASE3 --server-id=3 --wsrep_slave_threads=$SLAVE_THREADS \
	--core-file > $BUILDDIR/logs/node3.err 2>&1 &

  # ensure that node-3 has started and has joined the group post SST
  echo "Waiting for node-3 to start ....."
  NODE3PID=$!
  while true ; do

    sleep 1

    if egrep -qi  "Synchronized with group, ready for connections" $BUILDDIR/logs/node3.err ; then
     break
    fi

    if egrep -qi "gcomm: closed" $BUILDDIR/logs/node3.err; then
       echo "Failed to start node-3"
       $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n2.sock -u root shutdown
       $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n1.sock -u root shutdown
       exit 1
    fi

    if [ "${NODE3PID}" == "" ]; then
      echoit "Error! server not started.. Terminating!"
      egrep -i "ERROR|ASSERTION" $BUILDDIR/logs/node3.err
      exit 1
    fi
  done

  echo "Node-3 started"
  set -e
}

boot_node1()
{
  set +e
  echo "Creating seed-db for node-1"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=$MYSQL_VARDIR/node1 \
	--socket=/tmp/n1.sock \
	--log-error=$BUILDDIR/logs/node1.err \
	--initialize-insecure 2>&1 || exit 1;
  echo "Done"
  set -e

  start_node_1 $1
}

boot_node2()
{
  set +e
  echo "Creating seed-db for node-2"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=${MYSQL_VARDIR}/node2 \
	--socket=/tmp/n2.sock \
	--log-error=$BUILDDIR/logs/node2.err \
	--initialize-insecure 2>&1 || exit 1;
  echo "Done"
  set -e

  start_node_2 $1
}

boot_node3()
{
  set +e
  echo "Creating seed-db for node-3"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=${MYSQL_VARDIR}/node3 \
	--socket=/tmp/n3.sock \
	--log-error=$BUILDDIR/logs/node3.err \
	--initialize-insecure 2>&1 || exit 1;
  echo "Done"
  set -e

  start_node_3 $1
}

garbd_bootup()
{
  set +e
  echo "Starting garbd"

  ${MYSQL_BASEDIR}/bin/garbd --address gcomm://$LADDR1,$LADDR2,$LADDR3 \
	--group "my_wsrep_cluster" \
	--options "gmcast.listen_addr=tcp://$GARBDP" \
	--log $BUILDDIR/logs/garbd.log 2>&1 &
  GARBDPID=$!

  echo "Waiting for garbd to start"
  sleep 10
  echo "Done"

  set -e
}


sysbench_run()
{
  boot_node1 $1
  boot_node2 $1

  # load data. create seed data.
  $MYSQL_BASEDIR/bin/mysql -S /tmp/n1.sock -u root -e "create database test;" || true
  prepare /tmp/n1.sock $BUILDDIR/logs/sysbench_prepare.txt

  # will need SST for getting the seed-db
  boot_node3 $1

  # validate db before starting RW test.
  echo "Table count pre RW-load"
  ver_and_row /tmp/n1.sock
  ver_and_row /tmp/n2.sock
  ver_and_row /tmp/n3.sock

  # shutdown node-2 so that on re-join it will get IST from node-1/3
  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n2.sock -u root shutdown
  # let's wait for cluster to get the needed updates.
  sleep 2

  # RW workload
  rw_workload "/tmp/n1.sock,/tmp/n3.sock" $BUILDDIR/logs/sysbench_rw_run.txt $DURATION

  # given that node-2 was down for sometime it would need either IST
  start_node_2 $1

  # validate db before starting RW test.
  echo "Table count post RW-load"
  ver_and_row /tmp/n1.sock
  ver_and_row /tmp/n2.sock
  ver_and_row /tmp/n3.sock

  # start garbd to ensure that node-1 doesn't become non-primary with follow-up
  # action below.
  #garbd_bootup

  # shutdown node-2 and kill node-3 so that quorum is lost
  #$MYSQL_BASEDIR/bin/mysql -S /tmp/n1.sock -u root -e "show status like 'wsrep_cluster_status'";
  #$MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n2.sock -u root shutdown
  # let's wait for cluster to probe node-2 and mark it inactive.
  #sleep 10
  #kill -9 $NODE3PID
  # inactive timeout to get rid of node-3 is 15 secs.
  #sleep 20

  #$MYSQL_BASEDIR/bin/mysql -S /tmp/n1.sock -u root -e "show status like 'wsrep_cluster_status'";

  # do so RW-workload while other nodes are down but node-1 is still primary
  # due to garbd
  #rw_workload "/tmp/n1.sock" $BUILDDIR/logs/sysbench_rw_run.txt 30

  # restart node-2
  #start_node_2 $1

  # validate db before starting RW test.
  #echo "Table count post RW-load"
  #ver_and_row /tmp/n1.sock
  #ver_and_row /tmp/n2.sock

  # let's get rid of garbd now that node-2 is up and running
  #kill -9 $GARBDPID
  #sleep 5

  #ddl_workload /tmp/n1.sock $BUILDDIR/logs/sysbench_ddl.txt
  cleanup /tmp/n3.sock $BUILDDIR/logs/sysbench_cleanup.txt

  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n3.sock -u root shutdown
  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n2.sock -u root shutdown
  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n1.sock -u root shutdown
}

#-------------------------------------------------------------------------------
#
# Step-4: Start running sysbench
#

# CASE-1: sysbench with binlog-format=ROW
echo "Running sysbench with binlog-format=ROW"
sysbench_run "--log-bin=mysql-bin --binlog-format=ROW"
mv $node1 ${MYSQL_VARDIR}/with_binlog_node1
mv $node2 ${MYSQL_VARDIR}/with_binlog_node2
mv $node3 ${MYSQL_VARDIR}/with_binlog_node3
mkdir -p $node1
mkdir -p $node2
mkdir -p $node3

# CASE-2: sysbench run without binlog (will use emulation of binlog)
echo "--------------------------------------------------"
echo "Running sysbench with skip-log-bin"
echo "--------------------------------------------------"
sysbench_run --skip-log-bin
mv $node1 ${MYSQL_VARDIR}/without_binlog_node1
mv $node2 ${MYSQL_VARDIR}/without_binlog_node2
mv $node3 ${MYSQL_VARDIR}/without_binlog_node3
mkdir -p $node1
mkdir -p $node2
mkdir -p $node3

#-------------------------------------------------------------------------------
#
# Step-5: Cleanup
#
echo "Done running sysbench"
exit 0;
