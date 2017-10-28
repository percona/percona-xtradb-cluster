.. rn:: 5.6.37-26.21-3

=====================================
Percona XtraDB Cluster 5.6.37-26.21-3
=====================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.37-26.21-3 on October 27, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.37-26.21-3 is now the current release,
based on the following:

* `Percona Server 5.6.37-82.2 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.37-82.2.html>`_

* Galera Replication library 3.21

* wsrep API version 26

Fixed Bugs
==========

* Added access checks for DDL commands
  to make sure they do not get replicated
  if they failed without proper permissions.
  Previously, when a user tried to perform certain DDL actions
  that failed locally due to lack of privileges,
  the command could still be replicated to other nodes,
  because access checks were performed after replication.

  This vulnerability is identified as CVE-2017-15365.
