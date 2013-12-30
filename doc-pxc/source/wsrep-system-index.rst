.. _wsrep_system_index:

=====================================
 Index of wsrep system variables
=====================================

.. variable:: wsrep_OSU_method

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: TOI

This variable can be used to select schema upgrade method. Available values are:
  * TOI - Total Order Isolation - When this method is selected ``DDL`` is processed in the same order with regards to other transactions in each cluster node. This guarantees data consistency. In case of ``DDL`` statements cluster will have parts of database locked and it will behave like a single server. In some cases (like big ``ALTER TABLE``) this could have impact on cluster's performance and high availability, but it could be fine for quick changes that happen almost instantly (like fast index changes). When ``DDL`` is processed under total order isolation (TOI) the ``DDL`` statement will be replicated up front to the cluster. i.e. cluster will assign global transaction ID for the ``DDL`` statement before the ``DDL`` processing begins. Then every node in the cluster has the responsibility to execute the ``DDL`` in the given slot in the sequence of incoming transactions, and this ``DDL`` execution has to happen with high priority. 
  * RSU - Rolling Schema Upgrade - When this method is selected ``DDL`` statements won't be replicated across the cluster, instead it's up to the user to run them on each node separately. The node applying the changes will desynchronize from the cluster briefly, while normal work happens on all the other nodes. When the ``DDL`` statement is processed node will apply delayed replication events. The schema changes **must** be backwards compatible for this method to work, otherwise the node that receives the change will likely break Galera replication. If the replication breaks the SST will be triggered when the node tries to join again but the change will be undone. 

.. variable:: wsrep_auto_increment_control

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON

This variable manages the :variable:`auto_increment_increment` and :variable:`auto_increment_offset` variables automatically depending on the size of the cluster. This helps prevent ``auto_increment`` replication conflicts across the cluster by giving each node it's own range of ``auto_increment`` values.  
This may not be desirable depending on application's use and assumptions of auto-increments. It can be turned off in Master/Slave clusters.

.. variable:: wsrep_causal_reads

   :cli: Yes
   :conf: Yes
   :scope: Global, Local
   :dyn: Yes
   :default: OFF

In some cases master may apply event faster than a slave, which can cause master and slave being out-of-sync for a brief moment. When this variable is set to ``ON`` slave will wait till that event is applied before doing any other queries. Enabling this variable will also result in larger latencies.

.. variable:: wsrep_certify_nonPK

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON

When this variable is enabled, primary keys will be generated automatically for the rows that the rows don't have them. Using tables without primary keys is not recommended.

.. variable:: wsrep_cluster_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No

This minimally needs to be any single other cluster node's address that is alive and a member of the cluster. In practice, it is best (but not necessary) to provide a complete list of all possible cluster nodes.  This takes the form of: :: 

 gcomm://<node:ip>,<node:ip>,<node:ip>

If an empty ``gcomm://`` is provided, this tells the node to bootstrap it self (i.e., form a new cluster). This is not recommended for production after the cluster has been bootstrapped initially. 

.. variable:: wsrep_cluster_name

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: my_wsrep_cluster

This is the name of the cluster and should be identical on all nodes belonging to the same cluster.

.. variable:: wsrep_convert_LOCK_to_trx

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable is used to convert ``LOCK/UNLOCK TABLES`` statements to ``BEGIN/COMMIT``. Although this can help some older applications to work with multi-master setup it can also result in having huge writesets.

.. variable:: wsrep_data_home_dir

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: mysql :term:`datadir`

This variable can be used to set up the directory where wsrep provider will store its files (like ``grastate.dat``).

.. variable:: wsrep_dbug_option

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable is used to send the ``DBUG`` option to the wsrep provider.

.. variable:: wsrep_debug

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

When this variable is set to ``ON``, debug messages will also be logged to the error_log. This can be used when trying to diagnose the problem or when submitting a bug.

.. variable:: wsrep_desync

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

