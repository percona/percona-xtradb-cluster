How to setup 3 node cluster on single box
==========================================

This is how to setup 3-node cluster on the single physical box.

Assume you installed |Percona XtraDB Cluster| from binary .tar.gz into directory

/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6


Now we need to create couple my.cnf files and couple of data directories.

Assume we created (see the content of files at the end of document):

 * /etc/my.4000.cnf
 * /etc/my.5000.cnf
 * /etc/my.6000.cnf

and data directories:

 * /data/bench/d1
 * /data/bench/d2
 * /data/bench/d3

and assume the local IP address is 10.11.12.205.

Then we should be able to start initial node as (from directory /usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6): ::

        bin/mysqld --defaults-file=/etc/my.4000.cnf

Following output will let out know that node was started successfully: ::

        111215 19:01:49 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 0)
        111215 19:01:49 [Note] WSREP: New cluster view: global state: 4c286ccc-2792-11e1-0800-94bd91e32efa:0, view# 1: Primary, number of nodes: 1, my index: 0, protocol version 1


And you can check used ports: ::
        
        netstat -anp | grep mysqld
        tcp        0      0 0.0.0.0:4000                0.0.0.0:*                   LISTEN      8218/mysqld         
        tcp        0      0 0.0.0.0:4010                0.0.0.0:*                   LISTEN      8218/mysqld


After first node, we start second and third: ::

        bin/mysqld --defaults-file=/etc/my.5000.cnf
        bin/mysqld --defaults-file=/etc/my.6000.cnf

Successful start will produce the following output: ::

        111215 19:22:26 [Note] WSREP: Shifting JOINER -> JOINED (TO: 2)
        111215 19:22:26 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 2)
        111215 19:22:26 [Note] WSREP: Synchronized with group, ready for connections


Now you can connect to any node and create database, which will be automatically propagated to other nodes: ::
        
        mysql -h127.0.0.1 -P5000 -e "CREATE DATABASE hello_peter"


Configuration files (/etc/my.4000.cnf): ::

  /etc/my.4000.cnf

  [mysqld]
  gdb

  datadir=/data/bench/d1
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6

  port = 4000
  socket=/tmp/mysql.4000.sock

  user=root

  binlog_format=ROW

  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6/lib/libgalera_smm.so

  wsrep_cluster_address=gcomm://

  wsrep_provider_options = "gmcast.listen_addr=tcp://0.0.0.0:4010; "
  wsrep_sst_receive_address=10.11.12.205:4020

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node4000

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

First node in the cluster has no IP parameter in wsrep_cluster_address because at that point in time there aren't any other servers in the cluster that this node can be connected to. Once you have a cluster running and you want to add/reconnect another node to it, you then supply an address of one of the cluster members in the cluster address.

Configuration files (/etc/my.5000.cnf). PLEASE see the difference in *wsrep_cluster_address*: ::

  /etc/my.5000.cnf
  [mysqld]
  gdb

  datadir=/data/bench/d2
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6

  port = 5000
  socket=/tmp/mysql.5000.sock

  user=root

  binlog_format=ROW

  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6/lib/libgalera_smm.so

  wsrep_cluster_address=gcomm://10.11.12.205:4010

  wsrep_provider_options = "gmcast.listen_addr=tcp://0.0.0.0:5010; "
  wsrep_sst_receive_address=10.11.12.205:5020

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node5000

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2


Configuration files (/etc/my.6000.cnf). PLEASE see the difference in *wsrep_cluster_address*: ::

  /etc/my.6000.cnf

  [mysqld]
  gdb

  datadir=/data/bench/d3
  basedir=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6

  port = 6000
  socket=/tmp/mysql.6000.sock

  user=root

  binlog_format=ROW

  wsrep_provider=/usr/local/Percona-XtraDB-Cluster-5.5.17-22.1-3673.Linux.x86_64.rhel6/lib/libgalera_smm.so

  wsrep_cluster_address=gcomm://10.11.12.205:4010

  wsrep_provider_options = "gmcast.listen_addr=tcp://0.0.0.0:6010; "
  wsrep_sst_receive_address=10.11.12.205:6020

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node6000

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2
