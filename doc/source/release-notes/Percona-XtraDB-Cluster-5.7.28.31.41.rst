.. rn:: 5.7.28-31.41

================================================================================
|PXC| |release|
================================================================================

Percona is happy to announce the release of Percona XtraDB Cluster |release| on
|date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_ or from our
:ref:`software repositories <install>`.

|PXC| |release| is now the current release, based on the following:

* `Percona Server for MySQL 5.7.28-31
  <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.28-31.html>`_
* Galera/Codership WSREP API Release 5.7.28
* Galera Replication library 3.28

Bugs Fixed
================================================================================


- :pxcbug:`2729`: A cluster node could hang when trying to access a table which was being updated by another node.
- :pxcbug:`2704`: After a row was updated with a variable-length unique key, the entire cluster could crash.

.. |release| replace:: 5.7.28-31.41
.. |date| replace:: December 11, 2019
