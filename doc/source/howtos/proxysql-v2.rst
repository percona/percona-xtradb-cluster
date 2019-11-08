.. _pxc.proxysql.v2:

================================================================================
The |proxysql-admin| Tool with ProxySQL v2
================================================================================

With |proxysql| 2.0.3, the |proxysql-admin| tool now uses the native ProxySQL
support for |PXC| and does not require custom bash scripts to keep track of PXC
status. As a result, proxysql_galera_checker and proxysql_node_monitor have been
removed.

Summary of Changes in |proxysql-admin| Tool with ProxySQL v2
================================================================================

.. contents::
   :local:

Added Features
--------------------------------------------------------------------------------

- New option ``--use-ssl`` to use SSL for connections between ProxySQL and the backend database servers
- New option ``--max-transactions-behind`` to determine the maximum number of writesets that can be queued before the node is SHUNNED to avoid stale reads. The default value is 100
- New operation ``--update-cluster`` to update the cluster membership by adding server nodes as found. (Note that nodes are added but not removed).  The ``--writer-hg`` option may be used to specify which galera hostgroup to update. The ``--remove-all-servers`` option instructs to remove all servers from the mysql_servers table before updating the cluster.
- Hostgroups can be specified on the command-line: ``--writer-hg``, ``--reader-hg``, ``--backup-writer-hg``, and ``--offline-hg``.
  Previously, these host groups were only read from the configuration file.
- The ``--enable`` and ``--update-cluster`` options used simultaneously have special meaning. If the cluster has not been enabled, then ``--enable`` is run.  If the cluster has already been enabled, then ``--update-cluster`` is run.
- New command ``--is-enabled`` to see if a cluster has been enabled. This command checks for the existence of a row in the mysql_galera_hostgroups table.  The ``--writer-hg`` option may be used to specify the writer hostgroup used to search the mysql_galera_hostgroups table.
- New command ``--status`` to display galera hostgroup information. This command lists all rows in the current ``mysql_galera_hostgroups`` table as well as all servers that belong to these hostgroups.  With the ``--writer-hg`` option, only the information for the galera hostgroup with that writer hostgroup is displayed.

Changed Features
--------------------------------------------------------------------------------

- Setting ``--node-check-interval`` changes the ProxySQL global variable ``mysql-monitor_galera_healthcheck_interval``. Note that this is a global variable, not a per-cluster variable.
- The option ``--write-node``  takes only a single address as a parameter. In the singlewrite mode we only set the weight if ``--write-node`` specifies *address:port*.  A priority list of addresses is no longer accepted.
- The option ``--writers-as-readers`` option accepts a different set of values. Due to changes in the behavior of ProxySQL between version 1.4 and version 2.0 related to Galera support, the values of ``--writers-as-readers`` have been changed.  This option accepts the following values: yes, no, and backup.

  :yes: writers, backup-writers, and read-only nodes can act as readers.
  :no: only read-only nodes can act as readers.
  :backup: only backup-writers can act as readers.

- The commands ``--syncusers``, ``--sync-multi-cluster-users``, ``--adduser``, and ``--disable`` can use the ``--writer-hg`` option.
- The command ``--disable`` removes all users associated with the galera cluster hostgroups. Previously, this command only removed the users with the **CLUSTER_APP_USERNAME**.
- The command ``--disable`` accepts the ``--writer-hg`` option to disable the Galera cluster associated with that hostgroup overriding the value specified in the configuration file.

Removed Features
--------------------------------------------------------------------------------

- Asynchronous slave reader support has been removed: the ``--include-slaves`` option is not supported.
- A list of nodes in the priority order is not supported in |proxysql| v2. Only a single node is supported at this time.
- Since the galera_proxysql_checker and galera_node_monitor scripts are no longer run in the scheduler, automatic cluster membership updates are not supported.
- Checking the pxc_maint_mode variable is no longer supported
- Using desynced nodes if no other nodes are available is no longer supported.
- The server status is no longer maintained in the mysql_servers table.

