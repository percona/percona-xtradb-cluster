.. _Errata:

====================================
 Percona XtraDB Cluster Errata
====================================

Following are issues which may impact you while running PXC:

 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1192834: Joiner may crash after SST from donor with compaction enabled. Workaround is to disable the index compaction (compact under [xtrabackup]), if enabled. 
 - https://bugs.launchpad.net/percona-xtrabackup/+bug/1217426: When empty test directory is present on donor, it is not created on joiner, so when tables are created after SST on donor, the joiner later on will fail with inconsistency. Workaround is to either drop the test database or populate it with a table before SST.
 - https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1098566: innodb_data_home_dir is not supported. Depends on https://bugs.launchpad.net/percona-xtrabackup/+bug/1164945 for the fix.
 - https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1112363: After debian upgrade, certain DDL will be replicated across nodes from the node where the upgrade took place and not work. This is fairly benign and shouldn't disrupt the service. The workaround is to 'touch /etc/mysql/NO-DEBIAN-START'. This also circumvents other debian-specific startup 'tasks' which can disrupt startup.

Also make sure to check limitations page :ref:`here <limitations>`. You can also review this `milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/future-5.5>`_ for features/bugfixes to be included in future releases (ie. releases after the upcoming/recent release).
