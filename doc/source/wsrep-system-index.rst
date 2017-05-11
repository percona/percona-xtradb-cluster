.. _wsrep_system_index:

===============================
Index of wsrep system variables
===============================

.. variable:: wsrep_auto_increment_control

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON

This variable manages the :variable:`auto_increment_increment` and :variable:`auto_increment_offset` variables automatically depending on the size of the cluster. This helps prevent ``auto_increment`` replication conflicts across the cluster by giving each node it's own range of ``auto_increment`` values.  
This may not be desirable depending on application's use and assumptions of auto-increments. It can be turned off in Master/Slave clusters.

.. variable:: wsrep_causal_reads

   :version 5.6.20-25.7: Variable deprecated
   :cli: Yes
   :conf: Yes
   :scope: Global, Session
   :dyn: Yes
   :default: OFF

In some cases, master may apply events faster than a slave,
which can cause master and slave to become out of sync for a brief moment.
When this variable is set to ``ON``, the slave will wait
until that event is applied before doing any other queries.
Enabling this variable will also result in larger latencies.

.. note:: This variable was deprecated because enabling it
   is the equivalent of setting :variable:`wsrep_sync_wait` to ``1``.

.. variable:: wsrep_certify_nonPK

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON

When this variable is enabled, primary keys will be generated automatically for rows that don't have them. Using tables without primary keys is not recommended.

.. variable:: wsrep_cluster_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable needs to specify at least one other node's address that is alive and a member of the cluster. In practice, it is best (but not necessary) to provide a complete list of all possible cluster nodes. The value should be of the following format: ::

 gcomm://<node1_ip>,<node2_ip>,<node3_ip>

Besides the IP address of the node, you can also specify port and options, for example: ::

 gcomm://192.168.0.1:4567?gmcast.listen_addr=0.0.0.0:5678

If an empty ``gcomm:/ /`` is provided, the node will bootstrap itself (that is, form a new cluster). This is not recommended for production after the cluster has been bootstrapped initially. If you want to bootstrap a new cluster, you should pass the ``--wsrep-new-cluster`` option when starting.

.. variable:: wsrep_cluster_name

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: my_wsrep_cluster

This is the name of the cluster and should be identical on all nodes belonging to the same cluster.

.. note:: It should not exceed 32 characters.

.. variable:: wsrep_convert_lock_to_trx

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

When this variable is set to ``ON``, debug messages will also be logged to the ``error_log``. This can be used when trying to diagnose problems or when submitting a bug.

.. variable:: wsrep_desync
 
   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF
 
This variable controls whether the node participates in Flow Control.
Setting this variable to ``ON`` does not automatically mean
that a node will be out of sync with the cluster.
It will continue to replicate the writesets as usual.
The only difference is that flow control will no longer
take care of the desynced node.
The result is that if :variable:`wsrep_local_recv_queue`
gets higher than maximum allowed,
all the other nodes will ignore the replication lag on the desynced node.
Toggling this back to ``OFF`` will require an IST or an SST,
depending on how long it was desynchronized.
This is similar to cluster desynchronization,
which occurs during RSU TOI.

It's not a good idea to keep desync set for a long period of time,
nor should you desync several nodes at once.
Also, you'll need to desync a node before it starts
causing flow control for it to have any effect.

A node can also be desynchronized with  ``/*! WSREP_DESYNC */`` query comment.

The following logic applies to desynced nodes:

* If a node is explicitly desynced,
  then implicitly desyncing a node using RSU/FTWRL is allowed.

* If a node is implicitly desynced using RSU/FTWRL,
  then explicitly desyncing a node is blocked
  until implicit desync is complete.

* If a node is explicitly desynced and then implicitly desycned using RSU/FTWRL, 
  then any request for another implicit desync is blocked
  until the former implicit desync is complete.

