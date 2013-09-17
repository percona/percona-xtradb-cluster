.. _Errata:

====================================
 Percona XtraDB Cluster Errata
====================================

Following are issues which may impact you while running PXC:

 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1192834: Crash with index compaction and binary logging enabled after SST.
 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1217426: When empty test directory is present on donor, it is not created on joiner, so when tables are created after SST on donor, the joiner later on will fail with inconsistency.
 - https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1098566: innodb_data_home_dir is not supported. Depends on https://bugs.launchpad.net/percona-xtrabackup/+bug/1164945 for the fix.

Also make sure to check limitations page :ref:`here <limitations>`.
