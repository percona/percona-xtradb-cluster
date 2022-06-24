.. _ubuntu_howto:

============================================
Configuring Percona XtraDB Cluster on Ubuntu
============================================

This tutorial describes how to install and configure three Percona XtraDB Cluster nodes
on Ubuntu 14 LTS servers, using the packages from Percona repositories.

* Node 1

  * Host name: ``pxc1``
  * IP address: ``192.168.70.61``

* Node 2

  * Host name: ``pxc2``
  * IP address: ``192.168.70.62``

* Node 3

  * Host name: ``pxc3``
  * IP address: ``192.168.70.63``

Prerequisites
=============

The procedure described in this tutorial requires he following:

* All three nodes have Ubuntu 14 LTS installed.
* Firewall on all nodes is configured to allow connecting
  to ports 3306, 4444, 4567 and 4568.
* AppArmor profile for MySQL is `disabled <http://www.mysqlperformanceblog.com/2012/12/20/percona-xtradb-cluster-selinux-is-not-always-the-culprit/>`_.

Step 1. Installing PXC
======================

Install Percona XtraDB Cluster on all three nodes as described in :ref:`apt`.

.. note:: Debian/Ubuntu installation prompts for root password.
   For this tutorial, set it to ``Passw0rd``.
   After the packages have been installed,
   ``mysqld`` will start automatically.
   Stop ``mysqld`` on all three nodes using ``sudo systemctl stop mysql``.

Step 2. Configuring the first node
==================================

Individual nodes should be configured to be able to bootstrap the cluster.
For more information about bootstrapping the cluster, see :ref:`bootstrap`.

1. Make sure that the configuration file :file:`/etc/mysql/my.cnf`
   for the first node (``pxc1``) contains the following: ::

    [mysqld]

    datadir=/var/lib/mysql
    user=mysql

    # Path to Galera library
    wsrep_provider=/usr/lib/libgalera_smm.so

    # Cluster connection URL contains the IPs of node#1, node#2 and node#3
    wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

    # In order for Galera to work correctly binlog format should be ROW
    binlog_format=ROW

    # Using the MyISAM storage engine is not recommended
    default_storage_engine=InnoDB

    # This InnoDB autoincrement locking mode is a requirement for Galera
    innodb_autoinc_lock_mode=2

    # Node #1 address
    wsrep_node_address=192.168.70.61

    # SST method
    wsrep_sst_method=xtrabackup-v2

    # Cluster name
    wsrep_cluster_name=my_ubuntu_cluster


#. Start the first node with the following command:
   
   .. code-block:: bash

      [root@pxc1 ~]# systemctl start mysql@bootstrap.service

   This command will start the first node and bootstrap the cluster.

#. After the first node has been started,
   cluster status can be checked with the following command:

   .. code-block:: mysql

      mysql> show status like 'wsrep%';
      +----------------------------+--------------------------------------+
      | Variable_name              | Value                                |
      +----------------------------+--------------------------------------+
      | wsrep_local_state_uuid     | b598af3e-ace3-11e2-0800-3e90eb9cd5d3 |
      ...
      | wsrep_local_state          | 4                                    |
      | wsrep_local_state_comment  | Synced                               |
      ...
      | wsrep_cluster_size         | 1                                    |
      | wsrep_cluster_status       | Primary                              |
      | wsrep_connected            | ON                                   |
      ...
      | wsrep_ready                | ON                                   |
      +----------------------------+--------------------------------------+
      75 rows in set (0.00 sec)

  This output shows that the cluster has been successfully bootstrapped.

