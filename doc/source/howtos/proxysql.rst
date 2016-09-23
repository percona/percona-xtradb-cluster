.. _load_balancing_with_proxysql:

============================
Load balancing with ProxySQL
============================

`ProxySQL <http://www.proxysql.com>`_ is a high-performance SQL proxy.
ProxySQL runs as a daemon watched by a monitoring process.
The process monitors the daemon and restarts it in case of a crash
to minimize downtime.

The daemon accepts incoming traffic from |MySQL| clients
and forwards it to backend |MySQL| servers.

The proxy is designed to run continuously without needing to be restarted.
Most configuration can be done at runtime
using queries similar to SQL statements.
These include runtime parameters, server grouping,
and traffic-related settings.

ProxySQL supports |PXC| node status check using scheduler.

.. note:: For more information about ProxySQL, see `ProxySQL documentation
   <https://github.com/sysown/proxysql/tree/master/doc>`_.

Installing ProxySQL
===================

ProxySQL is available from the Percona software repositories.
If that is what you used to :ref:`install PXC <install>`
or any other Percona software, run the corresponding command:

* On Debian or Ubuntu:

.. code-block:: bash

   $ sudo apt-get install proxysql

* On Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

   $ sudo yum install proxysql

Alternatively, you can download packages from
https://www.percona.com/downloads/proxysql/.

To start ProxySQL, run the following command:

.. code-block:: bash

   $ sudo service proxysql start

.. _default-credentials:

.. warning:: Do not run ProxySQL with default credentials in production.

   Before starting the ``proxysql`` service,
   you can change the defaults in the :file:`/etc/proxysql.cnf` file
   by changing the ``admin_credentials`` variable.
   For more information, see `Global Variables
   <https://github.com/sysown/proxysql/blob/master/doc/global_variables.md>`_.

Automatic Configuration
=======================

The ``proxysql`` package from Percona includes the ``proxysql-admin`` tool
for configuring |PXC| nodes with ProxySQL::

 Usage: proxysql-admin [ options ]

 Options:
  --proxysql-user=user_name        User to use when connecting to the ProxySQL service
  --proxysql-password[=password]   Password to use when connecting to the ProxySQL service
  --proxysql-port=port_num         Port to use when connecting to the ProxySQL service
  --proxysql-host=host_name        Hostname to use when connecting to the ProxySQL service
  --cluster-user=user_name         User to use when connecting to the Percona XTraDB Cluster node
  --cluster-password[=password]    Password to use when connecting to the Percona XTraDB Cluster node
  --cluster-port=port_num          Port to use when connecting to the Percona XTraDB Cluster node
  --cluster-host=host_name         Hostname to use when connecting to the Percona XTraDB Cluster node
  --enable                         Auto-configure Percona XtraDB Cluster nodes into ProxySQL
  --disable                        Remove Percona XtraDB Cluster configurations from ProxySQL
  --galera-check-interval          Interval for monitoring proxysql_galera_checker script (in milliseconds)
  --mode                           ProxySQL read/write configuration mode, currently it only support 'loadbal' mode

.. note:: Before using the ``proxysql-admin`` tool,
   ensure that ProxySQL and nodes you want to add are running.

Enabling ProxySQL
-----------------

Use the ``--enable`` option to do the following:

* Add |PXC| node into the ProxySQL database

* Add the following monitoring scripts into the ProxySQL ``scheduler`` table,
  if they are not available:

  * ``proxysql_node_monitor`` checks cluster node membership
    and re-configures ProxySQL if the membership changes
  * ``proxysql_galera_checker`` checks for desynced nodes
    and temporarily deactivates them

* Create two new |PXC| users with the ``USAGE`` privilege on the node
  and add them to ProxySQL configuration, if they are not configured.
  One user is for monitoring cluster nodes,
  and the other one is for managing connections.

The following example shows how to add a |PXC| node
with IP address 10.101.6.1 to ProxySQL. 

.. code-block:: bash

   $ ./proxysql-admin --proxysql-user=admin --proxysql-password=admin \
        --proxysql-port=6032 --proxysql-host=127.0.0.1 \
        --cluster-user=root --cluster-password=root \
        --cluster-port=3306 --cluster-host=10.101.6.1 \
        --enable

   Configuring ProxySQL monitoring user..
   Enter ProxySQL monitoring username: monitor
   Enter ProxySQL monitoring password: 
   
   User monitor@'%' has been added with USAGE privilege
   
   Adding the Percona XtraDB Cluster server nodes to ProxySQL
   
   Configuring Percona XtraDB Cluster user to connect through ProxySQL
   Enter Percona XtraDB Cluster user name: proxysql_user
   Enter Percona XtraDB Cluster user password: 
   
   User proxysql_user@'%' has been added with USAGE privilege, please make sure to grant appropriate privileges
   
   Percona XtraDB Cluster ProxySQL monitoring daemon started
   ProxySQL configuration completed!

