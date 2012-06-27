How to setup 3 node cluster in EC2 enviroment
==============================================

This is how to setup 3-node cluster in EC2 enviroment.

Assume you are running *m1.xlarge* instances with OS *Red Hat Enterprise Linux 6.1 64-bit*.

Install XtraDB Cluster from RPM:

1. Install Percona's regular and testing repositories: ::

        rpm -Uhv http://repo.percona.com/testing/centos/6/os/noarch/percona-testing-0.0-1.noarch.rpm
        rpm -Uhv http://www.percona.com/downloads/percona-release/percona-release-0.0-1.x86_64.rpm

2. Install Percona XtraDB Cluster packages: ::

        yum install Percona-XtraDB-Cluster-server Percona-XtraDB-Cluster-client

3. Create data directories: ::

        mkdir -p /mnt/data
        mysql_install_db --datadir=/mnt/data --user=mysql

4. Stop firewall. Cluster requires couple TCP ports to operate. Easiest way: :: 

        service iptables stop

If you want to open only specific ports, you need to open 3306, 4444, 4567, 4568 ports.
For example for 4567 port (substitute 192.168.0.1 by your IP): ::

        iptables -A INPUT -i eth0 -p tcp -m tcp --source 192.168.0.1/24 --dport 4567 -j ACCEPT


5. Create /etc/my.cnf files.

On the first node (assume IP 10.93.46.58): ::

  [mysqld]
  datadir=/mnt/data
  user=mysql

  binlog_format=ROW

  wsrep_provider=/usr/lib64/libgalera_smm.so

  wsrep_cluster_address=gcomm://

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node1

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2


On the second node: ::

  [mysqld]
  datadir=/mnt/data
  user=mysql

  binlog_format=ROW

  wsrep_provider=/usr/lib64/libgalera_smm.so

  wsrep_cluster_address=gcomm://10.93.46.58

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node2

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

On the third (and following nodes) config is similar, with the following change: ::

  wsrep_node_name=node3

6. Start mysqld

On the first node: ::

   /usr/sbin/mysqld
   or
   mysqld_safe

You should be able to see in console (or in error-log file): ::

  111216  0:16:42 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.5.17'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release alpha22.1, Revision 3673 wsrep_22.3.r3673
  111216  0:16:42 [Note] WSREP: Assign initial position for certification: 0, protocol version: 1
  111216  0:16:42 [Note] WSREP: Synchronized with group, ready for connections

On the second (and following nodes): ::

   /usr/sbin/mysqld
   or
   mysqld_safe

You should be able to see in console (or in error-log file): ::

  111216  0:21:39 [Note] WSREP: Flow-control interval: [12, 23]
  111216  0:21:39 [Note] WSREP: Shifting OPEN -> PRIMARY (TO: 0)
  111216  0:21:39 [Note] WSREP: New cluster view: global state: f912d2eb-27a2-11e1-0800-f34c520a3d4b:0, view# 2: Primary, number of nodes: 2, my index: 1, protocol version 1
  111216  0:21:39 [Warning] WSREP: Gap in state sequence. Need state transfer.
  111216  0:21:41 [Note] WSREP: Running: 'wsrep_sst_rsync 'joiner' '10.93.62.178' '' '/mnt/data/' '/etc/my.cnf' '1694' 2>sst.err'
  111216  0:21:41 [Note] WSREP: Prepared SST request: rsync|10.93.62.178:4444/rsync_sst
  111216  0:21:41 [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.
  111216  0:21:41 [Note] WSREP: Assign initial position for certification: 0, protocol version: 1
  111216  0:21:41 [Note] WSREP: prepared IST receiver, listening in: tcp://10.93.62.178:4568
  111216  0:21:41 [Note] WSREP: Node 1 (node2) requested state transfer from '*any*'. Selected 0 (node1)(SYNCED) as donor.
  111216  0:21:41 [Note] WSREP: Shifting PRIMARY -> JOINER (TO: 0)
  111216  0:21:41 [Note] WSREP: Requesting state transfer: success, donor: 0
  111216  0:21:42 [Note] WSREP: 0 (node1): State transfer to 1 (node2) complete.
  111216  0:21:42 [Note] WSREP: Member 0 (node1) synced with group.
  111216  0:21:42 [Note] WSREP: SST complete, seqno: 0
  111216  0:21:42 [Note] Plugin 'FEDERATED' is disabled.
  111216  0:21:42 InnoDB: The InnoDB memory heap is disabled
  111216  0:21:42 InnoDB: Mutexes and rw_locks use GCC atomic builtins
  111216  0:21:42 InnoDB: Compressed tables use zlib 1.2.3
  111216  0:21:42 InnoDB: Using Linux native AIO
  111216  0:21:42 InnoDB: Initializing buffer pool, size = 128.0M
  111216  0:21:42 InnoDB: Completed initialization of buffer pool
  111216  0:21:42 InnoDB: highest supported file format is Barracuda.
  111216  0:21:42  InnoDB: Waiting for the background threads to start
  111216  0:21:43 Percona XtraDB (http://www.percona.com) 1.1.8-20.1 started; log sequence number 1597945
  111216  0:21:43 [Note] Event Scheduler: Loaded 0 events
  111216  0:21:43 [Note] WSREP: Signalling provider to continue.
  111216  0:21:43 [Note] WSREP: Received SST: f912d2eb-27a2-11e1-0800-f34c520a3d4b:0
  111216  0:21:43 [Note] WSREP: SST finished: f912d2eb-27a2-11e1-0800-f34c520a3d4b:0
  111216  0:21:43 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.5.17'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release alpha22.1, Revision 3673 wsrep_22.3.r3673
  111216  0:21:43 [Note] WSREP: 1 (node2): State transfer from 0 (node1) complete.
  111216  0:21:43 [Note] WSREP: Shifting JOINER -> JOINED (TO: 0)
  111216  0:21:43 [Note] WSREP: Member 1 (node2) synced with group.
  111216  0:21:43 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 0)
  111216  0:21:43 [Note] WSREP: Synchronized with group, ready for connections

When all nodes are in SYNCED stage your cluster is ready!

7. Connect to database on any node and create database: ::

        $ mysql -uroot
        > CREATE DATABASE hello_tom;

The new database will be propagated to all nodes.

Enjoy!


