.. rn:: 5.6.14-25.1

======================================
 |Percona XtraDB Cluster| 5.6.14-25.1
======================================

Percona is glad to announce the first Beta release of |Percona XtraDB Cluster| 5.6 on November 21st, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.6.14-25.1/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.14-62.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.14-62.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.1 <https://launchpad.net/galera/3.x/25.3.1>`_ and on `Codership wsrep API 5.6.14-25.1 <https://launchpad.net/codership-mysql/5.6/5.6.14-25.1>`_ is now the first **BETA** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.14-25.1 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.14-25.1>`_ at Launchpad.

This release contains all of the features and bug fixes in `Percona XtraDB Cluster 5.5.34-23.7.6 <http://www.percona.com/doc/percona-xtradb-cluster/release-notes/Percona-XtraDB-Cluster-5.5.34-23.7.6.html>`_, plus the following:

New Features
============

 |Percona XtraDB Cluster| is now using `Galera Replicator 3.1 <https://launchpad.net/galera/3.x/25.3.1>`_ and `Codership wsrep API 5.6.14-25.1 <https://launchpad.net/codership-mysql/5.6/5.6.14-25.1>`_.

 |Percona XtraDB Cluster| has implemented a number of `XtraDB performance improvements <http://www.percona.com/doc/percona-server/5.6/performance/xtradb_performance_improvements_for_io-bound_highly-concurrent_workloads.html>`_ for I/O-bound high-concurrency workloads. 

 |Percona XtraDB Cluster| has implemented a number of performance improvements for `Page cleaner thread tuning <http://www.percona.com/doc/percona-server/5.6/performance/page_cleaner_tuning.html#page-cleaner-tuning>`_. 

 ``ALL_O_DIRECT`` method for `innodb_flush_method <http://www.percona.com/doc/percona-server/5.6/scalability/innodb_io.html#innodb_flush_method>`_ has been ported from 5.5 version.

 `Statement Timeout <http://www.percona.com/doc/percona-server/5.6/management/statement_timeout.html#statement-timeout>`_ feature has been ported from the Twitter branch.

 |Percona XtraDB Cluster| has `extended <http://www.percona.com/doc/percona-server/5.6/flexibility/extended_select_into_outfile.html#extended-select-into-outfile>`_ the ``SELECT INTO ... OUTFILE`` and ``SELECT INTO DUMPFILE`` to add the support for ``UNIX`` sockets and named pipes.

 |Percona XtraDB Cluster| has implemented more efficient log block checksums with new `innodb_log_checksum_algorithm <http://www.percona.com/doc/percona-server/5.6/scalability/innodb_io.html#innodb_log_checksum_algorithm>`_ variable.

 |Percona XtraDB Cluster| now supports `Per-query variable statements <http://www.percona.com/doc/percona-server/5.6/flexibility/per_query_variable_statement.html#per-query-variable-statement>`_.

 Limited support for Query Cache has been implemented. Query cache cannot still be fully enabled during the startup. To enable query cache, ``mysqld`` should be started with ``query_cache_type=1`` and ``query_cache_size=0`` and then query_cache_size should be changed to desired value during runtime.

 ``RPM`` packages are now made `relocatable <http://rpm5.org/docs/api/relocatable.html>`_ which means they now support installation to custom prefixes.

Bugs fixed 
==========

 Some wsrep variables (:variable:`wsrep_provider`, :variable:`wsrep_provider_options`, :variable:`wsrep_cluster_address`...) could be "doubly" allocated which caused memory leaks. Bug fixed :bug:`1072839`.
 
 If ``SELECT FOR UPDATE...`` query was aborted due to multi-master conflict, the client wouldn't get back the deadlock error. From client perspective the transaction would be successful. Bug fixed :bug:`1187739`.

 Temporary tables are not replicated, but any DDL on those tables were, which would generates error messages on other nodes. Bugs fixed :bug:`1194156`, :bug:`1084702`, :bug:`1212247`.

 When setting the :variable:`gcache.size` to a larger value than the default 128M, the mysql service command did not allow enough time for the file to be preallocated. Bug fixed :bug:`1207500`.
 
 When installing first ``Percona-XtraDB-Cluster-client`` and then ``Percona-XtraDB-Cluster-server`` on two single statements or a single statement with both packages , yum would install ``percona-xtrabackup-20`` instead ``percona-xtrabackup`` package as dependency of ``Percona-XtraDB-Cluster-server``. Bug fixed :bug:`1226185`.

 Different mutex implementation in the 5.6 could lead to server assertion error. Bug fixed :bug:`1233383`.

 Enabling :variable:`wsrep_log_conflicts` variable could cause issues with ``lock_mutex``. Bug fixed :bug:`1234382`.

 Server could freeze with mixed DML/DDL load because TOI brute force aborts were not properly propagated. Bug fixed :bug:`1239571`.

 ``CREATE TABLE AS SELECT`` would fail with explicit temporary tables, when was binlogging enabled and :variable:`autocommit` was set to ``0``. Bug fixed :bug:`1240098`.

 Transaction cleanup function did not get called for autocommit statements after rollback, it would stay in ``LOCAL_COMMIT`` even after rollback finished which caused problems when the next transaction started. Bug fixed :bug:`1240567`.

 ``DDL`` statements like ``CREATE TEMPORARY TABLE LIKE`` would be replicated and applied in all cluster nodes. This caused temporary table definitions to pile up in slave threads. Bug fixed :bug:`1246257`.
 
 ``CREATE TABLE AS SELECT`` was not replicated, if the select result set was empty. Bug fixed :bug:`1246921`.

 Write set flags defined in wsrep API are now exposed to application side appliers too. Bug fixed :bug:`1247402`.

 Local brute force aborts are counted accurately. Bug fixed :bug:`1247971`.

 Certain combinations of transaction rollbacks could leave stale transactional ``MDL`` locks. Bug fixed :bug:`1247978`. 

 After turning ``UNIV_SYNC_DEBUG`` on, node that was started from clean state would crash immediately at startup. Bug fixed :bug:`1248908`.
 
 Server built with ``UNIV_SYNC_DEBUG`` would assert if SQL load has ``DELETE`` statements on tables with foreign key constraints with ``ON DELETE CASCADE`` option. Bug fixed :bug:`1248921`.

 Xtrabackup SST dependencies have been added as Optional/Recommended/Suggested dependencies. Bug fixed :bug:`1250326`.

Other bugs fixed: bug fixed :bug:`1020457`, bug fixed :bug:`1250865`, bug fixed :bug:`1249753`, bug fixed :bug:`1248396`, bug fixed :bug:`1247980`, bug fixed :bug:`1238278`, bug fixed :bug:`1234421`, bug fixed :bug:`1228341`, bug fixed :bug:`1228333`, bug fixed :bug:`1225236`, bug fixed :bug:`1221354`, bug fixed :bug:`1217138`, bug fixed :bug:`1206648`, bug fixed :bug:`1200647`, bug fixed :bug:`1180792`, bug fixed :bug:`1163283`, bug fixed :bug:`1234229`, bugs fixed :bug:`1250805`, bug fixed :bug:`1233301`, and bug fixed :bug:`1210509`.

We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