Limitations
--------------------------------------------------------------------------------

- With ``--writers-as-readers=backup`` read-only nodes are not allowed. This a
  limitation of ProxySQL 2.0.  Note that backup is the default value of
  ``--writers-as-readers`` when ``--mode=singlewrite``

Installing ProxySQL v2
================================================================================

If that is what you used to :ref:`install PXC <install>` or any other Percona
software, run the corresponding command:

* Installing on Debian or Ubuntu:

.. code-block:: bash

   $ sudo apt-get install proxysql2

* Installing on Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

   $ sudo yum install proxysql2

Alternatively, you can download the packages from
https://www.percona.com/downloads/proxysql2/.

To start ProxySQL, run the following command:

.. code-block:: bash

   $ sudo service proxysql2 start

.. _default-credentials:

.. warning::

   **Do not run |proxysql| with default credentials in production.**

   Before starting the ``proxysql`` service, you can change the
   defaults in the :file:`/etc/proxysql.cnf` file by changing the
   ``admin_credentials`` variable.  For more information, see `Global
   Variables
   <https://github.com/sysown/proxysql/blob/master/doc/global_variables.md>`_.

Automatic Configuration
================================================================================

The ``proxysql2`` package from Percona includes the ``proxysql-admin`` tool for
configuring |PXC| nodes with ProxySQL.

Before using the ``proxysql-admin`` tool, ensure that ProxySQL and |PXC| nodes
you want to add are running. For security purposes, please ensure to change the
default user settings in the ProxySQL configuration file.

.. important::

   The ``proxysql-admin`` tool can only be used for *initial* ProxySQL
   configuration.

   The *ProxySQL Admin* (|proxysql-admin| tool) is specially developed by
   Percona to automate this configuration. Bug reports and feature proposals
   are welcome in the ProxySQL Admin `issue tracking system
   <https://jira.percona.com/projects/PSQLADM>`_.

To view the usage information, run ``proxysql-admin`` without any options:

