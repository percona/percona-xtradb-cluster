.. rn:: 5.6.21-25.8

======================================
 |Percona XtraDB Cluster| 5.6.21-25.8 
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on November 25th 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.21-25.8/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.21-70.1 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.21-70.1.html>`_ including all the bug fixes in it, `Galera Replicator 3.8 <https://github.com/codership/galera/milestones/3.8>`_ and on `Codership wsrep API 25.7 <https://launchpad.net/codership-mysql/+milestone/5.6.21-25.7>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.21-25.8 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.21-25.8>`_ at Launchpad.

New Features
============

 Galera 3.8 introduces auto-eviction for nodes in the cluster experiencing network issues like packet loss. It is off by default and is turned on with :variable:`evs.auto_evict` option. This feature requires EVS protocol version (:variable:`evs.version`) ``1``. During the EVS protocol upgrade all membership changes are communicated over EVS protocol version ``0`` to preserve backwards compatibility, protocol is upgraded to the highest commonly supported version when forming a new group so if there exist a single node with older version in the group, the group protocol version remains as ``0`` and auto-eviction is not functional. (:bug:`1274192`).

 |Percona XtraDB Cluster| now supports backup locks in XtraBackup SST (in the default ``xtrabackup-v2`` :variable:`wsrep_sst_method`). `Backup locks <http://www.percona.com/doc/percona-server/5.6/management/backup_locks.html>`_ are used in lieu of ``FLUSH TABLES WITH READ LOCK`` on the donor during SST. This should allow for minimal disruption of existing and incoming queries, even under high load. Thus, this should allow for even faster SST and node being in 'donor/desynced' state. This also introduces following constraints: |Percona XtraDB Cluster| 5.6.21 requires |Percona XtraBackup| 2.2.5 or higher; An older (< 5.6.21) joiner cannot SST from a newer (>= 5.6.21) donor. This is enforced through SST versioning (sent from joiner to donor during SST) and logged to error log explicitly. (:bug:`1390552`).

 |Percona XtraDB Cluster| is now shipped with Galera MTR test suite. 

Bugs fixed 
==========

 |Percona XtraDB Cluster| now shows a warning in case additional utilities, like ``pv`` which may not affect critical path of SST, are not installed. Bug fixed :bug:`1248688`.

 Fixed the ``UNIV_DEBUG`` build failures. Bug fixed :bug:`1384413`.

 ``mysqldump`` SST can now use username/password from :variable:`wsrep_sst_auth` under group of ``[sst]`` in :file:`my.cnf` in order not to display the credentials in the error log. Bug fixed :bug:`1293798`.
 
 Normal shutdown under load would cause server to remain hanging because replayer failed to finish. Bug fixed :bug:`1358701`.

 :variable:`wsrep_causal_reads` variable was not honored when declared as global. Bug fixed :bug:`1361859`.

 Assertion failure ``lock != ctx->wait_lock`` has been fixed. Bug fixed :bug:`1364840`.

 ``garbd`` would not work when cluster address was specified without the port. Bug fixed :bug:`1365193`.

 Fixed wsrep options compiler warnings in *Fedora* 20. Bug fixed :bug:`1369916`.

 If ``mysqld`` gets killed during the SST it will leave an unclean data directory behind. This would cause |Percona XtraDB Cluster| to fail when the server would be started next time because the data directory would be corrupted. This was fixed by resuming the startup in case :variable:`wsrep-recover` failed to recover due to corrupted data directory. The old behavior is still achievable through :variable:`--exit-on-recover-fail` command line parameter to ``mysqld_safe`` or ``exit-on-recover-fail`` under ``[mysqld_safe]`` in :file:`my.cnf`. Bug fixed :bug:`1378578`.

 |Percona XtraDB Cluster| now reads environment variables for mysqld from following files (if present): /etc/default/mysql in Debian/Ubuntu; ``/etc/sysconfig/mysql`` in CentOS 6 or lower; ``/etc/sysconfig/mysql`` in CentOS 7 with ``mysql.service``; ``/etc/sysconfig/XYZ`` in CentOS 7 with ``mysql@XYZ.service`` (``/etc/sysconfig/bootstrap`` is supplied by default). Bug fixed :bug:`1381492`.

 ``gvwstate.dat`` file was removed on joiner when :ref:`xtrabackup_sst` method was used. Bug fixed :bug:`1388059`.

 |Percona XtraDB Cluster| now detects older joiners which don't have the backup lock support. Bug fixed :bug:`1390552`.

 Longer ``wsrep-recover`` is now handled gracefully in Debian init scripts rather than returning immediately with a false positive fail.

 ``wsrep-recover`` log is now also written to mysql error log now.

 Issue with stale PID files and Debian init script have been fixed now. It now emits a warning for stale PID files. 

 :file:`sst_in_progress` file is not removed anymore in case of failed SST.

 In case stored procedure containing a non-InnoDB statement (MyISAM) performed autocommit, that commit would be entered two times: at statement end and next time at stored procedure end. Bug fixed :wsrepbug:`2`.
 
 TOI now skips replication if all tables are temporary. Bugs fixed :wsrepbug:`11` and :wsrepbug:`13`.

 Two appliers conflicting with local transaction and resulting later in (acceptable) BF-BF lock conflict, would cause cluster to hang when the other BF thread would not grant the lock back after its local transaction got aborted. Bug fixed :wsrepbug:`7`.

 Bootstrapping a node tried to resolve gcomm address list specified in :variable:`wsrep-cluster-address`. Bug fixed :githubbug:`88`.

 ``xtrabackup-v2`` SST did not clean the undo log directory. Bug fixed :bug:`1394836`.

 Inserts to a table with autoincrement primary key could result in duplicate key error if another node joined or dropped from the cluster during the insert processing. Bug fixed :bug:`1366997`.

Other bugs fixed :bug:`1378138`, :bug:`1377226`, :bug:`1376965`, :bug:`1356859`, :bug:`1026181`, :bug:`1367173`, :bug:`1390482`, :bug:`1391634`, and :bug:`1392369`.

Help us improve quality by reporting any bugs you encounter using our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_. As always, thanks for your continued support of Percona!

