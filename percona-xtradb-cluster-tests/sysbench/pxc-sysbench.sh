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
pgrep -f mysqld
pkill -f mysqld
sleep 10
pgrep mysqld || pkill -9 -f mysqld
set -e
sleep 5

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

SYSBENCH=`which sysbench` || (echo "Can't locate sysbench" && exit 1)
echo "Using $SYSBENCH"
XB_VER=2.4.1

usage() {
    echo "-d <insecs> -s <rsync|mysqldump|xtrabackup> -w <pxc/pxb tar directory> -l <sysbench lua script folder> -n <number-of-table> -x <each-table-size> -t <num-of-threads>"
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

cd $WORKDIR
mkdir ${BUILD_NUMBER}
cd ${BUILD_NUMBER}
mv $WORKDIR/Percona-XtraDB-Cluster*.gz .
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
    mv $WORKDIR/percona-xtrabackup*.gz .
    XB_TAR=`ls -1ct percona-xtrabackup*.tar.gz | head -n1`
    tar -xf $XB_TAR
    XB_BASE="percona-xtrabackup-${XB_VER}-Linux-x86_64"
    export PATH="$WORKDIR/$XB_BASE/bin:$PATH"
fi

PXC_TAR=`ls -1ct Percona-XtraDB-Cluster*.tar.gz | head -n1`
PXC_BASE="$(tar tf $PXC_TAR | head -1 | tr -d '/')"
tar -xf $PXC_TAR

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
    $SYSBENCH --test=$LUASCRIPTS/parallel_prepare.lua --report-interval=10 \
              --oltp-auto-inc=1 --mysql-engine-trx=yes \
              --mysql-table-engine=innodb \
              --oltp-table-size=$TABLESIZE --oltp_tables_count=$NUMBEROFTABLES \
              --mysql-db=test --mysql-user=root  --num-threads=$THREADS \
              --db-driver=mysql --mysql-socket=$socket \
              prepare 2>&1 | tee $logfile || exit 1;
    echo "Seed data created"
}

ver_and_row(){
    local socket=$1

    $MYSQL_BASEDIR/bin/mysql -S $socket -u root -e "show global variables like 'version';"
    $MYSQL_BASEDIR/bin/mysql -S $socket -u root -e "select count(*) from test.sbtest1;"
}

# Read-Write testing workload
rw_workload() {
    local socket=$1
    local logfile=$2

    echo "Sysbench Run: OLTP RW testing"
    $SYSBENCH --mysql-table-engine=innodb --num-threads=$THREADS \
              --report-interval=10 --oltp-auto-inc=1 \
              --max-time=$DURATION --max-requests=0 \
              --test=$LUASCRIPTS/oltp.lua \
              --init-rng=on --oltp_index_updates=10 \
              --oltp_non_index_updates=10 --oltp_distinct_ranges=15 \
              --oltp_order_ranges=15 --oltp_tables_count=$NUMBEROFTABLES \
              --mysql-db=test --mysql-user=root --db-driver=mysql \
              --mysql-socket=$socket run  2>&1 | tee $logfile || exit 1;
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
              --report-interval=10 --oltp-auto-inc=1 \
              --max-time=$DURATION --max-requests=0 \
              --test=$BUILDDIR/sysbench/sysbench/tests/db/oltp_ddl.lua \
              --init-rng=on --oltp_index_updates=10 --oltp_non_index_updates=10 \
              --oltp_distinct_ranges=15 --oltp_order_ranges=15 \
              --oltp_tables_count=$NUMBEROFTABLES --mysql-db=test \
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
    $SYSBENCH --test=$LUASCRIPTS/parallel_prepare.lua  \
        --oltp_tables_count=$NUMBEROFTABLES --mysql-db=test --mysql-user=root  \
        --db-driver=mysql --mysql-socket=$socket cleanup 2>&1 | tee $logfile
}

MYSQL_BASEDIR="$BUILDDIR/$PXC_BASE"
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

SUSER=root
SPASS=

node1="${MYSQL_VARDIR}/node1"
mkdir -p $node1
node2="${MYSQL_VARDIR}/node2"
mkdir -p $node2
node3="${MYSQL_VARDIR}/node3"
mkdir -p $node3

