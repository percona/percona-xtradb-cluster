.. _centos_howto:

============================================
Configuring Percona XtraDB Cluster on CentOS
============================================

This tutorial describes how to install and configure three |PXC| nodes
on CentOS 6.8 servers, using the packages from Percona repositories.

* Node 1

  * Host name: ``percona1``
  * IP address: ``192.168.70.71``

* Node 2

  * Host name: ``percona2``
  * IP address: ``192.168.70.72``

* Node 3

  * Host name: ``percona3``
  * IP address: ``192.168.70.73``

Prerequisites
=============

The procedure described in this tutorial requires the following:

* All three nodes have CentOS 6.8 installed.
* The firewall on all nodes is configured to allow connecting
  to ports 3306, 4444, 4567 and 4568.
* SELinux on all nodes is disabled.

Step 1. Installing PXC
======================

Install |PXC| on all three nodes as described in :ref:`yum`.

Step 2. Configuring the first node
==================================

Individual nodes should be configured to be able to bootstrap the cluster.
For more information about bootstrapping the cluster, see :ref:`bootstrap`.

1. Make sure that the configuration file :file:`/etc/my.cnf`
   on the first node (``percona1``) contains the following::

      [mysqld]

      datadir=/var/lib/mysql
      user=mysql

      # Path to Galera library
      wsrep_provider=/usr/lib64/libgalera_smm.so

      # Cluster connection URL contains the IPs of node#1, node#2 and node#3
      wsrep_cluster_address=gcomm://192.168.70.71,192.168.70.72,192.168.70.73

      # In order for Galera to work correctly binlog format should be ROW
      binlog_format=ROW

      # MyISAM storage engine has only experimental support
      default_storage_engine=InnoDB

      # This InnoDB autoincrement locking mode is a requirement for Galera
      innodb_autoinc_lock_mode=2

      # Node 1 address
      wsrep_node_address=192.168.70.71

      # SST method
      wsrep_sst_method=xtrabackup-v2

      # Cluster name
      wsrep_cluster_name=my_centos_cluster

      # Authentication for SST method
      wsrep_sst_auth="sstuser:s3cret"

#. Start the first node with the following command::

      [root@percona1 ~]# /etc/init.d/mysql bootstrap-pxc

   .. note:: In case you're running CentOS 7,
      the bootstrap service should be used instead: ::

         [root@percona1 ~]#  systemctl start mysql@bootstrap.service

   The previous command will start the cluster
   with initial :variable:`wsrep_cluster_address` variable
   set to ``gcomm://``.
   If the node or |MySQL| are restarted later,
   there will be no need to change the configuration file.

#. After the first node has been started,
   cluster status can be checked with the following command:

   .. code-block:: mysql

      mysql> show status like 'wsrep%';
      +----------------------------+--------------------------------------+
      | Variable_name              | Value                                |
      +----------------------------+--------------------------------------+
      | wsrep_local_state_uuid     | c2883338-834d-11e2-0800-03c9c68e41ec |
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
      40 rows in set (0.01 sec)

   This output shows that the cluster has been successfully bootstrapped.

.. note:: It is not recommended to leave an empty password
   for the root account. Password can be changed as follows:

   .. code-block:: mysql

      mysql@percona1> UPDATE mysql.user SET password=PASSWORD("Passw0rd") where user='root';
      mysql@percona1> FLUSH PRIVILEGES;

To perform :ref:`state_snapshot_transfer` using |XtraBackup|,
set up a new user with proper `privileges <https://www.percona.com/doc/percona-xtrabackup/2.2/innobackupex/privileges.html>`__:

.. code-block:: mysql

   mysql@percona1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 's3cret';
   mysql@percona1> GRANT PROCESS, RELOAD, LOCK TABLES, REPLICATION CLIENT ON *.* TO 'sstuser'@'localhost';
   mysql@percona1> FLUSH PRIVILEGES;

.. note:: MySQL root account can also be used for performing SST,
   but it is more secure to use a different (non-root) user for this.

Step 3. Configuring the second node
===================================

