.. rn:: 5.6.19-25.6

======================================
 |Percona XtraDB Cluster| 5.6.19-25.6 
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on July 21st 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.19-25.6/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.19-67.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.19-67.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.6 <https://github.com/codership/galera/issues?milestone=1&page=1&state=closed>`_ and on `Codership wsrep API 25.6 <https://launchpad.net/wsrep-group/+milestone/5.6.19-25.6>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.19-25.6 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.19-25.6>`_ at Launchpad.

New Features
============
 
 During joiner joining the group, state message exchange provides us with ``gcache seqno limits``. That info is now used to choose a donor through IST first, if not possible, only then SST is attempted. :variable:`wsrep_sst_donor` is also honored here. This is also segment aware. (:bug:`1252461`)

 Asynchronous replication slave thread is stopped when the node tries to apply next replication event while the node is in non-primary state. But it would then remain stopped after node successfully re-joined the cluster. A new variable :variable:`wsrep_restart_slave` has been implemented, which controls if |MySQL| slave should be restarted automatically when the node joins back to the cluster. (:bug:`1288479`)

 Handling install message and install state message processing has been improved to make group forming more stable in case many nodes are joining the cluster. (:githubbug:`14`)

 |Percona XtraDB Cluster| now supports storing the Primary Component state to disk by setting the :variable:`pc.recovery` variable to ``true``. The Primary Component can then recover automatically when all nodes that were part of the last saved state reestablish communications with each other. This feature can be used for automatic recovery from full cluster crashes, such as in the case of a data center power outage and graceful full cluster restarts without the need for explicitly bootstrapping a new Primary Component. (:githubbug:`10`)

 New :variable:`wsrep_evs_repl_latency` status variable has been implemented which provides the group communication replication latency information. (:githubbug:`15`)

 Node consistency issues with foreign keys have been fixed. This fix introduces two new variables: :variable:`wsrep_slave_FK_checks` and :variable:`wsrep_slave_UK_checks`. These variables are set to ``TRUE`` and ``FALSE`` respectively by default. They control whether Foreign Key and Unique Key checking is done for applier threads. (:bug:`1260713`).

