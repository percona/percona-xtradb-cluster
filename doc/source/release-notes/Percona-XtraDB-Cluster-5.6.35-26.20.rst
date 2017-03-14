.. rn:: 5.6.35-26.20

===================================
Percona XtraDB Cluster 5.6.35-26.20
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.35-26.20 on March 10, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.35-26.20 is now the current release,
based on the following:

* `Percona Server 5.6.35-80.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.35-80.0.html>`_

* Galera Replication library 3.20

* wsrep API version 26

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.35-26.20 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.35-26.20>`_.

Fixed Bugs
==========

* :jirabug:`BLD-593`: Limited the use of ``rm`` and ``chown``
  by ``mysqld_safe`` to avoid exploits of the CVE-2016-5617 vulnerability.
  For more information, see :bug:`1660265`.

  Credit to Dawid Golunski (https://legalhackers.com).

* :jirabug:`BLD-610`: Added version number to the dependency requirements
  of the full RPM package.

* :jirabug:`BLD-645`: Fixed ``mysqld_safe`` to support options
  with a forward slash (``/``).
  For more information, see :bug:`1652838`.