sysbench_run()
{
  set +e

  echo "Starting PXC node1"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=$MYSQL_VARDIR/node1 \
	--socket=/tmp/n1.sock \
	--log-error=$BUILDDIR/logs/node1.err \
	--skip-grant-tables --initialize 2>&1 || exit 1;

  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.1 \
        --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node1 \
	--loose-debug-sync-timeout=600 --default-storage-engine=InnoDB \
	--default-tmp-storage-engine=InnoDB --skip-performance-schema \
	--innodb_file_per_table $1 --wsrep-slave-threads=2 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
	--wsrep_cluster_address=gcomm:// \
	--wsrep_sst_receive_address=$RADDR1 --wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR1 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--skip-name-resolve --log-error=$BUILDDIR/logs/node1.err \
	--socket=/tmp/n1.sock --log-output=none \
	--port=$RBASE1 --skip-grant-tables \
	--core-file > $BUILDDIR/logs/node1.err 2>&1 &

  echo "Waiting for node-1 to start ....."
  sleep 10

  echo "Starting PXC node2"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=${MYSQL_VARDIR}/node2 \
	--socket=/tmp/n2.sock \
	--log-error=$BUILDDIR/logs/node2.err \
	--skip-grant-tables --initialize 2>&1 || exit 1;

  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.2 \
        --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node2 \
	--loose-debug-sync-timeout=600 --default-storage-engine=InnoDB \
	--default-tmp-storage-engine=InnoDB --skip-performance-schema \
	--innodb_file_per_table $1 --wsrep-slave-threads=2 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
        --wsrep_cluster_address=gcomm://$LADDR1,gcomm://$LADDR3 \
	--wsrep_sst_receive_address=$RADDR2 --wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR2 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--skip-name-resolve --log-error=$BUILDDIR/logs/node2.err \
	--socket=/tmp/n2.sock --log-output=none \
	--port=$RBASE2 --skip-grant-tables \
	--core-file > $BUILDDIR/logs/node2.err 2>&1 &

  echo "Waiting for node-2 to start ....."
  sleep 10

  set -e

  # load data. create seed data.
  $MYSQL_BASEDIR/bin/mysql -S /tmp/n1.sock -u root -e "create database test;" || true
  prepare /tmp/n1.sock $BUILDDIR/logs/sysbench_prepare.txt

  set +e
  echo "Starting PXC node3"
  # this will get SST.
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --basedir=${MYSQL_BASEDIR} \
	--datadir=${MYSQL_VARDIR}/node3 \
	--socket=/tmp/n3.sock \
	--log-error=$BUILDDIR/logs/node3.err \
	--skip-grant-tables --initialize 2>&1 || exit 1;

  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.3 \
        --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node3 \
	--loose-debug-sync-timeout=600 --default-storage-engine=InnoDB \
	--default-tmp-storage-engine=InnoDB --skip-performance-schema \
	--innodb_file_per_table $1 --wsrep-slave-threads=2 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
        --wsrep_cluster_address=gcomm://$LADDR1,gcomm://$LADDR2 \
	--wsrep_sst_receive_address=$RADDR3 --wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR3 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--skip-name-resolve --log-error=$BUILDDIR/logs/node3.err \
	--socket=/tmp/n3.sock --log-output=none \
	--port=$RBASE3 --skip-grant-tables \
	--core-file > $BUILDDIR/logs/node3.err 2>&1 &

  # let's wait for server(s) to start
  echo "Waiting for node-3 to start ....."
  sleep 10

  set -e

  # validate db before starting RW test.
  echo "Table count pre RW-load"
  ver_and_row /tmp/n1.sock
  ver_and_row /tmp/n2.sock
  ver_and_row /tmp/n3.sock

  # shutdown node-2 so that on re-join it will get IST from node-1/3
  ${MYSQL_BASEDIR}/bin/mysqladmin -uroot -S/tmp/n2.sock shutdown > /dev/null 2>&1
  echo "Waiting for node-2 to shutdown....."
  sleep 10
  rw_workload "/tmp/n1.sock,/tmp/n3.sock" $BUILDDIR/logs/sysbench_rw_run.txt

  echo "Restarting node-2"
  ${MYSQL_BASEDIR}/bin/mysqld --no-defaults --defaults-group-suffix=.2 \
      --basedir=${MYSQL_BASEDIR} --datadir=$MYSQL_VARDIR/node2 \
	--loose-debug-sync-timeout=600 --default-storage-engine=InnoDB \
	--default-tmp-storage-engine=InnoDB --skip-performance-schema \
	--innodb_file_per_table $1 --wsrep-slave-threads=2 \
	--innodb_autoinc_lock_mode=2 --innodb_locks_unsafe_for_binlog=1 \
	--wsrep-provider=${MYSQL_BASEDIR}/lib/libgalera_smm.so \
        --wsrep_cluster_address=gcomm://$LADDR1,gcomm://$LADDR3 \
	--wsrep_sst_receive_address=$RADDR2 --wsrep_node_incoming_address=$ADDR \
	--wsrep_provider_options=gmcast.listen_addr=tcp://$LADDR2 \
	--wsrep_sst_method=$SST_METHOD --wsrep_sst_auth=$SUSER:$SPASS \
	--wsrep_node_address=$ADDR --innodb_flush_method=O_DIRECT \
	--core-file --loose-new --sql-mode=no_engine_substitution \
	--loose-innodb --secure-file-priv= --loose-innodb-status-file=1 \
	--skip-name-resolve --log-error=$BUILDDIR/logs/node2.err \
	--socket=/tmp/n2.sock --log-output=none \
	--port=$RBASE2 --skip-grant-tables \
	--core-file > $BUILDDIR/logs/node2.err 2>&1 &

  echo "Waiting for node-2 to start ....."

  # ensure that node-2 has restarted and has joined the group post IST
  MPID="$!"
  while true ; do
    sleep 10
    if egrep -qi  "Synchronized with group, ready for connections" $BUILDDIR/logs/node2.err ; then
     break
    fi
    if [ "${MPID}" == "" ]; then
      echoit "Error! server not started.. Terminating!"
      egrep -i "ERROR|ASSERTION" $BUILDDIR/logs/node2.err
      exit 1
    fi
  done

  # validate db before starting RW test.
  echo "Table count post RW-load"
  ver_and_row /tmp/n1.sock
  ver_and_row /tmp/n2.sock
  ver_and_row /tmp/n3.sock

  #ddl_workload /tmp/n1.sock $BUILDDIR/logs/sysbench_ddl.txt
  cleanup /tmp/n3.sock $BUILDDIR/logs/sysbench_cleanup.txt

  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n1.sock -u root shutdown
  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n2.sock -u root shutdown
  $MYSQL_BASEDIR/bin/mysqladmin --socket=/tmp/n3.sock -u root shutdown
}

#-------------------------------------------------------------------------------
#
# Step-4: Start running sysbench
#

# CASE-1: sysbench with binlog-format=ROW
echo "Running sysbench with binlog-format=ROW"
sysbench_run --binlog-format=ROW
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
