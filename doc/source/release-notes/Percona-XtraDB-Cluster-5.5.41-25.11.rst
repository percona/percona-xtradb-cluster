.. rn:: 5.5.41-25.11

=======================================
 |Percona XtraDB Cluster| 5.5.41-25.11
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on March 30th, 2015. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.41-25.11/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.5.41-37.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.41-37.0.html>`_ including all the bug fixes in it, Galera Replicator 2.11 and on wsrep API 25.11, |Percona XtraDB Cluster| `5.5.41-25.11 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.41-25.11>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

Bugs fixed 
==========

 :ref:`XtraBackup SST <xtrabackup_sst>` wouldn't stop when |MySQL| was ``SIGKILLed``. This would prevent |MySQL| to initiate a new transfer as port 4444 was already utilized. Bug fixed :bug:`1380697`.

 ``garbd`` was returning incorrect return code, ie. when ``garbd`` was already started, return code was ``0``. Bugs fixed :bug:`1308103` and :bug:`1422863`.

 :file:`wsrep_sst_xtrabackup-v2` script was causing |innobackupex| to print a false positive stack trace into the log. Bug fixed :bug:`1407599`.

 |MyISAM| DDL (``CREATE TABLE`` only) isn't replicated anymore when :variable:`wsrep_replicate_myisam` is ``OFF``. Note, for older nodes in the cluster, :variable:`wsrep_replicate_myisam` should work since the TOI decision (for MyISAM DDL) is done on origin node. Mixing of non-MyISAM and MyISAM tables in the same DDL statement is not recommended with :variable:`wsrep_replicate_myisam` ``OFF`` since if any table in list is |MyISAM|, the whole DDL statement is not put under TOI (total order isolation). This also doesn't work if :variable:`default_storage_engine` is set to ``MyISAM`` (which is not recommended for |Percona XtraDB Cluster|) and a table is created without the ``ENGINE`` option. Bug fixed :bug:`1402338`.

 |Percona XtraDB Cluster| now shows a warning in case additional utilities, like ``pv`` which may not affect critical path of SST, are not installed. Bug fixed :bug:`1248688`.

 :variable:`wsrep_causal_reads` variable was not honored when declared as global. Bug fixed :bug:`1361859`.

 ``garbd`` would not work when cluster address was specified without the port. Bug fixed :bug:`1365193`.

 ``garbd`` was running as root user on *Debian*. Bug fixed :bug:`1392388`.

 Errors in ``garbd`` init script stop/start functions have been fixed. Bug fixed :bug:`1367956`.

 If ``mysqld`` gets killed during the SST it will leave an unclean data directory behind. This would cause |Percona XtraDB Cluster| to fail when the server would be started next time because the data directory would be corrupted. This was fixed by resuming the startup in case :variable:`wsrep-recover` failed to recover due to corrupted data directory. The old behavior is still achievable through :variable:`--exit-on-recover-fail` command line parameter to ``mysqld_safe`` or ``exit-on-recover-fail`` under ``[mysqld_safe]`` in :file:`my.cnf`. Bug fixed :bug:`1378578`.

 :file:`gvwstate.dat` file was removed on joiner when :ref:`XtraBackup SST <xtrabackup_sst>` method was used. Bug fixed :bug:`1388059`.

 ``xtrabackup-v2`` SST did not clean the undo log directory. Bug fixed :bug:`1394836`.

 ``stderr`` of SST/Innobackupex is logged to syslog with appropriate tags if ``sst-syslog`` is in ``[sst]`` or ``[mysqld_safe]`` has syslog in :file:`my.cnf`. This can be overridden by setting the :variable:`sst-syslog` to ``-1`` in ``[sst]``. Bug fixed :bug:`1399134`.

 ``clustercheck`` can now check if the node is ``PRIMARY`` or not, to allow for synced nodes which go out of ``PRIMARY`` not to take any writes/reads. Bug fixed :bug:`1403566`.

 Race condition between donor and joiner in :ref:`xtrabackup_sst` has been fixed. This caused :ref:`XtraBackup SST <xtrabackup_sst>` to fail when joiner took longer to spawn the second listener for SST. Bug fixed :bug:`1405668`.

 |SST| will now fail early if the :file:`xtrabackup_checkpoints` file is missing on the joiner side. Bug fixed :bug:`1405985`.

 ``socat`` utility was not properly terminated after a timeout. Bug fixed :bug:`1409710`.

 10 seconds timeout in :ref:`xtrabackup_sst` script was not enough for the joiner to delete existing files before it started the socat receiver on systems with big ``datadir``. Bug fixed :bug:`1413879`.

 Conflict between `enforce_storage_engine <http://www.percona.com/doc/percona-server/5.6/management/enforce_engine.html#enforce_storage_engine>`_ and :variable:`wsrep_replicate_myisam` for ``CREATE TABLE`` has been fixed. Bug fixed :bug:`1435482`.
 
 SST processes are now spawned with ``fork/exec`` instead of ``posix_spawn`` to allow for better cleanup of child processes in event of non-graceful termination (``SIGKILL`` or a crash etc.). Bug fixed :bug:`1382797`.

 Variable length arrays in WSREP code were causing debug builds to fail. Bug fixed :bug:`1409042`.

 Signal handling in ``mysqld`` has been fixed for SST processes. Bug fixed :bug:`1399175`.

 Inserts to a table with autoincrement primary key could result in duplicate key error if another node joined or dropped from the cluster during the insert processing. Bug fixed :bug:`1366997`.

Other bugs fixed: :bug:`1391634` and :bug:`1396757`.

