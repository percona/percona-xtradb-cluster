.. rn:: 5.6.24-25.11

=======================================
 |Percona XtraDB Cluster| 5.6.24-25.11 
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on June 3rd 2015. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.24-25.11/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.24-72.2 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.24-72.2.html>`_ including all the bug fixes in it, *Galera Replicator* `3.11 <https://github.com/codership/galera/milestones/25.3.11>`_ and on *Codership wsrep API* `25.11 <https://github.com/codership/mysql-wsrep/milestones/5.6.x-25.11>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.24-25.11 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.24-25.11>`_ at Launchpad.

New Features
============

 |Percona XtraDB Cluster| now allows reads in non-primary state by introducing a new session variable, :variable:`wsrep_dirty_reads`. This variable is boolean and is ``OFF`` by default. When set to ``ON``, a |Percona XtraDB Cluster| node accepts queries that only read, but not modify data even if the node is in the non-PRIM state (:bug:`1407770`).

 |Percona XtraDB Cluster| now allows queries against ``INFORMATION_SCHEMA`` and ``PERFORMANCE_SCHEMA`` even with variables :variable:`wsrep_ready` and :variable:`wsrep_dirty_reads` set to ``OFF``. This allows monitoring applications to monitor the node when it is even in non-PRIM state (:bug:`1409618`).
 
 :variable:`wsrep_replicate_myisam` variable is now both session and global variable (:bug:`1280280`).

 |Percona XtraDB Cluster| now uses ``getifaddrs`` for node address detection. It detects the address from the interface to which ``mysqld`` is bound if the address is not obtained from :variable:`bind_address` or :variable:`wsrep_node_address`, when both are unset (:bug:`1252700`). 

 |Percona XtraDB Cluster| has implemented two new status variables: :variable:`wsrep_cert_bucket_count` and :variable:`wsrep_gcache_pool_size` for better instrumentation of galera memory usage. :variable:`wsrep_cert_bucket_count` shows the number of cells in the certification index hash-table and :variable:`wsrep_gcache_pool_size` shows the size of the page pool and/or dynamic memory allocated for gcache (in bytes).

Bugs fixed 
==========

 Using concurrent ``REPLACE``, ``LOAD DATA REPLACE`` or ``INSERT ON DUPLICATE KEY UPDATE`` statements in the ``READ COMMITTED`` isolation level or with the :variable:`innodb_locks_unsafe_for_binlog` option enabled could lead to a unique-key constraint violation. Bug fixed :bug:`1308016`.

 Using the Rolling Schema Upgrade as a schema upgrade method due to conflict with :variable:`wsrep_desync` would allows only one ``ALTER TABLE`` to run concurrently. Bugs fixed :bug:`1330944` and :bug:`1330941`.

 SST would resume even when the donor was already detected as being in ``SYNCED`` state. This was caused when :variable:`wsrep_desync` was manually set to ``OFF`` which caused the conflict and resumed the donor sooner. Bug fixed :bug:`1288528`.

 DDL would fail on a node when running a TOI DDL, if one of the nodes has the table locked. Bug fixed :bug:`1376747`.

 ``xinet.d`` mysqlchk file was missing ``type = UNLISTED`` to work out of the box. Bug fixed :bug:`1418614`.

 Conflict between `enforce_storage_engine <http://www.percona.com/doc/percona-server/5.6/management/enforce_engine.html#enforce_storage_engine>`_ and :variable:`wsrep_replicate_myisam` for ``CREATE TABLE`` has been fixed. Bug fixed :bug:`1435482`.

 A specific trigger execution on the master server could cause a slave assertion error under row-based replication. The trigger would satisfy the following conditions: 1) it sets a savepoint; 2) it declares a condition handler which releases this savepoint; 3) the trigger execution passes through the condition handler. Bug fixed :bug:`1438990`.

 |Percona XtraDB Cluster| *Debian* init script was testing connection with wrong credentials. Bug fixed :bug:`1439673`.

 Race condition between IST and SST fixed in xtrabackup-v2 SST. Bugs fixed :bug:`1441762`, :bug:`1443881`, and :bug:`1451524`.

 SST will now fail when move-back fails instead of continuing and failing at the next step. Bug fixed :bug:`1451670`.

 |Percona XtraDB Cluster| ``.deb`` binaries were built without fast mutexes. Bug fixed :bug:`1457118`.

 The error message text returned to the client in the non-primary mode is now more descriptive (``"WSREP has not yet prepared node for application use"``), instead of ``"Unknown command"`` returned previously. Bug fixed :bug:`1426378`.

 Out-of-bount memory access issue in ``seqno_reset()`` function has been fixed.

 :variable:`wsrep_local_cached_downto` would underflow when the node on which it is queried has no writesets in gcache.

 :variable:`wsrep_OSU_method` is now both Global and Session variable. 
 
Other bugs fixed: :bug:`1290526`.