.. code-block:: text

   Usage: proxysql-admin [ options ]
   Options:

   --config-file=<config-file>        Read login credentials from a configuration file
                                      (command line options override any configuration file values)
 
   --writer-hg=<number>               The hostgroup that all traffic will be sent to
                                      by default. Nodes that have 'read-only=0' in MySQL
                                      will be assigned to this hostgroup.
   --backup-writer-hg=<number>        If the cluster has multiple nodes with 'read-only=0'
                                      and max_writers set, then additional nodes (in excess
                                      of max_writers), will be assigned to this hostgroup.
   --reader-hg=<number>               The hostgroup that read traffic should be sent to.
                                      Nodes with 'read-only=0' in MySQL will be assigned
                                      to this hostgroup.
   --offline-hg=<number>              Nodes that are determined to be OFFLINE will
                                      assigned to this hostgroup.
 
   --proxysql-datadir=<datadir>       Specify the proxysql data directory location
   --proxysql-username=<user_name>    ProxySQL service username
   --proxysql-password[=<password>]   ProxySQL service password
   --proxysql-port=<port_num>         ProxySQL service port number
   --proxysql-hostname=<host_name>    ProxySQL service hostname
 
   --cluster-username=<user_name>     Percona XtraDB Cluster node username
   --cluster-password[=<password>]    Percona XtraDB Cluster node password
   --cluster-port=<port_num>          Percona XtraDB Cluster node port number
   --cluster-hostname=<host_name>     Percona XtraDB Cluster node hostname
 
   --cluster-app-username=<user_name> Percona XtraDB Cluster node application username
   --cluster-app-password[=<password>] Percona XtraDB Cluster node application passwrod
   --without-cluster-app-user         Configure Percona XtraDB Cluster without application user
 
   --monitor-username=<user_name>     Username for monitoring Percona XtraDB Cluster nodes through ProxySQL
   --monitor-password[=<password>]    Password for monitoring Percona XtraDB Cluster nodes through ProxySQL
   --use-existing-monitor-password    Do not prompt for a new monitor password if one is provided.
 
   --node-check-interval=<NUMBER>     The interval at which the proxy should connect
                                      to the backend servers in order to monitor the
                                      Galera staus of a node (in milliseconds).
                                      (default: 5000)
   --mode=[loadbal|singlewrite]       ProxySQL read/write configuration mode
                                      currently supporting: 'loadbal' and 'singlewrite'
                                      (default: 'singlewrite')
   --write-node=<IPADDRESS>:<PORT>    Specifies the node that is to be used for
                                      writes for singlewrite mode.  If left unspecified,
                                      the cluster node is then used as the write node.
                                      This only applies when 'mode=singlewrite' is used.
   --max-connections=<NUMBER>         Value for max_connections in the mysql_servers table.
                                      This is the maximum number of connections that
                                      ProxySQL will open to the backend servers.
                                      (default: 1000)
   --max-transactions-behind=<NUMBER> Determines the maximum number of writesets a node
                                      can have queued before the node is SHUNNED to avoid
                                      stale reads.
                                      (default: 100)
   --use-ssl=[yes|no]                 If set to 'yes', then connections between ProxySQL
                                      and the backend servers will use SSL.
                                      (default: no)
   --writers-are-readers=[yes|no|backup]
                                      If set to 'yes', then all writers (backup-writers also)
                                      are added to the reader hostgroup.
                                      If set to 'no', then none of the writers (backup-writers also)
                                      will be added to the reader hostgroup.
                                      If set to 'backup', then only the backup-writers
                                      will be added to the reader hostgroup.
                                      (default: backup)
   --remove-all-servers               When used with --update-cluster, this will remove all
                                      servers belonging to the current cluster before
                                      updating the list.
   --debug                            Enables additional debug logging.
   --help                             Dispalys this help text.
 
   These options are the possible operations for proxysql-admin.
   One of the options below must be provided.
   --adduser                          Adds the Percona XtraDB Cluster application user to the ProxySQL database
   --disable, -d                      Remove any Percona XtraDB Cluster configurations from ProxySQL
   --enable, -e                       Auto-configure Percona XtraDB Cluster nodes into ProxySQL
   --update-cluster                   Updates the cluster membership, adds new cluster nodes
                                      to the configuration.
   --update-mysql-version             Updates the `mysql-server_version` variable in ProxySQL with the version
                                      from a node in the cluster.
   --quick-demo                       Setup a quick demo with no authentication
   --syncusers                        Sync user accounts currently configured in MySQL to ProxySQL
                                      May be used with --enable.
                                      (deletes ProxySQL users not in MySQL)
   --sync-multi-cluster-users         Sync user accounts currently configured in MySQL to ProxySQL
                                      May be used with --enable.
                                      (doesn't delete ProxySQL users not in MySQL)
   --add-query-rule                   Create query rules for synced mysql user. This is applicable only
                                      for singlewrite mode and works only with --syncusers
                                      and --sync-multi-cluster-users options
   --is-enabled                       Checks if the current configuration is enabled in ProxySQL.
   --status                           Returns a status report on the current configuration.
                                      If "--writer-hg=<NUM>" is specified, than the
                                      data corresponding to the galera cluster with that
                                      writer hostgroup is displayed. Otherwise, information
                                      for all clusters will be displayed.
   --force                            This option will skip existing configuration checks in mysql_servers, 
                                      mysql_users and mysql_galera_hostgroups tables. This option will only 
				      work with ``proxysql-admin --enable``.
   --disable-updates                  Disable admin updates for ProxySQL cluster for the
                                      current operation. The default is to not change the
                                      admin variable settings.  If this option is specifed,
                                      these options will be set to false.
                                      (default: updates are not disabled)
   --version, -v                      Prints the version info
 
Preparing Configuration File
================================================================================

It is recommended to provide the connection and authentication information in
the ProxySQL configuration file (:file:`/etc/proxysql-admin.cnf`). Do not
specify this information on the command line.

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
   export MONITOR_USERNAME="monitor"
   export MONITOR_PASSWORD="monit0r"
   
   # Application user to connect to pxc-node through proxysql
   export CLUSTER_APP_USERNAME="proxysql_user"
   export CLUSTER_APP_PASSWORD="passw0rd"
   
   # ProxySQL hostgroup IDs
   export WRITER_HOSTGROUP_ID='10'
   export READER_HOSTGROUP_ID='11'
   export BACKUP_WRITER_HOSTGROUP_ID='12'
   export OFFLINE_HOSTGROUP_ID='13'

   # ProxySQL read/write configuration mode.
   export MODE="singlewrite"

   # max_connections default (used only when INSERTing a new mysql_servers entry)
   export MAX_CONNECTIONS="1000"

   # Determines the maximum number of writesets a node can have queued
   # before the node is SHUNNED to avoid stale reads.
   export MAX_TRANSACTIONS_BEHIND=100

   # Connections to the backend servers (from ProxySQL) will use SSL
   export USE_SSL="no"

   # Determines if a node should be added to the reader hostgroup if it has
   # been promoted to the writer hostgroup.
   # If set to 'yes', then all writers (including backup-writers) are added to
   # the read hostgroup.
   # If set to 'no', then none of the writers (including backup-writers) are added.
   # If set to 'backup', then only the backup-writers will be added to
   # the read hostgroup.
   export WRITERS_ARE_READERS="backup"

.. _pxc.proxysql.v2.admin-tool:

Running |proxysql-admin| tool
================================================================================

It is recommended to :ref:`change default ProxySQL credentials
<default-credentials>` before running ProxySQL in production.  Make sure that
you provide ProxySQL location and credentials in the configuration file.

Provide superuser credentials for one of the |PXC| nodes.  The
``proxysql-admin`` script will detect other nodes in the cluster automatically.

.. contents::
   :local:

.. _pxc.proxysql.v2.admin-tool.enable:

--enable
--------------------------------------------------------------------------------

This option creates the entry for the Galera hostgroups and adds the |PXC| nodes
to ProxySQL.
  
It will also add two new users into the Percona XtraDB Cluster with the USAGE
privilege; one is for monitoring the cluster nodes through ProxySQL, and another
is for connecting to the PXC Cluster node via the ProxySQL console.

.. code-block:: bash  

   $ sudo proxysql-admin --config-file=/etc/proxysql-admin.cnf --enable

.. admonition:: Output

   .. code-block:: text

      This script will assist with configuring ProxySQL for use with
      Percona XtraDB Cluster (currently only PXC in combination
      with ProxySQL is supported)
      
      ProxySQL read/write configuration mode is singlewrite
      
      Configuring the ProxySQL monitoring user.
      ProxySQL monitor user name as per command line/config-file is monitor
      
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
      
      mysql --user=proxysql_user -p --host=localhost --port=6033 --protocol=tcp
   
You can use the following login credentials to connect your application through
ProxySQL:

.. code-block:: bash

   $ mysql --user=proxysql_user -p --host=127.0.0.1 --port=6033 --protocol=tcp
   mysql> select hostgroup_id,hostname,port,status from runtime_mysql_servers;

.. admonition:: Example of output

   .. code-block:: text

      +--------------+-----------+-------+--------+
      | hostgroup_id | hostname  | port  | status |
      +--------------+-----------+-------+--------+
      | 10           | 127.0.0.1 | 25000 | ONLINE |
      | 11           | 127.0.0.1 | 25100 | ONLINE |
      | 11           | 127.0.0.1 | 25200 | ONLINE |
      | 12           | 127.0.0.1 | 25100 | ONLINE |
      | 12           | 127.0.0.1 | 25200 | ONLINE |
      +--------------+-----------+-------+--------+
      5 rows in set (0.00 sec)


.. code-block:: mysql

   mysql> select * from mysql_galera_hostgroups\G

.. admonition:: Output

   .. code-block:: text

      writer_hostgroup: 10
      backup_writer_hostgroup: 12
      reader_hostgroup: 11
      offline_hostgroup: 13
      active: 1
      max_writers: 1
      writer_is_also_reader: 2
      max_transactions_behind: 100
      comment: NULL
      1 row in set (0.00 sec)

The ``--enable`` command may be used together with ``--update-cluster``.  If the
cluster has not been setup, then the enable function will be run.  If the
cluster has been setup, then the update cluster function will be run.

.. _pxc.proxysql.v2.admin-tool.disable:

--disable
--------------------------------------------------------------------------------

This option will remove Percona XtraDB Cluster nodes from ProxySQL and stop
the ProxySQL monitoring daemon.

.. code-block:: bash

   $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --disable

.. admonition:: Output

   .. code-block:: text

      Removing cluster application users from the ProxySQL database.
      Removing cluster nodes from the ProxySQL database.
      Removing query rules from the ProxySQL database if any.
      Removing the cluster from the ProxySQL database.
      ProxySQL configuration removed!
 
A specific galera cluster can be disabled by using the --writer-hg option with
``--disable``.

.. _pxc.proxysql.v2.admin-tool.adduser:

--adduser
--------------------------------------------------------------------------------

This option will aid with adding the Cluster application user to the ProxySQL
database for you

.. code-block:: bash

   $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --adduser

.. admonition:: Output

   .. code-block:: text

      Adding Percona XtraDB Cluster application user to ProxySQL database
      Enter Percona XtraDB Cluster application user name: root   
      Enter Percona XtraDB Cluster application user password: 
      Added Percona XtraDB Cluster application user to ProxySQL database!

.. _pxc.proxysql.v2.admin-tool.syncusers:

--syncusers
--------------------------------------------------------------------------------

This option will sync user accounts currently configured in Percona XtraDB Cluster
with the ProxySQL database except password-less users and admin users.
It also deletes ProxySQL users not in Percona XtraDB Cluster from the ProxySQL database.

.. code-block:: bash

   $ /usr/bin/proxysql-admin --syncusers

.. admonition:: Output

   .. code-block:: bash

      Syncing user accounts from Percona XtraDB Cluster to ProxySQL
      Synced Percona XtraDB Cluster users to the ProxySQL database!

.. rubric:: From ProxySQL DB

.. code-block:: mysql

   mysql> select username from mysql_users;

.. admonition:: Output

   +---------------+
   | username      |
   +---------------+
   | monitor       |
   | one           |
   | proxysql_user |
   | two           |
   +---------------+
   4 rows in set (0.00 sec)

.. rubric:: From PXC

.. code-block:: bash

   mysql> select user,host from mysql.user where authentication_string!='' and user not in ('admin','mysql.sys');

.. admonition:: Output

   +---------------+-------+
   | user          | host  |
   +---------------+-------+
   | monitor       | 192.% |
   | proxysql_user | 192.% |
   | two           | %     |
   | one           | %     |
   +---------------+-------+
   4 rows in set (0.00 sec)

.. _pxc.proxysql.v2.admin-tool.sync-multi-cluster-users:

--sync-multi-cluster-users
--------------------------------------------------------------------------------

This option works in the same way as --syncusers but it does not delete ProxySQL
users that are not present in the Percona XtraDB Cluster. It is to be used when
syncing proxysql instances that manage multiple clusters.

.. _pxc.proxysql.v2.admin-tool.add-query-rule:

--add-query-rule
--------------------------------------------------------------------------------

Create query rules for synced mysql user. This is applicable only for
singlewrite mode and works only with :ref:`pxc.proxysql.v2.admin-tool.syncusers`
and :ref:`pxc.proxysql.v2.admin-tool.sync-multi-cluster-users` options.

.. code-block:: text

   Syncing user accounts from PXC to ProxySQL

   Note : 'admin' is in proxysql admin user list, this user cannot be added to ProxySQL
   -- (For more info, see https://github.com/sysown/proxysql/issues/709)
   Adding user to ProxySQL: test_query_rule
   Added query rule for user: test_query_rule

   Synced PXC users to the ProxySQL database!

.. _pxc.proxysql.v2.admin-tool.quick-demo:

--quick-demo
--------------------------------------------------------------------------------

This option is used to setup a dummy proxysql configuration.

.. code-block:: bash

   $ sudo  proxysql-admin --quick-demo

.. admonition:: Output

   .. code-block:: text

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

.. code-block:: mysql

   mysql> select hostgroup_id,hostname,port,status from runtime_mysql_servers;

.. admonition:: Output

   .. code-block:: text

      +--------------+-----------+-------+--------+
      | hostgroup_id | hostname  | port  | status |
      +--------------+-----------+-------+--------+
      | 10           | 127.0.0.1 | 25000 | ONLINE |
      | 11           | 127.0.0.1 | 25100 | ONLINE |
      | 11           | 127.0.0.1 | 25200 | ONLINE |
      | 12           | 127.0.0.1 | 25100 | ONLINE |
      | 12           | 127.0.0.1 | 25200 | ONLINE |
      +--------------+-----------+-------+--------+
      5 rows in set (0.00 sec)

.. _pxc.proxysql.v2.admin-tool.update-cluster:

--update-cluster
--------------------------------------------------------------------------------

This option will check the Percona XtraDB Cluster to see if any new nodes have
joined the cluster.  If so, the new nodes are added to ProxySQL.  Any offline
nodes are not removed from the cluster by default.

If used with ``--remove-all-servers``, then the server list for this configuration
will be removed before running the update cluster function.

A specific galera cluster can be updated by using the ``--writer-hg`` option
with ``--update-cluster``.  Otherwise the cluster specified in the config file
will be updated.

If ``--write-node`` is used with ``--update-cluster``, then that node will
be made the writer node (by giving it a larger weight), if the node is in
the server list and is ONLINE.  This should only be used if the mode is _singlewrite_.

.. code-block:: bash

   $ sudo proxysql-admin --update-cluster --writer-hg=10 --remove-all-servers

.. admonition:: Output

   .. code-block:: text

      Removing all servers from ProxySQL
      Cluster node (127.0.0.1:25000) does not exist in ProxySQL, adding to the writer hostgroup(10)
      Cluster node (127.0.0.1:25100) does not exist in ProxySQL, adding to the writer hostgroup(10)
      Cluster node (127.0.0.1:25200) does not exist in ProxySQL, adding to the writer hostgroup(10)
      Waiting for ProxySQL to process the new nodes...

      Cluster node info
      +---------------+-------+-----------+-------+-----------+
      | hostgroup     | hg_id | hostname  | port  | weight    |
      +---------------+-------+-----------+-------+-----------+
      | writer        | 10    | 127.0.0.1 | 25000 | 1000      |
      | reader        | 11    | 127.0.0.1 | 25100 | 1000      |
      | reader        | 11    | 127.0.0.1 | 25200 | 1000      |
      | backup-writer | 12    | 127.0.0.1 | 25100 | 1000      |
      | backup-writer | 12    | 127.0.0.1 | 25200 | 1000      |
      +---------------+-------+-----------+------+------------+

      Cluster membership updated in the ProxySQL database!

.. _pxc.proxysql.v2.admin-tool.is-enabled:

--is-enabled
--------------------------------------------------------------------------------

This option will check if a galera cluster (specified by the writer hostgroup,
either from ``--writer-hg`` or from the config file) has any active entries
in the ``mysql_galera_hostgroups`` table in ProxySQL.

======  ========================================================================
Value   Is returned if there is
======  ========================================================================
0       An entry corresponding to the writer hostgroup and is set to *active*
        in ProxySQL.
1       No entry corresponding to the writer hostgroup.
2       An entry corresponding to the writer hostgroup but is not active.
======  ========================================================================

.. code-block:: bash

   $ sudo proxysql-admin --is-enabled --writer-hg=10
   The current configuration has been enabled and is active

   $ sudo proxysql-admin --is-enabled --writer-hg=20
   ERROR (line:2925) : The current configuration has not been enabled

.. _pxc.proxysql.v2.admin-tool.status:

--status
--------------------------------------------------------------------------------

If used with the ``--writer-hg`` option, this will display information about the
given Galera cluster which uses that writer hostgroup.  Otherwise it will
display information about all Galera hostgroups (and their servers) being
supported by this ProxySQL instance.

.. code-block:: bash

   $ sudo proxysql-admin --status --writer-hg=10

.. admonition:: Output

   .. code-block:: bash

      mysql_galera_hostgroups row for writer-hostgroup: 10
      +--------+--------+---------------+---------+--------+-------------+-----------------------+------------------+
      | writer | reader | backup-writer | offline | active | max_writers | writer_is_also_reader | max_trans_behind |
      +--------+--------+---------------+---------+--------+-------------+-----------------------+------------------+
      | 10     | 11     | 12            | 13      | 1      | 1           | 2                     | 100              |
      +--------+--------+---------------+---------+--------+-------------+-----------------------+------------------+

      mysql_servers rows for this configuration
      +---------------+-------+-----------+-------+--------+-----------+----------+---------+-----------+
      | hostgroup     | hg_id | hostname  | port  | status | weight    | max_conn | use_ssl | gtid_port |
      +---------------+-------+-----------+-------+--------+-----------+----------+---------+-----------+
      | writer        | 10    | 127.0.0.1 | 25000 | ONLINE | 1000000   | 1000     | 0       | 0         |
      | reader        | 11    | 127.0.0.1 | 25100 | ONLINE | 1000      | 1000     | 0       | 0         |
      | reader        | 11    | 127.0.0.1 | 25200 | ONLINE | 1000      | 1000     | 0       | 0         |
      | backup-writer | 12    | 127.0.0.1 | 25100 | ONLINE | 1000      | 1000     | 0       | 0         |
      | backup-writer | 12    | 127.0.0.1 | 25200 | ONLINE | 1000      | 1000     | 0       | 0         |
      +---------------+-------+-----------+-------+--------+-----------+----------+---------+-----------+

.. _pxc.proxysql.v2.admin-tool.force:

--force
--------------------------------------------------------------------------------

This will skip existing configuration checks with the ``--enable`` option in
`mysql_servers`, `mysql_users`, and `mysql_galera_hostgroups` tables.

.. _pxc.proxysql.v2.admin-tool.update-mysql-version:

--update-mysql-version
--------------------------------------------------------------------------------

This option will updates mysql server version (specified by the writer
hostgroup, either from ``--writer-hg`` or from the config file) in proxysql db based
on online writer node.

.. code-block:: bash

   $  sudo proxysql-admin --update-mysql-version --writer-hg=10
   ProxySQL MySQL version changed to 5.7.26


Extra options
================================================================================

.. contents::
   :local:

.. _pxc.proxysql.v2.admin-tool.mode:

--mode
--------------------------------------------------------------------------------

This option allows you to setup the read/write mode for PXC cluster nodes in
the ProxySQL database based on the hostgroup. For now, the only supported modes
are `loadbal` and `singlewrite`. `singlewrite` is the default mode, and it will
configure Percona XtraDB Cluster to only accept writes on a single node only.
Depending on the value of ``--writers-are-readers``, the write node may
accept read requests also.

All other remaining nodes will be read-only and will only receive read statements.

With the ``--write-node`` option we can control which node ProxySQL will use as
the writer node. The writer node is specified as an address:port -
**10.0.0.51:3306** If ``--write-node`` is used, the writer node is given a weight of
**1000000** (the default weight is **1000**).

The mode `loadbal` on the other hand is a load balanced set of evenly weighted
read/write nodes.

.. rubric:: `singlewrite` mode setup:

.. code-block:: bash

   $ sudo grep "MODE" /etc/proxysql-admin.cnf
   $ export MODE="singlewrite"
   $ sudo proxysql-admin --config-file=/etc/proxysql-admin.cnf --write-node=127.0.0.1:25000 --enable

.. admonition:: Output

   .. code-block:: text

   ProxySQL read/write configuration mode is singlewrite
   [..]
   ProxySQL configuration completed!

.. code-block:: mysql

   mysql> select hostgroup_id,hostname,port,status from runtime_mysql_servers;

.. admonition:: Output

   .. code-block:: text

      +--------------+-----------+-------+--------+
      | hostgroup_id | hostname  | port  | status |
      +--------------+-----------+-------+--------+
      | 10           | 127.0.0.1 | 25000 | ONLINE |
      | 11           | 127.0.0.1 | 25100 | ONLINE |
      | 11           | 127.0.0.1 | 25200 | ONLINE |
      | 12           | 127.0.0.1 | 25100 | ONLINE |
      | 12           | 127.0.0.1 | 25200 | ONLINE |
      +--------------+-----------+-------+--------+
      5 rows in set (0.00 sec)

.. rubric:: `loadbal` mode setup

.. code-block:: bash

   $ sudo proxysql-admin --config-file=/etc/proxysql-admin.cnf --mode=loadbal --enable

.. admonition:: Output

   .. code-block:: bash

      This script will assist with configuring ProxySQL (currently only Percona XtraDB cluster in combination with ProxySQL is supported)

      ProxySQL read/write configuration mode is loadbal
      [..]
      ProxySQL has been successfully configured to use with Percona XtraDB Cluster

      You can use the following login credentials to connect your application through ProxySQL

.. code-block:: bash

   $ mysql --user=proxysql_user --password=*****  --host=127.0.0.1 --port=6033 --protocol=tcp

.. code-block:: mysql

   mysql> select hostgroup_id,hostname,port,status from runtime_mysql_servers;

.. admonition:: Output

   .. code-block:: text

      +--------------+-----------+-------+--------+
      | hostgroup_id | hostname  | port  | status |
      +--------------+-----------+-------+--------+
      | 10           | 127.0.0.1 | 25000 | ONLINE |
      | 10           | 127.0.0.1 | 25100 | ONLINE |
      | 10           | 127.0.0.1 | 25200 | ONLINE |
      +--------------+-----------+-------+--------+
      3 rows in set (0.01 sec)

.. _pxc.proxysql.v2.admin-tool.node-check-interval:

--node-check-interval
--------------------------------------------------------------------------------

This option configures the interval for the cluster node health monitoring by
ProxySQL (in milliseconds). This is a global variable and will be used by all
clusters that are being served by this ProxySQL instance.  This can only be
used with ``--enable``.

.. code-block:: bash

   $ proxysql-admin --config-file=/etc/proxysql-admin.cnf --node-check-interval=5000 --enable

.. _pxc.proxysql.v2.admin-tool.write-node:

--write-node
--------------------------------------------------------------------------------

This option is used to choose which node will be the writer node when the mode
is `singlewrite`.  This option can be used with `--enable` and `--update-cluster`.

A single IP address and port combination is expected.  For instance,
"--write-node=127.0.0.1:3306"

ProxySQL Status
================================================================================

Simple script to dump ProxySQL config and stats

.. code-block:: bash

   $ proxysql-status admin admin 127.0.0.1 6032

.. |proxysql| replace:: ProxySQL
.. |proxysql-admin| replace:: ``proxysql-admin``
