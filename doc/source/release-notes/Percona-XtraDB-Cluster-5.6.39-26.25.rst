.. rn:: 5.6.39-26.25

===================================
Percona XtraDB Cluster 5.6.39-26.25
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.39-26.25 on February 26, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.39-26.25 is now the current release,
based on the following:

* `Percona Server 5.6.39 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.39-83.1.html>`_

* Galera/Codership WSREP API Release 5.6.39

* Galera Replication library 3.23

Starting from this release, Percona XtraDB Cluster issue
tracking system was moved from launchpad to
`JIRA <https://jira.percona.com/projects/PXC>`_.
All Percona software is open-source and free.

Fixed Bugs
==========

* :jirabug:`PXC-904`: Replication filters were not working with
  account management statements like ``CREATE USER`` in case of
  galera replication; as a result such commands were blocked by the
  replication filters on async slave nodes but not on galera ones.

* :jirabug:`PXC-2043`: SST script was trying to use ``pv`` (the pipe
  viewer) for :variable:`progress` and :variable:`rlimit` options
  even on nodes with no ``pv`` installed, resulting in SST fail
  instead of just ignoring these options for inappropriate nodes.

* :jirabug:`PXC-911`: When node's own IP address was defined in the
  :variable:`wsrep_cluster_address` variable, the node was receiving
  "no messages seen in" warnings from it's own IP address in the
  info log.

This release also contains fixes for the following CVE issues:
CVE-2018-2562, CVE-2018-2573, CVE-2018-2583, CVE-2018-2590,
CVE-2018-2591, CVE-2018-2612, CVE-2018-2622, CVE-2018-2640,
CVE-2018-2645, CVE-2018-2647, CVE-2018-2665, CVE-2018-2668,
CVE-2018-2696, CVE-2018-2703, CVE-2017-3737.
