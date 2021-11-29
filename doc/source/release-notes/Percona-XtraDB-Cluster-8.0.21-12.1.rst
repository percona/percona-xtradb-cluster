.. _PXC-8.0.21-12.1:

================================================================================
*Percona XtraDB Cluster* 8.0.21-12.1
================================================================================

:Date: December 28, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_

Percona XtraDB Cluster 8.0.21-12.1 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.21-12 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.21-12.html>`__ for more details on these changes.

Implement an inconsistency voting policy. In the best case scenario, the node with the inconsistent data is aborted and the cluster continues to operate. 

Improvements
================================================================================

* :jirabug:`PXC-2574`: Improve messaging for "BF lock wait long" timeout



Bugs Fixed
================================================================================

* :jirabug:`PXC-3353`: Modify error handling in Garbd when donor crashes during SST or when an invalid donor name is passed to it
* :jirabug:`PXC-3468`: Resolve package conflict when installing PXC 5.7 on RHEL/CentOS8
* :jirabug:`PXC-3418`: Prevent DDL-DML deadlock by making in-place ALTER take shared MDL for the whole duration.
* :jirabug:`PXC-3416`: Fix memory leaks in garbd when started with invalid group name
* :jirabug:`PXC-3445`: Correct MTR test failures
* :jirabug:`PXC-3442`: Fix crash when log_slave_updates=ON and consistency check statement is executed
* :jirabug:`PXC-3424`: Fix error handling when the donor is not able to serve SST
* :jirabug:`PXC-3404`: Fix memory leak in garbd while processing CC actions
* :jirabug:`PXC-3191`: Modify Read-Only checks on wsrep_* tables when in super_read_only



Known Issues (unfixed problems that you should be aware of)
================================================================================

* :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tries to join an SSL-enabled cluster
* :jirabug:`PXC-3092`: Log a warning at startup if a keyring is specified but cluster traffic encryption is turned off
* :jirabug:`PXC-3093`: Completed SST Transfer incorrectly logged by garbd (Timing is not correct)
* :jirabug:`PXC-3159`: Modify the error handling to close the communication channels and abort the joiner node when the donor crashes
