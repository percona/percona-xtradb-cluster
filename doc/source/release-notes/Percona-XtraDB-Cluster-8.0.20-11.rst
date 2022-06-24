.. _PXC-8.0.20-11:

================================================================================
*Percona XtraDB Cluster* 8.0.20-11
================================================================================

:Date: October 1, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_

Percona XtraDB Cluster 8.0.20-11 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.20-11 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.20-11.html>`__ for more details on these changes.

Improvements
================================================================================

* :jirabug:`PXC-2603`: Update Index for PXC status variables - apply consistent definitions



Bugs Fixed
================================================================================

* :jirabug:`PXC-3159`: Modify error handling to close the communication channels and abort the joiner node when donor crashes (previously was Known Issue)
* :jirabug:`PXC-3352`: Modify wsrep_row_upd_check_foreign_constraints() to remove the check for DELETE
* :jirabug:`PXC-3371`: Fix Directory creation in build-binary.sh
* :jirabug:`PXC-3370`: Provide binary tarball with shared libs and glibc suffix & minimal tarballs
* :jirabug:`PXC-3360`: Update sysbench commands in PXC-ProxySQL configuration doc page
* :jirabug:`PXC-3312`: Prevent cleanup of statement diagnostic area in case of transaction replay.
* :jirabug:`PXC-3167`: Correct GCache buffer repossession processing
* :jirabug:`PXC-3347`: Modify PERCONA_SERVER_EXTENSION for bintarball and modify MYSQL_SERVER_SUFFIX



Known Issues (unfixed problems that you should be aware of)
================================================================================

* :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tries to join SSL-enabled cluster
* :jirabug:`PXC-3092`: Log warning at startup if keyring is specified but cluster traffic encryption is turned off
* :jirabug:`PXC-3093`: Garbd logs Completed SST Transfer Incorrectly (Timing is not correct)
