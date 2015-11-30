.. rn:: 5.6.15-25.3

======================================
 |Percona XtraDB Cluster| 5.6.15-25.3
======================================

Percona is glad to announce the first General Availability release of |Percona XtraDB Cluster| 5.6 on January 30th 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.15-25.3/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.15-63.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.15-63.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.3 <https://launchpad.net/galera/+milestone/25.3.3>`_ and on `Codership wsrep API 5.6.15-25.2 <https://launchpad.net/codership-mysql/+milestone/5.6.15-25.2>`_ is now the first **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.15-25.3 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.15-25.3>`_ at Launchpad.

New Features
============

 New meta packages are now available in order to make the :ref:`Percona XtraDB Cluster installation <installation>` easier.

 *Debian*/*Ubuntu* debug packages are now available for Galera and ``garbd``.

 :ref:`xtrabackup-v2 <xtrabackup_sst>` |SST| now supports the GTID replication.

Bugs fixed 
==========

 Node would get stuck and required restart if ``DDL`` was performed after ``FLUSH TABLES WITH READ LOCK``. Bug fixed :bug:`1265656`.

 Galera provider pause has been fixed to avoid potential deadlock with replicating threads.

 Default value for :variable:`binlog_format` is now ``ROW``. This is done so that |Percona XtraDB Cluster| is not started with wrong defaults leading to non-deterministic outcomes like crash. Bug fixed :bug:`1243228`.

 During the installation of ``percona-xtradb-cluster-garbd-3.x`` package, *Debian* tries to start it, but as the configuration is not set, it would fail to start and leave the installation in ``iF`` state. Bug fixed :bug:`1262171`.

 Runtime checks have been added for dynamic variables which are Galera incompatible. Bug fixed :bug:`1262188`.

 During the upgrade process, parallel applying could hit an unresolvable conflict when events were replicated from |Percona XtraDB Cluster| 5.5 to |Percona XtraDB Cluster| 5.6. Bug fixed :bug:`1267494`.

 :ref:`xtrabackup-v2 <xtrabackup_sst>` is now used as default |SST| method in :variable:`wsrep_sst_method`. Bug fixed :bug:`1268837`.

 ``FLUSH TABLES WITH READ LOCK`` behavior on the same connection was changed to conform to |MySQL| behavior. Bug fixed :bug:`1269085`.

 Read-only detection has been added in clustercheck, which can be helpful during major upgrades (this is used by ``xinetd`` for HAProxy etc.) Bug fixed :bug:`1269469`.
 
 Binary log directory is now being cleanup as part of the :ref:`XtraBackup SST <xtrabackup_sst>`. Bug fixed :bug:`1273368`.

 First connection would hang after changing the :variable:`wsrep_cluster_address` variable. Bug fixed :bug:`1022250`.
 
 When :variable:`gmcast.listen_addr` was set manually it did not allow nodes own address in gcomm address list. Bug fixed :bug:`1099478`.

 GCache file allocation could fail if file size was a multiple of page size. Bug fixed :bug:`1259952`.

 Group remerge after partitioning event has been fixed. Bug fixed :bug:`1232747`.

 Fixed the OpenSSL linking exceptions. Bug fixed :bug:`1259063`.

 Fixed multiple build bugs: :bug:`1262716`, :bug:`1269063`, :bug:`1269351`, :bug:`1272723`, :bug:`1272732`, and :bug:`1261996`.

Other bugs fixed: :bug:`1273101`, :bug:`1272961`, :bug:`1271264`, and :bug:`1253055`.

We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