Bugs fixed 
==========

 Fixed the race condition in Foreign Key processing that could cause assertion. Bug fixed :bug:`1342959`.
 
 The restart sequence in ``scripts/mysql.server`` would fail to capture and return if the start call failed to start the server, so a restart could occur that failed upon start-up, and the script would still return ``0`` as if it worked without any issues. Bug fixed :bug:`1339894`.

 Updating a unique key value could cause server hang if slave node has enabled parallel slaves. Bug fixed :bug:`1280896`.
 
 |Percona XtraDB Cluster| has implemented threadpool scheduling fixes. Bug fixed :bug:`1333348`.

 ``garbd`` was returning incorrect return code, ie. when ``garbd`` was already started, return code was ``0``. Bug fixed :bug:`1308103`.

 :variable:`wsrep_sst_rsync` would silently fail on joiner when ``rsync`` server port was already taken. Bug fixed :bug:`1099783`.

 Example ``wsrep_notify`` script failed on node shutdown. Bug fixed :bug:`1132955`.

 When :variable:`gmcast.listen_addr` was configured to a certain address, local connection point for outgoing connections was not bound to listen address. This would happen if OS has multiple interfaces with IP addresses in the same subnet, it may happen that OS would pick wrong IP for local connection point and other nodes would see connections originating from IP address which was not listened to. Bug fixed :bug:`1240964`.

 Issue with re-setting galera provider (in :variable:`wsrep_provider_options`) has been fixed. Bug fixed :bug:`1260283`.

 Variable  :variable:`wsrep_provider_options` couldn't be set in runtime if no provider was loaded. Bug fixed :bug:`1260290`.

 |Percona XtraDB Cluster| couldn't be built with *Bison* 3.0. Bug fixed :bug:`1262439`.

 ``mysqld`` wasn't handling exceeding max writeset size wsrep error correctly. Bug fixed :bug:`1270920`.

 When ``FLUSH TABLES WITH READ LOCK`` was used on a node with :variable:`wsrep_causal_reads` set to ``1`` while there was a ``DML`` on other nodes then, subsequent SELECTs/SHOW STATUS didn't hang earlier providing non-causal output, that has been fixed here. Bug fixed :bug:`1271177`.

 Lowest group communication layer (evs) would fail to handle the situation properly when big number of nodes would suddenly start to see each other. Bugs fixed :bug:`1271918` and :bug:`1249805`.

 |Percona XtraDB Cluster| server package no longer conflicts with ``mysql-libs`` package from *CentOS* repository. Bug fixed :bug:`1278516`.

 The mysql-debug ``UNIV_DEBUG`` binary was missing from RPM/DEB server packages. Bug fixed :bug:`1290087`.
 
 XtraBackup SST would fail if `progress <http://www.percona.com/doc/percona-xtradb-cluster/5.6/manual/xtrabackup_sst.html#progress>`_ option was used with large number of files. Bug fixed :bug:`1294431`.

 When Query Cache was used and a node would go into non-PRIM state, queries which returned results earlier (and cached into query cache) would still return results whereas newer queries (or the ones not cached) would return ``unknown command``. Bug fixed :bug:`1296403`.

 Brute Force abort did not work with INSERTs to table with single unique key. Bug fixed :bug:`1299116`.

 |InnoDB| buffer pool dump and load was not working as expected due to :variable:`wsrep_recover` overwriting the buffer pool dump file. Bug fixed :bug:`1305955`.

 Close referenced table opened in the same function when foreign constraints were checked, otherwise it could lead to server stall when running ``DROP TABLE``. Bug fixed :bug:`1309241`.

 Compiling on *FreeBSD* 10.0 with ``CLANG`` would result in fatal error. Bug fixed :bug:`1309507`.

 Truncating the sorted version of multi-byte character conversion could lead to wsrep certification failures. Bug fixed :bug:`1314854`.

 Cluster node acting as async slave would stop with the wrong position after hitting :variable:`max_write_set_size`. Bug fixed :bug:`1309669`.

 Fixed the events replication inconsistencies. Bug fixed :bug:`1312618`.

 :variable:`wsrep_slave_threads` was counted towards :variable:`max_connections` which could cause ``ERROR 1040 (HY000): Too many connections`` error. Bug fixed :bug:`1315588`.

 Leaving node was not set nonoperational if processed leave message originated from different view than the current one which could cause other nodes to crash. Bug fixed :bug:`1323412` (:githubbug:`41`).

 ``garb`` couldn't be started with ``init`` script on *RHEL* 6.5. Bug fixed :bug:`1323652`.

 SST would fail when binlogs were in dedicated directory that's located inside ``datadir``. This bug was a regression introduced by bug fix for :bug:`1273368`. Bug fixed :bug:`1326012`.

 GTID of TOI operations is now also synced to |InnoDB| tablespace in order to get consistent backups. Bug fixed :bug:`1329055`.

 ``mysql-debug`` is now distributed with binary ``tar.gz``  along with RPM and DEB packages. Bug fixed :bug:`1332073`.

 Startup failure with ``Undetected state gap`` has been fixed. Bug fixed :bug:`1334606`.

 Galera3 is now installed in ``/usr/lib/galera3/libgalera_smm.so`` with a  compatibility symlink to ``/usr/lib/libgalera_smm.so``. Bug fixed :bug:`1279328`.

 Galera could not be compiled on PowerPC. Bug fixed :githubbug:`59`.

 Cluster could stall if leaving node failed to acknowledge all messages it had received due to exception and remaining nodes failed to reach consensus because of that. Bug fixed :githubbug:`37`.  

 When two node instances were set up on the same server with the two different IPs, they couldn't not work well because they were use wrong IP addresses. Bug fixed :githubbug:`31`.

 Automated donor selection with segments gave inconsistent results. Bug fixed :githubbug:`29`.

Other bug fixes: :bug:`1297822`, :bug:`1269811`, :bug:`1262887`, :bug:`1244835`, :bug:`1338995`, :githubbug:`11`, :githubbug:`40`, :githubbug:`38`, :githubbug:`33`, and :githubbug:`24`.

Help us improve quality by reporting any bugs you encounter using our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_. As always, thanks for your continued support of Percona!

