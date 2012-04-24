#!/bin/bash 
# put this file to /usr/local/bin
#
# This script checks if a mysql server is healthy running on localhost. It will 
# return: 
# 
# "HTTP/1.x 200 OK\r" (if mysql is running smoothly) 
# 
# - OR - 
# 
# "HTTP/1.x 500 Internal Server Error\r" (else) 
# 
# The purpose of this script is make haproxy capable of monitoring mysql properly 
# 
# Author: Unai Rodriguez 
# 
# It is recommended that a user with limited privileges is created to be used by 
# this script. Something like this: 
# 
# mysql> GRANT PROCESS on mysql.* TO 'clustercheckuser'@'localhost' \ 
#     -> IDENTIFIED BY 'clustercheckpassword!' WITH GRANT OPTION; 
# mysql> flush privileges; 

MYSQL_HOST="localhost" 
MYSQL_PORT="3306" 
MYSQL_USERNAME="clustercheckuser" 
MYSQL_PASSWORD="clustercheckpassword!" 
TMP_FILE="/tmp/clustercheck.out" 
ERR_FILE="/tmp/clustercheck.err" 

# 
# We perform a simple query that should return a few results :-p 
# 

WSSREP_STATUS=`mysql --host=$MYSQL_HOST --user=$MYSQL_USERNAME --password=$MYSQL_PASSWORD -e "show status like 'wsrep_local_state';" | awk '{if (NR!=1){print $2}}' 2>/dev/null` 
 
if [ "$WSSREP_STATUS" == "4" ] 
then 
    # mysql is fine, return http 200 
    /bin/echo -e "HTTP/1.1 200 OK\r\n" 
    /bin/echo -e "Content-Type: Content-Type: text/plain\r\n" 
    /bin/echo -e "\r\n" 
    /bin/echo -e "Node is running.\r\n" 
    /bin/echo -e "\r\n" 
else 
    # mysql is fine, return http 503 
    /bin/echo -e "HTTP/1.1 503 Service Unavailable\r\n" 
    /bin/echo -e "Content-Type: Content-Type: text/plain\r\n" 
    /bin/echo -e "\r\n" 
    /bin/echo -e "Node is *down*.\r\n" 
    /bin/echo -e "\r\n" 
fi 