To perform :ref:`state_snapshot_transfer` using |XtraBackup|,
set up a new user with proper `privileges
<http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_:

.. code-block:: mysql

   mysql@pxc1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 's3cretPass';
   mysql@pxc1> GRANT PROCESS, RELOAD, LOCK TABLES, REPLICATION CLIENT ON *.* TO 'sstuser'@'localhost';
   mysql@pxc1> FLUSH PRIVILEGES;

.. note:: MySQL root account can also be used for performing SST,
   but it is more secure to use a different (non-root) user for this.

Step 3. Configuring the second node
===================================

1. Make sure that the configuration file :file:`/etc/mysql/my.cnf`
   on the second node (``pxc2``) contains the following: ::

    [mysqld]

    datadir=/var/lib/mysql
    user=mysql

    # Path to Galera library
    wsrep_provider=/usr/lib/libgalera_smm.so

    # Cluster connection URL contains IPs of node#1, node#2 and node#3
    wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

    # In order for Galera to work correctly binlog format should be ROW
    binlog_format=ROW

    # Using the MyISAM storage engine is not recommended
    default_storage_engine=InnoDB

    # This InnoDB autoincrement locking mode is a requirement for Galera
    innodb_autoinc_lock_mode=2

    # Node #2 address
    wsrep_node_address=192.168.70.62

    # Cluster name
    wsrep_cluster_name=my_ubuntu_cluster

    # SST method
    wsrep_sst_method=xtrabackup-v2


#. Start the second node with the following command: ::

    [root@pxc2 ~]# systemctl start mysql

#. After the server has been started,
   it should receive SST automatically.
   Cluster status can now be checked on both nodes.
   The following is an example of status from the second node (``pxc2``):

   .. code-block:: mysql

     mysql> show status like 'wsrep%';
     +----------------------------+--------------------------------------+
     | Variable_name              | Value                                |
     +----------------------------+--------------------------------------+
     | wsrep_local_state_uuid     | b598af3e-ace3-11e2-0800-3e90eb9cd5d3 |
     ...
     | wsrep_local_state          | 4                                    |
     | wsrep_local_state_comment  | Synced                               |
     ...
     | wsrep_cluster_size         | 2                                    |
     | wsrep_cluster_status       | Primary                              |
     | wsrep_connected            | ON                                   |
     ...
     | wsrep_ready                | ON                                   |
     +----------------------------+--------------------------------------+
     40 rows in set (0.01 sec)

   This output shows that the new node has been successfully added to the cluster.

Step 4. Configuring the third node
==================================

1. Make sure that the MySQL configuration file :file:`/etc/mysql/my.cnf`
   on the third node (``pxc3``) contains the following: ::

    [mysqld]

    datadir=/var/lib/mysql
    user=mysql

    # Path to Galera library
    wsrep_provider=/usr/lib/libgalera_smm.so

    # Cluster connection URL contains IPs of node#1, node#2 and node#3
    wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

    # In order for Galera to work correctly binlog format should be ROW
    binlog_format=ROW

    # Using the MyISAM storage engine is not recommended
    default_storage_engine=InnoDB

    # This InnoDB autoincrement locking mode is a requirement for Galera
    innodb_autoinc_lock_mode=2

    # Node #3 address
    wsrep_node_address=192.168.70.63

    # Cluster name
    wsrep_cluster_name=my_ubuntu_cluster

    # SST method
    wsrep_sst_method=xtrabackup-v2

#. Start the third node with the following command: ::

    [root@pxc3 ~]# systemctl start mysql

#. After the server has been started,
   it should receive SST automatically.
   Cluster status can be checked on all nodes.
   The following is an example of status from the third node (``pxc3``):

   .. code-block:: mysql

     mysql> show status like 'wsrep%';
     +----------------------------+--------------------------------------+
     | Variable_name              | Value                                |
     +----------------------------+--------------------------------------+
     | wsrep_local_state_uuid     | b598af3e-ace3-11e2-0800-3e90eb9cd5d3 |
     ...
     | wsrep_local_state          | 4                                    |
     | wsrep_local_state_comment  | Synced                               |
     ...
     | wsrep_cluster_size         | 3                                    |
     | wsrep_cluster_status       | Primary                              |
     | wsrep_connected            | ON                                   |
     ...
     | wsrep_ready                | ON                                   |
     +----------------------------+--------------------------------------+
     40 rows in set (0.01 sec)

   This output confirms that the third node has joined the cluster.

Testing replication
===================

To test replication, lets create a new database on the second node,
create a table for that database on the third node,
and add some records to the table on the first node.

1. Create a new database on the second node:

   .. code-block:: mysql

      mysql@pxc2> CREATE DATABASE percona;
      Query OK, 1 row affected (0.01 sec)

#. Create a table on the third node:

   .. code-block:: mysql

      mysql@pxc3> USE percona;
      Database changed

      mysql@pxc3> CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));
      Query OK, 0 rows affected (0.05 sec)

#. Insert records on the first node:

   .. code-block:: mysql

      mysql@pxc1> INSERT INTO percona.example VALUES (1, 'percona1');
      Query OK, 1 row affected (0.02 sec)

#. Retrieve all the rows from that table on the second node:

   .. code-block:: mysql

      mysql@pxc2> SELECT * FROM percona.example;
      +---------+-----------+
      | node_id | node_name |
      +---------+-----------+
      |       1 | percona1  |
      +---------+-----------+
      1 row in set (0.00 sec)

This simple procedure should ensure that all nodes in the cluster
are synchronized and working as intended.

