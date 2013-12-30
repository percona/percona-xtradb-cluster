.. rn:: 5.5.34-25.9

======================================
 |Percona XtraDB Cluster| 5.5.34-25.9
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on December 3rd, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.34-25.9/>`_ or from our :doc:`software repositories </installation>`.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.


New Features
============

 |Percona XtraDB Cluster| is now based on wsrep API 25 and Galera 25.2.x.

 ``RPM`` packages are now made `relocatable <http://rpm5.org/docs/api/relocatable.html>`_ which means they now support installation to custom prefixes.

 XtraBackup SST now supports :variable:`innodb_data_home_dir` and :variable:`innodb_log_home_dir` in the configuration file.

 The binaries are now statically linked with regard to ``Galera`` library which depended on ``OpenSSL`` library. 

Bugs fixed 
==========
 
 Product suffix has been added to the |Percona XtraDB Cluster| ``rpm`` packages, which means that packages have been renamed from ``Percona-XtraDB-Cluster-server`` to ``Percona-XtraDB-Cluster-server-55``. Bug fixed :bug:`1255616`.

 Fixed the dependency issue which caused Percona XtraDB Cluster 5.5 installation to fail on *Ubuntu* 12.04. Bug fixed :bug:`1247861`.
 
 When installing first ``Percona-XtraDB-Cluster-client`` and then ``Percona-XtraDB-Cluster-server`` on two single statements or a single statement with both packages , yum would install ``percona-xtrabackup-20`` instead ``percona-xtrabackup`` package as dependency of ``Percona-XtraDB-Cluster-server``. Bug fixed :bug:`1226185`.

 If ``SELECT FOR UPDATE...`` query was aborted due to multi-master conflict, the client wouldn't get back the deadlock error. From client perspective the transaction would be successful. Bug fixed :bug:`1187739`.

 Temporary tables are not replicated, but any DDL on those tables were (in this case it was ``TRUNCATE``), which would generates error messages on other nodes. Bug fixed :bug:`1194156`.

 When setting the :variable:`gcache.size` to a larger value than the default 128M, the mysql service command did not allow enough time for the file to be preallocated. Bug fixed :bug:`1207500`.

 ``CREATE TABLE AS SELECT`` would fail with explicit temporary tables, when binlogging was enabled and :variable:`autocommit` was set to ``0``. Bug fixed :bug:`1240098`. 

 Write set flags defined in wsrep API are now exposed to application side appliers too. Bug fixed :bug:`1247402`.

 Local brute force aborts are counted accurately. Bug fixed :bug:`1247971`.
 
 Certain combinations of transaction rollbacks could leave stale transactional ``MDL`` locks and cause deadlocks. Bug fixed :bug:`1247978`.

 After turning ``UNIV_SYNC_DEBUG`` on, node that was started from clean state would crash immediately at startup. Bug fixed :bug:`1248908`.

 Server built with ``UNIV_SYNC_DEBUG`` would assert if SQL load has ``DELETE`` statements on tables with foreign key constraints with ``ON DELETE CASCADE`` option. Bug fixed :bug:`1248921`.

 Xtrabackup SST dependencies have been added as ``Suggested`` dependencies for *DEB* packages. Bug fixed :bug:`1250326`.

 ``init stop`` script on *CentOS* didn't wait for the server to be fully stopped. This would cause unsuccessful server restart because the ``start`` action would fail because the daemon would still be running. Bug fixed :bug:`1254153`.

 Memory leak in ``mem_root`` has been fixed. Bug fixed :bug:`1249753`.

 Galera is now packaged with garbd init script. Bug fixed :bug:`1256769`.

Other bugs fixed: bug fixed :bug:`1247980`, bug fixed :bug:`891476`, bugs fixed :bug:`1250805`, bug fixed :bug:`1253923`.
 
.. note:: 

 Because some package names have been changed with the product suffix and and additional dependencies being added please check the :ref:`manual <installation>` before starting new installations. *Debian* users are requested to use ``apt-get dist-upgrade`` or ``apt-get install`` for upgrade, for more refer to installation :ref:`guide <apt-repo>`. 

Based on `Percona Server 5.5.34-32.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.34-32.0.html>`_ including all the bug fixes in it, `Galera Replicator <https://launchpad.net/galera/+milestone/25.2.8>`_ and on `Codership wsrep API 5.5.34-25.9 <https://launchpad.net/codership-mysql/+milestone/5.5.34-25.9>`_, |Percona XtraDB Cluster| `5.5.34-25.9 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.34-25.9>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

|Percona XtraDB Cluster| `Errata <http://www.percona.com/doc/percona-xtradb-cluster/errata.html>`_ can be found in our documentation.
