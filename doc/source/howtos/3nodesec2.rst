.. _3nodec2:

=====================================================
How to set up a three-node cluster in EC2 environment
=====================================================

This manual assumes you are running three EC2 instances
with Red Hat Enterprise Linux 7 64-bit.

* ``node1``: ``10.93.46.58``
* ``node2``: ``10.93.46.59``
* ``node3``: ``10.93.46.60``

.. rubric:: Recommendations on launching EC2 instances

1. Select `instance types <https://aws.amazon.com/ec2/instance-types/>`_ that support Enhanced Networking functionality. Good network performance critical for synchronous replication used in Percona XtraDB Cluster.
#. When adding instance storage volumes, choose the ones with good I/O performance:

   * instances with NVMe are preferred
   * GP2 SSD are preferred to GP3 SSD volume types due to I/O latency
   * over sized GP2 SSD are preferred to IO1 volume types due to cost
     
#. Attach Elastic network interfaces with static IPs or assign Elastic IP addresses to your instances. Thereby IP addresses are preserved on instances in case of reboot or restart. This is required as each Percona XtraDB Cluster member includes the ``wsrep_cluster_address`` option in its configuration which points to other cluster members.
#. Launch instances in different availability zones to avoid cluster downtime in case one of the zones experiences power loss or network connectivity issues.
   
.. seealso::

   Amazon EC2 Documentation:
       https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/EC2_GetStarted.html

To set up Percona XtraDB Cluster:

1. Remove Percona XtraDB Cluster and Percona Server for MySQL packages for older versions:

   - Percona XtraDB Cluster 5.6, 5.7
   - Percona Server for MySQL 5.6, 5.7

#. Install Percona XtraDB Cluster as described in :ref:`yum`.

#. Create data directories:

   .. code-block:: bash

      $ mkdir -p /mnt/data
      $ mysql_install_db --datadir=/mnt/data --user=mysql

#. Stop the firewall service:

   .. code-block:: bash

      $ service iptables stop

   .. note:: Alternatively, you can keep the firewall running,
      but open ports 3306, 4444, 4567, 4568.
      For example to open port 4567 on 192.168.0.1:

      .. code-block:: bash

         iptables -A INPUT -i eth0 -p tcp -m tcp --source 192.168.0.1/24 --dport 4567 -j ACCEPT

#. Create :file:`/etc/my.cnf` files:

   Contents of the configuration file on the first node::

    [mysqld]
    datadir=/mnt/data
    user=mysql

    binlog_format=ROW

    wsrep_provider=/usr/lib64/libgalera_smm.so
    wsrep_cluster_address=gcomm://10.93.46.58,10.93.46.59,10.93.46.60

    wsrep_slave_threads=2
    wsrep_cluster_name=trimethylxanthine
    wsrep_sst_method=xtrabackup-v2
    wsrep_node_name=node1

    innodb_autoinc_lock_mode=2

   For the second and third nodes change the following lines::

    wsrep_node_name=node2

    wsrep_node_name=node3

#. Start and bootstrap Percona XtraDB Cluster on the first node:

   .. code-block:: bash

      [root@pxc1 ~]# systemctl start mysql@bootstrap.service

   You should see the following output::

    2014-01-30 11:52:35 23280 [Note] /usr/sbin/mysqld: ready for connections.
    Version: '...'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release ..., Revision ..., wsrep_version

#. Start the second and third nodes:

   .. code-block:: bash

      $ sudo systemctl start mysql


   The output should be similar to the following:

   .. code-block:: text

      ... [Note] WSREP: Flow-control interval: [28, 28]
      ... [Note] WSREP: Restored state OPEN -> JOINED (2)
      ... [Note] WSREP: Member 2 (percona1) synced with group.
      ... [Note] WSREP: Shifting JOINED -> SYNCED (TO: 2)
      ... [Note] WSREP: New cluster view: global state: 4827a206-876b-11e3-911c-3e6a77d54953:2, view# 7: Primary, number of nodes: 3, my index: 2, protocol version 2
      ... [Note] WSREP: SST complete, seqno: 2
      ... [Note] Plugin 'FEDERATED' is disabled.
      ... [Note] InnoDB: The InnoDB memory heap is disabled
      ... [Note] InnoDB: Mutexes and rw_locks use GCC atomic builtins
      ... [Note] InnoDB: Compressed tables use zlib 1.2.3
      ... [Note] InnoDB: Using Linux native AIO
      ... [Note] InnoDB: Not using CPU crc32 instructions
      ... [Note] InnoDB: Initializing buffer pool, size = 128.0M
      ... [Note] InnoDB: Completed initialization of buffer pool
      ... [Note] InnoDB: Highest supported file format is Barracuda.
      ... [Note] InnoDB: 128 rollback segment(s) are active.
      ... [Note] InnoDB: Waiting for purge to start
      ... [Note] InnoDB:  Percona XtraDB (http://www.percona.com) ... started; log sequence number 1626341
      ... [Note] RSA private key file not found: /var/lib/mysql//private_key.pem. Some authentication plugins will not work.
      ... [Note] RSA public key file not found: /var/lib/mysql//public_key.pem. Some authentication plugins will not work.
      ... [Note] Server hostname (bind-address): '*'; port: 3306
      ... [Note] IPv6 is available.
      ... [Note]   - '::' resolves to '::';
      ... [Note] Server socket created on IP: '::'.
      ... [Note] Event Scheduler: Loaded 0 events
      ... [Note] /usr/sbin/mysqld: ready for connections.
      Version: '...'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release ..., Revision ..., wsrep_version
      ... [Note] WSREP: inited wsrep sidno 1
      ... [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.
      ... [Note] WSREP: REPL Protocols: 5 (3, 1)
      ... [Note] WSREP: Assign initial position for certification: 2, protocol version: 3
      ... [Note] WSREP: Service thread queue flushed.
      ... [Note] WSREP: Synchronized with group, ready for connections

   When all nodes are in SYNCED state, your cluster is ready.

#. You can try connecting to MySQL on any node and create a database::

        $ mysql -uroot
        > CREATE DATABASE hello_tom;

The new database will be propagated to all nodes.

