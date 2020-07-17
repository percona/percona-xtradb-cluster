.. _sandbox:

================================================================================
Setting up PXC reference architecture with ProxySQL
================================================================================

This manual describes how to set up |PXC| in a virtualized test sandbox.

The procedure assumes Amazon EC2 micro instances running CentOS 7.  However, it
should apply to any virtualization technology (for example, VirtualBox) with any
Linux distribution.

This manual requires three virtual machines for |PXC| nodes, and one for the
ProxySQL, which redirects requests to the nodes.  Running ProxySQL on an
application server, instead of having it as a dedicated entity, removes the
unnecessary extra network roundtrip, because the load balancing layer in |PXC|
scales well with application servers.

1. Install |PXC| on the three cluster nodes, as described in :ref:`yum`.

#. Install :ref:`ProxySQL <load_balancing_with_proxysql>` and ``sysbench`` on the client node:

   .. code-block:: bash

      yum -y install proxysql2 sysbench

#. Make sure that the :file:`my.cnf` configuration file on the first node
   contains the following::

      [mysqld]
      server_id=1
      binlog_format=ROW
      log_bin=mysql-bin
      wsrep_cluster_address=gcomm://
      wsrep_provider=/usr/lib/libgalera_smm.so
      datadir=/var/lib/mysql

      wsrep_slave_threads=2
      wsrep_cluster_name=pxctest
      wsrep_sst_method=xtrabackup
      wsrep_node_name=ip-10-112-39-98

      log_slave_updates

      innodb_autoinc_lock_mode=2
      innodb_buffer_pool_size=400M
      innodb_log_file_size=64M

#. Start the first node

#. Adjust the :file:`my.cnf` configuration files
   on the second and third nodes to contain the same configuration settings,
   except the following:

   * Second node::

        server_id=2
        wsrep_cluster_address=gcomm://10.116.39.76
        wsrep_node_name=ip-10-244-33-92

   * Third node::

        server_id=3
        wsrep_cluster_address=gcomm://10.116.39.76
        wsrep_node_name=ip-10-194-10-179

   .. note::

      * ``server_id`` can be any unique number
      * ``wsrep_cluster_address`` is the IP address of the first node
      * ``wsrep_node_name`` can be any unique name, for example,
        the output of the ``hostname`` command

#. Start the second and third nodes.

   When a new node joins the cluster, |SST| is performed by taking a backup
   using XtraBackup, then copying it to the new node.  After a successful |SST|,
   you should see the following in the error log:

   .. code-block:: text

      ...
      ... [Note] WSREP: State transfer required:
           Group state: 77c9da88-b965-11e1-0800-ea53b7b12451:97
           Local state: 00000000-0000-0000-0000-000000000000:-1
      ... [Note] WSREP: New cluster view: global state: 77c9da88-b965-11e1-0800-ea53b7b12451:97, view# 18: Primary, number of nodes: 3, my index: 0, protocol version 2
      ... [Warning] WSREP: Gap in state sequence. Need state transfer.
      ...
      
   For debugging information about the |SST|, you can check the :file:`sst.err`
   file and the error log.

   After |SST| finishes, you can check the cluster size as follows:

   .. code-block:: mysql

      mysql> show global status like 'wsrep_cluster_size';
      +--------------------+-------+
      | Variable_name      | Value |
      +--------------------+-------+
      | wsrep_cluster_size | 3     |
      +--------------------+-------+
      1 row in set (0.00 sec)

