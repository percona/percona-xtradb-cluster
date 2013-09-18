.. _Errata:

====================================
 Percona XtraDB Cluster Errata
====================================

Following are issues which may impact you while running PXC:

 - bug :bug:`1192834`: Joiner may crash after SST from donor with compaction enabled. Workaround is to disable the index compaction (compact under [xtrabackup]), if enabled. This crash requires specific configuration, hence you may not be affected.
 - bug :bug:`1217426`: When empty test directory is present on donor, it is not created on joiner, so when tables are created after SST on donor, the joiner later on will fail with inconsistency. Workaround is to either drop the test database or populate it with a table before SST.
 - :bug:`1098566`: :variable:`innodb_data_home_dir` is not supported. Depends on bug :bug:`1164945` for the fix.
 - bug :bug:`1112363`: After debian upgrade, certain DDL will be replicated across nodes from the node where the upgrade took place and not work. This is fairly benign and shouldn't disrupt the service. The workaround is to 'touch /etc/mysql/NO-DEBIAN-START'. This also circumvents other debian-specific startup 'tasks' which can disrupt startup.
 - For Debian/Ubuntu users: |Percona XtraDB Cluster| :rn:`5.5.33-23.7.6` includes a new dependency, the ``socat`` package. If the ``socat`` is not previously installed, ``percona-xtradb-cluster-server-5.5`` may be held back. In order to upgrade, you need to either install ``socat`` before running the ``apt-get upgrade`` or by writing the following command: ``apt-get install percona-xtradb-cluster-server-5.5``. For *Ubuntu* users the ``socat`` package is in the universe repository, so the repository will have to be enabled in order to install the package.


Also make sure to check limitations page :ref:`here <limitations>`. You can also review this `milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/future-5.5>`_ for features/bugfixes to be included in future releases (ie. releases after the upcoming/recent release).
