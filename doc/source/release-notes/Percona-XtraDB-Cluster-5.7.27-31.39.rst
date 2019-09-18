.. rn:: 5.7.27-31.39

================================================================================
|PXC| |release|
================================================================================

Percona is happy to announce the release of Percona XtraDB Cluster |release| on
|date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_ or from our
:ref:`software repositories <install>`.

|PXC| |release| is now the current release, based on the following:

* `Percona Server for MySQL 5.7.27-30
  <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.27-30.html>`_
* Galera/Codership WSREP API Release 5.7.27
* Galera Replication library 3.28

Bugs Fixed
================================================================================

- :pxcbug:`2432`: PXC was not updating the information_schema user/client statistics properly.
- :pxcbug:`2555`: SST initialization delay: fixed a bug where the SST
  process took too long to detect if a child process was running.
- :pxcbug:`2557`: Fixed a crash when a node goes NON-PRIMARY and SHOW STATUS is executed.
- :pxcbug:`2592`: PXC restarting automatically on data inconsistency.
- :pxcbug:`2605`: PXC could crash when log_slow_verbosity included InnoDB.  Fixed upstream PS-5820.
- :pxcbug:`2639`: Fixed an issue where a SQL admin command (like OPTIMIZE) could cause a deadlock.

.. |release| replace:: 5.7.27-31.39
.. |date| replace:: September 18, 2019