.. note:: The previous example uses default ProxySQL credentials,
   host name (in this case localhost) and port number.
   It is recommended to
   :ref:`change the default credentials <default-credentials>`
   before running ProxySQL in production.

.. note:: You must provide superuser credentials
   for the |PXC| node you are adding.

Disabling ProxySQL
------------------

Use the ``--disable`` option to do the following:

* Remove |PXC| node from the ProxySQL database

* Stop the ProxySQL monitoring daemon for this node

The following example shows how to disable ProxySQL
and remove the |PXC| node added in the previous example:

.. code-block:: bash

   $ ./proxysql-admin --proxysql-user=admin --proxysql-password=admin \
        --proxysql-port=6032 --proxysql-host=127.0.0.1 \
        --cluster-user=root --cluster-password=root \
        --cluster-port=3306 --cluster-host=10.101.6.1 \
        --disable

   ProxySQL configuration removed! 

Checking ProxySQL Status
------------------------

Use the ``--status`` option to check the ProxySQL status for a node.
The following example shows how to check the status of the node
that was disabled in the previous example:

.. code-block:: bash

   $ ./proxysql-admin --proxysql-user=admin --proxysql-password=admin \
        --proxysql-port=6032 --proxysql-host=127.0.0.1 \
        --cluster-user=root --cluster-password=root \
        --cluster-port=3306 --cluster-host=10.101.6.1 \
        --status

   Percona XtraDB Cluster ProxySQL monitoring daemon is not running

Manual Configuration
====================

This tutorial describes how to configure ProxySQL with three |PXC| nodes.

+--------+-----------+---------------+
| Node   | Host Name | IP address    |
+========+===========+===============+
| Node 1 | pxc1      | 192.168.70.61 |
+--------+-----------+---------------+
| Node 2 | pxc2      | 192.168.70.62 |
+--------+-----------+---------------+
| Node 3 | pxc3      | 192.168.70.63 |
+--------+-----------+---------------+
| Node 4 | proxysql  | 192.168.70.64 |
+--------+-----------+---------------+

ProxySQL can be configured either using the :file:`/etc/proxysql.cnf` file
or through the admin interface.
The recommended way to configure ProxySQL is using the admin interface.
This way you can change the configuration dynamically
(without having to restart the proxy).

To connect to the ProxySQL admin interface, you need a ``mysql`` client.
You can either connect to the admin interface from |PXC| nodes
that already have the ``mysql`` client installed (Node 1, Node 2, Node 3)
or install the client on Node 4 and connect locally.
For this tutorial, install |PXC| on Node 4:

* On Debian or Ubuntu:

  .. code-block:: bash

     root@proxysql:~# apt-get install percona-xtradb-cluster-client-5.7

* On Red Hat Enterprise Linux or CentOS:

  .. code-block:: bash

     [root@proxysql ~]# yum install Percona-XtraDB-Cluster-client-57

To connect to the admin interface,
use the credentials, host name and port specified in the `global variables
<https://github.com/sysown/proxysql/blob/master/doc/global_variables.md>`_.

.. warning:: Do not use default credentials in production!

The following example shows how to connect to the ProxySQL admin interface
with default credentials:

.. code-block:: bash

   root@proxysql:~# mysql -uadmin -padmin -h127.0.0.1 -P6032

   Welcome to the MySQL monitor.  Commands end with ; or \g.
   Your MySQL connection id is 2
   Server version: 5.1.30 (ProxySQL Admin Module)

   Copyright (c) 2009-2016 Percona LLC and/or its affiliates
   Copyright (c) 2000, 2016, Oracle and/or its affiliates. All rights reserved.

   Oracle is a registered trademark of Oracle Corporation and/or its
   affiliates. Other names may be trademarks of their respective
   owners.

   Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

   mysql@proxysql>

To see the ProxySQL databases and tables use the following commands:

