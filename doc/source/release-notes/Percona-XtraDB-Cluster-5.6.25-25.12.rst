.. rn:: 5.6.25-25.12

=======================================
 |Percona XtraDB Cluster| 5.6.25-25.12 
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6.25-25.12 on September 20th 2015. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.25-25.12/>`_ or from our :ref:`software repositories <installation>`.

Based on `Percona Server 5.6.25-73.1 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.25-73.1.html>`_ including all the bug fixes in it, *Galera Replicator* `3.12 <https://github.com/codership/galera/issues?q=milestone%3A25.3.12>`_ and on *Codership wsrep API* `25.11 <https://github.com/codership/mysql-wsrep/issues?q=milestone%3A5.6.26-25.11>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.24-25.12 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.24-25.12>`_ at Launchpad.

New Features
============

 |Percona XtraDB Cluster| has implemented `Support for PROXY protocol <https://www.percona.com/doc/percona-server/5.6/flexibility/proxy_protocol_support.html#proxy-protocol-support>`_. The implementation is based on a patch developed by Thierry Fournier.

 MTR coverage has been added to all tests in galera suite. Many bugs associated with mtr tests have been fixed.

 A new variable :variable:`gcache.keep_pages_count`, analogous to :variable:`gcache.keep_pages_size`, has been added. The variable limits the number of overflow pages rather than the total memory occupied by all overflow pages. Whenever either of the variables are updated at runtime to a non-zero value, cleanup is called on excess overflow pages to delete them. This feature also fixes the bugs with integer overflow in the ``gcache`` module.

 Updates have been made to wsrep code to ensure greater concordance with binary log and GTID so that failover of async slaves, among nodes of the cluster is seamless and consistent. To ensure this in :bug:`1421360`, all ``FLUSH`` commands (except ``FLUSH BINARY LOG`` and ``FLUSH LOGS``, and read lock-based flush such as ``FLUSH TABLES WITH READ LOCK`` and ``FLUSH TABLES FOR EXPORT``), ``ANALYZE TABLE``, Percona Server-specific flush statements for `user statistics <https://www.percona.com/doc/percona-server/5.6/diagnostics/user_stats.html>`_ and `page tracking bitmaps <https://www.percona.com/doc/percona-server/5.6/management/changed_page_tracking.html>`_ are executed under Total Order Isolation (TOI) so that they are replicated to other nodes in the cluster when they are written to binary log. 

 |Percona XtraDB Cluster| has temporarily disabled savepoints in triggers and stored functions. The reason is that even having fixed bug :bug:`1438990` and bug :bug:`1464468` we have found more cases where savepoints in triggers break binary logging and replication, resulting in server crashes and broken slaves. This feature will be disabled until the above issues are properly resolved.

Bugs fixed 
==========

 ``SHOW STATUS LIKE ...`` and ``SHOW STATUS`` were taking time proportional to size of :variable:`gcache.size`. Bug fixed :bug:`1462674`.

 When disk space would get filled with :variable:`gcache.page` files, Galera would crash when the next page file was created. Bug fixed :bug:`1488535`.

 XtraBackup SST didn't clobber :file:`backup-my.cnf` which caused SST to fail. Bug fixed :bug:`1431101`.

 Error from ``process::wait`` was not checked in joiner thread leading to joiner starting erroneously even when SST had failed. Bug fixed :bug:`1402166`.

 Due to an regression introduced in |Percona XtraDB Cluster| :rn:`5.6.24-25.11`, update of the :variable:`wsrep_cluster_address` variable, following the update of :variable:`wsrep_provider_options` variable would cause the server to deadlock. Bug fixed PXC-421.

 ``mysqldump`` SST could stall due to a regression in desync mutex introduced in |Percona XtraDB Cluster| :rn:`5.6.24-25.11` by fixing the bug :bug:`1288528`. Bug fixed PXC-423.

 ``mysql_tzinfo_to_sql`` sets :variable:`wsrep_replicate_myisam` variable at session scope so that statements are replicated correctly. Bug fixed PXC-332.

 ``Percona-XtraDB-Cluster-devel-56`` package was not included in the ``Percona-XtraDB-Cluster-full-56`` metapackage on *CentOS* due to a conflict with upstream ``mysql`` package. Bug fixed PXC-381.

 Running ``service mysql start`` and then ``service mysql@boostrap start`` afterwards would cause server shutdown. Bug fixed PXC-385.

 ``NO_WRITE_TO_BINLOG`` / ``LOCAL`` for ``ANALYZE TABLE``, ``OPTIMIZE TABLE``, ``REPAIR TABLE``, ``FLUSH`` commands will ensure it is not written to binary log (as in mysql async replication) and not replicated in wsrep. Bug fixed PXC-391. 

 ``FLUSH TABLES WITH READ LOCK`` failure (with non-existent tables) didn't resume the galera provider, causing deadlock. Bug fixed PXC-399.

 |Percona XtraDB Cluster| will not blocking DDL statements on tables which are used with ``... FOR EXPORT`` or ``... WITH READ LOCK``, it will give return an error message about read lock. Bug fixed PXC-403.

 Fixed the update of :variable:`variable wsrep_slave_threads` variable regarding the default value assignment, invalid value truncation, and error issued while threads are still being closed. Bug fixed PXC-420.

 Server would crash during startup if :variable:`gcs.fc_limit` variable was specified twice in :variable:`wsrep_provider_options`. Bug fixed PXC-428.

 The mysql client in |Percona XtraDB Cluster| has been built with system ``readline`` instead of ``editline``. Bug fixed PXC-430.

 Bugs in 32-bit galera associated with ``statvfs`` in ``available_storage`` and integer overflow after multiplication in offset calculation have been fixed. Bug fixed PXC-433.

 Galera 3 was failing to build on all non-intel platforms. Architecture specific ``CCFLAGS`` have been removed and provision for inheriting ``CCFLAGS``, ``CFLAGS``, ``CXXFLAGS`` and ``LDFLAGS`` have been added to ``SConstruct``. Bug fixed PXC-326.

 Non-global read locks such as ``FLUSH TABLES WITH READ LOCK`` and ``FLUSH TABLES FOR EXPORT`` paused galera provider but didn't block commit globally which :variable:`wsrep_to_isolation_begin` (for DDL) was made aware of. Bug fixed PXC-403.

Following bug fixes have been ported from |Percona Server| `5.6.26-74.0 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.26-74.0.html)>`_: bug :bug:`1454441`, bug :bug:`1470677`, bug :bug:`1472256`, and bug :bug:`1472251`.

Other bugs fixed: PXC-370, PXC-429, PXC-415, PXC-400 and PXC-416.
