.. rn:: 5.5.34-23.7.6

========================================
 |Percona XtraDB Cluster| 5.5.34-23.7.6
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on November 4th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.34-23.7.6/>`_ or from our :doc:`software repositories </installation>`.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

Bugs fixed 
==========

Fixed the issues with upgrade on *Debian* caused by ``debian-start`` script. Bug fixed :bug:`1112363`.

Adding the ``AUTO_INCREMENT`` column to a table would result in data inconsistency because different nodes use different :variable:`auto_increment_offset`. Bug fixed :bug:`587170`.

``LOAD DATA INFILE`` was not replicating properly in cluster. Bug fixed :bug:`1206129`.

After a ``COMMIT`` processing was interrupted (e.g. certification failure) it didn't always release transactional ``MDL`` locks, which could lead to server deadlock while performing DDL. Bug fixed :bug:`1212955`.

:variable:`pc.checksum` option was enabled by default, although its purpose was just to help with debugging. Bug fixed :bug:`1229107`.

Server startup isn't interrupted if the :file:`/var/lock/subsys/mysql` file was not cleaned up by the init script. Bug fixed :bug:`1231067`.

State Snapshot Transfer cleanup of existing data directory is now configurable with a ``cpat`` parameter. Bug fixed :bug:`1231121`.

Handler command and concurrent ``DDL`` could cause node hanging. The problem would happen when a handler session was aborted due to replication conflict. Bug fixed :bug:`1233353`.

Command ``service mysql status`` was not producing valid exit codes, which could break the configuration management systems. Bug fixed :bug:`1237021`.

Option :variable:`wsrep-new-cluster` could lead to server crash. Bug fixed :bug:`1238331`.

No key information was provided for ``DROP VIEW TOI`` statement. Bug fixed :bug:`1240040`.

:variable:`innodb_locks_unsafe_for_binlog` option  is no longer checked because slave threads are ``READ-COMMITTED`` by default. Bug fixed :bug:`1240112`.

If IO cache contained the ``Query_log_event``, database pointer would be reset during transaction replay. This could lead to "1046 No database selected." error when next statement is issued. Bug fixed :bug:`1241760`.

After test shutting down all the nodes indicate that all write set handlers were not released. Bug fixed :bug:`1244661`.

``MDL`` locks were not released properly on slaves at commit time. Bug fixed :bug:`1245333`.

Implemented number of thread pool scheduler fixes. Bug fixed :bug:`1240500`.

Running ``FLUSH STATUS`` would zero-up :variable:`wsrep_cluster_size` and :variable:`wsrep_local_index` variables. Bug fixed :bug:`1232789`.

Fixed the dependency conflict between the ``Percona-XtraDB-Cluster-shared`` and ``Percona-Server-shared-compat`` packages. Bug fixed :bug:`1084309`.

Fixed the memory leak in the ``wsrep_write_cache()`` function. Bug fixed :bug:`1230160`.

:ref:`xtrabackup_sst` implementation added in |Percona XtraDB Cluster| :rn:`5.5.33-23.7.6` has been renamed xtrabackup-v2, so :variable:`wsrep_sst_method` =xtrabackup will use xtrabackup implementation before :rn:`5.5.33-23.7.6` and will be compatible with older |Percona XtraDB Cluster| versions. Bug fixed :bug:`1228618`.

Support for SSL encryption for just the key and crt files as implemented in `Galera <http://www.codership.com/wiki/doku.php?id=ssl_support>`_ can be enabled with ``encrypt=3`` option. This has been implemented in :rn:`5.5.34-23.7.6` for compatibility with Galera. Bug fixed :bug:`1235244`.

Other bugs fixed: bug fixed :bug:`1244741`, bug fixed :bug:`1244667`, bug fixed :bug:`1243150`, bug fixed :bug:`1232890`, bug fixed :bug:`999492`, bug fixed :bug:`1245769`, bug fixed :bug:`1244100`.
 
Based on `Percona Server 5.5.34-32.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.34-32.0.html>`_ including all the bug fixes in it, `Galera Replicator <https://launchpad.net/galera/+milestone/23.2.7>`_ and on `Codership wsrep API 5.5.34-25.9 <https://launchpad.net/codership-mysql/+milestone/5.5.34-25.9>`_, |Percona XtraDB Cluster| `5.5.34-23.7.6 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.34-23.7.6>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

|Percona XtraDB Cluster| `Errata <http://www.percona.com/doc/percona-xtradb-cluster/errata.html>`_ can be found in our documentation.
