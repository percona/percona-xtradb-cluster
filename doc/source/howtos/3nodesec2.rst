.. _3nodec2:

====================================================
How to set up a three-node cluster in EC2 enviroment
====================================================

This manual assumes you are running *m1.xlarge* instances
with Red Hat Enterprise Linux 6.1 64-bit.

* ``node1``: ``10.93.46.58``
* ``node2``: ``10.93.46.59``
* ``node3``: ``10.93.46.60``

To set up |PXC|:

1. Remove any |PXC| 5.5, |PS| 5.5, and |PS| 5.6 packages.

#. nstall |PXC| as described in :ref:`yum`.

#. Create data directories:

   .. code-block:: bash

      mkdir -p /mnt/data
      mysql_install_db --datadir=/mnt/data --user=mysql

#. Stop the firewall service:

   .. code-block:: bash

      service iptables stop

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
    wsrep_sst_method=rsync
    wsrep_node_name=node1

    innodb_autoinc_lock_mode=2

   For the second and third nodes change the following lines::

    wsrep_node_name=node2

    wsrep_node_name=node3

#. Start and bootstrap |PXC| on the first node:

   .. code-block:: bash

      [root@node1 ~]# /etc/init.d/mysql bootstrap-pxc

   You should see the following output::

    2014-01-30 11:52:35 23280 [Note] /usr/sbin/mysqld: ready for connections.
    Version: '5.6.15-56'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), Release 25.3, Revision 706, wsrep_25.3.r4034

#. Start the second and third nodes:

   .. code-block:: bash

      [root@node2 ~]# /etc/init.d/mysql start

   You should see the following output::

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

   When all nodes are in SYNCED state, your cluster is ready.

#. You can try connecting to MySQL on any node and create a database::

        $ mysql -uroot
        > CREATE DATABASE hello_tom;

The new database will be propagated to all nodes.

