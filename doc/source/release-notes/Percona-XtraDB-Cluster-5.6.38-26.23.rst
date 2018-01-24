.. rn:: 5.6.38-26.23

===================================
Percona XtraDB Cluster 5.6.38-26.23
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.38-26.23 on January 24, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.38-26.23 is now the current release,
based on the following:

* `Percona Server 5.6.38 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.38-83.0.html>`_

* Galera/Codership WSREP API Release 5.6.38

* Galera Replication library 3.21

All Percona software is open-source and free.



Fixed Bugs
==========

* :jirabug:`PXC-889`: Fixed an issue where a node with an invalid value for
  :variable:`wsrep_provider` was allowed to start up and operate in standalone
  mode, which could lead to data inconsistency. The node will now abort in
  this case. Bug fixed :bug:`1728774`

* Ensured that a node, because of data inconsistency, isolates itself before
  leaving the cluster, thus allowing pending nodes to re-evaluate the quorum.
  Bug fixed :bug:`1704404`

* :jirabug:`PXC-875`: Fixed an issue where toggling :variable:`wsrep_provider`
  off and on failed to reset some internal variables and resulted in PXC
  logging an "Unsupported protocol downgrade" warning. Bug fixed
  :bug:`1379204`

* :jirabug:`PXC-883`: Fixed ``ROLLBACK TO SAVEPOINT`` incorrect operation
  on slaves by avoiding useless wsrep plugin register for a savepoint
  rollback. Bug fixed :bug:`1700593`

* :jirabug:`PXC-887`: gcache .page files were unnecessarily created due to
  an error in projecting gcache free size when configured to recover on
  restart.

* Fixed transaction loss after recovery by
  avoiding interruption of the binlog recovery based on wsrep saved position.
  Bug fixed :bug:`1734113`

* Fixed empty gtid_executed variable after recovering
  the position of a node with ``--wsrep_recover``.

* Fixed certification failure in the case of a node
  restarting at the same time when frequent ``TRUNCATE TABLE`` commands and
  DML writes occur simultaneously on other nodes. Bug fixed :bug:`1737731`

* :jirabug:`PXC-914`: Suppressing DDL/TOI replication in case of
  :variable:`sql_log_bin` zero value didn't work when DDL statement was
  modifying an existing table, resulting in an error.