1. Make sure that the configuration file :file:`/etc/my.cnf`
   on the second node (``percona2``) contains the following::

      [mysqld]

      datadir=/var/lib/mysql
      user=mysql

      # Path to Galera library
      wsrep_provider=/usr/lib64/libgalera_smm.so

      # Cluster connection URL contains IPs of node#1, node#2 and node#3
      wsrep_cluster_address=gcomm://192.168.70.71,192.168.70.72,192.168.70.73

      # In order for Galera to work correctly binlog format should be ROW
      binlog_format=ROW

      # MyISAM storage engine has only experimental support
      default_storage_engine=InnoDB

      # This InnoDB autoincrement locking mode is a requirement for Galera
      innodb_autoinc_lock_mode=2

      # Node 2 address
      wsrep_node_address=192.168.70.72

      # Cluster name
      wsrep_cluster_name=my_centos_cluster

      # SST method
      wsrep_sst_method=xtrabackup-v2

      #Authentication for SST method
      wsrep_sst_auth="sstuser:s3cret"

#. Start the second node with the following command:

.. code-block:: bash

      [root@percona2 ~]# /etc/init.d/mysql start

#. After the server has been started,
   it should receive |SST| automatically.
   This means that the second node won't have empty root password anymore.
   In order to connect to the cluster and check the status,
   the root password from the first node should be used.
   Cluster status can be checked on both nodes.
   The following is an example of status from the second node (``percona2``):

   .. code-block:: mysql

      mysql> show status like 'wsrep%';
      +----------------------------+--------------------------------------+
      | Variable_name              | Value                                |
      +----------------------------+--------------------------------------+
      | wsrep_local_state_uuid     | c2883338-834d-11e2-0800-03c9c68e41ec |
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

1. Make sure that the MySQL configuration file :file:`/etc/my.cnf`
   on the third node (``percona3``) contains the following: ::

      [mysqld]

      datadir=/var/lib/mysql
      user=mysql

      # Path to Galera library
      wsrep_provider=/usr/lib64/libgalera_smm.so

      # Cluster connection URL contains IPs of node#1, node#2 and node#3
      wsrep_cluster_address=gcomm://192.168.70.71,192.168.70.72,192.168.70.73

      # In order for Galera to work correctly binlog format should be ROW
      binlog_format=ROW

      # MyISAM storage engine has only experimental support
      default_storage_engine=InnoDB

      # This InnoDB autoincrement locking mode is a requirement for Galera
      innodb_autoinc_lock_mode=2

      # Node #3 address
      wsrep_node_address=192.168.70.73

      # Cluster name
      wsrep_cluster_name=my_centos_cluster

      # SST method
      wsrep_sst_method=xtrabackup-v2

      #Authentication for SST method
      wsrep_sst_auth="sstuser:s3cret"

#. Start the third node with the following command:

.. code-block:: bash

      [root@percona3 ~]# /etc/init.d/mysql start

#. After the server has been started,
   it should receive SST automatically.
   Cluster status can be checked on all three nodes.
   The following is an example of status from the third node (``percona3``):

   .. code-block:: mysql

      mysql> show status like 'wsrep%';
      +----------------------------+--------------------------------------+
      | Variable_name              | Value                                |
      +----------------------------+--------------------------------------+
      | wsrep_local_state_uuid     | c2883338-834d-11e2-0800-03c9c68e41ec |
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

To test replication, lets create a new database on second node,
create a table for that database on the third node,
and add some records to the table on the first node.

1. Create a new database on the second node:

   .. code-block:: mysql

      mysql@percona2> CREATE DATABASE percona;
      Query OK, 1 row affected (0.01 sec)

#. Create a table on the third node:

   .. code-block:: mysql

      mysql@percona3> USE percona;
      Database changed

      mysql@percona3> CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));
      Query OK, 0 rows affected (0.05 sec)

#. Insert records on the first node:

   .. code-block:: mysql

      mysql@percona1> INSERT INTO percona.example VALUES (1, 'percona1');
      Query OK, 1 row affected (0.02 sec)

#. Retrieve all the rows from that table on the second node:

   .. code-block:: mysql

      mysql@percona2> SELECT * FROM percona.example;
      +---------+-----------+
      | node_id | node_name |
      +---------+-----------+
      |       1 | percona1  |
      +---------+-----------+
      1 row in set (0.00 sec)

This simple procedure should ensure that all nodes in the cluster
are synchronized and working as intended.
