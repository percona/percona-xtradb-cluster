.. rn:: 5.7.28-31.41.2

================================================================================
|PXC| |release|
================================================================================

:Date: |date|
:Installation: :ref:`install`

Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_ or from our
:ref:`software repositories <install>`.

|PXC| |release| is now the current release, based on the following:

* `Percona Server for MySQL 5.7.28-31
  <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.28-31.html>`_
* Galera/Codership WSREP API Release 5.7.28
* Galera Replication library 3.28

|PXC| |release| requires `PXB 2.4.17 <https://www.percona.com/doc/percona-xtrabackup/2.4/release-notes/2.4/2.4.17.html>`_.

This release fixes security vulnerability *CVE-2020-10997*

Bugs Fixed
================================================================================

- :pxcbug:`3117`: The transition key was hardcoded

.. |release| replace:: 5.7.28-31.41.2
.. |date| replace:: April 13, 2020
