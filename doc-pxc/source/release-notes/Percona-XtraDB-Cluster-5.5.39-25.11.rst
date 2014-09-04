.. rn:: 5.5.39-25.11

=======================================
 |Percona XtraDB Cluster| 5.5.39-25.11
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on September 5th, 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.39-25.11/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.5.39-36.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.39-36.0.html>`_ including all the bug fixes in it, Galera Replicator 2.11 and on wsrep API `25.11 <https://launchpad.net/codership-mysql/+milestone/5.5.38-25.11>`_), |Percona XtraDB Cluster| `5.5.39-25.11 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.39-25.11>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.


New Features
============

 New session variable :variable:`wsrep_sync_wait` has been implemented to control causality check. The old session variable :variable:`wsrep_causal_reads` is deprecated but is kept for backward compatibility (:bug:`1277053`).

 `systemd <http://freedesktop.org/wiki/Software/systemd/>`_ integration with *RHEL*/*CentOS* 7 is now available for |Percona XtraDB Cluster| (:bug:`1342223`).

Bugs fixed 
==========

 |Percona XtraDB Cluster| has implemented threadpool scheduling fixes. Bug fixed :bug:`1333348`.

 When :variable:`gmcast.listen_addr` was configured to a certain address, local connection point for outgoing connections was not bound to listen address. This would happen if OS has multiple interfaces with IP addresses in the same subnet, it may happen that OS would pick wrong IP for local connection point and other nodes would see connections originating from IP address which was not listened to. Bug fixed :bug:`1240964`.

 Client connections were closed unconditionally before generating SST request. Fixed by avoiding closing connections when wsrep is initialized before storage engines. Bug fixed :bug:`1258658`.

 Issue with re-setting galera provider (in :variable:`wsrep_provider_options`) has been fixed. Bug fixed :bug:`1260283`.

 Variable  :variable:`wsrep_provider_options` couldn't be set in runtime if no provider was loaded. Bug fixed :bug:`1260290`.

 Node consistency issues with foreign keys have been fixed. This fix introduces two new variables: :variable:`wsrep_slave_FK_checks` and :variable:`wsrep_slave_UK_checks`. These variables are set to ``TRUE`` and ``FALSE`` respectively by default. They control whether Foreign Key and Unique Key checking is done for applier threads. (:bug:`1260713`).

 When ``FLUSH TABLES WITH READ LOCK`` was used on a node with :variable:`wsrep_causal_reads` set to ``1`` while there was a ``DML`` on other nodes then, subsequent SELECTs/SHOW STATUS didn't hang earlier providing non-causal output, that has been fixed here. Bug fixed :bug:`1271177`.
 
 Lowest group communication layer (evs) would fail to handle the situation properly when big number of nodes would suddenly start to see each other. Bugs fixed :bug:`1271918` and :bug:`1249805`.

 Updating a unique key value could cause server hang if slave node has enabled parallel slaves. Bug fixed :bug:`1280896`.

 Fixed the events replication inconsistencies. Bug fixed :bug:`1312618`.

 Truncating the sorted version of multi-byte character conversion could lead to wsrep certification failures. Bug fixed :bug:`1314854`.

 :variable:`wsrep_slave_threads` was counted towards :variable:`max_connections` which could cause ``ERROR 1040 (HY000): Too many connections`` error. Bug fixed :bug:`1315588`.

 Leaving node was not set nonoperational if processed leave message originated from different view than the current one which could cause other nodes to crash. Bug fixed :bug:`1323412` (:githubbug:`41`).

 ``garb`` couldn't be started with ``init`` script on *RHEL* 6.5. Bug fixed :bug:`1323652`.

 SST would fail when binlogs were in dedicated directory that's located inside ``datadir``. This bug was a regression introduced by bug fix for :bug:`1273368`. Bug fixed :bug:`1326012`.

 GTID of TOI operations is now also synced to |InnoDB| tablespace in order to get consistent backups. Bug fixed :bug:`1329055`.

 ``mysql-debug`` (``UNIV_DEBUG``) is now distributed with binary ``tar.gz``  along with RPM and DEB packages. Bug fixed :bug:`1332073`.

 Startup failure with ``Undetected state gap`` has been fixed. Bug fixed :bug:`1334606`.
 
 The restart sequence in ``scripts/mysql.server`` would fail to capture and return if the start call failed to start the server, so a restart could occur that failed upon start-up, and the script would still return ``0`` as if it worked without any issues. Bug fixed :bug:`1339894`.
 
 wsrep consistency check is now enabled for ``REPLACE ... SELECT`` as well. This was implemented because ``pt-table-checksum`` uses ``REPLACE .. SELECT`` during checksumming. Bug fixed :bug:`1343209`.

 A memory leak in ``wsrep_mysql_parse`` function has been fixed. Bug fixed :bug:`1345023`.

 ``SHOW STATUS`` was generating debug output in the error log. Bug fixed :bug:`1347818`.

 The netcat in garbd init script has been replaced with nmap for compatibility in *CentOS* 7. Bug fixed :bug:`1349384`.
 
 Fixed ``netcat`` in SST script for *CentOS* 7 ``nmap-ncat``. Bug fixed :bug:`1359767`.

 ``percona-xtradb-cluster-garbd-3.x`` package was installed incorrectly on *Debian*/*Ubuntu*. Bugs fixed :bug:`1360633` and :bug:`1334530`.

Other bugs fixed: :bug:`1334331`, :bug:`1338995`, :bug:`1280270`, :bug:`1272982`, :bug:`1190774`, and :bug:`1251765`.


|Percona XtraDB Cluster| `Errata <http://www.percona.com/doc/percona-xtradb-cluster/errata.html>`_ can be found in our documentation.
