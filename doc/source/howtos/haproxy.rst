=============================
 Load balancing with HAProxy
=============================

This section describes how to configure |HAProxy| to work in front of the cluster.

Here is the simple configuration file example for |HAProxy| ::

        # this config needs haproxy-1.4.20

        global 
                log 127.0.0.1   local0
                log 127.0.0.1   local1 notice
                maxconn 4096
                uid 99
                gid 99
                daemon
                #debug
                #quiet

        defaults
                log     global
                mode    http
                option  tcplog
                option  dontlognull
                retries 3
                redispatch
                maxconn 2000
                contimeout      5000
                clitimeout      50000
                srvtimeout      50000

        listen mysql-cluster 0.0.0.0:3306
            mode tcp
            balance roundrobin
            option mysql-check user root

            server db01 10.4.29.100:3306 check
            server db02 10.4.29.99:3306 check
            server db03 10.4.29.98:3306 check

With this configuration |HAProxy| will load balance between three nodes. In this case it only checks if mysqld listens on port 3306, but it doesn't take into an account state of the node. So it could be sending queries to the node that has mysqld running even if it's in "JOINING" or "DISCONNECTED" state.  

To check the current status of a node we need a more complex checks. This idea was taken from `codership-team google groups <https://groups.google.com/group/codership-team/browse_thread/thread/44ee59c8b9c458aa/98b47d41125cfae6>`_.

To implement this setup you will need two scripts: 

  *  **clustercheck** (place to /usr/local/bin) and a config for xinetd and 
  *  **mysqlchk** (place to /etc/xinetd.d) on each node. 

Both scripts are available in binaries and source distributions of |Percona XtraDB Cluster|.

You'll need to change /etc/services file by adding the following line on each node: :: 

        mysqlchk        9200/tcp                # mysqlchk

The configuration file for |HAProxy| in this case may look like this: ::

        # this config needs haproxy-1.4.20

        global
                log 127.0.0.1   local0
                log 127.0.0.1   local1 notice
                maxconn 4096
                uid 99
                gid 99
                #daemon
                debug
                #quiet

        defaults
                log     global
                mode    http
                option  tcplog
                option  dontlognull
                retries 3
                redispatch
                maxconn 2000
                contimeout      5000
                clitimeout      50000
                srvtimeout      50000

        listen mysql-cluster 0.0.0.0:3306
            mode tcp
            balance roundrobin
            option  httpchk

            server db01 10.4.29.100:3306 check port 9200 inter 12000 rise 3 fall 3 
            server db02 10.4.29.99:3306 check port 9200 inter 12000 rise 3 fall 3 
            server db03 10.4.29.98:3306 check port 9200 inter 12000 rise 3 fall 3 

