How to setup 3 node cluster on single box
==========================================

This example shows how to setup 3-node cluster on the single physical box. Assume you installed |Percona XtraDB Cluster| from binary .tar.gz into directory :: 

/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64

To start the cluster with three nodes, three :file:`my.cnf` mysql configuration files should be created with three separate data directories.

For this example we created (see the content of files at the end of document):

 * /etc/my.4000.cnf
 * /etc/my.5000.cnf
 * /etc/my.6000.cnf

and data directories:

 * /data/bench/d1
 * /data/bench/d2
 * /data/bench/d3

In this example local IP address is 192.168.2.21

Then we should be able to start initial node as (from directory /usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64): ::

        bin/mysqld_safe --defaults-file=/etc/my.4000.cnf

Following output will let out know that node was started successfully: ::

        111215 19:01:49 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 0)
        111215 19:01:49 [Note] WSREP: New cluster view: global state: 4c286ccc-2792-11e1-0800-94bd91e32efa:0, view# 1: Primary, number of nodes: 1, my index: 0, protocol version 1

And you can check used ports: ::
        
        netstat -anp | grep mysqld
        tcp        0      0 192.168.2.21:4030           0.0.0.0:*                   LISTEN      21895/mysqld        
        tcp        0      0 0.0.0.0:4000                0.0.0.0:*                   LISTEN      21895/mysqld 


After first node, we start second and third: ::

        bin/mysqld_safe --defaults-file=/etc/my.5000.cnf
        bin/mysqld_safe --defaults-file=/etc/my.6000.cnf

Successful start will produce the following output: ::

        111215 19:22:26 [Note] WSREP: Shifting JOINER -> JOINED (TO: 2)
        111215 19:22:26 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 2)
        111215 19:22:26 [Note] WSREP: Synchronized with group, ready for connections

Cluster size can be checked with the: :: 

        mysql -h127.0.0.1 -P6000 -e "show global status like 'wsrep_cluster_size';"
        +--------------------+-------+
        | Variable_name      | Value |
        +--------------------+-------+
        | wsrep_cluster_size | 3     |
        +--------------------+-------+

Now you can connect to any node and create database, which will be automatically propagated to other nodes: ::
        
        mysql -h127.0.0.1 -P5000 -e "CREATE DATABASE hello_peter"

In this example variable :variable:`wsrep_urls` is being used instead of :variable:`wsrep_cluster_address`. With this configuration, node will first try to reach a cluster on port 4030, if there is no cluster node, then it will try on port 5030 and then 6030. If no nodes are up, it will start a new cluster. Variable :variable:`wsrep_urls` goes into the [mysql_safe] section so it's important that the mysql server instance is started with the `/bin/mysql_safe` and not `bin/mysqld`.

Configuration files (/etc/my.4000.cnf): ::

  /etc/my.4000.cnf

  [mysqld_safe]
  wsrep_urls=gcomm://192.168.2.21:4030,gcomm://192.168.2.21:5030,gcomm://192.168.2.21:6030,gcomm://

  [mysqld]
  port = 4000
  socket=/tmp/mysql.4000.sock
  datadir=/data/bench/d1
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64
  user=mysql
  log_error=error.log
  binlog_format=ROW
  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64/lib/libgalera_smm.so
  wsrep_sst_receive_address=192.168.2.21:4020
  wsrep_node_incoming_address=192.168.2.21 
  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_provider_options = "gmcast.listen_addr=tcp://192.168.2.21:4030;"
  wsrep_sst_method=rsync
  wsrep_node_name=node4000
  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2


Configuration files (/etc/my.5000.cnf): ::

  /etc/my.5000.cnf

  [mysqld_safe]
  wsrep_urls=gcomm://192.168.2.21:4030,gcomm://192.168.2.21:5030,gcomm://192.168.2.21:6030,gcomm://

  [mysqld]
  port = 5000
  socket=/tmp/mysql.5000.sock
  datadir=/data/bench/d2
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64
  user=mysql
  log_error=error.log
  binlog_format=ROW
  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64/lib/libgalera_smm.so
  wsrep_sst_receive_address=192.168.2.21:5020
  wsrep_node_incoming_address=192.168.2.21 
  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_provider_options = "gmcast.listen_addr=tcp://192.168.2.21:5030;"
  wsrep_sst_method=rsync
  wsrep_node_name=node5000
  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

Configuration files (/etc/my.6000.cnf): ::

  /etc/my.6000.cnf

  [mysqld_safe]
  wsrep_urls=gcomm://192.168.2.21:4030,gcomm://192.168.2.21:5030,gcomm://192.168.2.21:6030,gcomm://

  [mysqld]
  port = 6000
  socket=/tmp/mysql.6000.sock
  datadir=/data/bench/d3
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64
  user=mysql
  log_error=error.log
  binlog_format=ROW
  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.24-23.6.342.Linux.x86_64/lib/libgalera_smm.so
  wsrep_sst_receive_address=192.168.2.21:6020
  wsrep_node_incoming_address=192.168.2.21 
  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_provider_options = "gmcast.listen_addr=tcp://192.168.2.21:6030;"
  wsrep_sst_method=rsync
  wsrep_node_name=node6000
  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2