When this variable is set to ``ON``, the node is desynced from the cluster. Toggling this back will require a IST or a SST depending on how long it was desynced. This is similar to desync which occurs during RSU TOI. This can also be done with  ``/*! WSREP_DESYNC */`` query comment.

.. variable:: wsrep_drupal_282555_workaround

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF 

This variable was introduced as workaround for Drupal/MySQL bug `#282555 <http://drupal.org/node/282555>`_. In some cases duplicate key error would occur when inserting the ``default`` value in into the ``auto_increment`` field. 

.. variable:: wsrep_forced_binlog_format

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: NONE

This variable defines a binlog format that will be always be effective regardless of session binlog format setting. Supported options for this variable are:
  * ROW
  * STATEMENT
  * MIXED
  * NONE - This option resets the forced state of the binlog format

.. variable:: wsrep_log_conflicts

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable is used to control whether sole cluster conflicts should be logged. When enabled details of conflicting |InnoDB| lock will be logged.

.. variable:: wsrep_max_ws_rows

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 131072 (128K) 

This variable is used to control maximum number of rows each writeset can contain. Anything bigger than this will be rejected.

.. variable:: wsrep_max_ws_size

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 1073741824 (1G)

This variable is used to control maximum writeset size (in bytes). Anything bigger than this will be rejected.

.. variable:: wsrep_mysql_replication_bundle

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0 (no grouping)
   :range: 0-1000

This variable controls how many replication events will be grouped together. Replication events are grouped in SQL slave thread by skipping events which may cause commit. This way the wsrep node acting in |MySQL| slave role and all other wsrep nodes in provider replication group, will see same (huge) transactions. This implementation is still experimental. This may help with the bottleneck of having only one |MySQL| slave facing commit time delay of synchronous provider.

.. variable:: wsrep_node_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :format: <ip address>[:port]
   :default: Usually set up as primary network interface (eth0)

This variable is used to specify the network address of the node. In some cases when there are multiple NICs available, state transfer might not work if the default NIC is on different network. Setting this variable explicitly to the correct value will makes SST and IST work correctly out of the box. Even in the multi-network setups, IST/SST can be configured to use other interfaces/addresses. 

.. variable:: wsrep_node_incoming_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: <:variable:`wsrep_node_address`>:3306

This is the address at which the node accepts client connections. This is information is used for status variable :variable:`wsrep_incoming_addresses` which shows all the active cluster nodes.

.. variable:: wsrep_node_name

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable is used to set up the unique node name.

.. variable:: wsrep_notify_cmd

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable is used to set the notification `command <http://www.codership.com/wiki/doku.php?id=notification_command>`_ that server will execute every time cluster membership or local node status changes.

.. variable:: wsrep_on

   :cli: No
   :conf: No
   :scope: Local, Global
   :dyn: Yes
   :default: ON

This variable is used to enable/disable wsrep replication. When set to ``OFF`` server will stop replication and behave like standalone |MySQL| server. 

.. variable:: wsrep_provider

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: None

This variable should contain the path to the Galera library (like :file:`/usr/lib64/libgalera_smm.so`).

.. variable:: wsrep_provider_options

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No

This variable contains settings currently used by Galera library.

.. variable:: wsrep_recover

   :cli: Yes
   :conf: No
   :scope: Global
   :dyn: No
   :default: OFF

When server is started with this variable (as ``--wsrep-recover``) it will parse Global Transaction ID from log, and if the GTID is found, output to stderr (which usually goes into the log). This option is used to recover GTID, mysqld is called with this automatically in mysqld_safe, hence running this manually is not required, also no need to set it in my.cnf.

.. variable:: wsrep_reject_queries

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: NONE

This variable can be used to reject queries for that node. This can be useful if someone wants to manually run maintenance on the node like mysqldump without need to change the settings on the load balancer. Following values are supported: 

 - ``NONE`` - default - nothing is rejected. 
 - ``ALL`` - all queries are rejected with 'Error 1047: Unknown command'. 
 - ``ALL_KILL`` - all queries are rejected and existing client connections are also killed without waiting. 
   
