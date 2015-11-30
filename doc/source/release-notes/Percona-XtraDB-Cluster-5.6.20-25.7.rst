.. rn:: 5.6.20-25.7

======================================
 |Percona XtraDB Cluster| 5.6.20-25.7 
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on September 1st 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.20-25.7/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.20-68.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.20-68.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.7 <https://github.com/codership/galera/issues?milestone=1&page=1&state=closed>`_ and on `Codership wsrep API 25.7 <https://launchpad.net/wsrep-group/+milestone/5.6.20-25.6>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.20-25.7 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.20-25.7>`_ at Launchpad.

New Features
============
 
 `systemd <http://freedesktop.org/wiki/Software/systemd/>`_ integration with *RHEL*/*CentOS* 7 is now available for |Percona XtraDB Cluster| (:bug:`1342223`).

 New session variable :variable:`wsrep_sync_wait` has been implemented to control causality check. The old session variable :variable:`wsrep_causal_reads` is deprecated but is kept for backward compatibility (:bug:`1277053`).

Bugs fixed 
==========

 Running ``START TRANSACTION WITH CONSISTENT SNAPSHOT``, ``mysqldump`` with :option:`--single-transaction` or ``mydumper`` with disabled binlog would lead to a server crash. Bug fixed :bug:`1353644`.

 ``percona-xtradb-cluster-garbd-3.x`` package was installed incorrectly on *Debian*/*Ubuntu*. Bug fixed :bug:`1360633`.

 Fixed ``netcat`` in SST script for Centos7 ``nmap-ncat``. Bug fixed :bug:`1359767`.

 TO isolation was run even when wsrep plugin was not loaded. Bug fixed :bug:`1358681`.

 The error from net read was not handled in native |MySQL| mode. This would cause duplicate key error if there was unfinished transaction at the time of shutdown, because it would be committed during the startup recovery. Bug fixed :bug:`1358264`.

 The netcat in garbd init script has been replaced with nmap for compatibility in *CentOS* 7. Bug fixed :bug:`1349384`.

 ``SHOW STATUS`` was generating debug output in the error log. Bug fixed :bug:`1347818`.

 Incorrect source string length could lead to server crash. This fix allows maximum of 3500 bytes of key material to be populated, longer keys will be truncated. Bug fixed :bug:`1347768`.

 A memory leak in ``wsrep_mysql_parse`` function has been fixed. Bug fixed :bug:`1345023`.

 wsrep consistency check is now enabled for ``REPLACE ... SELECT`` as well. This was implemented because ``pt-table-checksum`` uses ``REPLACE .. SELECT`` during checksumming. Bug fixed :bug:`1343209`.
 
 Client connections were closed unconditionally before generating SST request. Fixed by avoiding closing connections when wsrep is initialized before storage engines. Bug fixed :bug:`1258658`.
 
 Session-level binlog_format change to ``STATEMENT`` is now allowed to support ``pt-table-checksum``. A warning (to not use it otherwise) is also added to error log.

Other bug fixes: :bug:`1280270`.

Help us improve quality by reporting any bugs you encounter using our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_. As always, thanks for your continued support of Percona!

