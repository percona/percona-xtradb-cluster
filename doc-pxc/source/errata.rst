.. _Errata:

====================================
 Percona XtraDB Cluster Errata
====================================

Known Issues
-------------

Following are issues which may impact you while running PXC:
 - If the error log files are created inside datadir without the default '.err' extension (as set in my.cnf), they may be removed during SST. The workaround is to define the error log in my.cnf to be either outside datadir or with '.err' extension if inside. The default log is `hostname`.err, so it should be fine if not set in my.cnf.
 - bug :bug:`1226185`: percona-xtrabackup-20 may get installed as a dependency instead of latest percona-xtrabackup during a fresh install due to certain yum issues. Workaround is documented here - https://bugs.launchpad.net/percona-xtradb-cluster/+bug/1226185/comments/2.
 - bug :bug:`1192834`: Joiner may crash after SST from donor with compaction enabled. Workaround is to disable the index compaction (compact under [xtrabackup]), if enabled. This crash requires specific configuration, hence you may not be affected.
 - bug :bug:`1217426`: When empty test directory is present on donor, it is not created on joiner, so when tables are created after SST on donor, the joiner later on will fail with inconsistency. Workaround is to either drop the test database or populate it with a table before SST. This is currently a limitation of Xtrabackup itself, hence, needs to be fixed there.
 - bug :bug:`1098566`: :variable:`innodb_data_home_dir` is not supported. Depends on bug :bug:`1164945` for the fix.
 - bug :bug:`1112363`: After debian upgrade, certain DDL will be replicated across nodes from the node where the upgrade took place and not work. This is fairly benign and shouldn't disrupt the service. The workaround is to 'touch /etc/mysql/NO-DEBIAN-START'. This also circumvents other debian-specific startup 'tasks' which can disrupt startup.
 - For Debian/Ubuntu users: |Percona XtraDB Cluster| :rn:`5.5.33-23.7.6` includes a new dependency, the ``socat`` package. If the ``socat`` is not previously installed, ``percona-xtradb-cluster-server-5.5`` may be held back. In order to upgrade, you need to either install ``socat`` before running the ``apt-get upgrade`` or by writing the following command: ``apt-get install percona-xtradb-cluster-server-5.5``. For *Ubuntu* users the ``socat`` package is in the universe repository, so the repository will have to be enabled in order to install the package.


Also make sure to check limitations page :ref:`here <limitations>`. You can also review this `milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/future-5.5>`_ for features/bugfixes to be included in future releases (ie. releases after the upcoming/recent release).


New Behavior
-------------

These may seem like bugs but they are not:

 - A startup after unclean stop will result in startup failing with '... lock file ...' exists. This is due to a stale lock file in /var/lock/subsys created by earlier startup (which failed). To 'fix' this just remove the lock file and start. This is due to a side-effect of the fix of bug :bug:`1211505`. This is intended to be the ideal behaviour since earlier the startup (even for PS/MySQL) didn't care for the stale lock file but instead just retouched it, giving the illusion of a clean stop earlier.


Incompatibilities
-------------------

Following are incompatibilities between PXC 5.5.33 and older versions:
 - wsrep_sst_donor now supports comma separated list of nodes as a consequence of bug :bug:`1129512`. This only affects if you use this option as a list. This is not compatible with nodes running older PXC, hence **entire** cluster will be affected as far as SST is concerned, since donor nodes won't recognise the request from joiner nodes if former are not upgraded. Minimal workaround is to upgrade Galera package or to not use this new feature (wsrep_sst_donor with single node can still be used). However, upgrading the full PXC is strongly recommended, however, just upgrading PXC galera package will do for this.
 - Due to bug :bug:`1222122` Xtrabackup SST works differently, hence won't be compatible with older Xtrabackup SST found in older PXC versions. Hence it is strongly recommended to upgrade the donor/joiner if other node is upgraded already before SST. Workarounds:

  * Use the rsync SST method and use Xtrabackup SST after upgrades are done, if the need arises later on.
  * Upgrade after the SST is done.
  * Copy the newer wsrep_sst_xtrabackup and wsrep_sst_common (from newer package) to the donor node (when joiner is upgraded to 5.5.33) and vice versa (when donor is upgrade, copy to joiner). Take care of the dependencies required - socat and xbstream being the default one. Refer to this `document <http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html>`_ for more details. This is mostly for 
    contingency situations, the first two should be tried as much as possible.
