.. orphan::

Using ProxySQL v1 with ``proxysql-admin``
================================================================================

ProxySQL Version 1.4 does not natively support |PXC| and |proxysql-admin| tool
requires custom bash scripts to keep track of PXC status:
``proxysql_galera_checker`` and ``proxysql_node_monitor``.

Installing ProxySQL v1
================================================================================

If that is what you used to :ref:`install PXC <install>` or any other Percona
software, run the corresponding command:

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
for configuring |PXC| nodes with ProxySQL.

.. note:: The *ProxySQL Admin* is specially developed by Percona to
   automate this configuration. Bug reports and feature proposals are welcome
   in the ProxySQL Admin `issue tracking system <https://jira.percona.com/projects/PSQLADM>`_.

.. note:: The ``proxysql-admin`` tool can only be used
   for *initial* ProxySQL configuration.

To view usage information, run ``proxysql-admin`` without any options:

.. code-block:: text

   Usage: [ options ]
   Options:
   --config-file=<config-file>        Read login credentials from a configuration file
                                      (command line options override any configuration file login credentials)
   --proxysql-datadir=<datadir>       Specify the proxysql data directory location
   --proxysql-username=user_name      ProxySQL service username
   --proxysql-password[=password]     ProxySQL service password
   --proxysql-port=port_num           ProxySQL service port number
   --proxysql-hostname=host_name      ProxySQL service hostname
   --cluster-username=user_name       Percona XtraDB Cluster node username
   --cluster-password[=password]      Percona XtraDB Cluster node password
   --cluster-port=port_num            Percona XtraDB Cluster node port number
   --cluster-hostname=host_name       Percona XtraDB Cluster node hostname
   --cluster-app-username=user_name   Percona XtraDB Cluster node application username
   --cluster-app-password[=password]  Percona XtraDB Cluster node application passwrod
   --without-cluster-app-user         Configure Percona XtraDB Cluster without application user
   --monitor-username=user_name       Username for monitoring Percona XtraDB Cluster nodes through ProxySQL
   --monitor-password[=password]      Password for monitoring Percona XtraDB Cluster nodes through ProxySQL
   --use-existing-monitor-password    Do not prompt for a new monitor password if one is provided.
   --node-check-interval=3000         Interval for monitoring node checker script (in milliseconds)
                                      (default: 3000)
   --mode=[loadbal|singlewrite]       ProxySQL read/write configuration mode
                                      currently supporting: 'loadbal' and 'singlewrite'
                                      (default: 'singlewrite')
   --write-node=host_name:port        Writer node to accept write statments.
                                      This option is supported only when using --mode=singlewrite
                                      Can accept comma delimited list with the first listed being
                                      the highest priority.
   --include-slaves=host_name:port    Add specified slave node(s) to ProxySQL, these nodes will go
                                      into the reader hostgroup and will only be put into
                                      the writer hostgroup if all cluster nodes are down (this
                                      depends on the value of --use-slave-as-writer).
                                      Slaves must be read only.  Can accept a comma delimited list.
                                      If this is used make sure 'read_only=1' is in the slave's my.cnf
   --use-slave-as-writer=<yes/no>     If this value is 'yes', then a slave may be used as a writer
                                      if the entire cluster is down. If 'no', then a slave
                                      will not be used as a writer. This option is required
                                      if '--include-slaves' is used.
   --writer-is-reader=<value>         Defines if the writer node also accepts writes.
                                      Possible values are 'always', 'never', and 'ondemand'.
                                      'ondemand' means that the writer node only accepts reads
                                      if there are no other readers.
                                      (default: 'ondemand')
   --max-connections=<NUMBER>         Value for max_connections in the mysql_servers table.
                                      This is the maximum number of connections that
                                      ProxySQL will open to the backend servers.
                                      (default: 1000)
   --debug                            Enables additional debug logging.
   --help                             Dispalys this help text.
 
   These options are the possible operations for proxysql-admin.
   One of the options below must be provided.
   --adduser                          Adds the Percona XtraDB Cluster application user to the ProxySQL database
   --disable, -d                      Remove any Percona XtraDB Cluster configurations from ProxySQL
   --enable, -e                       Auto-configure Percona XtraDB Cluster nodes into ProxySQL
   --quick-demo                       Setup a quick demo with no authentication
   --syncusers                        Sync user accounts currently configured in MySQL to ProxySQL
                                      May be used with --enable.
                                      (deletes ProxySQL users not in MySQL)
   --sync-multi-cluster-users         Sync user accounts currently configured in MySQL to ProxySQL
                                      May be used with --enable.
                                      (doesn't delete ProxySQL users not in MySQL)
   --version, -v                      Print version info   

.. note::

   Before using the ``proxysql-admin`` tool, ensure that ProxySQL and
   |PXC| nodes you want to add are running. For security purposes,
   please ensure to change the default user settings in the ProxySQL
   configuration file.

Preparing Configuration File
----------------------------

It is recommended to provide connection and authentication information
in the ProxySQL configuration file (:file:`/etc/proxysql-admin.cnf`),
instead of specifying it on the command line.

By default, the configuration file contains the following:

.. code-block:: text

   # proxysql admin interface credentials.
   export PROXYSQL_DATADIR='/var/lib/proxysql'
   export PROXYSQL_USERNAME='admin'
   export PROXYSQL_PASSWORD='admin'
   export PROXYSQL_HOSTNAME='localhost'
   export PROXYSQL_PORT='6032'
   
   # PXC admin credentials for connecting to pxc-cluster-node.
   export CLUSTER_USERNAME='admin'
   export CLUSTER_PASSWORD='admin'
   export CLUSTER_HOSTNAME='localhost'
   export CLUSTER_PORT='3306'
   
   # proxysql monitoring user. proxysql admin script will create this user in pxc to monitor pxc-nodes.
   export MONITOR_USERNAME='monitor'
   export MONITOR_PASSWORD='monit0r'
   
   # Application user to connect to pxc-node through proxysql
   export CLUSTER_APP_USERNAME='proxysql_user'
   export CLUSTER_APP_PASSWORD='passw0rd'
   
   # ProxySQL read/write hostgroup 
   export WRITE_HOSTGROUP_ID='10'
   export READ_HOSTGROUP_ID='11'
   
   # ProxySQL read/write configuration mode.
   export MODE="singlewrite"
   
   # Writer-is-reader configuration
   export WRITER_IS_READER="ondemand"
   
   # max_connections default (used only when INSERTing a new mysql_servers entry)
   export MAX_CONNECTIONS="1000"
    
   
.. note:: It is recommended to
   :ref:`change default ProxySQL credentials <default-credentials>`
   before running ProxySQL in production.
   Make sure that you provide ProxySQL location and credentials
   in the configuration file.

   Provide superuser credentials for one of the |PXC| nodes.
   The ``proxysql-admin`` script will detect
   other nodes in the cluster automatically.

Enabling ProxySQL
-----------------

Use the ``--enable`` option to automatically configure a |PXC| node
into ProxySQL.
The ``proxysql-admin`` tool will do the following:

* Add |PXC| node into the ProxySQL database

* Add the ``proxysql_galera_checker`` monitoring script
  into the ProxySQL ``scheduler`` table if it is not available.
  This script checks for desynced nodes and temporarily deactivates them.
  It also calls the ``proxysql_node_monitor`` script,
  which checks cluster node membership
  and re-configures ProxySQL if the membership changes.

* Create two new |PXC| users with the ``USAGE`` privilege on the node and add
  them to ProxySQL configuration, if they are not already configured.  ProxySQL
  uses one user for monitoring cluster nodes, and the other one is used for
  communicating with the cluster. Make sure to use super user credentials
  from Cluster to setup the default users.

.. warning::

   Running more then one copy of ``proxysql_galera_check`` in the same runtime
   environment simultaneously is not supported and may lead to undefined
   behavior.

   To avoid this problem, Galera process identification prevents a duplicate
   script execution in most cases. However, in some rare cases, it may be
   possible to circumvent this check if you run more then one copy of
   ``proxysql_galera_check``.

The following example shows how to add a |PXC| node
using the ProxySQL configuration file
with all necessary connection and authentication information:

.. code-block:: bash

   $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --enable

.. admonition:: Output

   .. code-block:: text
   
      This script will assist with configuring ProxySQL for use with
      Percona XtraDB Cluster (currently only PXC in combination with ProxySQL is supported)
   
      ProxySQL read/write configuration mode is singlewrite
   
      Configuring the ProxySQL monitoring user.  ProxySQL monitor user name as per
      command line/config-file is monitor
   
      User 'monitor'@'127.%' has been added with USAGE privileges
   
      Configuring the Percona XtraDB Cluster application user to connect through ProxySQL
      Percona XtraDB Cluster application user name as per command line/config-file is proxysql_user
   
      Percona XtraDB Cluster application user 'proxysql_user'@'127.%' has been added with ALL privileges, this user is created for testing purposes
      Adding the Percona XtraDB Cluster server nodes to ProxySQL
   
      Write node info

      +-----------+--------------+-------+--------+
      | hostname  | hostgroup_id | port  | weight |
      +-----------+--------------+-------+--------+
      | 127.0.0.1 | 10           | 26100 | 1000   |
      +-----------+--------------+-------+--------+
   
      ProxySQL configuration completed!
   
      ProxySQL has been successfully configured to use with Percona XtraDB Cluster
   
      You can use the following login credentials to connect your application through ProxySQL
   
      $ mysql --user=proxysql_user -p --host=localhost --port=6033 --protocol=tcp

.. code-block:: mysql

   mysql> select hostgroup_id,hostname,port,status,comment from mysql_servers;

.. admonition:: Output

   .. code-block:: text

      +--------------+-----------+-------+--------+---------+
      | hostgroup_id | hostname  | port  | status | comment |
      +--------------+-----------+-------+--------+---------+
      | 11           | 127.0.0.1 | 25400 | ONLINE | READ    |
      | 10           | 127.0.0.1 | 25000 | ONLINE | WRITE   |
      | 11           | 127.0.0.1 | 25100 | ONLINE | READ    |
      | 11           | 127.0.0.1 | 25200 | ONLINE | READ    |
      | 11           | 127.0.0.1 | 25300 | ONLINE | READ    |
      +--------------+-----------+-------+--------+---------+
      5 rows in set (0.00 sec)
   
Disabling ProxySQL
------------------

Use the ``--disable`` option to remove a |PXC| node's configuration
from ProxySQL.
The ``proxysql-admin`` tool will do the following:

* Remove |PXC| node from the ProxySQL database
* Stop the ProxySQL monitoring daemon for this node
* Remove the application user for this cluster
* Remove any query rules set up for this cluster

The following example shows how to disable ProxySQL
and remove the |PXC| node:

.. code-block:: bash

   $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --disable
   ProxySQL configuration removed!

Additional Options
------------------

The following extra options can be used:

* ``--adduser``

  Add |PXC| application user to ProxySQL database.

  .. code-block:: bash

     $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --adduser

     Adding Percona XtraDB Cluster application user to ProxySQL database
     Enter Percona XtraDB Cluster application user name: cluster_user
     Enter Percona XtraDB Cluster application user password: cluster_passw0Rd
     Added Percona XtraDB Cluster application user to ProxySQL database!

* ``--syncusers``

  Sync user accounts currently configured in |PXC| to ProxySQL database
  except users with no password and the ``admin`` user.

  .. note:: This option also deletes users
     that are not in |PXC| from ProxySQL database.

* ``--sync-multi-cluster-users``

  This option works in the same way as --syncusers but it does not delete ProxySQL
  users that are not present in the Percona XtraDB Cluster. It is to be used when
  syncing proxysql instances that manage multiple clusters.




* ``--node-check-interval``

  This option configures the interval for monitoring via the 
  ``proxysql_galera_checker`` script (in milliseconds).

  .. code-block:: bash

     $ proxysql-admin --config-file=/etc/proxysql-admin.cnf \
        --node-check-interval=5000 --enable

* ``--mode``

  Set the read/write mode for |PXC| nodes in ProxySQL database,
  based on the hostgroup.
  Supported modes are ``loadbal`` and ``singlewrite``.

  * ``singlewrite`` is the default mode,
    it will accept writes only on one single node
    (based on the info you provide in ``--write-node``).
    Remaining nodes will accept only read statements.

    Servers can be separated by commas, for example::

     10.0.0.51:3306,10.0.0.52:3306

    In the previous example, ``10.0.0.51:3306`` will be in the writer hostgroup
    if it is ONLINE.
    If it is OFFLINE, then ``10.0.0.52:3306`` will go into the writer hostgroup.
    And if that node also goes down, then one of the remaining nodes
    will be randomly chosen for the writer hostgroup.
    The configuration file is deleted when ``--disable`` is used.

  * ``singlewrite`` mode setup:

    .. code-block:: bash
 
       $ sudo grep "MODE" /etc/proxysql-admin.cnf
       export MODE="singlewrite"
       $ sudo proxysql-admin --config-file=/etc/proxysql-admin.cnf --write-node=127.0.0.1:25000 --enable
       ProxySQL read/write configuration mode is singlewrite
       [..]
       ProxySQL configuration completed!
  
    To check the configuration you can run:

    .. code-block:: text

       mysql> SELECT hostgroup_id,hostname,port,status,comment FROM mysql_servers;
       +--------------+-----------+-------+--------+---------+
       | hostgroup_id | hostname  | port  | status | comment |
       +--------------+-----------+-------+--------+---------+
       | 11           | 127.0.0.1 | 25400 | ONLINE | READ    |
       | 10           | 127.0.0.1 | 25000 | ONLINE | WRITE   |
       | 11           | 127.0.0.1 | 25100 | ONLINE | READ    |
       | 11           | 127.0.0.1 | 25200 | ONLINE | READ    |
       | 11           | 127.0.0.1 | 25300 | ONLINE | READ    |
       +--------------+-----------+-------+--------+---------+
       5 rows in set (0.00 sec)

  * The ``loadbal`` mode uses a set of evenly weighted read/write nodes.

    ``loadbal`` mode setup:

    .. code-block:: bash

       $ sudo proxysql-admin --config-file=/etc/proxysql-admin.cnf --mode=loadbal --enable

       This script will assist with configuring ProxySQL (currently only Percona XtraDB cluster in combination with ProxySQL is supported)

       ProxySQL read/write configuration mode is loadbal
       [..]
       ProxySQL has been successfully configured to use with Percona XtraDB Cluster

       You can use the following login credentials to connect your application through ProxySQL

       mysql --user=proxysql_user --password=*****  --host=127.0.0.1 --port=6033 --protocol=tcp 

    .. code-block:: text

       mysql> SELECT hostgroup_id,hostname,port,status,comment FROM mysql_servers;
       +--------------+-----------+-------+--------+-----------+
       | hostgroup_id | hostname  | port  | status | comment   |
       +--------------+-----------+-------+--------+-----------+
       | 10           | 127.0.0.1 | 25400 | ONLINE | READWRITE |
       | 10           | 127.0.0.1 | 25000 | ONLINE | READWRITE |
       | 10           | 127.0.0.1 | 25100 | ONLINE | READWRITE |
       | 10           | 127.0.0.1 | 25200 | ONLINE | READWRITE |
       | 10           | 127.0.0.1 | 25300 | ONLINE | READWRITE |
       +--------------+-----------+-------+--------+-----------+
       5 rows in set (0.01 sec)

* ``--quick-demo``

  This option is used to setup dummy ProxySQL configuration.

  .. code-block:: bash

    $ sudo  proxysql-admin  --enable --quick-demo

    You have selected the dry test run mode. WARNING: This will create a test user (with all privileges) in the Percona XtraDB Cluster & ProxySQL installations.

    You may want to delete this user after you complete your testing!

    Would you like to proceed with '--quick-demo' [y/n] ? y

    Setting up proxysql test configuration!

    Do you want to use the default ProxySQL credentials (admin:admin:6032:127.0.0.1) [y/n] ? y
    Do you want to use the default Percona XtraDB Cluster credentials (root::3306:127.0.0.1) [y/n] ? n

    Enter the Percona XtraDB Cluster username (super user): root
    Enter the Percona XtraDB Cluster user password: 
    Enter the Percona XtraDB Cluster port: 25100
    Enter the Percona XtraDB Cluster hostname: localhost


    ProxySQL read/write configuration mode is singlewrite

    Configuring ProxySQL monitoring user..

    User 'monitor'@'127.%' has been added with USAGE privilege

    Configuring the Percona XtraDB Cluster application user to connect through ProxySQL

    Percona XtraDB Cluster application user 'pxc_test_user'@'127.%' has been added with ALL privileges, this user is created for testing purposes

    Adding the Percona XtraDB Cluster server nodes to ProxySQL

    ProxySQL configuration completed!

    ProxySQL has been successfully configured to use with Percona XtraDB Cluster

    You can use the following login credentials to connect your application through ProxySQL

    mysql --user=pxc_test_user  --host=127.0.0.1 --port=6033 --protocol=tcp 

* ``--include-slaves=host_name:port``

  This option helps to include specified slave node(s) to ProxySQL database.
  These nodes will go into the reader hostgroup and will only be put into the
  writer hostgroup if all cluster nodes are down. Slaves must be read only. Can
  accept comma delimited list. If this is used, make sure ``read_only=1`` is
  included into the slave's ``my.cnf`` configuration file.

  .. note:: With ``loadbal`` mode slave hosts only accept read/write requests
     when all cluster nodes are down.

ProxySQL Status script
----------------------

There is a simple script to dump ProxySQL configuration and statistics:

.. code-block:: text

   Usage:

   proxysql-status admin admin 127.0.0.1 6032

.. |proxysql-admin| replace:: ``proxysql-admin``
