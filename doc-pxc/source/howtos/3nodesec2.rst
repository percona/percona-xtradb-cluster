How to setup 3 node cluster in EC2 enviroment
==============================================

This is how to setup 3-node cluster in EC2 enviroment.

Assume you are running *m1.xlarge* instances with OS *Red Hat Enterprise Linux 6.1 64-bit*.
Make sure to remove existing PXC-5.5 and PS-5.5/5.6 packages before proceeding.

Install |Percona XtraDB Cluster| from RPM:

1. Install Percona's regular and testing repositories: ::

        rpm -Uhv http://repo.percona.com/testing/centos/6/os/noarch/percona-testing-0.0-1.noarch.rpm
        rpm -Uhv http://www.percona.com/downloads/percona-release/percona-release-0.0-1.x86_64.rpm

2. Install Percona XtraDB Cluster packages: ::

        yum install Percona-XtraDB-Cluster-server-56 Percona-XtraDB-Cluster-client-56 Percona-XtraDB-Cluster-galera-3

3. Create data directories: ::

        mkdir -p /mnt/data
        mysql_install_db --datadir=/mnt/data --user=mysql

4. Stop firewall. Cluster requires couple TCP ports to operate. Easiest way: :: 

        service iptables stop

If you want to open only specific ports, you need to open 3306, 4444, 4567, 4568 ports.
For example for 4567 port (substitute 192.168.0.1 by your IP): ::

        iptables -A INPUT -i eth0 -p tcp -m tcp --source 192.168.0.1/24 --dport 4567 -j ACCEPT


5. Create :file:`/etc/my.cnf` files.

 On the first node (assume IP 10.93.46.58): ::

  [mysqld]
  datadir=/mnt/data
  user=mysql

  binlog_format=ROW

  wsrep_provider=/usr/lib64/libgalera_smm.so
  wsrep_cluster_address=gcomm://10.93.46.58,10.93.46.59,10.93.46.60

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node1

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

 On the second node (assume IP 10.93.46.59): ::

  [mysqld]
  datadir=/mnt/data
  user=mysql

  binlog_format=ROW

  wsrep_provider=/usr/lib64/libgalera_smm.so
  wsrep_cluster_address=gcomm://10.93.46.58,10.93.46.59,10.93.46.60

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node2

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

On the third (and following nodes) configuration is similar, with the following change: ::

  wsrep_node_name=node3

In this example variable :variable:`wsrep_urls` is being used instead of :variable:`wsrep_cluster_address`. With this configuration, node will first try to reach a cluster on `10.93.46.58:4567` if there is no cluster node, then it will try on `10.93.46.59:4567` and then `10.93.46.60:4567`. If no nodes are up, it will start a new cluster. Variable :variable:`wsrep_urls` goes into the [mysql_safe] section so it's important that the mysql server instance is started with the `/bin/mysql_safe` and not `bin/mysqld`.

6. Start the |Percona XtraDB Cluster|

On the first node: ::

   [root@node1 ~]# /etc/init.d/mysql bootstrap-pxc

You should be able to see in console (or in error-log file): ::

  2014-01-30 11:52:35 23280 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.6.15-56'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release 25.3, Revision 706, wsrep_25.3.r4034


On the second (and following nodes): ::

   [root@node2 ~]# /etc/init.d/mysql start

You should be able to see in console (or in error-log file): ::

  2014-01-30 09:52:42 26104 [Note] WSREP: Flow-control interval: [28, 28]
  2014-01-30 09:52:42 26104 [Note] WSREP: Restored state OPEN -> JOINED (2)
  2014-01-30 09:52:42 26104 [Note] WSREP: Member 2 (percona1) synced with group.
  2014-01-30 09:52:42 26104 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 2)
  2014-01-30 09:52:42 26104 [Note] WSREP: New cluster view: global state: 4827a206-876b-11e3-911c-3e6a77d54953:2, view# 7: Primary, number of nodes: 3, my index: 2, protocol version 2
  2014-01-30 09:52:42 26104 [Note] WSREP: SST complete, seqno: 2
  2014-01-30 09:52:42 26104 [Note] Plugin 'FEDERATED' is disabled.
  2014-01-30 09:52:42 26104 [Note] InnoDB: The InnoDB memory heap is disabled
  2014-01-30 09:52:42 26104 [Note] InnoDB: Mutexes and rw_locks use GCC atomic builtins
  2014-01-30 09:52:42 26104 [Note] InnoDB: Compressed tables use zlib 1.2.3
  2014-01-30 09:52:42 26104 [Note] InnoDB: Using Linux native AIO
  2014-01-30 09:52:42 26104 [Note] InnoDB: Not using CPU crc32 instructions
  2014-01-30 09:52:42 26104 [Note] InnoDB: Initializing buffer pool, size = 128.0M
  2014-01-30 09:52:42 26104 [Note] InnoDB: Completed initialization of buffer pool
  2014-01-30 09:52:43 26104 [Note] InnoDB: Highest supported file format is Barracuda.
  2014-01-30 09:52:43 26104 [Note] InnoDB: 128 rollback segment(s) are active.
  2014-01-30 09:52:43 26104 [Note] InnoDB: Waiting for purge to start
  2014-01-30 09:52:43 26104 [Note] InnoDB:  Percona XtraDB (http://www.percona.com) 5.6.15-rel62.0 started; log sequence number 1626341
  2014-01-30 09:52:43 26104 [Note] RSA private key file not found: /var/lib/mysql//private_key.pem. Some authentication plugins will not work.
  2014-01-30 09:52:43 26104 [Note] RSA public key file not found: /var/lib/mysql//public_key.pem. Some authentication plugins will not work.
  2014-01-30 09:52:43 26104 [Note] Server hostname (bind-address): '*'; port: 3306
  2014-01-30 09:52:43 26104 [Note] IPv6 is available.
  2014-01-30 09:52:43 26104 [Note]   - '::' resolves to '::';
  2014-01-30 09:52:43 26104 [Note] Server socket created on IP: '::'.
  2014-01-30 09:52:43 26104 [Note] Event Scheduler: Loaded 0 events
  2014-01-30 09:52:43 26104 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.6.15-56'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release 25.3, Revision 706, wsrep_25.3.r4034
  2014-01-30 09:52:43 26104 [Note] WSREP: inited wsrep sidno 1
  2014-01-30 09:52:43 26104 [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.
  2014-01-30 09:52:43 26104 [Note] WSREP: REPL Protocols: 5 (3, 1)
  2014-01-30 09:52:43 26104 [Note] WSREP: Assign initial position for certification: 2, protocol version: 3
  2014-01-30 09:52:43 26104 [Note] WSREP: Service thread queue flushed.
  2014-01-30 09:52:43 26104 [Note] WSREP: Synchronized with group, ready for connections

When all nodes are in SYNCED stage your cluster is ready!

7. Connect to database on any node and create database: ::

        $ mysql -uroot
        > CREATE DATABASE hello_tom;

The new database will be propagated to all nodes.

Enjoy!


