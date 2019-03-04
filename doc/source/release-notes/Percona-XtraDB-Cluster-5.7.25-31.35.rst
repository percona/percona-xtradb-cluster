.. rn:: 5.7.25-31.35

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of Percona XtraDB Cluster |release| on
|date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_ or from our
:ref:`software repositories <install>`.

This release of |PXC| includes the support of Ubuntu 18.10 (Cosmic Cuttlefish).
|PXC| |release| is now the current release, based on the following:

* `Percona Server for MySQL 5.7.25 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.25-28.html>`_
* Galera Replication library 3.25
* Galera/Codership WSREP API Release 5.7.24


Bugs Fixed
================================================================================

- :pxcbug:`2346`: ``mysqld`` could crash when executing ``mysqldump
  --single-transaction`` while the binary log is disabled. This problem was also
  reported in :pxcbug:`1711`, :pxcbug:`2371`, :pxcbug:`2419`.
- :pxcbug:`2388`: In some cases, ``DROP FUNCTION`` with an explicit name was not
  replicated.

Other bugs fixed: :pxcbug:`1711`, :pxcbug:`2371`, :pxcbug:`2419`

.. |release| replace:: 5.7.25-31.35
.. |date| replace:: February 28, 2019
