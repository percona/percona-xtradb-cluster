.. _PXC-5.6.47-28.40:

================================================================================
*Percona XtraDB Cluster* 5.6.47-28.40
================================================================================

:Date: May 4, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.6/installation.html>`_

Improvements
================================================================================

* :jirabug:`PXC-2197`: Modified the SST Documentation to Include Package Dependencies for Percona XtraBackup (PXB).
* :jirabug:`PXC-2602`: Added the ability to configure xbstream options with wsrep_sst_xtrabackup.



Bugs Fixed
================================================================================

* :jirabug:`PXC-2954`: DDL to add FK on one node fails but completes on other nodes causing inconsistency
* :jirabug:`PXC-2904`: Ensured the "Percona-XtraDB-Cluster-57" yum package installed the required xtrabackup version.
* :jirabug:`PXC-2684`: Modified error handling to prevent deadlock when stored procedure was aborted.
* :jirabug:`PXC-2912`: Modified the netcat Configuration to Include the -N option, which is required by more recent versions of netcat. The option allows the shutdown of the network socket after the input EOF.


