.. _PXC-8.0.23-14.1:

================================================================================
*Percona XtraDB Cluster* 8.0.23-14.1
================================================================================

:Date: June 9, 2021
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_.

Percona XtraDB Cluster 8.0.23-14.1 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.23-14 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.23-14.html>`__ for more details on these changes.

Improvements
================================================================================

* :jirabug:`PXC-3092`: Log a warning at startup if a keyring is specified, but the cluster traffic encryption is turned off

Bugs Fixed
================================================================================


* :jirabug:`PXC-3464`: Data is not propagated with SET SESSION sql_log_bin = 0

* :jirabug:`PXC-3146`: Galera/SST is not looking for the default data directory location for SSL certs

* :jirabug:`PXC-3226`: Results from CHECK TABLE from PXC server can cause the client libraries to crash

* :jirabug:`PXC-3381`: Modify GTID functions to use a different char set

* :jirabug:`PXC-3437`: Node fails to join in the endless loop

* :jirabug:`PXC-3446`: Memory leak during server shutdown

* :jirabug:`PXC-3538`: Garbd crashes after successful backup

* :jirabug:`PXC-3580`: Aggressive network outages on one node makes the whole cluster unusable

* :jirabug:`PXC-3596`: Node stuck in aborting SST

* :jirabug:`PXC-3645`: Deadlock during ongoing transaction and RSU