.. code-block:: mysql

  mysql@proxysql> SHOW DATABASES;
  +-----+---------+-------------------------------+
  | seq | name    | file                          |
  +-----+---------+-------------------------------+
  | 0   | main    |                               |
  | 2   | disk    | /var/lib/proxysql/proxysql.db |
  | 3   | stats   |                               |
  | 4   | monitor |                               |
  +-----+---------+-------------------------------+
  4 rows in set (0.00 sec)

  mysql@proxysql> SHOW TABLES;
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

For more information about admin databases and tables,
see `Admin Tables
<https://github.com/sysown/proxysql/blob/master/doc/admin_tables.md>`_

.. note::

  ProxySQL has 3 areas where the configuration can reside:

  * MEMORY (your current working place)

  * RUNTIME (the production settings)

  * DISK (durable configuration, saved inside an SQLITE db)

  When you change a parameter, you change it in MEMORY area.
  That is done by design to allow you to test the changes
  before pushing to production (RUNTIME), or save them to disk.

--- NOT UPDATED BELOW ---

Adding the cluster nodes to ProxySQL
------------------------------------

Configuring the backends in ProxySQL is as easy as inserting records into
``mysql_servers`` table representing those backends, and specifying the correct
``hostgroup_id`` based on their roles:

This example adds three |PXC| nodes to write hostgroups, this means that all
three servers will be receiving both write and read traffic:

.. code-block:: mysql

   mysql@proxysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.61',3306);
   mysql@proxysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.62',3306);
   mysql@proxysql> INSERT INTO mysql_servers(hostgroup_id, hostname, port) VALUES (0,'192.168.70.63',3306);

To check if servers were added correctly you can run:

.. code-block:: mysql

  mysql@proxysql> SELECT * FROM mysql_servers;

  +--------------+---------------+------+--------+--------+-------------+-----------------+---------------------+---------+----------------+
  | hostgroup_id | hostname      | port | status | weight | compression | max_connections | max_replication_lag | use_ssl | max_latency_ms |
  +--------------+---------------+------+--------+--------+-------------+-----------------+---------------------+---------+----------------+
  | 0            | 192.168.70.61 | 3306 | ONLINE | 1      | 0           | 1000            | 0                   | 0       | 0              |
  | 0            | 192.168.70.62 | 3306 | ONLINE | 1      | 0           | 1000            | 0                   | 0       | 0              |
  | 0            | 192.168.70.63 | 3306 | ONLINE | 1      | 0           | 1000            | 0                   | 0       | 0              |
  +--------------+---------------+------+--------+--------+-------------+-----------------+---------------------+---------+----------------+
  3 rows in set (0.00 sec)

Monitoring
----------

ProxySQL constantly monitors the servers it has configured. To enable this you
will need to create and configure a user.

To create the user on the cluster add the user on any of the nodes, following
example adds the user on node2:

.. code-block:: mysql

  mysql@pxc2> CREATE USER 'proxysql'@'%' IDENTIFIED BY 'ProxySQLPa55';
  mysql@pxc2> GRANT USAGE ON *.* TO 'proxysql'@'%';

You need to configure the same user on the ProxySQL node:

.. code-block:: mysql

  mysql@proxysql> UPDATE global_variables SET variable_value='proxysql'
                WHERE variable_name='mysql-monitor_username';
  mysql@proxysql> UPDATE global_variables SET variable_value='ProxySQLPa55'
                WHERE variable_name='mysql-monitor_password';

At this stage the user is not activated, it's only configured. To load such
configuration at runtime you need to issue a ``LOAD`` command. Also these
configuration changes won't persist after ProxySQL is shutdown because they
re all in-memory. To persist these configuration changes you need to issue the
correct ``SAVE`` command to save these changes onto on-disk database
configuration.

Load the user into runtime and save the changes to disk:

.. code-block:: mysql

  mysql@proxysql> LOAD MYSQL VARIABLES TO RUNTIME;
  mysql@proxysql> SAVE MYSQL VARIABLES TO DISK;

You can now check if monitoring is working correctly:

