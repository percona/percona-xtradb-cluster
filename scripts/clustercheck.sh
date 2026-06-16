#!/bin/bash 
#
# Script to make a proxy (ie HAProxy) capable of monitoring Percona XtraDB Cluster nodes properly
#
# Authors:
# Raghavendra Prabhu <raghavendra.prabhu@percona.com>
# Olaf van Zandwijk <olaf.vanzandwijk@nedap.com>
#
# Based on the original script from Unai Rodriguez and Olaf (https://github.com/olafz/percona-clustercheck)
#
# Grant privileges required:
# GRANT PROCESS ON *.* TO 'clustercheckuser'@'localhost' IDENTIFIED BY 'clustercheckpassword!';

if [[ $1 == '-h' || $1 == '--help' ]];then
    echo "Usage:"
    echo "  $0 <user> <pass> <available_when_donor=0|1> <log_file> <available_when_readonly=0|1> <defaults_extra_file>"
    echo "      Pass credentials directly on the command line. Default form, compatible"
    echo "      with existing operator scripts."
    echo ""
    echo "  $0 - <available_when_donor=0|1> <log_file> <available_when_readonly=0|1> <defaults_extra_file>"
    echo "      Use '-' in place of <user> <pass> to keep credentials off the command"
    echo "      line. The mysql client reads [client] user= and password= from"
    echo "      <defaults_extra_file> (typically /etc/mysql/clustercheck.cnf, 0640"
    echo "      root:mysql). Used by the systemd socket-activated clustercheck@.service"
    echo "      shipped with PXC; see /usr/share/mysql/clustercheck.cnf.example."
    exit
fi

# Leading "-" omits --user/--password so mysql reads credentials only from defaults_extra_file.
if [[ "$1" == "-" ]]; then
  shift
  MYSQL_USERNAME=""
  MYSQL_PASSWORD=""
  AVAILABLE_WHEN_DONOR=${1:-0}
  ERR_FILE="${2:-/dev/null}"
  AVAILABLE_WHEN_READONLY=${3:-1}
  DEFAULTS_EXTRA_FILE=${4:-/etc/my.cnf}
else
  MYSQL_USERNAME="${1-clustercheckuser}"
  MYSQL_PASSWORD="${2-clustercheckpassword!}"
  AVAILABLE_WHEN_DONOR=${3:-0}
  ERR_FILE="${4:-/dev/null}"
  AVAILABLE_WHEN_READONLY=${5:-1}
  DEFAULTS_EXTRA_FILE=${6:-/etc/my.cnf}
fi

# if ERR_FILE cannot actually be opened for append, fall back to
# /dev/null. Otherwise the very first 'mysql ... 2>${ERR_FILE}' below would
# fail on the redirect (e.g. ENXIO) before mysql even runs, leaving the
# status array empty and the node falsely reported as down.
#
# The case is /proc/self/fd/2 when stderr is a systemd journal
# stream socket: opening that path returns ENXIO and the redirect aborts the
# command. Any unwritable path (read-only fs, missing parent dir, denied perm)
# would do the same, so we test once here and substitute /dev/null.
if ! ( : >> "$ERR_FILE" ) 2>/dev/null; then
    ERR_FILE=/dev/null
fi

#Timeout exists for instances where mysqld may be hung
TIMEOUT=10

EXTRA_ARGS=""
if [[ -n "$MYSQL_USERNAME" ]]; then
    EXTRA_ARGS="$EXTRA_ARGS --user=${MYSQL_USERNAME}"
fi
if [[ -n "$MYSQL_PASSWORD" ]]; then
    EXTRA_ARGS="$EXTRA_ARGS --password=${MYSQL_PASSWORD}"
fi
if [[ -r $DEFAULTS_EXTRA_FILE ]];then 
    MYSQL_CMDLINE="mysql --defaults-extra-file=$DEFAULTS_EXTRA_FILE -nNE --connect-timeout=$TIMEOUT \
                    ${EXTRA_ARGS}"
else 
    MYSQL_CMDLINE="mysql -nNE --connect-timeout=$TIMEOUT ${EXTRA_ARGS}"
fi
#
# Perform the query to check the wsrep_local_state
#
PXC_NODE_STATUS=($($MYSQL_CMDLINE -e "SHOW STATUS LIKE 'wsrep_local_state';SHOW VARIABLES LIKE 'pxc_maint_mode';SHOW GLOBAL STATUS LIKE 'wsrep_cluster_status';" \
     2>${ERR_FILE} | grep -A 1 -E 'wsrep_local_state$|pxc_maint_mode$|wsrep_cluster_status$' | sed -n -e '2p' -e '5p' -e '8p' | tr '\n' ' '))

# ${PXC_NODE_STATUS[0]} - wsrep_local_state
# ${PXC_NODE_STATUS[1]} - pxc_maint_mode
# ${PXC_NODE_STATUS[2]} - wsrep_cluster_status

if [[ ${PXC_NODE_STATUS[2]} == 'Primary' &&  ( ${PXC_NODE_STATUS[0]} -eq 4 || \
    ( ${PXC_NODE_STATUS[0]} -eq 2 && ${AVAILABLE_WHEN_DONOR} -eq 1 ) ) \
    && ${PXC_NODE_STATUS[1]} == 'DISABLED' ]];
then 

    # Check only when set to 0 to avoid latency in response.
    if [[ $AVAILABLE_WHEN_READONLY -eq 0 ]];then
        READ_ONLY=$($MYSQL_CMDLINE -e "SHOW GLOBAL VARIABLES LIKE 'read_only';" \
                    2>${ERR_FILE} | tail -1 2>>${ERR_FILE})

        if [[ "${READ_ONLY}" == "ON" ]];then 
            # Percona XtraDB Cluster node local state is 'Synced', but it is in
            # read-only mode. The variable AVAILABLE_WHEN_READONLY is set to 0.
            # => return HTTP 503
            # Shell return-code is 1
            echo -en "HTTP/1.1 503 Service Unavailable\r\n" 
            echo -en "Content-Type: text/plain\r\n" 
            echo -en "Connection: close\r\n" 
            echo -en "Content-Length: 43\r\n" 
            echo -en "\r\n" 
            echo -en "Percona XtraDB Cluster Node is read-only.\r\n" 
            sleep 0.1
            exit 1
        fi

    fi
    # Percona XtraDB Cluster node local state is 'Synced' => return HTTP 200
    # Shell return-code is 0
    echo -en "HTTP/1.1 200 OK\r\n" 
    echo -en "Content-Type: text/plain\r\n" 
    echo -en "Connection: close\r\n" 
    echo -en "Content-Length: 40\r\n" 
    echo -en "\r\n" 
    echo -en "Percona XtraDB Cluster Node is synced.\r\n" 
    sleep 0.1
    exit 0
else 
    # Percona XtraDB Cluster node local state is not 'Synced' => return HTTP 503
    # Shell return-code is 1
    echo -en "HTTP/1.1 503 Service Unavailable\r\n" 
    echo -en "Content-Type: text/plain\r\n" 
    echo -en "Connection: close\r\n" 
    echo -en "Content-Length: 57\r\n" 
    echo -en "\r\n" 
    echo -en "Percona XtraDB Cluster Node is not synced or non-PRIM. \r\n" 
    sleep 0.1
    exit 1
fi 
