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
        #
        # Passing arguments to clustercheck
        # <user> <pass> <available_when_donor=0|1> <log_file> <available_when_readonly=0|1> <defaults_extra_file>"
        # Recommended: server_args   = user pass 1 /var/log/log-file 0 /etc/my.cnf.local"
        # Compatibility: server_args = user pass 1 /var/log/log-file 1 /etc/my.cnf.local"
        # 55-to-56 upgrade: server_args = user pass 1 /var/log/log-file 0 /etc/my.cnf.extra"
        #
        # recommended to put the IPs that need 
        # to connect exclusively (security purposes) 
        per_source      = UNLIMITED 
} 
