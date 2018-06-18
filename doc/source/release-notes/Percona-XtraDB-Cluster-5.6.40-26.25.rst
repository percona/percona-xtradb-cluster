.. rn:: 5.6.40-26.25

===================================
Percona XtraDB Cluster 5.6.40-26.25
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.40-26.25 on June 20, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.40-26.25 is now the current release,
based on the following:

* `Percona Server 5.6.40 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.40-84.0.html>`_

* Galera/Codership WSREP API Release 5.6.39

* Galera Replication library 3.23

All Percona software is open-source and free.

New feature
===========

* :jirabug:`PXC-907`: New variable :variable:`wsrep_RSU_commit_timeout` allows
  to configure RSU wait for active commit connection timeout (in microseconds).

Fixed Bugs
==========

* :jirabug:`PXC-2128`: Duplicated auto-increment values were set for the
  concurrent sessions on cluster reconfiguration due to the erroneous
  readjustment.

* :jirabug:`PXC-2059`: Error message about the necessity of the ``SUPER``
  privilege appearing in case of the ``CREATE TRIGGER`` statements fail due to
  enabled WSREP was made more clear.

* :jirabug:`PXC-2091`: Check for the maximum number of rows, that can be
  replicated as a part of a single transaction because of the Galera limit, was
  enforced even when replication was disabled with ``wsrep_on=OFF``.

* :jirabug:`PXC-2103`: Interruption of the local running transaction in a
  ``COMMIT`` state by a replicated background transaction while waiting for the
  binlog backup protection caused the commit fail and, eventually, an assert in
  Galera.

* :jirabug:`PXC-2130`: |PXC| failed to build with Python 3.
