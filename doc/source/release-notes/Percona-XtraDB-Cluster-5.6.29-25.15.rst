.. rn:: 5.6.29-25.15

===================================
Percona XtraDB Cluster 5.6.29-25.15 
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.29-25.15 on May 20, 2016.
Binaries are available from the
`downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.29-25.15/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.29-25.15 is now the current release,
based on the following:

* `Percona Server 5.6.29-76.2 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.29-76.2.html>`_

* `Galera Replicator 3.15 <https://github.com/codership/galera/issues?q=milestone%3A25.3.15>`_

* `Codership wsrep API 25.15 <https://github.com/codership/mysql-wsrep/issues?q=milestone%3A5.6.29-25.15>`_

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.29-25.15 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.29-25.15>`_.

For more information about relevant Codership releases, see `this announcement <http://galeracluster.com/2016/03/announcing-galera-cluster-5-5-48-and-5-6-29-with-galera-3-15/>`_.

Bugs Fixed
==========

* Node eviction in the middle of SST now causes the node
  to shut down properly.

* After an error during node startup,
  the state is now marked *unsafe* only if SST is required.

* Fixed data inconsistency during multi-insert auto-increment
  workload on async master with ``binlog-format=STATEMENT``
  when a node begins async slave with ``wsrep_auto_increment_control=ON``.

* Fixed crash when a prepare statement is aborted
  (due to conflict with applier) and then replayed.

* Removed a special case condition in ``wsrep_recover()``
  that whould not happen under normal conditions.

* Percona XtraDB Cluster no longer fails during SST,
  if a node reserves a very large amount of memory for InnoDB buffer pool.

* If the value of ``wsrep_cluster_address`` is not valid,
  trying to create a slave thread will now generate a warning
  instead of an error, and the thread will not be created.

* Fixed error with loading data infile (LDI)
  into a multi-partitioned table.

* The ``wsrep_node_name`` variable now defaults to host name.

* Starting ``mysqld`` with unknown option now fails
  with a clear error message, instead of randomly crashing.

* Optimized the operation of SST and IST
  when a node fails during startup.

* The ``wsrep_desync`` variable can now be enabled only
  after a node is synced with cluster.
  That is, it cannot be set during node bootup configuration).

* Fixed crash when setting a high flow control limit
  (``fc_limit``) and the recv queue fills up.

* Only the default 16 KB page size (``innodb_page_size=16384``)
  is accepted until the relevant upstream bug is fixed by Codership
  (see https://github.com/codership/galera/issues/398).
  All other sizes will report ``Invalid page size`` and shut down
  (the server will not start up).

* If a node is executing RSU/FTWRL,
  explicit desync of the node will not happen
  until the implicit desync action is complete.

* Fixed multiple bugs in test suite to improve quality assurance.

