.. rn:: 5.6.46-28.38

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of |PXC| |release|
on |date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_ or from our
:ref:`software repositories <installation>`.

|PXC| |release| is now the current release, based on the following:

- `Percona Server 5.6.46
  <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.46-86.2.html>`_
- Codership WSREP API release 5.6.46
- Codership Galera library 3.28

All Percona software is open-source and free.

Bugs Fixed
================================================================================

- :jirabug:`2729`: A cluster node could hang when trying to access a table which
  was being updated by another node.
- :jirabug:`2704`: After a row was updated with a variable-length unique key,
  the entire cluster could crash.

.. |release| replace:: 5.6.46-28.38
.. |date| replace:: December 4, 2019
