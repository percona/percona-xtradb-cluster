.. _centos_howto:

Installing Percona XtraDB Cluster on *CentOS*
=============================================

This tutorial will show how to install the |Percona XtraDB Cluster| on three CentOS 6.3 servers, using the packages from Percona repositories.

This cluster will be assembled of three servers/nodes: ::
 
  node #1
  hostname: percona1
  IP: 192.168.70.71

  node #2
  hostname: percona2
  IP: 192.168.70.72

  node #3
  hostname: percona3
  IP: 192.168.70.73

Prerequisites 
-------------

 * All three nodes have a CentOS 6.3 installation. 
 
 * Firewall has been set up to allow connecting to ports 3306, 4444, 4567 and 4568

 * SELinux is disabled

Installation
------------

Percona repository should be set up as described in the :ref:`yum-repo` guide. To enable the repository following command should be used: :: 

  $ rpm -Uhv http://www.percona.com/downloads/percona-release/percona-release-0.0-1.x86_64.rpm

Following command will install |Percona XtraDB Cluster| packages: :: 

  $ yum install Percona-XtraDB-Cluster-server-55 Percona-XtraDB-Cluster-client-55 Percona-XtraDB-Cluster-galera-2

When these two commands have been executed successfully on all three nodes |Percona XtraDB Cluster| is installed.

Configuring the nodes
---------------------

Individual nodes should be configured to be able to bootstrap the cluster. More details about bootstrapping the cluster can be found in the :ref:`bootstrap` guide.

Configuration file :file:`/etc/my.cnf` for the first node should look like: ::

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

  # This is a recommended tuning variable for performance
  innodb_locks_unsafe_for_binlog=1

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #1 address
  wsrep_node_address=192.168.70.71

  # SST method
  wsrep_sst_method=xtrabackup

  # Cluster name
  wsrep_cluster_name=my_centos_cluster

  # Authentication for SST method
  wsrep_sst_auth="sstuser:s3cret"


After this, first node can be started with the following command: ::

  [root@percona1 ~]# /etc/init.d/mysql start --wsrep-cluster-address="gcomm://"
 
This command will start the cluster with initial :variable:`wsrep_cluster_address` set to ``gcomm://``. This way the cluster will be bootstrapped and in case the node or |MySQL| have to be restarted later, there would be no need to change the configuration file.

After the first node has been started, cluster status can be checked by: 

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

It's recommended not to leave the empty password for the root account. Password can be changed with: 

.. code-block:: mysql 

  mysql@percona1> UPDATE mysql.user SET password=PASSWORD("Passw0rd") where user='root';
  mysql@percona1> FLUSH PRIVILEGES;

In order to perform successful :ref:`state_snapshot_transfer` using |XtraBackup| new user needs to be set up with proper `privileges <http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_: 

.. code-block:: mysql

  mysql@percona1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 's3cret';
  mysql@percona1> GRANT RELOAD, LOCK TABLES, REPLICATION CLIENT ON *.* TO 'sstuser'@'localhost';
  mysql@percona1> FLUSH PRIVILEGES;


.. note:: 

 MySQL root account can also be used for setting up the SST with Percona XtraBackup, but it's recommended to use a different (non-root) user for this.

Configuration file :file:`/etc/my.cnf` on the second node (``percona2``) should look like this: ::

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

  # This is a recommended tuning variable for performance
  innodb_locks_unsafe_for_binlog=1

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #2 address
  wsrep_node_address=192.168.70.72

  # Cluster name
  wsrep_cluster_name=my_centos_cluster

  # SST method
  wsrep_sst_method=xtrabackup

  #Authentication for SST method
  wsrep_sst_auth="sstuser:s3cret"
 
Second node can be started with the following command: ::

  [root@percona2 ~]# /etc/init.d/mysql start

After the server has been started it should receive the state snapshot transfer automatically. This means that the second node won't have the empty root password anymore. In order to connect to the cluster and check the status changed root password from the first node should be used. Cluster status can now be checked on both nodes. This is the example from the second node (``percona2``): 

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

MySQL configuration file :file:`/etc/my.cnf` on the third node (``percona3``) should look like this: ::

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

  # This is a recommended tuning variable for performance
  innodb_locks_unsafe_for_binlog=1

  # This changes how InnoDB autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

  # Node #3 address
  wsrep_node_address=192.168.70.73

  # Cluster name
  wsrep_cluster_name=my_centos_cluster

  # SST method
  wsrep_sst_method=xtrabackup

  #Authentication for SST method
  wsrep_sst_auth="sstuser:s3cret"

Third node can now be started with the following command: :: 

  [root@percona3 ~]# /etc/init.d/mysql start

After the server has been started it should receive the SST same as the second node. Cluster status can now be checked on both nodes. This is the example from the third node (``percona3``): 

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

Testing the replication
-----------------------

Although the password change from the first node has replicated successfully, this example will show that writing on any node will replicate to the whole cluster. In order to check this, new database will be created on second node and table for that database will be created on the third node.

Creating the new database on the second node: 

.. code-block:: mysql 

  mysql@percona2> CREATE DATABASE percona;
  Query OK, 1 row affected (0.01 sec)

Creating the ``example`` table on the third node: 
  
.. code-block:: mysql 

  mysql@percona3> USE percona;
  Database changed

  mysql@percona3> CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));
  Query OK, 0 rows affected (0.05 sec)

Inserting records on the first node: 

.. code-block:: mysql 

  mysql@percona1> INSERT INTO percona.example VALUES (1, 'percona1');
  Query OK, 1 row affected (0.02 sec)

Retrieving all the rows from that table on the second node: 

.. code-block:: mysql 

  mysql@percona2> SELECT * FROM percona.example;
  +---------+-----------+
  | node_id | node_name |
  +---------+-----------+
  |       1 | percona1  |
  +---------+-----------+
  1 row in set (0.00 sec)

This small example shows that all nodes in the cluster are synchronized and working as intended.
