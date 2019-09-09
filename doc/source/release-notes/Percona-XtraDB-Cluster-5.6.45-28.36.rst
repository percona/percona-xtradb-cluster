.. rn:: 5.6.45-28.36

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of |PXC| |release|
on |date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_ or from our
:ref:`software repositories <installation>`.

|PXC| |release| is now the current release, based on the following:

- `Percona Server 5.6.45
  <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.45-86.1.html>`_
- Codership WSREP API release 5.6.45
- Codership Galera library 3.28

All Percona software is open-source and free.

Bugs Fixed
================================================================================

- :jirabug:`2432`: PXC was not updating the information_schema user/client
  statistics properly.
- :jirabug:`2555`: SST initialization delay: fixed a bug where the SST process
  took too long to detect if a child process was running.

.. |release| replace:: 5.6.45-28.36
.. |date| replace:: September 10, 2019
