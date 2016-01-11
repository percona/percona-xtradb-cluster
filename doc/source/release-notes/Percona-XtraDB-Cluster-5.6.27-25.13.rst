.. rn:: 5.6.27-25.13

===================================
Percona XtraDB Cluster 5.6.27-25.13 
===================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6.27-25.13 on January 11, 2016. Binaries are available from the `downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.27-25.13/>`_ or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.27-25.13 is now the current release, based on the following:

* `Percona Server 5.6.27-75.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.27-75.0.html>`_

* `Percona Server 5.6.27-76.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.27-76.0.html>`_

* `Galera Replicator 3.13 <https://github.com/codership/galera/issues?q=milestone%3A25.3.13>`_

* `Codership wsrep API 25.12 <https://github.com/codership/mysql-wsrep/issues?q=milestone%3A5.6.27-25.12>`_

All Percona software is open-source and free. Details of this release can be found in the `5.6.27-25.13 milestone on Launchpad <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.27-25.13>`_.

For more information about relevant Codership releases, see `this announcement <http://galeracluster.com/2015/11/announcing-galera-cluster-5-5-42-and-5-6-25-with-galera-3-12-2/>`_.

New Features
============

* There is a new script for building Percona XtraDB Cluster from source. For more information, see :ref:`compile`.

* :variable:`wsrep_on` is now a session only variable. That means toggling it will not affect other clients connected to said node. Only the session/client modifying it will be affected. Trying to toggle :variable:`wsrep_on` in the middle of a transaction will now result in an error. Trx will capture the state of :variable:`wsrep_on` during start and will continue to use it. Start here means when the first logical changing statement is executed within transaction context.

Bugs Fixed
==========

* :bug:`1261688` and :bug:`1292842`: Fixed race condition when two skipped replication transactions were rolled back, which caused ``[ERROR] WSREP: FSM: no such a transition ROLLED_BACK -> ROLLED_BACK`` with ``LOAD DATA INFILE``

* :bug:`1362830`: Corrected ``xtrabackup-v2`` script to consider only the last specified ``log_bin`` directive in :file:`my.cnf`. Multiple ``log_bin`` directives caused SST to fail.

* :bug:`1370532`: Toggling :variable:`wsrep_desync` while node is paused is now blocked.

* :bug:`1404168`: Removed support for `innodb_fake_changes <https://www.percona.com/doc/percona-server/5.6/management/innodb_fake_changes.html>`_ variable.

* :bug:`1455098`: Fixed failure of LDI on partitioned table. This was caused by partitioned table handler disabling bin-logging and Native Handler (InnoDB) failing to generate needed bin-logs eventually causing skipping of statement replication.

* :bug:`1503349`: ``garbd`` now uses default port number if it is not specified in ``sysconfig``.

* :bug:`1505184`: Corrected ``wsrep_sst_auth`` script to ensure that user name and password for SST is passed to XtraBackup through internal command-line invocation. ``ps -ef`` doesn't list these credentials so passing it internally is fine, too.

* :bug:`1520491`: ``FLUSH TABLE`` statements are not replicated any more, because it lead to an existing upstream fix pending deadlock error. This fix also takes care of original fix to avoid increment of local GTID.

* :bug:`1528020`: Fixed async slave thread failure caused by redundant updates of ``mysql.event`` table with the same value. Redundant updates are now avoided and will not be bin-logged.

* Fixed **garb** init script causing new UUIDs to be generated every time it runs. This error was due to missing ``base_dir`` configuration when ``gardb`` didn't have write-access to current working directory. ``garbd`` will now try to use ``cwd``. Then it will try to use ``/var/lib/galera`` (like most Linux daemons). If it fails to use or create ``/var/lib/galera``, it will throw a fatal error.

* Fixed replication of ``DROP TABLE`` statement with a mix of temporary and non-temporary tables (for example, ``DROP TABLE temp_t1, non_temp_t2``), which caused errorneous ``DROP TEMPORARY TABLE stmt`` on replicated node. Corrected it by detecting such scenarios and creating temporary table on the replicated node, which is then dropped by follow-up ``DROP`` statement. All this workload should be part of same unit as temporary tables are session-specific.

