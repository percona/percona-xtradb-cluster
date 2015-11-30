.. rn:: 5.6.26-25.12

=======================================
 |Percona XtraDB Cluster| 5.6.26-25.12 
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6.26-25.12 on October 14th 2015. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.26-25.12/>`_ or from our :ref:`software repositories <installation>`.

Based on `Percona Server 5.6.26-74.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.26-74.0.html>`_ including all the bug fixes in it, *Galera Replicator* `3.12 <https://github.com/codership/galera/issues?q=milestone%3A25.3.12>`_ and on *Codership wsrep API* `25.11 <https://github.com/codership/mysql-wsrep/issues?q=milestone%3A5.6.26-25.11>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.26-25.12 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.26-25.12>`_ at Launchpad.

New Features
============

 Variable :variable:`wsrep_dirty_reads` now has Global scope as well. (:bug:`1505609`). 

Bugs Fixed
==========

 Asynchronous replication slave thread would continue to run and execute even if the asynchronous slave was non-primary. Bug fixed :wsrepbug:`188`.

 If a cluster conflict happened while asynchronous replication was running it would cause the following message to be printed repeatedly in the error log: ``conflict state 7 after post commit``. Bug fixed :wsrepbug:`181`.

 If a transaction was Brute-Force aborted, subsequent prepared statement would either cause the client to assert or fail to return a result set. Bug fixed :wsrepbug:`126`.

 Server assertion would happen when the ``Perfomance Schema`` instrumentation from a previous prepared statement was not correctly reset before the next one or if a statement could not be certified because one of the nodes did not respond. Bug fixed :bug:`125`.