#. When all cluster nodes are started, configure ProxySQL using the admin interface.

   To connect to the ProxySQL admin interface, you need a ``mysql`` client.
   You can either connect to the admin interface from |PXC| nodes
   that already have the ``mysql`` client installed (Node 1, Node 2, Node 3)
   or install the client on Node 4 and connect locally:

   To connect to the admin interface, use the credentials, host name and port
   specified in the `global variables
   <https://github.com/sysown/proxysql/blob/master/doc/global_variables.md>`_.

   .. warning::

      Do not use default credentials in production!

   The following example shows how to connect to the ProxySQL admin interface
   with default credentials:

   .. code-block:: bash

      root@proxysql:~# mysql -u admin -padmin -h 127.0.0.1 -P 6032

      Welcome to the MySQL monitor.  Commands end with ; or \g.
      Your MySQL connection id is 2
      Server version: 5.5.30 (ProxySQL Admin Module)

      Copyright (c) 2009-2020 Percona LLC and/or its affiliates
      Copyright (c) 2000, 2020, Oracle and/or its affiliates. All rights reserved.

      Oracle is a registered trademark of Oracle Corporation and/or its
      affiliates. Other names may be trademarks of their respective
      owners.

      Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

      mysql>

   To see the ProxySQL databases and tables use the following commands:

   .. code-block:: text

      mysql> SHOW DATABASES;
      +-----+---------+-------------------------------+
      | seq | name    | file                          |
      +-----+---------+-------------------------------+
      | 0   | main    |                               |
      | 2   | disk    | /var/lib/proxysql/proxysql.db |
      | 3   | stats   |                               |
      | 4   | monitor |                               |
      +-----+---------+-------------------------------+
      4 rows in set (0.00 sec)

   .. code-block:: text

      mysql> SHOW TABLES;
      +--------------------------------------+
      | tables                               |
      +--------------------------------------+
      | global_variables                     |
      | mysql_collations                     |
      | mysql_query_rules                    |
      | mysql_replication_hostgroups         |
      | mysql_servers                        |
      | mysql_users                          |
      | runtime_global_variables             |
      | runtime_mysql_query_rules            |
      | runtime_mysql_replication_hostgroups |
      | runtime_mysql_servers                |
      | runtime_scheduler                    |
      | scheduler                            |
      +--------------------------------------+
      12 rows in set (0.00 sec)

   For more information about admin databases and tables, see `Admin Tables
   <https://github.com/sysown/proxysql/blob/master/doc/admin_tables.md>`_

   .. note::

      ProxySQL has 3 areas where the configuration can reside:

      * MEMORY (your current working place)
      * RUNTIME (the production settings)
      * DISK (durable configuration, saved inside an SQLITE database)

      When you change a parameter, you change it in MEMORY area.
      That is done by design to allow you to test the changes
      before pushing to production (RUNTIME), or save them to disk.

.. rubric:: Adding cluster nodes to ProxySQL

To configure the backend |PXC| nodes in ProxySQL, insert corresponding
records into the ``mysql_servers`` table.

ProxySQL uses the concept of *hostgroups* to group cluster nodes.  This enables
you to balance the load in a cluster by routing different types of traffic to
different groups.  There are many ways you can configure hostgroups (for example
master and slaves, read and write load, etc.)  and a every node can be a member
of multiple hostgroups.

This example adds three |PXC| nodes to the default hostgroup (``0``), which
receives both write and read traffic:

.. code-block:: text

   mysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.61',3306);
   mysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.62',3306);
   mysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.63',3306);

To see the nodes:

.. code-block:: text

   mysql> SELECT * FROM mysql_servers;

   +--------------+---------------+------+--------+     +---------+
   | hostgroup_id | hostname      | port | status | ... | comment |
   +--------------+---------------+------+--------+     +---------+
   | 0            | 192.168.70.61 | 3306 | ONLINE |     |         | 
   | 0            | 192.168.70.62 | 3306 | ONLINE |     |         | 
   | 0            | 192.168.70.63 | 3306 | ONLINE |     |         | 
   +--------------+---------------+------+--------+     +---------+
   3 rows in set (0.00 sec)

.. rubric:: Creating ProxySQL Monitoring User

To enable monitoring of |PXC| nodes in ProxySQL, create a user with ``USAGE``
privilege on any node in the cluster and configure the user in ProxySQL.

The following example shows how to add a monitoring user on Node 2:

 .. code-block:: text

    mysql> CREATE USER 'proxysql'@'%' IDENTIFIED BY 'ProxySQLPa55';
    mysql> GRANT USAGE ON *.* TO 'proxysql'@'%';

The following example shows how to configure this user on the ProxySQL node:

.. code-block:: text

   mysql> UPDATE global_variables SET variable_value='proxysql'
         WHERE variable_name='mysql-monitor_username';
   mysql> UPDATE global_variables SET variable_value='ProxySQLPa55'
         WHERE variable_name='mysql-monitor_password';

To load this configuration at runtime, issue a ``LOAD`` command.  To save these
changes to disk (ensuring that they persist after ProxySQL shuts down), issue a
``SAVE`` command.

.. code-block:: text

   mysql> LOAD MYSQL VARIABLES TO RUNTIME;
   mysql> SAVE MYSQL VARIABLES TO DISK;

To ensure that monitoring is enabled, check the monitoring logs:

.. code-block:: text

   mysql> SELECT * FROM monitor.mysql_server_connect_log ORDER BY time_start_us DESC LIMIT 6;
   +---------------+------+------------------+----------------------+---------------+
   | hostname      | port | time_start_us    | connect_success_time | connect_error |
   +---------------+------+------------------+----------------------+---------------+
   | 192.168.70.61 | 3306 | 1469635762434625 | 1695                 | NULL          |
   | 192.168.70.62 | 3306 | 1469635762434625 | 1779                 | NULL          |
   | 192.168.70.63 | 3306 | 1469635762434625 | 1627                 | NULL          |
   | 192.168.70.61 | 3306 | 1469635642434517 | 1557                 | NULL          |
   | 192.168.70.62 | 3306 | 1469635642434517 | 2737                 | NULL          |
   | 192.168.70.63 | 3306 | 1469635642434517 | 1447                 | NULL          |
   +---------------+------+------------------+----------------------+---------------+
   6 rows in set (0.00 sec)

.. code-block:: text

   mysql> SELECT * FROM monitor.mysql_server_ping_log ORDER BY time_start_us DESC LIMIT 6;
   +---------------+------+------------------+-------------------+------------+
   | hostname      | port | time_start_us    | ping_success_time | ping_error |
   +---------------+------+------------------+-------------------+------------+
   | 192.168.70.61 | 3306 | 1469635762416190 | 948               | NULL       |
   | 192.168.70.62 | 3306 | 1469635762416190 | 803               | NULL       |
   | 192.168.70.63 | 3306 | 1469635762416190 | 711               | NULL       |
   | 192.168.70.61 | 3306 | 1469635702416062 | 783               | NULL       |
   | 192.168.70.62 | 3306 | 1469635702416062 | 631               | NULL       |
   | 192.168.70.63 | 3306 | 1469635702416062 | 542               | NULL       |
   +---------------+------+------------------+-------------------+------------+
   6 rows in set (0.00 sec)

The previous examples show that ProxySQL is able to connect and ping the nodes
you added.

To enable monitoring of these nodes, load them at runtime:

.. code-block:: text

   mysql> LOAD MYSQL SERVERS TO RUNTIME;

.. _proxysql-client-user:

.. rubric:: Creating ProxySQL Client User

ProxySQL must have users that can access backend nodes to manage connections.

To add a user, insert credentials into ``mysql_users`` table:

.. code-block:: text

   mysql> INSERT INTO mysql_users (username,password) VALUES ('sbuser','sbpass');
   Query OK, 1 row affected (0.00 sec)

.. note::

   ProxySQL currently doesn't encrypt passwords.

Load the user into runtime space and save these changes to disk (ensuring that
they persist after ProxySQL shuts down):

.. code-block:: text

   mysql> LOAD MYSQL USERS TO RUNTIME;
   mysql> SAVE MYSQL USERS TO DISK;

To confirm that the user has been set up correctly, you can try to log in:

.. code-block:: bash

   root@proxysql:~# mysql -u sbuser -psbpass -h 127.0.0.1 -P 6033

   Welcome to the MySQL monitor.  Commands end with ; or \g.
   Your MySQL connection id is 1491
   Server version: 5.5.30 (ProxySQL)

   Copyright (c) 2009-2020 Percona LLC and/or its affiliates
   Copyright (c) 2000, 2020, Oracle and/or its affiliates. All rights reserved.

   Oracle is a registered trademark of Oracle Corporation and/or its
   affiliates. Other names may be trademarks of their respective
   owners.

   Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

To provide read/write access to the cluster for ProxySQL, add this user on one
of the |PXC| nodes:

.. code-block:: text

   mysql> CREATE USER 'sbuser'@'192.168.70.64' IDENTIFIED BY 'sbpass';
   Query OK, 0 rows affected (0.01 sec)

   mysql> GRANT ALL ON *.* TO 'sbuser'@'192.168.70.64';
   Query OK, 0 rows affected (0.00 sec)

Testing the cluster with sysbench
=================================

After you set up |PXC| in a sandbox, you can test it using
`sysbench <https://launchpad.net/sysbench/>`_.
This example shows how to do it with ``sysbench`` from the EPEL repository.

1. Create a database and a user for ``sysbench``:

   .. code-block:: mysql

      mysql> create database sbtest;
      Query OK, 1 row affected (0.01 sec)

      mysql> grant all on sbtest.* to 'sbtest'@'%' identified by 'sbpass';
      Query OK, 0 rows affected (0.00 sec)

      mysql> flush privileges;
      Query OK, 0 rows affected (0.00 sec)

#. Populate the table with data for the benchmark:

   .. code-block:: bash

      sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes \
      --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3307 \
      --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 prepare

#. Run the benchmark on port 3307:

   .. code-block:: bash

      sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes \
      --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3307 \
      --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 \
      --num-threads=8 run

#. Run the same benchmark on port 3306:

   .. code-block:: bash

      sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes \
      --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3306 \
      --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 \
      --num-threads=8 run
