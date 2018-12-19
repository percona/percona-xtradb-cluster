.. rn:: 5.6.42-28.30

===================================
Percona XtraDB Cluster 5.6.42-28.30
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.42-28.30 on January 4, 2019.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.42-28.30 is now the current release,
based on the following:

* `Percona Server 5.6.42 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.42-84.2.html>`_

* Codership WSREP API release 5.6.42

* Codership Galera library 3.25

All Percona software is open-source and free.

Fixed Bugs
==========


* :jirabug:`PXC-2281`: Debug symbols were missing in Debian dbg packages.

* :jirabug:`PXC-2220`: Starting two instances of |PXC| on the same node could
  cause writing transactions to a page store instead of a galera.cache ring
  buffer, resulting in huge memory consumption because of retaining already
  applied write-sets.

* :jirabug:`PXC-2230`: :variable:`gcs.fc_limit=0` not allowed as dynamic
  setting to avoid generating flow control on every message was still possible
  in ``my.cnf`` due to the inconsistent check.

* :jirabug:`PXC-2238`: setting :variable:`read_only=1` caused race
  condition.
