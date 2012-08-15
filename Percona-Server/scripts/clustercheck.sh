#!/bin/bash 
#
# Script to make a proxy (ie HAProxy) capable of monitoring Percona XtraDB Cluster nodes properly
#
# Author: Olaf van Zandwijk <olaf.vanzandwijk@nedap.com>
# Documentation and download: https://github.com/olafz/percona-clustercheck
#
# Based on the original script from Unai Rodriguez 
#

MYSQL_USERNAME="clustercheckuser" 
MYSQL_PASSWORD="clustercheckpassword!" 
ERR_FILE="/dev/null" 
AVAILABLE_WHEN_DONOR=0

#
# Perform the query to check the wsrep_local_state
#
WSREP_STATUS=`mysql --user=${MYSQL_USERNAME} --password=${MYSQL_PASSWORD} -e "SHOW STATUS LIKE 'wsrep_local_state';" 2>${ERR_FILE} | awk '{if (NR!=1){print $2}}' 2>${ERR_FILE}` 
 
if [[ "${WSREP_STATUS}" == "4" ]] || [[ "${WSREP_STATUS}" == "2" && ${AVAILABLE_WHEN_DONOR} == 1 ]]
then 
    # Percona XtraDB Cluster node local state is 'Synced' => return HTTP 200
    /bin/echo -en "HTTP/1.1 200 OK\r\n" 
    /bin/echo -en "Content-Type: text/plain\r\n" 
    /bin/echo -en "\r\n" 
    /bin/echo -en "Percona XtraDB Cluster Node is synced.\r\n" 
    /bin/echo -en "\r\n" 
else 
    # Percona XtraDB Cluster node local state is not 'Synced' => return HTTP 503
    /bin/echo -en "HTTP/1.1 503 Service Unavailable\r\n" 
    /bin/echo -en "Content-Type: text/plain\r\n" 
    /bin/echo -en "\r\n" 
    /bin/echo -en "Percona XtraDB Cluster Node is not synced.\r\n" 
    /bin/echo -en "\r\n" 
fi 
