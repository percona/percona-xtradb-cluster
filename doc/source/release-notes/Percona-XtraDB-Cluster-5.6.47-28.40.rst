.. _PXC-5.6.47-28.40:

================================================================================
*Percona XtraDB Cluster* 5.6.47-28.40
================================================================================

:Date: May 4, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.6/installation.html>`_

Improvements
================================================================================

* :jirabug:`PXC-2197`: Modified SST Documentation to Include Package Dependencies for Percona XtraBackup (PXB)
* :jirabug:`PXC-2602`: Added Ability to Configure xbstream options with wsrep_sst_xtrabackup



Bugs Fixed
================================================================================

* :jirabug:`PXC-2684`: Modified Error Handling to Prevent Deadlock when Stored Procedure Aborted
* :jirabug:`PXC-2904`: Ensured the PXC yum package installed the required xtrabackup version.
* :jirabug:`PXC-2912`: Modified netcat Configuration to Include -N Flag on Donor