Note, that this doesn't affect galera replication in any way, only the applications which connect to database are affected. If you are looking for desyncing a node then :variable:`wsrep_desync` is the right option for that.
   
.. variable:: wsrep_replicate_myisam

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: Off

This variable controls if *MyISAM* will be replicated or not. *MyISAM* replication is still experimental and that is one of the reasons why this variable is set to ``OFF`` by default. 

.. variable:: wsrep_retry_autocommit

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable sets the number of times autocommitted transactions will be tried in the cluster if it encounters certification errors. In case there is a conflict, it should be safe for the cluster node to simply retry the statement without the client's knowledge with the hopes that it will pass the next time. This can be useful to help an application using autocommit to avoid the deadlock errors that can be triggered by replication conflicts. If this variable is set to ``0`` transaction won't be retried and if it is set to ``1`` it will be retried once.

.. variable:: wsrep_slave_threads

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable controls the number of threads that can apply replication transactions in parallel. Galera supports true parallel replication, replication that applies transactions in parallel only when it is safe to do so. The default value can be increased for better throughput. If any replication consistency problems are encountered, it's recommended to set this back to ``1`` to see if that resolves the issue.

.. variable:: wsrep_sst_auth

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :format: <username>:<password>

This variable should contain the authentication information needed for State Snapshot Transfer. Required information depends on the method selected in the :variable:`wsrep_sst_method`. More information about required authentication can be found in the :ref:`state_snapshot_transfer` documentation. This variable will appear masked in the logs and in the ``SHOW VARIABLES`` query.

.. variable:: wsrep_sst_donor

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable contains the name (:variable:`wsrep_node_name`) of the preferred donor for the SST. If no node is selected as a preferred donor it will be chosen from one of the available nodes automatically.

.. variable:: wsrep_sst_donor_rejects_queries

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

When this variable is enabled SST donor node will not accept incoming queries, instead it will reject queries with ``UNKNOWN COMMAND`` error code. This can be used to signal load-balancer that the node isn't available.

.. variable:: wsrep_sst_method

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: rsync
   :recommended: xtrabackup-v2

This variable sets up the method for taking the State Snapshot Transfer (SST). Available options are:
 * xtrabackup - uses Percona XtraBackup to perform the SST, this method requires :variable:`wsrep_sst_auth` to be set up with <user>:<password> which |XtraBackup| will use on donor. Privileges and permissions needed for running |XtraBackup| can be found `here <http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_.
 * xtrabackup-v2 - This is same as xtrabackup SST except that it uses newer protocol, hence is not compatible. This is the **recommended** option for PXC 5.5.34 and above. For more details, please check :ref:`xtrabackup_sst` and :ref:`errata`.
 * rsync - uses ``rsync`` to perform the SST, this method doesn't use the :variable:`wsrep_sst_auth`
 * mysqldump - uses ``mysqldump`` to perform the SST, this method requires :variable:`wsrep_sst_auth` to be set up with <user>:<password>, where user has root privileges on the server.
 * custom_script_name - Galera supports `Scriptable State Snapshot Transfer <http://www.codership.com/wiki/doku.php?id=scriptable_state_snapshot_transfer>`_. This enables users to create their own custom script for performing an SST.
 * skip - this option can be used to skip the SST, it can be used when initially starting the cluster and manually restore the same data to all nodes. It shouldn't be used as permanent setting because it could lead to data inconsistency across the nodes.

.. note:: 
    Note the following:
        * mysqldump SST is not recommended unless it is required for specific reasons. Also, it is not compatible with ``bind_address = 127.0.0.1 or localhost`` and will cause startup to fail if set so.
        * Xtrabackup-v2 SST is currently recommended if you have innodb-log-group_home-dir/innodb-data-home-dir in your cnf. Refer to :option:`sst-special-dirs` for more.

.. variable:: wsrep_sst_receive_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: AUTO

This variable is used to configure address on which the node expects the SST.

.. variable:: wsrep_start_position

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable contains the ``UUID:seqno`` value. By setting all the nodes to have the same value for this option cluster can be set up without the state transfer.

