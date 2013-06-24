.. rn:: 5.5.31-23.7.5

========================================
 |Percona XtraDB Cluster| 5.5.31-23.7.5
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on June 24th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.31-23.7.5/>`_ or from our :doc:`software repositories </installation>`.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

New Features
============

 Information about the wsrep sequence number has been added to ``INFORMATION_SCHEMA`` :table:`INNODB_TRX` table.

 |Percona XtraDB Cluster| can now be :ref:`bootstrapped <bootstrap>` with the new ``bootstrap-pxc`` option in the init script.

 |Percona XtraDB Cluster| has implemented  parallel copying for rsync :ref:`state_snapshot_transfer` method. Bug fixed :bug:`1167331` (*Mrten*).

 |Percona XtraDB Cluster| has implemented new *Python* version of the `clustercheck <https://github.com/Oneiroi/clustercheck>`_ script.

 |Percona XtraDB Cluster| now has better integration with |Percona XtraBackup| 2.1 by implementing new ``xbstream`` option for xtrabackup :ref:`state_snapshot_transfer` method.


Bugs fixed 
==========

 Fixed the packaging issues that caused |Percona XtraDB Cluster| to conflict with |MySQL| libraries when performing fresh *CentOS* installation. Bug fixed :bug:`1183669`.

 The RPM installer script had the :term:`datadir` hardcoded to :file:`/var/lib/mysql` instead of ``using my_print_defaults`` to get the datadir info. bug fixed :bug:`1172621`.

 Fixed the package conflict for ``percona-xtradb-cluster-client-5.5`` package. Bug fixed :bug:`1172621`.

 Fixed the ``Percona-Server-shared-55`` and ``Percona-XtraDB-Cluster-shared`` ``RPM`` package dependences. Bug fixed :bug:`1050654`.

 ``mysql_tzinfo_to_sql`` script failed with |Percona XtraDB Cluster| since |MyISAM| DDL/DML were not consistently replicated. Fixed by enabling session variable :variable:`wsrep_replicate_myisam` in the generator file. Bug fixed :bug:`1161432`.

 Startup script on *Debian* and *Ubuntu* was failing prematurely during ``SST``. Bug fixed :bug:`1099428`.

 Due to *Debian* way of stopping/reloading/status checking, there was failure after SST since the machine-specific *Debian* maintenance password didn't work. Fixes by using signals instead of ``mysqladmin`` as it is done in *CentOS*. Bug fixed :bug:`1098360`.

 :variable:`wsrep-recover` would create ``ibdata1`` and ``ib_logfile*`` files which the SST after that would remove. Bug fixed :bug:`1191767`. 

 When running :variable:`wsrep_recover` to recover galera co-ordinates, LRU recover (and its blocking counterpart) were causing issues. Bug fixed :bug:`1108035`.

 |Percona XtraDB Cluster| improved status visibility in the ``processlist`` on other nodes when provider is locked with ``FLUSH TABLES WITH READ LOCK`` on one of them. Bug fixed :bug:`1109341`.

Crash/stalling bugs
------------------- 

 |Percona Server| `Crash-Resistant Replication <http://www.percona.com/doc/percona-server/5.5/reliability/crash_resistant_replication.html>`_ was causing conflicts with :variable:`wsrep_crash_recovery` on a node which is a slave to an async master in standard |MySQL| replication. Bugs fixed :bug:`1182441` and :bug:`1180802`.
 
 |Percona XtraDB Cluster| node would hang on shutdown when :variable:`thread_handling` was set to  `pool-of-threads <http://www.percona.com/doc/percona-server/5.5/performance/threadpool.html#thread-pool>`_. Bug Fixed :bug:`1185523`.  

 ``FLUSH TABLES WITH READ LOCK`` didn't behave as expected, it had different behavior than the upstream |MySQL| version. Bug fixed :bug:`1170821`.

 When rsync :ref:`state_snapshot_transfer` failed, the rsync daemon would remain running which caused ``pid`` errors on subsequent ``SST`` retries. Bug fixed :bug:`1169676`.

 When doing cascading Foreign Key processing |Percona XtraDB Cluster| was doing unnecessary Foreign Key checks, which are needed only for the purpose of getting Foreign Key parent reference for child table modifications. Bug fixed :bug:`1134892`.

 High priority threads (slave threads, replaying threads, total order isolation threads) were not protected from kill signal and attempt to kill high priority thread could cause server to hang. Bug fixed :bug:`1088267`.

 Galera provider may deadlock if applier thread is still executing ``apply_trx()`` while processing commit, it could cause corresponding transaction to be purged from cert index.

Regressions
-----------

 Previous |Percona XtraDB Cluster| release introduced the regression in the RPM packaging that prevented the server from restarting following an upgrade. Bug fixed :bug:`1168032`. 
 
Other bug fixes: bug fixed :bug:`1187526`, bug fixed :bug:`1133266`, bug fixed :bug:`1079688`, bug fixed :bug:`1177211`, bug fixed :bug:`1166423`, bug fixed :bug:`1084573`, bug fixed :bug:`1069829`, bug fixed :bug:`1012138`, bug fixed :bug:`1170066`, bug fixed :bug:`1170706`, bug fixed :bug:`1182187`, bug fixed :bug:`1183530`, bug fixed :bug:`1183997`, bug fixed :bug:`1184034`, bug fixed :bug:`1191763`, bug fixed :bug:`1069829`.

Based on `Percona Server 5.5.31-30.3 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.31-30.3.html>`_ including all the bug fixes in it and on `Codership wsrep API 5.5.31-23.7.5 <https://launchpad.net/codership-mysql/+milestone/5.5.31-23.7.5>`_, |Percona XtraDB Cluster| `5.5.31-23.7.5 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.31-23.7.5>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

