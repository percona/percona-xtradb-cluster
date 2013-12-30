.. _Errata:

==============================================
 Percona XtraDB Cluster Errata (as of 5.5.34)
==============================================

Known Issues
-------------

Following are issues which may impact you while running PXC:
 - Create Table As Select (CTAS) can cause deadlocks and server hang when used with explicit TEMPORARY tables.
 - bug :bug:`1192834`: Joiner may crash after SST from donor with compaction enabled. Workaround is to disable the index compaction (compact under [xtrabackup]), if enabled. This crash requires specific configuration, hence you may not be affected. Also, this doesn't require any fix from PXC, but Xtrabackup with the fix included should do.
 - For Debian/Ubuntu users: |Percona XtraDB Cluster| :rn:`5.5.33-23.7.6` onwards has a new dependency, the ``socat`` package. If the ``socat`` is not previously installed, ``percona-xtradb-cluster-server-5.5`` may be held back. In order to upgrade, you need to either install ``socat`` before running the ``apt-get upgrade`` or with the following command: ``apt-get install percona-xtradb-cluster-server-5.5``. For *Ubuntu* users the ``socat`` package is in the universe repository, so the repository will have to be enabled in order to install the package.
 - For fresh installs, if you skip the galera package on yum install command-line, yum may end up installing either of Percona-XtraDB-Cluster-galera-2 or 3. Hence, it is imperative to provide Percona-XtraDB-Cluster-galera-3 for PXC56 and Percona-XtraDB-Cluster-galera-2 for PXC55 on yum install command-line. Similarly for apt-get. Upgrades are not affected. This is not a bug but a situation due to existence of multiple providers. Please refer to installation guides of yum and apt for more details.
 


Also make sure to check limitations page :ref:`here <limitations>`. You can also review this `milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/future-5.5>`_ for features/bugfixes to be included in future releases (ie. releases after the upcoming/recent release).

Major changes
--------------- 

Xtrabackup SST method compatibility notes:
 - **5.5.33** introduced an enhanced Xtrabackup method that broke backward compatibility of SST with older versions of PXC.  This is fixed in **5.5.34** by renaming the new method ``xtrabackup-v2``, with default still being ``xtrabackup`` for backward compatibility.  The following should be considered when upgrading:

    - If you are upgrading from **5.5.32** and lower:  **5.5.34** Xtrabackup SST is backwards compatible.   It is recommended you skip **5.5.33** when upgrading.
    - If you are upgrading from **5.5.33**: When upgrading, change your wsrep_sst_method=xtrabackup-v2 before restarting the **5.5.34** and up nodes

    Once you are on **5.5.34** and up, you can migrate to the new method by changing your wsrep_method on all your nodes.  Note that the SST method of the Joiner is what is used when SST is needed, but the Donor must support the same method.

 - Since, newer features, henceforth, are added only to xtrabackup-v2, it is recommended to use xtrabackup-v2 (to use new features) and use xtrabackup only when older (older than **5.5.33**) nodes are present. Also, note that, xtrabackup should support all the major options `here <http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html>`_, any v2 only options will be noted specifically (but older versions of xtrabackup still won't support newer features like encryption, compression, hence wsrep_sst_method=xtrabackup is recommended only for basic SST and during upgrades to maintain compatibility).
 - So, if you are doing SST between a PXC **5.5.33** and PXC **5.5.34** node, due the above change, SST can fail, the workaround for this is to ``ln -s /usr/bin/wsrep_sst_xtrabackup /usr/bin/wsrep_sst_xtrabackup-v2`` on the donor node if it is 33 (and joiner is 34),  ``ln -sf /usr/bin/wsrep_sst_xtrabackup-v2 /usr/bin/wsrep_sst_xtrabackup`` on the donor node if it is 34 (and joiner is 33). But this is not recommended, especially the latter (strong discouraged). Hence, all nodes with PXC **5.5.33** are strongly recommended to upgrade to PXC **5.5.34** if SST backward compatibility is a concern.

Incompatibilities
-------------------

Following are incompatibilities between PXC 5.5.33 (and higher) and older versions:
 - wsrep_sst_donor now supports comma separated list of nodes as a consequence of bug :bug:`1129512`. This only affects if you use this option as a list. This is not compatible with nodes running older PXC, hence **entire** cluster will be affected as far as SST is concerned, since donor nodes won't recognise the request from joiner nodes if former are not upgraded. Minimal workaround is to upgrade Galera package or to not use this new feature (wsrep_sst_donor with single node can still be used). However, upgrading the full PXC is strongly recommended, however, just upgrading PXC galera package will do for this.
