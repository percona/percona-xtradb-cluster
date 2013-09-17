.. _Errata:

====================================
 Percona XtraDB Cluster Known Issues
====================================

Following are issues which may impact you while running PXC:

 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1192834: Crash with index compaction and binary logging enabled.
 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1217426: When empty test directory is present on donor, it is not created on joiner, so when tables are created after SST on donor, the joiner later on will fail with inconsistency.
