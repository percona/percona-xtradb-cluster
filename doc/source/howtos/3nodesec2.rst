How to setup 3 node cluster in EC2 enviroment
==============================================

This is how to setup 3-node cluster in EC2 enviroment.

Assume you are running *m1.xlarge* instances with OS *Red Hat Enterprise Linux 6.1 64-bit*.

Install XtraDB Cluster from RPM:

1. Install Percona's regular and testing repositories: ::

        rpm -Uhv http://repo.percona.com/testing/centos/6/os/noarch/percona-testing-0.0-1.noarch.rpm
        rpm -Uhv http://www.percona.com/downloads/percona-release/percona-release-0.0-1.x86_64.rpm

2. Install Percona XtraDB Cluster packages: ::

        yum install Percona-XtraDB-Cluster-server-55 Percona-XtraDB-Cluster-client-55 Percona-XtraDB-Cluster-galera-2

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
  wsrep_cluster_address=gcomm://10.93.46.58,10.93.46.59,10.93.46.60

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node1

  innodb_autoinc_lock_mode=2

 On the second node (assume IP 10.93.46.59): ::

  [mysqld]
  datadir=/mnt/data
  user=mysql

  wsrep_cluster_address=gcomm://10.93.46.58,10.93.46.59,10.93.46.60
  binlog_format=ROW

  wsrep_provider=/usr/lib64/libgalera_smm.so

  wsrep_slave_threads=2
  wsrep_cluster_name=trimethylxanthine
  wsrep_sst_method=rsync
  wsrep_node_name=node2

  innodb_autoinc_lock_mode=2

On the third (and following nodes) configuration is similar, with the following change: ::

  wsrep_node_name=node3

6. Start the |Percona XtraDB Cluster|

On the first node: ::

   [root@node1 ~]# /etc/init.d/mysql bootstrap-pxc

You should be able to see in console (or in error-log file): ::

  140113 12:23:43 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.5.34-55'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), wsrep_25.9.r3928

On the second (and following nodes): ::

   [root@node2 ~]# /etc/init.d/mysql start

You should be able to see in console (or in error-log file): ::

  140113 17:29:34 [Note] WSREP: New cluster view: global state: fb031d6e-7c4c-11e3-bf3a-ae73fcffc079:0, view# 8: Primary, number of nodes: 2, my index: 1, protocol version 2
  140113 17:29:34 [Warning] WSREP: Gap in state sequence. Need state transfer.
  140113 17:29:36 [Note] WSREP: Running: 'wsrep_sst_rsync --role 'joiner' --address '10.93.46.59' --auth '' --datadir '/mnt/data/' --defaults-file '/etc/my.cnf' --parent '29627''
  cat: /var/lib/mysql//rsync_sst.pid: No such file or directory
  140113 17:29:36 [Note] WSREP: Prepared SST request: rsync|10.93.46.59:4444/rsync_sst
  140113 17:29:36 [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.
  140113 17:29:36 [Note] WSREP: Assign initial position for certification: 0, protocol version: 2
  140113 17:29:36 [Warning] WSREP: Failed to prepare for incremental state transfer: Local state UUID (00000000-0000-0000-0000-000000000000) does not match group state UUID (fb031d6e-7c4c-11e3-bf3a-ae73fcffc079): 1 (Operation not permitted)
  at galera/src/replicator_str.cpp:prepare_for_IST():445. IST will be unavailable.
  140113 17:29:36 [Note] WSREP: Node 1 (node1) requested state transfer from '*any*'. Selected 0 (node1)(SYNCED) as donor.
  140113 17:29:36 [Note] WSREP: Shifting PRIMARY -> JOINER (TO: 0)
  140113 17:29:36 [Note] WSREP: Requesting state transfer: success, donor: 0
  140113 17:29:39 [Note] WSREP: 0 (node1): State transfer to 1 (node1) complete.
  140113 17:29:39 [Note] WSREP: Member 0 (node1) synced with group.
  WSREP_SST: [INFO] Joiner cleanup. (20140113 17:29:39.792)
  WSREP_SST: [INFO] Joiner cleanup done. (20140113 17:29:40.302)
  140113 17:29:40 [Note] WSREP: SST complete, seqno: 0
  140113 17:29:40 [Note] Plugin 'FEDERATED' is disabled.
  140113 17:29:40 InnoDB: The InnoDB memory heap is disabled
  140113 17:29:40 InnoDB: Mutexes and rw_locks use GCC atomic builtins
  140113 17:29:40 InnoDB: Compressed tables use zlib 1.2.3
  140113 17:29:40 InnoDB: Using Linux native AIO
  140113 17:29:40 InnoDB: Initializing buffer pool, size = 128.0M
  140113 17:29:40 InnoDB: Completed initialization of buffer pool
  140113 17:29:40 InnoDB: highest supported file format is Barracuda.
  InnoDB: Log scan progressed past the checkpoint lsn 1598437
  140113 17:29:40  InnoDB: Database was not shut down normally!
  InnoDB: Starting crash recovery.
  InnoDB: Reading tablespace information from the .ibd files...
  InnoDB: Restoring possible half-written data pages from the doublewrite
  InnoDB: buffer...
  InnoDB: Doing recovery: scanned up to log sequence number 1598607
  140113 17:29:40  InnoDB: Waiting for the background threads to start
  140113 17:29:41 Percona XtraDB (http://www.percona.com) 5.5.34-rel32.0 started; log sequence number 1598607
  140113 17:29:41 [Note] Event Scheduler: Loaded 0 events
  140113 17:29:41 [Note] WSREP: Signalling provider to continue.
  140113 17:29:41 [Note] WSREP: SST received: fb031d6e-7c4c-11e3-bf3a-ae73fcffc079:0
  140113 17:29:41 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.5.34-55'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), wsrep_25.9.r3928
  140113 17:29:41 [Note] WSREP: 1 (node1): State transfer from 0 (node1) complete.
  140113 17:29:41 [Note] WSREP: Shifting JOINER -> JOINED (TO: 0)
  140113 17:29:41 [Note] WSREP: Member 1 (node1) synced with group.
  140113 17:29:41 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 0)
  140113 17:29:41 [Note] WSREP: Synchronized with group, ready for connections
  140113 17:29:41 [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.

When all nodes are in SYNCED stage your cluster is ready!

7. Connect to database on any node and create database: ::

        $ mysql -uroot
        > CREATE DATABASE hello_tom;

The new database will be propagated to all nodes.

Enjoy!