.. variable:: wsrep_dirty_reads

   :version 5.6.24-25.11: Variable introduced
   :version 5.6.26-25.12: Variable available both on session and global scope
   :cli: Yes
   :conf: Yes
   :scope: Session, Global
   :dyn: Yes
   :default: OFF

This variable is boolean and is ``OFF`` by default. When set to ``ON``, a |Percona XtraDB Cluster| node accepts queries that only read, but not modify data even if the node is in the non-PRIM state 

.. variable:: wsrep_drupal_282555_workaround

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF 

This variable was introduced as a workaround for Drupal/MySQL bug `#282555 <http://drupal.org/node/282555>`_. In some cases, duplicate key errors would occur when inserting the ``default`` value into the ``auto_increment`` field. 

.. variable:: wsrep_forced_binlog_format

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: NONE

This variable defines a binlog format that will always be effective regardless of session binlog format setting. Possible values for this variable are:
  * ``ROW``
  * ``STATEMENT``
  * ``MIXED``
  * ``NONE``: Resets the forced state of the binlog format (default)

.. variable:: wsrep_load_data_splitting

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON 

This variable controls whether ``LOAD DATA`` transaction splitting is wanted or not. It doesn't work as expected with ``autocommit=0`` when enabled.

.. variable:: wsrep_log_conflicts

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable is used to control whether sole cluster conflicts should be logged. When enabled, details of conflicting |InnoDB| lock will be logged.

.. variable:: wsrep_max_ws_rows

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ``0`` (no limit)

This variable is used to control the maximum number of rows
each writeset can contain.

By default, there is no limit for maximum number of rows in a writeset.
The maximum allowed value ``1048576``.

.. variable:: wsrep_max_ws_size

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ``2147483647`` (2 GB)

This variable is used to control maximum writeset size (in bytes).
Anything bigger than the specified value will be rejected.

You can set it to any value between ``1024`` and the default ``2147483647``.

.. variable:: wsrep_mysql_replication_bundle

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0 (no grouping)
   :range: 0-1000

This variable controls how many replication events will be grouped together. Replication events are grouped in SQL slave thread by skipping events which may cause commit. This way the wsrep node acting in |MySQL| slave role and all other wsrep nodes in provider replication group, will see same (huge) transactions. The implementation is still experimental. This may help with the bottleneck of having only one |MySQL| slave facing commit time delay of synchronous provider.

.. variable:: wsrep_node_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :format: <ip address>[:port]
   :default: Usually set up as primary network interface (``eth0``)

This variable is used to specify the network address of the node. In some cases, when there are multiple NICs available, state transfer might not work if the default NIC is on different network. Setting this variable explicitly to the correct value will make SST and IST work correctly out of the box. Even in multi-network setups, IST/SST can be configured to use other interfaces/addresses. 

.. variable:: wsrep_node_incoming_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: <:variable:`wsrep_node_address`>:3306

This is the address at which the node accepts client connections. This information is used for status variable :variable:`wsrep_incoming_addresses` which shows all the active cluster nodes.

.. variable:: wsrep_node_name

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: Host name

Unique name of the node. Defaults to the host name.

.. variable:: wsrep_notify_cmd

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable is used to set the `notification command <http://galeracluster.com/documentation-webpages/notificationcmd.html>`_ that the server should execute every time cluster membership or local node status changes.

.. variable:: wsrep_on

   :version 5.6.27-25.13: Variable available only in session scope
   :cli: No
   :conf: No
   :scope: Session
   :dyn: Yes
   :default: ON

This variable can be used to disable wsrep replication
from the current session to the rest of the cluster
without the node leaving the cluster.

.. variable:: wsrep_OSU_method

   :version 5.6.24-25.11: Variable available both in global and session scope
   :cli: Yes
   :conf: Yes
   :scope: Global and Session
   :dyn: Yes
   :default: TOI

This variable can be used to select schema upgrade method. Available values are:

