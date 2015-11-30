.. rn:: 5.6.22-25.8

======================================
 |Percona XtraDB Cluster| 5.6.22-25.8 
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on March 5th 2015. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.22-25.8/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.22-72.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.22-72.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.9 <https://github.com/codership/galera/issues?q=milestone%3A25.3.9>`_ and on `Codership wsrep API 25.8 <https://launchpad.net/codership-mysql/+milestone/5.6.21-25.8>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.22-25.8 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.22-25.8>`_ at Launchpad.

Bugs fixed 
==========

 :ref:`XtraBackup SST <xtrabackup_sst>` wouldn't stop when |MySQL| was ``SIGKILLed``. This would prevent |MySQL| to initiate a new transfer as port 4444 was already utilized. Bug fixed :bug:`1380697`.

 :file:`wsrep_sst_xtrabackup-v2` script was causing |innobackupex| to print a false positive stack trace into the log. Bug fixed :bug:`1407599`.

 |MyISAM| DDL (``CREATE/DROP``) isn't replicated any more when :variable:`wsrep_replicate_myisam` is ``OFF``. Note, for older nodes in the cluster, :variable:`wsrep-replicate-myisam` should work since the TOI decision (for MyISAM DDL) is done on origin node. Mixing of non-MyISAM and MyISAM tables in the same DDL statement is not recommended with :variable:`wsrep_replicate_myisam` ``OFF`` since if any table in list is |MyISAM|, the whole DDL statement is not put under TOI (total order isolation). Bug fixed :bug:`1402338`.

 :variable:`gcache.mem_size` has been deprecated. A warning will now be generated if the variable has value different than ``0``. Bug fixed :bug:`1392408`.

 ``stderr`` of SST/Innobackupex is logged to syslog with appropriate tags if ``sst-syslog`` is in ``[sst]`` or ``[mysqld_safe]`` has syslog in :file:`my.cnf`. This can be overridden by setting the :variable:`sst-syslog` to ``-1`` in ``[sst]``. Bug fixed :bug:`1399134`.

 ``clustercheck`` can now check if the node is ``PRIMARY`` or not, to allow for synced nodes which go out of ``PRIMARY`` not to take any writes/reads. Bug fixed :bug:`1403566`.

 |SST| will now fail early if the :file:`xtrabackup_checkpoints` is missing on the joiner side. Bug fixed :bug:`1405985`.

 ``socat`` utility was not properly terminated after a timeout. Bug fixed :bug:`1409710`.

 When started (without bootstrap), the node would hang if it couldn't find a primary node. Bug fixed :bug:`1413258`.

 10 seconds timeout in :ref:`xtrabackup_sst` script was not enough for the joiner to delete existing files before it started the socat receiver on systems with big ``datadir``. Bug fixed :bug:`1413879`.

 Non booststrap node could crash while attempting to perform ``table%cache`` operations with the ``BF applier failed to open_and_lock_tables`` warning. Bug fixed :bug:`1414635`.

 |Percona XtraDB Cluster| 5.6 would crash on ``ALTER TABLE`` / ``CREATE INDEX`` with ``Failing assertion: table->n_rec_locks == 0``. Bug fixed :bug:`1282707`.

 Variable length arrays in WSREP code were causing debug builds to fail. Bug fixed :bug:`1409042`.

 Race condition between donor and joiner in :ref:`xtrabackup_sst` has been fixed. This caused :ref:`XtraBackup SST <xtrabackup_sst>` to fail when joiner took longer to spawn the second listener for SST. Bug fixed :bug:`1405668`.

 Signal handling in ``mysqld`` has been fixed for SST processes. Bug fixed :bug:`1399175`.

 SST processes are now spawned with ``fork/exec`` instead of ``posix_spawn`` to allow for better cleanup of child processes in event of non-graceful termination (``SIGKILL`` or a crash etc.). Bug fixed :bug:`1382797`.

 :variable:`wsrep_local_cached_downto` would underflow when the node on which it is queried had no writesets in gcache. Bug fixed :bug:`1262179`.
 
 A typo in :variable:`wsrep_provider_options` could cause an unhandled exception. Bug fixed :githubbug:`215`.

 Interrupted IST would result in ``HA_ERR_KEY_NOT_FOUND`` error in subsequent IST. Bug fixed :githubbug:`210`.

 ``garb`` logger was showing incorrect error message. Bug fixed :githubbug:`168`.

Other bugs fixed: :bug:`1275814`. 

Known Issues
============

For those affected by crashes on donor during SST due to backup locks (:bug:`1401133`), please add the following to your :file:`my.cnf` configuration file: ::

  [sst]
  inno-backup-opts='--no-backup-locks'

option as a workaround to force ``FLUSH TABLES WITH READ LOCK`` (**NOTE:** This workaround will is available only if you're using |Percona XtraBackup| 2.2.9 or newer.). Or as an alternative you can set your environment variable ``FORCE_FTWRL`` to ``1``.

Help us improve quality by reporting any bugs you encounter using our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_. As always, thanks for your continued support of Percona!