.. code-block:: mysql

  mysql@proxysql> SELECT * FROM monitor.mysql_server_connect_log ORDER BY time_start DESC LIMIT 6;
  +---------------+------+------------------+----------------------+---------------+
  | hostname      | port | time_start       | connect_success_time | connect_error |
  +---------------+------+------------------+----------------------+---------------+
  | 192.168.70.61 | 3306 | 1469635762434625 | 1695                 | NULL          |
  | 192.168.70.62 | 3306 | 1469635762434625 | 1779                 | NULL          |
  | 192.168.70.63 | 3306 | 1469635762434625 | 1627                 | NULL          |
  | 192.168.70.61 | 3306 | 1469635642434517 | 1557                 | NULL          |
  | 192.168.70.62 | 3306 | 1469635642434517 | 2737                 | NULL          |
  | 192.168.70.63 | 3306 | 1469635642434517 | 1447                 | NULL          |
  +---------------+------+------------------+----------------------+---------------+
  6 rows in set (0.00 sec)

.. code-block:: mysql

  mysql> SELECT * FROM monitor.mysql_server_ping_log ORDER BY time_start DESC LIMIT 6;
  +---------------+------+------------------+-------------------+------------+
  | hostname      | port | time_start       | ping_success_time | ping_error |
  +---------------+------+------------------+-------------------+------------+
  | 192.168.70.61 | 3306 | 1469635762416190 | 948               | NULL       |
  | 192.168.70.62 | 3306 | 1469635762416190 | 803               | NULL       |
  | 192.168.70.63 | 3306 | 1469635762416190 | 711               | NULL       |
  | 192.168.70.61 | 3306 | 1469635702416062 | 783               | NULL       |
  | 192.168.70.62 | 3306 | 1469635702416062 | 631               | NULL       |
  | 192.168.70.63 | 3306 | 1469635702416062 | 542               | NULL       |
  +---------------+------+------------------+-------------------+------------+
  6 rows in set (0.00 sec)

With these simple checks you can see that ProxySQL is able to connect and ping
the servers you added.

Now that servers are alive and correctly monitored you can enable them:

.. code-block:: mysql

  mysql@proxysql> LOAD MYSQL SERVERS TO RUNTIME;

Monitoring the PXC nodes
~~~~~~~~~~~~~~~~~~~~~~~~

ProxySQL cannot detect a node which is not in a ``Synced`` state just using the
default ProxySQL monitoring. To check the |PXC| node status, ProxySQL has
developed an external :file:`proxysql_galera_checker` script, which
continuously monitors the |PXC| Node State Changes. You can find this script in
:file:`/usr/bin/proxysql_galera_checker`.

This script needs to be loaded with ProxySQL `Scheduler
<https://github.com/sysown/proxysql/blob/master/doc/scheduler.md>`_

Following command loads the script into Scheduler:

.. code-block:: mysql

  mysql@proxysql> INSERT INTO scheduler(id,interval_ms,filename,arg1,arg2,arg3,arg4)
    VALUES
    (1,'10000','/usr/bin/proxysql_galera_checker','127.0.0.1','6032','0',
    '/var/lib/proxysql/proxysql_galera_checker.log');

At this stage the job is not loaded at the runtime. You'll need to run ``LOAD
SCHEDULER TO RUNTIME`` command:

.. code-block:: mysql

  mysql@proxysql> LOAD SCHEDULER TO RUNTIME;

You can see in the :table:`runtime_scheduler` table if the script has been
loaded correctly:

.. code-block:: mysql

  mysql@proxysql> SELECT * FROM runtime_scheduler\G
  *************************** 1. row ***************************
           id: 1
  interval_ms: 10000
     filename: /usr/bin/proxysql/proxysql_galera_checker
         arg1: 127.0.0.1
         arg2: 6032
         arg3: 0
         arg4: /var/lib/proxysql/proxysql_galera_checker.log
         arg5: NULL
   1 row in set (0.00 sec)

To check the server status you should run:

.. code-block:: mysql

  mysql@proxysql> SELECT hostgroup_id,hostname,port,status FROM mysql_servers;
  +--------------+---------------+------+--------+
  | hostgroup_id | hostname      | port | status |
  +--------------+---------------+------+--------+
  | 0            | 192.168.70.61 | 3306 | ONLINE |
  | 0            | 192.168.70.62 | 3306 | ONLINE |
  | 0            | 192.168.70.63 | 3306 | ONLINE |
  +--------------+---------------+------+--------+
  3 rows in set (0.00 sec)

