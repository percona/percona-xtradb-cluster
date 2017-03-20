.. rn:: 5.7.17-27.20

===================================
Percona XtraDB Cluster 5.7.17-27.20
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.7.17-27.20 on March 16, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster 5.7.17-27.20 is now the current release,
based on the following:

* `Percona Server 5.7.17-11 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.17-11.html>`_

* Galera Replication library 3.20

* wsrep API version 27

All Percona software is open-source and free.
Details of this release can be found in the
`5.7.17-27.20 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.7.17-27.20>`_.

Fixed Bugs
==========

* :jirabug:`BLD-512`: Fixed startup of ``garbd``
  on Ubuntu 16.04.2 LTS (Xenial Xerus).

* :jirabug:`BLD-519`: Added the ``garbd`` debug package to the repository.

* :jirabug:`BLD-569`: Fixed ``grabd`` script to return non-zero
  if it fails to start.

* :jirabug:`BLD-570`: Fixed service script for ``garbd``
  on Ubuntu 16.04.2 LTS (Xenial Xerus) and Ubuntu 16.10 (Yakkety Yak).

* :jirabug:`BLD-593`: Limited the use of ``rm`` and ``chown``
  by ``mysqld_safe`` to avoid exploits of the CVE-2016-5617 vulnerability.
  For more information, see :bug:`1660265`.

  Credit to Dawid Golunski (https://legalhackers.com).

* :jirabug:`BLD-610`: Added version number to the dependency requirements
  of the full RPM package.

* :jirabug:`BLD-643`: Fixed ``systemctl`` to mark ``mysql`` process
  as inactive after it fails to start and not attempt to start it again.
  For more information, see :bug:`1662292`.

* :jirabug:`BLD-644`: Added the ``which`` package to |PXC| dependencies
  on CentOS 7.
  For more information, see :bug:`1661398`.

* :jirabug:`BLD-645`: Fixed ``mysqld_safe`` to support options
  with a forward slash (``/``).
  For more information, see :bug:`1652838`.

* :jirabug:`BLD-647`: Fixed ``systemctl`` to show correct status
  for ``mysql`` on CentOS 7.
  For more information, see :bug:`1644382`.

