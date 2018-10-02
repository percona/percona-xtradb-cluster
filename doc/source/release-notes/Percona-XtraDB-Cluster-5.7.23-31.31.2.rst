.. rn:: 5.7.23-31.31.2

=====================================
|product| |release|
=====================================

Percona is glad to announce the release of
|product| |release| on |release-date|.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

|product| |release| is now the current release,
based on the following:

* `Percona Server 5.7.23-23 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.23-23.html>`_
* Galera Replication library 3.24
* Galera/Codership WSREP API Release 5.7.23

All Percona software is open-source and free.

Fixed Bugs
==========

* :pxcbug:`2254`: A cluster conflict could cause a crash in |product| 5.7.23 if
  `autocommit=off`.

.. |product| replace:: Percona XtraDB Cluster
.. |release| replace:: 5.7.23-31.31.2
.. |release-date| replace:: October 2, 2018
