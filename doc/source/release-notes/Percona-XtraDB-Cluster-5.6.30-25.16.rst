.. rn:: 5.6.30-25.16

===================================
Percona XtraDB Cluster 5.6.30-25.16 
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.30-25.16 on June 10, 2016.
Binaries are available from the
`downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.30-25.16/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.30-25.16 is now the current release,
based on the following:

* `Percona Server 5.6.30-76.3 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.30-76.3.html>`_

* Galera Replication library 3.16

* wsrep API version 25

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.30-25.16 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.30-25.16>`_.

For more information about relevant Codership releases, see `this announcement <http://galeracluster.com/2016/05/announcing-galera-cluster-5-5-49-and-5-6-30-with-galera-3-16/>`_.

New Features
============

* PXC now uses ``wsrep_desync_count`` introduced in Galera 3.16 by Codership,
  instead of the concept that was previously implemented by Percona.
  The following logic applies:

  * If a node is explicitly desynced,
    then implicitly desyncing a node using RSU/FTWRL is allowed.
  * If a node is implicitly desynced using RSU/FTWRL,
    then explicitly desyncing a node is blocked
    until implicit desync is complete.
  * If a node is explicitly desynced
    and then implicitly desycned using RSU/FTWRL,
    then any request for another implicit desync is blocked
    until the former implicit desync is complete.

Fixed Bugs
==========

* Changing ``wsrep_provider`` while node is paused or desynced is not allowed.

* TOI now checks that a node is ready to process DDL and DML
  before starting execution, to prevent node from crashing
  if it becomes non-primary.

* The ``wsrep_row_upd_check_foreign_constraints`` function now checks
  that ``fk-reference-table`` is open before marking it open.