* Fixed error when :variable:`wsrep_cluster_name` value over 32 characters long caused gmcast message to exceed maximum length. Imposed a limit of 32 character on :variable:`wsrep_cluster_name`.

* Added code to properly handle default values for ``wsrep_*`` variables, which caused an error/crash.

* Fixed error when a ``CREATE TABLE AS SELECT`` (CTAS) statement still tried to certify a transaction on a table without primary key even if certification of tables without primary key was disabled. This error was caused by CTAS setting ``trx_id`` (``fake_trx_id``) to execute ``SELECT`` and failing to reset it back to ``-1`` during ``INSERT`` as certification is disabled.

* Fixed crashing of ``INSERT .... SELECT`` for MyISAM table with :variable:`wsrep_replicate_myisam` set to ``ON``. This was caused by TOI being invoked twice when source and destination tables were MyISAM.

* Fixed crash when caching write-set data beyond configured limit. This was caused by TOI flow failing to consider/check error resulting from limit enforcement.

* Fixed error when loading MyISAM table from schema temporary table (with :variable:`wsrep_replicate_myisam` set to ``ON``). This was caused by temporary table lookup being done using ``get_table_name()``, which could be misleading as ``table_name`` for temporary tables is set to temporary generated name. Original name of the table is part of ``table_alias``. The fix corrected condition to consider both ``table_name`` and ``alias_name``.

* Fixed error when changing :variable:`wsrep_provider` in the middle of a transaction or as part of a procedure/trigger. This is now blocked to avoid inconsistency.

* Fixed TOI state inconsistency caused by ``DELAYED_INSERT`` on MyISAM table (``TOI_END`` was not called). Now the ``DELAYED_`` qualifier will be ignored and statement will be interpreted as normal ``INSERT``.

* Corrected locking semantics for ``FLUSH TABLES WITH READ LOCK`` (FTWRL). It now avoids freeing inheritted lock if follow-up ``FLUSH TABLE`` statement fails. Only frees self-acquired lock.

* Fixed crash caused by ``GET_LOCK`` + :variable:`wsrep_drupal_282555_workaround`. ``GET_LOCK`` path failed to free all instances of user-level locks after it inherited ``multiple-user-locks`` from Percona Server. The cleanup code now removes all possible references of locks.

* Fixed cluster node getting stuck in ``Donor/Desync`` state after a hard recovery, because of an erroneous type cast in source code.

* Corrected DDL and DML semantics for MyISAM:

  * DDL (CREATE/DROP/TRUNCATE) on MyISAM will be replicated irrespective of :variable:`wsrep_replicate_miysam` value
  * DML (INSERT/UPDATE/DELETE) on MyISAM will be replicated only if :variable:`wsrep_replicate_myisam` is enabled
  * SST will get full transfer irrespective of :variable:`wsrep_replicate_myisam` value (it will get MyISAM tables from donor if any)
  * Difference in configuration of ``pxc-cluster`` node on `enforce_storage_engine <https://www.percona.com/doc/percona-server/5.6/management/enforce_engine.html>`_ front may result in picking up different engine for same table on different nodes
  * ``CREATE TABLE AS SELECT`` (CTAS) statements use non-TOI replication and are replicated only if there is involvement of InnoDB table that needs trx (involvement of MyISAM table will cause CTAS statement to skip replication)

* SST fails with ``innodb_data_home_dir``/``innodb_log_home_dir``. This was a bug in Percona XtraBackup, which is fixed in 2.3.3 and 2.2.12.

Known Issues
============

* `1330941 <https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1330941>`_: Conflict between :variable:`wsrep_OSU_method` set to ``RSU`` and :variable:`wsrep_desync` set to ``ON`` was not considered a bug.

* `1443755 <https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1443755>`_: Causal reads introduces surprising latency in single node clusters.

* `1522385 <https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1522385>`_: Holes are introduced in Master-Slave GTID eco-system on replicated nodes if any of the cluster nodes are acting as asynchronous slaves to an independent master.

* Enabling :variable:`wsrep_desync` (from previous ``OFF`` state) will wait until previous ``wsrep_desync=OFF`` operation is completed.

