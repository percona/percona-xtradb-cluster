.. rn:: 5.5.30-23.7.4

========================================
 |Percona XtraDB Cluster| 5.5.30-23.7.4
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on April 17th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.30-23.7.4/>`_ or from our :doc:`software repositories </installation>`.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

New Features
============

 |Percona XtraDB Cluster| has implemented initial implementation of weighted quorum. Weight for node can be assigned via :variable:`pc.weight` option in the :variable:`wsrep_provider_options` variable. Accepted values are in the range [0, 255] (inclusive). Quorum is computed using weighted sum over group members.

 |Percona XtraDB Cluster| binary will now be shipped with the ``libjemalloc`` library. For ``rpm/deb`` packages, this library will be available for download from our repositories. Benchmark showing the impact of memory allocators on |MySQL| performance can be found in this `blogpost <http://www.mysqlperformanceblog.com/2012/07/05/impact-of-memory-allocators-on-mysql-performance/>`_.

 This release of |Percona XtraDB Cluster| has fixed number of foreign key and packaging bugs.

Bugs fixed 
==========
 
 Fixed yum dependencies that were causing conflicts in ``CentOS`` 6.3 during installation. Bug fixed :bug:`1031427` (*Ignacio Nin*).

 In case the |Percona XtraDB Cluster| was built from the source rpm, wsrep revision information would be missing. Bug fixed :bug:`1128906` (*Alexey Bychko*).

 The method of generating md5 digest over tuples in a table with no primary key was not deterministic and this could cause a node failure. Bug fixed :bug:`1019473` (*Seppo Jaakola*).

 |Percona XtraDB Cluster| was built with YaSSL which could cause some of the programs that use it to crash. Fixed by building packages with OpenSSL support rather than the bundled YaSSL library. Bug fixed :bug:`1104977` (*Raghavendra D Prabhu*).

 Clustercheck script would hang in case the |MySQL| server on a node is hung. As a consequence clustercheck script would never fail-over that server. Bug fixed :bug:`1035927` (*Raghavendra D Prabhu*).

 High values in variables :variable:`evs.send_window` and :variable:`evs.user_send_window` could trigger cluster crash under high load. Bug fixed :bug:`1080539` (*Teemu Ollakka*).

 Standard |MySQL| port would be used when port number isn't explicitly defined in the :variable:`wsrep_node_incoming_address`. Bug fixed :bug:`1082406` (*Alex Yurchenko*).

 Dropping a non-existing temporary table would be replicated when TOI was used in :variable:`wsrep_OSU_method` variable. This bug was fixed for the case when ``DROP TEMPORARY TABLE`` statement was used, but it will still replicate in case ``DROP TABLE`` statement is used on a temporary table. Bug fixed :bug:`1084702` (*Seppo Jaakola*).

 In case two nodes in a 3-node cluster had to abort due to inconsistency, one wouldn't correctly notify the surviving node which would lead to surviving node to loose the primary component and cause subsequent downtime. Bug fixed :bug:`1108165` (*Alex Yurchenko*).

 In some cases non-uniform foreign key reference could cause a slave crash. Fixed by using primary key of the child table when appending exclusive key for cascading delete operation. Bug fixed :bug:`1089490` (*Seppo Jaakola*).

 Parallel applying would fail in case mixed ``CHAR`` and ``VARCHAR`` columns would be used in foreign key definitions. Bug fixed :bug:`1100496` (*Seppo Jaakola*).

 *Debian* packages included the old version of **innotop**. Fixed by removing **innotop** and its ``InnoDBParser`` Perl package from source and *Debian* installation. Bug fixed :bug:`1032139` (*Alexey Bychko*).

 The mysqld_safe script would fail to retrieve the galera replication position on ``Ubuntu`` 10.04, because the different shell was used. Bug fixed :bug:`1108431` (*Alex Yurchenko*).

 Cascading foreign key constraint could lead to unresolved replication conflict and leave a slave hanging. Bug fixed :bug:`1130888` (*Seppo Jaakola*).

 If |MySQL| replication threads were started before running wsrep recovery, this would lead to memory corruption and server crash. Bug fixed :bug:`1132974` (*Seppo Jaakola*).

 Conflicting prepared statements in multi-master use case could cause node to hang. This was happening due to prepared statement execution loop, which does not honor wsrep status codes correctly. Bug fixed :bug:`1144911` (*Seppo Jaakola*).

 :ref:`state_snapshot_transfer` with |Xtrabackup| would fail if the ``tmpdir`` was specified more than once in the |MySQL| configuration file (:file:`my.cnf`). Bugs fixed :bug:`1160047` and :bug:`1086978` (*Raghavendra D Prabhu*).

 Issues with compiling Galera on the ``ARM`` architecture has been fixed. Bug fixed :bug:`1133047` (*Alex Yurchenko*).

 Upstream bugfix for bug :mysqlbug:`59354` triggered a regression that could cause transaction conflicts. Bug fixed :bug:`1158221` (*Seppo Jaakola*).

 Galera builds would fail when they were built with the new ``boost`` library. Bug fixed :bug:`1131736` (*Alex Yurchenko*).

 Folder ``lost+found`` wasn't included in the rsync SST filter, which caused the SST failure due to insufficient privileges. Fixed by excluding ``lost+found`` folder if found. Bug fixed :bug:`1154095` (*Alex Yurchenko*).

 If variable :variable:`innodb_thread_concurrency` has been defined to throttle |InnoDB| access, and work load contained DDL statements, a cluster node could remain hanging for unresolved MDL conflict. Fixed by adding a new method to cancel a thread waiting for |InnoDB| concurrency. Bug fixed :bug:`1155183` (*Seppo Jaakola*).

 Handling of the network issues in Galera has been improved. Bug fixed :bug:`1153727` (*Teemu Ollakka*).

 Fixed the wrong path in the ``/etc/xinetd.d/mysqlchk`` script. Bugs fixed :bug:`1000761` and :bug:`1132934` (*Raghavendra D Prabhu*).

 When upgrading the Percona-XtraDB-Cluster-server package, ``/usr/bin/clustercheck`` would get overwritten, and any changes (such as username and password) would be lost. Bug fixed :bug:`1158443` (*Raghavendra D Prabhu*).

 In case ``CREATE TABLE AS SELECT`` statement was running in parallel with the DDL statement on the selected table, in some cases first statement could be left hanging. Bug fixed :bug:`1164893` (*Seppo Jaakola*).

 Galera builds would fail when ``gcc`` 4.8 was used. Bug fixed :bug:`1164992` (*Alex Yurchenko*).

 ``Percona-XtraDB-Cluster-galera`` package version number didn't match the :variable:`wsrep_provider_version` one. Bug fixed :bug:`1111672` (*Alexey Bychko*).

 Only ``rpm`` debug build was available for |Percona XtraDB Cluster|, fixed by providing the ``deb`` debug build as well. Bug fixed :bug:`1096123` (*Ignacio Nin*).
 
Other bug fixes: bug fixed :bug:`1162421` (*Seppo Jaakola*), bug fixed :bug:`1093054` (*Alex Yurchenko*), bug fixed :bug:`1166060` (*Teemu Ollakka*), bug fixed :bug:`1166065` (*Teemu Ollakka*).

Based on `Percona Server 5.5.30-30.2 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.30-30.2.html>`_ including all the bug fixes in it and on `Codership wsrep API 5.5.30-23.7.4 <https://launchpad.net/codership-mysql/+milestone/5.5.30-23.7.4>`_, |Percona XtraDB Cluster| 5.5.30-23.7.4 is now the current stable release. All of |Percona|'s software is open-source and free. 