.. note::

  Each server can have the following status:

  * ``ONLINE`` - backend server is fully operational.

  * ``SHUNNED`` - backend sever is temporarily taken out of use because of
    either too many connection errors in a time that was too short, or
    replication lag exceeded the allowed threshold.

  * ``OFFLINE_SOFT`` - when a server is put into ``OFFLINE_SOFT`` mode, new
    incoming connections aren't accepted anymore, while the existing
    connections are kept until they became inactive. In other words,
    connections are kept in use until the current transaction is completed.
    This allows to gracefully detach a backend.

  * ``OFFLINE_HARD`` - when a server is put into ``OFFLINE_HARD`` mode, the
    existing connections are dropped, while new incoming connections aren't
    accepted either. This is equivalent to deleting the server from a
    hostgroup, or temporarily taking it out of the hostgroup for maintenance
    work.

Configuring the user
--------------------

ProxySQL must have users that can access backend server to manage the
connections.

You can add users by inserting them into ``mysql_users`` table:

.. code-block:: mysql

   mysql@proxysql> INSERT INTO mysql_users (username,password) VALUES ('sbuser','sbpass');
   Query OK, 1 row affected (0.00 sec)

.. note::

  ProxySQL currently doesn't encrypt the passwords.

For user to become active you'll need to load it into runtime:

.. code-block:: mysql

  mysql@proxysql> LOAD MYSQL USERS TO RUNTIME;

To confirm that the user has been set up correctly you can try to log in:

.. code-block:: bash

  root@proxysql:~# mysql -u sbuser -p sbpass -h 127.0.0.1 -P 6033

  Welcome to the MySQL monitor.  Commands end with ; or \g.
  Your MySQL connection id is 1491
  Server version: 5.1.30 (ProxySQL)

  Copyright (c) 2009-2016 Percona LLC and/or its affiliates
  Copyright (c) 2000, 2016, Oracle and/or its affiliates. All rights reserved.

  Oracle is a registered trademark of Oracle Corporation and/or its
  affiliates. Other names may be trademarks of their respective
  owners.

  Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

In order for ProxySQL to have read/write access to the cluster you'll need to
add the same user on one of the nodes:

.. code-block:: mysql

  mysql@pxc3> CREATE USER 'sbuser'@'192.168.70.64' IDENTIFIED BY 'sbpass';
  Query OK, 0 rows affected (0.01 sec)

  mysql@pxc3> GRANT ALL ON *.* TO 'sbuser'@'192.168.70.64';
  Query OK, 0 rows affected (0.00 sec)

Testing the cluster with sysbench
---------------------------------

You can install sysbench from Percona repositories by running:

.. code-block:: bash

  root@proxysql:~# apt-get install sysbench

on Debian/Ubuntu distributions, or:

.. code-block:: bash

  [root@proxysql ~]# yum install sysbench

on CentOS/RHEL distributions.

Create the database that will be used for testing:

.. code-block:: mysql

  mysql@pxc1> CREATE DATABASE sbtest;

Populate the table with data for the benchmark:

.. code-block:: bash

  root@proxysql:~# sysbench --report-interval=5 --num-threads=4 --num-requests=0 \
  --max-time=20 --test=/usr/share/doc/sysbench/tests/db/oltp.lua --mysql-user='sbuser' \
  --mysql-password='sbpass' --oltp-table-size=10000 --mysql-host=127.0.0.1 \
  --mysql-port=6033 prepare

You can now run the benchmark:

.. code-block:: bash

  root@proxysql:~# sysbench --report-interval=5 --num-threads=4 --num-requests=0 \
  --max-time=20 --test=/usr/share/doc/sysbench/tests/db/oltp.lua --mysql-user='sbuser' \
  --mysql-password='sbpass' --oltp-table-size=10000 --mysql-host=127.0.0.1 \
  --mysql-port=6033 run

ProxySQL is collecting a good set of information while working. The ``stats``
schema contains the relevant tables, to visualize the list:

.. code-block:: mysql

  mysql@proxysql> SHOW TABLES FROM stats;
  +--------------------------------+
  | tables                         |
  +--------------------------------+
  | stats_mysql_query_rules        |
  | stats_mysql_commands_counters  |
  | stats_mysql_processlist        |
  | stats_mysql_connection_pool    |
  | stats_mysql_query_digest       |
  | stats_mysql_query_digest_reset |
  | stats_mysql_global             |
  +--------------------------------+

For example you can see the number of commands that are being run on the
cluster by running:

