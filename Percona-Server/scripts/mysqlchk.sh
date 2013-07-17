# default: on 
# description: mysqlchk 
service mysqlchk 
{ 
# this is a config for xinetd, place it in /etc/xinetd.d/
        disable = no 
        flags           = REUSE 
        socket_type     = stream 
        port            = 9200 
        wait            = no 
        user            = nobody 
        server          = /usr/bin/clustercheck
        log_on_failure  += USERID 
        only_from       = 0.0.0.0/0 
        # recommended to put the IPs that need 
        # to connect exclusively (security purposes) 
        per_source      = UNLIMITED 
} 
