.. rn:: 5.6.36-26.20

===================================
Percona XtraDB Cluster 5.6.36-26.20
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.36-26.20 on June 1, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.36-26.20 is now the current release,
based on the following:

* `Percona Server 5.6.36-82.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.36-82.0.html>`_

* Galera Replication library 3.20

* wsrep API version 26

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.36-26.20 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.36-26.20>`_.

Fixed Bugs
==========

* :jirabug:`PXC-749`: Fixed memory leak when running ``INSERT``
  on a table without primary key defined
  and :variable:`wsrep_certify_nonPK` disabled (set to ``0``).

  .. note:: It is recommended to have primary keys defined on all tables
     for correct write set replication.

* :jirabug:`PXC-813`: Fixed SST script to use UTC time format.

* :jirabug:`PXC-823`: Fixed SST flow to gracefully shut down JOINER node
  if SST fails because DONOR leaves the cluster due to network failure.
  This ensures that the DONOR is then able to recover to synced state
  when network connectivity is restored
  For more information, see :bug:`1684810`.