* ``TOI``: When the *Total Order Isolation* method is selected, data definition language (DDL) statements are processed in the same order with regards to other transactions in each cluster node. This guarantees data consistency. In case of DDL statements, cluster will have parts of database locked and it will behave like a single server. In some cases (like big ``ALTER TABLE``) this could have impact on cluster's performance and high availability, but it could be fine for quick changes that happen almost instantly (like fast index changes). When DDL statements are processed under TOI, the DDL statement will be replicated up front to the cluster. That is, cluster will assign global transaction ID for the DDL statement before DDL processing begins. Then every node in the cluster has the responsibility to execute the DDL statement in the given slot in the sequence of incoming transactions, and this DDL execution has to happen with high priority. 

* ``RSU``: When the *Rolling Schema Upgrade* method is selected, DDL statements won't be replicated across the cluster, instead it's up to the user to run them on each node separately. The node applying the changes will desynchronize from the cluster briefly, while normal work happens on all the other nodes. When a DDL statement is processed, node will apply delayed replication events. The schema changes **must** be backwards compatible for this method to work, otherwise the node that receives the change will likely break Galera replication. If replication breaks, SST will be triggered when the node tries to join again but the change will be undone.

For more information, see :variable:`wsrep_desync`.

.. note:: Prior to |Percona XtraDB Cluster| :rn:`5.6.24-25.11`, :variable:`wsrep_OSU_method` was only a global variable. Current behavior is now consistent with |MySQL| behavior for variables that have both global and session scope. This means if you want to change the variable in current session, you need to do it with: ``SET wsrep_OSU_method`` (without the ``GLOBAL`` keyword). Setting the variable with ``SET GLOBAL wsrep_OSU_method`` will change the variable globally but it won't have effect on the current session.

.. variable:: wsrep_preordered

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

Use this variable to enable new, transparent handling of preordered replication events (like replication from traditional master). When this variable is enabled, such events will be applied locally first before being replicated to the other nodes of the cluster. This could increase the rate at which they can be processed, which would be otherwise limited by the latency between the nodes in the cluster.

Preordered events should not interfere with events that originate on the local node. Therefore, you should not run local update queries on a table that is also being updated through asynchronous replication.

.. variable:: wsrep_provider

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: None

This variable should contain the path to the Galera library (like :file:`/usr/lib64/libgalera_smm.so` on *CentOS*/*RHEL* and :file:`/usr/lib/libgalera_smm.so` on *Debian*/*Ubuntu*).

.. variable:: wsrep_provider_options

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No

This variable contains settings currently used by Galera library.

.. variable:: wsrep_recover

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: OFF
   :location: mysqld_safe

When server is started with this variable, it will parse Global Transaction ID (GTID) from log, and if the GTID is found, assign it as initial position for actual server start. This option is used to recover GTID.

.. variable:: wsrep_reject_queries
 
   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: NONE
 
This variable can be used to reject queries for the node. This can be useful during upgrades for keeping node up (with provider enabled) without accepting queries. Using read-only is recommended here unless you want to kill existing queries. This variable accepts the following values:
 
* ``NONE``: Nothing is rejected (default)
* ``ALL``: All queries are rejected with ``Error 1047: Unknown command``
* ``ALL_KILL``: All queries are rejected and existing client connections are also killed without waiting
 
.. note:: This variable doesn't affect Galera replication in any way, only the applications which connect to database are affected. If you want to desync a node, then use :variable:`wsrep_desync`.

.. variable:: wsrep_replicate_myisam

   :version 5.6.24-25.11: Variable available both in global and session scope
   :cli: Yes
   :conf: Yes
   :scope: Session, Global
   :dyn: No
   :default: OFF

This variable defines whether MyISAM should be replicated or not. It is disabled by default, because MyISAM replication is still experimental.

On the global level, :variable:`wsrep_replicate_myisam` can be set only before boot-up. On session level, you can change it during runtime as well.

For older nodes in the cluster, :variable:`wsrep_replicate_myisam` should work since the TOI decision (for MyISAM DDL) is done on origin node. Mixing of non-MyISAM and MyISAM tables in the same DDL statement is not recommended when :variable:`wsrep_replicate_myisam` is disabled, since if any table in the list is MyISAM, the whole DDL statement is not put under TOI.

