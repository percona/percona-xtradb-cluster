.. rn:: 5.6.15-25.2

======================================
 |Percona XtraDB Cluster| 5.6.15-25.2
======================================

Percona is glad to announce the first Release Candidate release of |Percona XtraDB Cluster| 5.6 on December 18th 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.15-25.2/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.15-63.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.15-63.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.2 <https://launchpad.net/galera/3.x/25.3.2>`_ and on `Codership wsrep API 5.6.15-25.2 <https://launchpad.net/codership-mysql/+milestone/5.6.15-25.2>`_ is now the first **RELEASE CANDIDATE** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.15-25.2 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.15-25.2>`_ at Launchpad.

This release contains all of the features and bug fixes in `Percona XtraDB Cluster 5.5.34-25.9 <http://www.percona.com/doc/percona-xtradb-cluster/release-notes/Percona-XtraDB-Cluster-5.5.34-25.9.html>`_, plus the following:

New Features
============

 |Percona XtraDB Cluster| now supports stream compression/decompression with new xtrabackup-sst :option:`compressor/decompressor` options.

 New :variable:`wsrep_reject_queries` has been implemented that can be used to reject queries for that node. This can be useful if someone wants to manually run maintenance on the node like ``mysqldump`` without need to change the settings on the load balancer. 

 XtraBackup SST now supports :variable:`innodb_data_home_dir` and :variable:`innodb_log_group_home_dir` in the configuration file with :option:`sst_special_dirs` option.

 New :variable:`wsrep_local_cached_downto` status variable has been introduced. This variable shows the lowest sequence number in ``gcache``. This information can be helpful with determining IST and/or SST.

 ``Garbd`` init script and configuration files have been packaged for *CentOS* and *Debian*, in addition, in *Debian* garbd is packaged separately in ``percona-xtradb-cluster-garbd-3.x`` package.

Bugs fixed 
==========

 When :file:`grastate.dat` file was not getting zeroed appropriately it would lead to RBR error during the IST. Bug fixed :bug:`1180791`.

 ``init stop`` script on *CentOS* didn't wait for the server to be fully stopped. This would cause unsuccessful server restart because the ``start`` action would fail because the daemon would still be running. Bug fixed :bug:`1254153`. 

 ``DELETE FROM`` statement (without ``WHERE`` clause) would crash slaves if master did not have binlogging enabled. Bug fixed :bug:`1254179`.
 
 Missing protection for brute force threads against ``innodb lock wait time out`` would cause  applier to fail with lock wait timeout exceeded on ``rsync`` SST donor. Bug fixed :bug:`1255501`.

 Recent optimizations in 3.x branch introduced a regression in base filename construction which could lead big transactions fail with: ``WSREP: Failed to open file '...'``. Bug fixed :bug:`1255964`. 
 
 Joiner node would not initialize storage engines if ``rsync`` was used for SST and the first view was non-primary. Bug fixed :bug:`1257341`.

 Table level lock conflict resolving was releasing the wrong lock. Bug fixed :bug:`1257678`.

 Resolved the ``perl`` dependencies needed for |Percona XtraDB Cluster| 5.6. Bug fixed :bug:`1258563`.

 Obsolete dependencies have been removed from |Percona XtraDB Cluster|. Bug fixed :bug:`1259256`.

 ``CREATE TABLE AS SELECT`` process would remain hanging in case it was run in parallel with the DDL statement on the selected table. Bug fixed :bug:`1164893`.

 Naming of the *Galera* packages have been fixed to avoid the confusion, ie. ``Percona-XtraDB-Cluster-galera-56`` is ``Percona-XtraDB-Cluster-galera-3`` now. Bug fixed :bug:`1253923`.

 Fixed rsync SST for compatibility with ``rsync`` version 3.1.0. Bug fixed :bug:`1261673`.

Other bugs fixed: :bug:`1261138`, :bug:`1254633`.

.. note::

   On *CentOS* 5/6, those who may have installed ``percona-xtrabackup-20`` with |Percona XtraDB Cluster| 5.6 due to bug :bug:`1226185`, the upgrade may fail, the dependency issue has been fixed since then, hence replace ``percona-xtrabackup-20`` with ``percona-xtrabackup`` before upgrade.

We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