.. code-block:: mysql

  mysql@proxysql> SELECT * FROM stats_mysql_commands_counters;
  +-------------------+---------------+-----------+-----------+-----------+---------+---------+----------+----------+-----------+-----------+--------+--------+---------+----------+
  | Command           | Total_Time_us | Total_cnt | cnt_100us | cnt_500us | cnt_1ms | cnt_5ms | cnt_10ms | cnt_50ms | cnt_100ms | cnt_500ms | cnt_1s | cnt_5s | cnt_10s | cnt_INFs |
  +-------------------+---------------+-----------+-----------+-----------+---------+---------+----------+----------+-----------+-----------+--------+--------+---------+----------+
  | ALTER_TABLE       | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | ANALYZE_TABLE     | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | BEGIN             | 2212625       | 3686      | 55        | 2162      | 899     | 569     | 1        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | CHANGE_MASTER     | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | COMMIT            | 21522591      | 3628      | 0         | 0         | 0       | 1765    | 1590     | 272      | 1         | 0         | 0      | 0      | 0       | 0        |
  | CREATE_DATABASE   | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | CREATE_INDEX      | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  ...
  | DELETE            | 2904130       | 3670      | 35        | 1546      | 1346    | 723     | 19       | 1        | 0         | 0         | 0      | 0      | 0       | 0        |
  | DESCRIBE          | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  ...
  | INSERT            | 19531649      | 3660      | 39        | 1588      | 1292    | 723     | 12       | 2        | 0         | 1         | 0      | 1      | 2       | 0        |
  ...
  | SELECT            | 35049794      | 51605     | 501       | 26180     | 16606   | 8241    | 70       | 3        | 4         | 0         | 0      | 0      | 0       | 0        |
  | SELECT_FOR_UPDATE | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  ...
  | UPDATE            | 6402302       | 7367      | 75        | 2503      | 3020    | 1743    | 23       | 3        | 0         | 0         | 0      | 0      | 0       | 0        |
  | USE               | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  | SHOW              | 19691         | 2         | 0         | 0         | 0       | 0       | 1        | 1        | 0         | 0         | 0      | 0      | 0       | 0        |
  | UNKNOWN           | 0             | 0         | 0         | 0         | 0       | 0       | 0        | 0        | 0         | 0         | 0      | 0      | 0       | 0        |
  +-------------------+---------------+-----------+-----------+-----------+---------+---------+----------+----------+-----------+-----------+--------+--------+---------+----------+
  45 rows in set (0.00 sec)

Automatic Fail-over
-------------------

ProxySQL will automatically detect if any of the nodes is not available or not
synced with the cluster.

You can check the status of the nodes by running:

.. code-block:: mysql

  mysql@proxysql> SELECT hostgroup_id,hostname,port,status FROM mysql_servers;
  +--------------+---------------+------+--------+
  | hostgroup_id | hostname      | port | status |
  +--------------+---------------+------+--------+
  | 0            | 192.168.70.61 | 3306 | ONLINE |
  | 0            | 192.168.70.62 | 3306 | ONLINE |
  | 0            | 192.168.70.63 | 3306 | ONLINE |
  +--------------+---------------+------+--------+
  3 rows in set (0.00 sec)

To see the problem detection we will now shutdown the node3 by running:

.. code-block:: bash

  root@pxc3:~# service mysql stop

ProxySQL will detect that node is down and update its status to
``OFFLINE_SOFT``:

.. code-block:: mysql

  mysql@proxysql> SELECT hostgroup_id,hostname,port,status FROM mysql_servers;
  +--------------+---------------+------+--------------+
  | hostgroup_id | hostname      | port | status       |
  +--------------+---------------+------+--------------+
  | 0            | 192.168.70.61 | 3306 | ONLINE       |
  | 0            | 192.168.70.62 | 3306 | ONLINE       |
  | 0            | 192.168.70.63 | 3306 | OFFLINE_SOFT |
  +--------------+---------------+------+--------------+
  3 rows in set (0.00 sec)

If you start the node3 again, scrip will detect the change and mark the node as
``ONLINE``:

.. code-block:: bash

  root@pxc3:~# service mysql start


.. code-block:: mysql

  mysql@proxysql> SELECT hostgroup_id,hostname,port,status FROM mysql_servers;
  +--------------+---------------+------+--------+
  | hostgroup_id | hostname      | port | status |
  +--------------+---------------+------+--------+
  | 0            | 192.168.70.61 | 3306 | ONLINE |
  | 0            | 192.168.70.62 | 3306 | ONLINE |
  | 0            | 192.168.70.63 | 3306 | ONLINE |
  +--------------+---------------+------+--------+
  3 rows in set (0.00 sec)