.. note:: You should keep in mind the following when using MyISAM replication:

  * DDL (CREATE/DROP/TRUNCATE) statements on MyISAM will be replicated irrespective of :variable:`wsrep_replicate_miysam` value
  * DML (INSERT/UPDATE/DELETE) statements on MyISAM will be replicated only if :variable:`wsrep_replicate_myisam` is enabled
  * SST will get full transfer irrespective of :variable:`wsrep_replicate_myisam` value (it will get MyISAM tables from donor)
  * Difference in configuration of ``pxc-cluster`` node on `enforce_storage_engine <https://www.percona.com/doc/percona-server/5.6/management/enforce_engine.html>`_ front may result in picking up different engine for same table on different nodes
  * ``CREATE TABLE AS SELECT`` (CTAS) statements use non-TOI replication and are replicated only if there is involvement of InnoDB table that needs transactions (involvement of MyISAM table will cause CTAS statement to skip replication)


.. variable:: wsrep_restart_slave

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable controls if |MySQL| slave should be restarted automatically when node joins back to cluster, because asynchronous replication slave thread is stopped when the node tries to apply next replication event while the node is in non-primary state.

.. variable:: wsrep_retry_autocommit

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable sets the number of times autocommitted transactions will be tried in the cluster if it encounters certification errors. In case there is a conflict, it should be safe for the cluster node to simply retry the statement without the client's knowledge hoping that it will pass next time. This can be useful to help an application using autocommit to avoid deadlock errors that can be triggered by replication conflicts. If this variable is set to ``0`` transaction won't be retried and if it is set to ``1``, it will be retried once.

.. variable:: wsrep_slave_FK_checks

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: ON

This variable is used to control if Foreign Key checking is done for applier threads.

.. variable:: wsrep_slave_threads

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 1

This variable controls the number of threads that can apply replication transactions in parallel. Galera supports true parallel replication, replication that applies transactions in parallel only when it is safe to do so. The variable is dynamic, you can increase/decrease it at any time.

.. note:: When you decrease it, it won't kill the threads immediately but stop them after they are done applying current transaction (the effect with increase is immediate though). 

If any replication consistency problems are encountered, it's recommended to set this back to ``1`` to see if that resolves the issue. The default value can be increased for better throughput.

You may want to increase it as suggested `in Codership documentation <http://galeracluster.com/documentation-webpages/nodestates.html#flow-control>`_, in ``JOINED`` state for instance to speed up the catchup process to ``SYNCED``.

You can also estimate the optimal value for this from :variable:`wsrep_cert_deps_distance` as suggested `on this page <http://galeracluster.com/documentation-webpages/monitoringthecluster.html#checking-the-replication-health>`_.

You can also refer to `this <http://galeracluster.com/documentation-webpages/configurationtips.html#setting-parallel-slave-threads>`_ for more configuration tips.

.. variable:: wsrep_slave_UK_checks

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable is used to control if Unique Key checking is done for applier threads.

.. variable:: wsrep_sst_auth

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :format: <username>:<password>

This variable should contain the authentication information needed for State Snapshot Transfer (SST). Required information depends on the method selected in the :variable:`wsrep_sst_method`. More information about required authentication can be found in the :ref:`state_snapshot_transfer` documentation. This variable will appear masked in the logs and in the ``SHOW VARIABLES`` query.

.. variable:: wsrep_sst_donor

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable contains the name (:variable:`wsrep_node_name`) of the preferred donor for SST. If no node is selected as a preferred donor, it will be chosen from one of the available nodes automatically **if and only if** there is a terminating comma at the end (like 'node1,node2,'). Otherwise, if there is no terminating comma, the list of nodes in :variable:`wsrep_sst_donor` is considered absolute, and thus it won't fall back even if other nodes are available. Please check the note for :option:`sst-initial-timeout` if you are using it without terminating comma or want joiner to wait more than default 100 seconds.

