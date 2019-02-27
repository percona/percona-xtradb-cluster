.. rn:: 5.6.43-28.32

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of |PXC| |release|
on |date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_ or from our
:ref:`software repositories <installation>`.

This release of |PXC| includes the support of Ubuntu 18.10 (Cosmic
Cuttlefish). |PXC| |release| is now the current release, based on the following:

* `Percona Server 5.6.43-84.3
  <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.43-84.3.html>`_
* Codership WSREP API release 5.6.42
* Codership Galera library 3.25

All Percona software is open-source and free.

Bugs Fixed
================================================================================

- :jirabug:`2388`: In some cases, ``DROP FUNCTION`` with an explicit name was not
  replicated.

.. |release| replace:: 5.6.43-28.32
.. |date| replace:: February 28, 2019
