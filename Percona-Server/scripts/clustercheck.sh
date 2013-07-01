#!/bin/bash 
#
# Script to make a proxy (ie HAProxy) capable of monitoring Percona XtraDB Cluster nodes properly
#
# Author: Olaf van Zandwijk <olaf.vanzandwijk@nedap.com>
# Documentation and download: https://github.com/olafz/percona-clustercheck
#
# Based on the original script from Unai Rodriguez 
#

if [[ $1 == '-h' || $1 == '--help' ]];then
    echo "Usage: $0 <user> <pass> <available_when_donor=0|1> <log_file>"
    exit
fi

MYSQL_USERNAME="${1-clustercheckuser}" 
MYSQL_PASSWORD="${2-clustercheckpassword!}" 
AVAILABLE_WHEN_DONOR=${3:-0}
ERR_FILE="${4:-/dev/null}" 
#Timeout exists for instances where mysqld may be hung
TIMEOUT=10

#
# Perform the query to check the wsrep_local_state
#
WSREP_STATUS=`mysql -nNE --connect-timeout=$TIMEOUT --user=${MYSQL_USERNAME} --password=${MYSQL_PASSWORD} \
-e "SHOW STATUS LIKE 'wsrep_local_state';" 2>${ERR_FILE} | tail -1 2>>${ERR_FILE}` 
 
if [[ "${WSREP_STATUS}" == "4" ]] || [[ "${WSREP_STATUS}" == "2" && ${AVAILABLE_WHEN_DONOR} == 1 ]]
then 
    # Percona XtraDB Cluster node local state is 'Synced' => return HTTP 200
    # Shell return-code is 0
    echo -en "HTTP/1.1 200 OK\r\n" 
    echo -en "Content-Type: text/plain\r\n" 
    echo -en "Connection: close\r\n" 
    echo -en "Content-Length: 40\r\n" 
    echo -en "\r\n" 
    echo -en "Percona XtraDB Cluster Node is synced.\r\n" 
    exit 0
else 
    # Percona XtraDB Cluster node local state is not 'Synced' => return HTTP 503
    # Shell return-code is 1
    echo -en "HTTP/1.1 503 Service Unavailable\r\n" 
    echo -en "Content-Type: text/plain\r\n" 
    echo -en "Connection: close\r\n" 
    echo -en "Content-Length: 44\r\n" 
    echo -en "\r\n" 
    echo -en "Percona XtraDB Cluster Node is not synced.\r\n" 
    exit 1
fi 
