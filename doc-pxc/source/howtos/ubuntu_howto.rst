.. _ubuntu_howto:

Installing Percona XtraDB Cluster on *Ubuntu*
=============================================

This tutorial will show how to install the |Percona XtraDB Cluster| on three *Ubuntu* 12.04.2 LTS servers, using the packages from Percona repositories.

This cluster will be assembled of three servers/nodes: ::
 
  node #1
  hostname: pxc1
  IP: 192.168.70.61

  node #2
  hostname: pxc2
  IP: 192.168.70.62

  node #3
  hostname: pxc3
  IP: 192.168.70.63

Prerequisites 
-------------

 * All three nodes have a *Ubuntu* 12.04.2 LTS installation. 
 
 * Firewall has been set up to allow connecting to ports 3306, 4444, 4567 and 4568

 * AppArmor profile for |MySQL| is `disabled <http://www.mysqlperformanceblog.com/2012/12/20/percona-xtradb-cluster-selinux-is-not-always-the-culprit/>`_ 

Installation
------------

Installation information can be found in the :ref:`installation` guide

.. note:: 

 Debian/Ubuntu installation prompts for root password, this was set to: ``Passw0rd``. After the packages have been installed, ``mysqld`` will be started automatically. In this example mysqld is stopped on all three nodes after successful installation with: ``/etc/init.d/mysql stop``.

Configuring the nodes
---------------------

Individual nodes should be configured to be able to bootstrap the cluster. More details about bootstrapping the cluster can be found in the :ref:`bootstrap` guide.

Configuration file :file:`/etc/mysql/my.cnf` for the first node should look like: ::

  [mysqld]

  datadir=/var/lib/mysql
  user=mysql

  # Path to Galera library
  wsrep_provider=/usr/lib/libgalera_smm.so

  # Cluster connection URL contains the IPs of node#1, node#2 and node#3
  wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

  # In order for Galera to work correctly binlog format should be ROW
  binlog_format=ROW

  # MyISAM storage engine has only experimental support
  default_storage_engine=InnoDB

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #1 address
  wsrep_node_address=192.168.70.61

  # SST method
  wsrep_sst_method=xtrabackup-v2

  # Cluster name
  wsrep_cluster_name=my_ubuntu_cluster

  # Authentication for SST method
  wsrep_sst_auth="sstuser:s3cretPass"


After this, first node can be started with the following command: ::

  [root@pxc1 ~]# /etc/init.d/mysql bootstrap-pxc
 
This command will start the first node and bootstrap the cluster (more information about bootstrapping cluster can be found in :ref:`bootstrap` manual).

After the first node has been started, cluster status can be checked by: 

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
  40 rows in set (0.01 sec)

This output shows that the cluster has been successfully bootstrapped. 

In order to perform successful :ref:`state_snapshot_transfer` using |Percona XtraBackup| new user needs to be set up with proper `privileges <http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_: 

.. code-block:: mysql

  mysql@pxc1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 's3cretPass';
  mysql@pxc1> GRANT RELOAD, LOCK TABLES, REPLICATION CLIENT ON *.* TO 'sstuser'@'localhost';
  mysql@pxc1> FLUSH PRIVILEGES;


.. note:: 

 MySQL root account can also be used for setting up the :ref:`state_snapshot_transfer` with |Percona XtraBackup|, but it's recommended to use a different (non-root) user for this.

Configuration file :file:`/etc/mysql/my.cnf` on the second node (``pxc2``) should look like this: ::

  [mysqld]

  datadir=/var/lib/mysql
  user=mysql

  # Path to Galera library
  wsrep_provider=/usr/lib/libgalera_smm.so

  # Cluster connection URL contains IPs of node#1, node#2 and node#3
  wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

  # In order for Galera to work correctly binlog format should be ROW
  binlog_format=ROW

  # MyISAM storage engine has only experimental support
  default_storage_engine=InnoDB

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #2 address
  wsrep_node_address=192.168.70.62

  # Cluster name
  wsrep_cluster_name=my_ubuntu_cluster

  # SST method
  wsrep_sst_method=xtrabackup-v2

  #Authentication for SST method
  wsrep_sst_auth="sstuser:s3cretPass"
 
Second node can be started with the following command: ::

  [root@pxc2 ~]# /etc/init.d/mysql start

After the server has been started it should receive the state snapshot transfer automatically. Cluster status can now be checked on both nodes. This is the example from the second node (``pxc2``): 

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

MySQL configuration file :file:`/etc/mysql/my.cnf` on the third node (``pxc3``) should look like this: ::

  [mysqld]

  datadir=/var/lib/mysql
  user=mysql

  # Path to Galera library
  wsrep_provider=/usr/lib/libgalera_smm.so

  # Cluster connection URL contains IPs of node#1, node#2 and node#3
  wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

  # In order for Galera to work correctly binlog format should be ROW
  binlog_format=ROW

  # MyISAM storage engine has only experimental support
  default_storage_engine=InnoDB

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #3 address
  wsrep_node_address=192.168.70.63

  # Cluster name
  wsrep_cluster_name=my_ubuntu_cluster

  # SST method
  wsrep_sst_method=xtrabackup-v2

  #Authentication for SST method
  wsrep_sst_auth="sstuser:s3cretPass"

Third node can now be started with the following command: :: 

  [root@pxc3 ~]# /etc/init.d/mysql start

After the server has been started it should receive the SST same as the second node. Cluster status can now be checked on both nodes. This is the example from the third node (``pxc3``): 

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

Testing the replication
-----------------------

Although the password change from the first node has replicated successfully, this example will show that writing on any node will replicate to the whole cluster. In order to check this, new database will be created on second node and table for that database will be created on the third node.

Creating the new database on the second node: 

.. code-block:: mysql 

  mysql@pxc2> CREATE DATABASE percona;
  Query OK, 1 row affected (0.01 sec)

Creating the ``example`` table on the third node: 
  
.. code-block:: mysql 

  mysql@pxc3> USE percona;
  Database changed

  mysql@pxc3> CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));
  Query OK, 0 rows affected (0.05 sec)

Inserting records on the first node: 

.. code-block:: mysql 

  mysql@pxc1> INSERT INTO percona.example VALUES (1, 'percona1');
  Query OK, 1 row affected (0.02 sec)

Retrieving all the rows from that table on the second node: 

.. code-block:: mysql 

  mysql@pxc2> SELECT * FROM percona.example;
  +---------+-----------+
  | node_id | node_name |
  +---------+-----------+
  |       1 | percona1  |
  +---------+-----------+
  1 row in set (0.00 sec)

This small example shows that all nodes in the cluster are synchronized and working as intended.