.. variable:: wsrep_sst_donor_rejects_queries

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: OFF

This variable can be used to reject blocking client sessions on a node
serving as a donor during a blocking state transfer method.
Queries will return the ``UNKNOWN COMMAND`` error code.
This can be used to signal load-balancer that the node isn't available.

.. variable:: wsrep_sst_method

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: xtrabackup-v2
   :recommended: xtrabackup-v2

This variable sets up the method for taking the State Snapshot Transfer (SST). Available values are:

* ``xtrabackup``: Uses Percona XtraBackup to perform the SST, this method requires :variable:`wsrep_sst_auth` to be set up with <user>:<password> which |XtraBackup| will use on donor. Privileges and permissions needed for running |XtraBackup| can be found `here <http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_.

* ``xtrabackup-v2``: This is the same as ``xtrabackup``, except that it uses newer protocol, hence is not compatible. This is the **recommended** option for PXC 5.5.34 and above. For more details, please check :ref:`xtrabackup_sst` and :ref:`errata`. This is also the default SST method. For SST with older nodes (< 5.5.34), use ``xtrabackup`` as the SST method.

  .. note:: This method is currently recommended if you have ``innodb-log-group_home-dir/innodb-data-home-dir`` in your config. Refer to :option:`sst-special-dirs` for more information.

* ``rsync``: Uses ``rsync`` to perform the SST, this method doesn't use the :variable:`wsrep_sst_auth`

* ``mysqldump``: Uses ``mysqldump`` to perform the SST, this method requires :variable:`wsrep_sst_auth` to be set up with <user>:<password>, where user has root privileges on the server.

  .. note::  This mehotd is not recommended unless it is required for specific reasons. Also, it is not compatible with ``bind_address`` set to ``127.0.0.1`` or ``localhost``, and will cause startup to fail if set so.

* ``<custom_script_name>``: Galera supports `Scriptable State Snapshot Transfer <http://galeracluster.com/documentation-webpages/statetransfer.html#scriptable-state-snapshot-transfer>`_. This enables users to create their own custom script for performing an SST. For example, you can create a script :file:`/usr/bin/wsrep_MySST.sh` and specify ``MySST`` for this variable to run your custom SST script.

* ``skip``: Use this to skip SST, it can be used when initially starting the cluster and manually restoring the same data to all nodes. It shouldn't be used as permanent setting because it could lead to data inconsistency across the nodes.

.. note:: Only ``xtrabackup-v2`` and ``rsync`` provide ``gtid_mode async-slave`` support during SST.

.. variable:: wsrep_sst_receive_address

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: AUTO

This variable is used to configure address on which the node expects SST.

.. variable:: wsrep_start_position

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes

This variable contains the ``UUID:seqno`` value. By setting all the nodes to have the same value for this option, cluster can be set up without the state transfer.

.. variable:: wsrep_sync_wait

   :version 5.6.20-25.7: Variable introduced
   :cli: Yes
   :conf: Yes
   :scope: Session
   :dyn: Yes

This variable is used to control causality checks on certain statements.
Checks ensure that the statement is executed on a node
that is fully synced with the cluster.

The type of statements to undergo checks
is determined by bitmask:

* ``0``: Do not run causality checks for any statements.
  This is the default.

* ``1``: Perform checks for ``READ`` statements,
  including ``SELECT``, ``SHOW``, and ``BEGIN`` or ``START TRANSACTION``.

* ``2``: Perform checks for ``UPDATE`` and ``DELETE`` statements.

* ``3``: Perform checks for ``READ``, ``UPDATE``,
  and ``DELETE`` statements.

* ``4``: Perform checks for ``INSERT`` and ``REPLACE`` statements.

.. note:: Setting :variable:`wsrep_sync_wait` to ``1`` is the equivalent
   of setting :variable:`wsrep_causal_reads` to ``ON``.

