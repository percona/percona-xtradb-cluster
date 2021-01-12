.. _PXC-5.7.31-31.45:

================================================================================
*Percona XtraDB Cluster* 5.7.31-31.45
================================================================================

:Date: September 24, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.7/install/index.html>`_

Improvements
================================================================================

* :jirabug:`PXC-2187`: Enhance SST documentation to include a warning about the use of command-line parameters



Bugs Fixed
================================================================================

* :jirabug:`PXC-3352`: Modify wsrep_row_upd_check_foreign_constraints() to remove the check for DELETE
* :jirabug:`PXC-3243`: Modify the BF-abort process to propagate and abort and retry the Stored Procedure instead of the statement
* :jirabug:`PXC-3371`: Fix Directory creation in build-binary.sh
* :jirabug:`PXC-3370`: Provide binary tarball with shared libs and glibc suffix & minimal tarballs
* :jirabug:`PXC-3281`: Modify config to add default socket location


