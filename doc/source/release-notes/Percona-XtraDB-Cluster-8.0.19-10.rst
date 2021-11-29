.. _PXC-8.0.19-10:

================================================================================
*Percona XtraDB Cluster* 8.0.19-10
================================================================================

:Date: June 18, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_

Percona XtraDB Cluster 8.0.19-10 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.19-10 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.19-10.html>`__ for more details on these changes.

Improvements
================================================================================

* :jirabug:`PXC-2189`: Modify Reference Architecture for Percona XtraDB Cluster (PXC) to include ProxySQL
* :jirabug:`PXC-3182`: Modify processing to not allow writes on 8.0 nodes while 5.7 nodes are still on the cluster
* :jirabug:`PXC-3187`: Add dependency package installation note in PXC binary tarball installation doc.
* :jirabug:`PXC-3138`: Document mixed cluster write (PXC8 while PXC5.7 nodes are still part of the cluster) should not be completed.
* :jirabug:`PXC-3066`: Document that pxc-encrypt-cluster-traffic=OFF is not just about traffic encryption
* :jirabug:`PXC-2993`: Document the dangers of running with strict mode disabled and Group Replication at the same time
* :jirabug:`PXC-2980`: Modify Documentation to include AutoStart Up Process after Installation
* :jirabug:`PXC-2604`: Modify garbd processing to support Operator



Bugs Fixed
================================================================================

* :jirabug:`PXC-3298`: Correct galera_var_reject_queries test to remove display value width
* :jirabug:`PXC-3320`: Correction on PXC installation doc
* :jirabug:`PXC-3270`: Modify wsrep_ignore_apply_errors variable default to restore 5.x behavior
* :jirabug:`PXC-3179`: Correct replication of CREATE USER ... RANDOM PASSWORD
* :jirabug:`PXC-3080`: Modify to process the ROTATE_LOG_EVENT synchronously to perform proper cleanup
* :jirabug:`PXC-2935`: Remove incorrect assertion when --thread_handling=pool-of-threads is used
* :jirabug:`PXC-2500`: Modify ALTER USER processing when executing thread is Galera applier thread to correct assertion
* :jirabug:`PXC-3234`: Correct documentation link in spec file
* :jirabug:`PXC-3204`: Modify to set wsrep_protocol_version correctly when wsrep_auto_increment_control is disabled
* :jirabug:`PXC-3189`: Correct SST processing for super_read_only
* :jirabug:`PXC-3184`: Modify startup to correct crash when socat not found and SST Fails
* :jirabug:`PXC-3169`: Modify wsrep_reject_queries to enhance error messaging
* :jirabug:`PXC-3165`: Allow COM_FIELD_LIST to be executed when WSREP is not ready
* :jirabug:`PXC-3145`: Modify to end mysqld process when the joiner fails during an SST
* :jirabug:`PXC-3043`: Update required donor version to PXC 5.7.28 (previously was Known Issue)
* :jirabug:`PXC-3036`: Document correct method for starting, stopping, bootstrapping
* :jirabug:`PXC-3287`: Correct link displayed on \help client command
* :jirabug:`PXC-3031`: Modify processing for garbd to prevent issues when multiple requests are started at approximately the same time and request an SST transfers to prevent SST from hanging



Known Issues
================================================================================

* :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tries to join SSL-enabled cluster
* :jirabug:`PXC-3092`: Abort startup if keyring is specified but cluster traffic encryption is turned off
* :jirabug:`PXC-3093`: Garbd logs Completed SST Transfer Incorrectly (Timing is not correct)
* :jirabug:`PXC-3159`: Killing the Donor or Connection lost during SST Process Leaves Joiner Hanging


