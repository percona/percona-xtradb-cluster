.. rn:: 5.5.37-25.10

=======================================
 |Percona XtraDB Cluster| 5.5.37-25.10
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on May 8th, 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.37-25.10/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.5.37-35.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.37-35.0.html>`_ including all the bug fixes in it, Galera Replicator 2.10 (including fixes in `2.9 <https://launchpad.net/galera/+milestone/25.2.9>`_ and `2.10 <https://launchpad.net/galera/+milestone/25.2.10>`_ milestones) and on wsrep API 25.10 (including fixes in `25.10 <`https://launchpad.net/codership-mysql/+milestone/5.5.37-25.10>`_), |Percona XtraDB Cluster| `5.5.37-25.10 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.37-25.10>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.


New Features
============

 |Percona XtraDB Cluster| now supports stream compression/decompression with new xtrabackup-sst :option:`compressor/decompressor` options.

 ``Garbd`` init script and configuration files have been packaged for *CentOS* and *Debian*, in addition, in *Debian* garbd is packaged separately in ``percona-xtradb-cluster-garbd-2.x`` package.

 New meta packages are now available in order to make the :ref:`Percona XtraDB Cluster installation <installation>` easier.

 *Debian*/*Ubuntu* debug packages are now available for Galera and ``garbd``.

 New SST options have been implemented: :option:`inno-backup-opts`, :option:`inno-apply-opts`, :option:`inno-move-opts` which pass options to backup, apply and move stages of innobackupex.
 
 Initial configurable timeout, of 100 seconds, to receive a first packet via SST has been implemented, so that if donor dies somewhere in between, joiner doesn't hang. Timeout can be configured with the :option:`sst-initial-timeout` variable.

 The joiner would wait and not fall back to choosing other potential donor nodes (not listed in :variable:`wsrep_sst_donor`) by their state. This happened even when comma was added at the end. This fixes it for that particular case.

 Support for Query Cache has been implemented. 

Bugs fixed 
==========

 To avoid disabling Parallel Apply in case of ``SAVEPOINT`` or ``ROLLBACK TO SAVEPOINT`` the :variable:`wsrep_cert_deps_distance` being ``1.0`` at all times. Bug fixed :bug:`1277703`.

 First connection would hang after changing the :variable:`wsrep_cluster_address` variable. Bug fixed :bug:`1022250`.

 When :variable:`gmcast.listen_addr` was set manually it did not allow nodes own address in gcomm address list. Bug fixed :bug:`1099478`.
 
 :variable:`wsrep_sst_rsync` would silently fail on joiner when ``rsync`` server port was already taken. Bug fixed :bug:`1099783`.

 Example ``wsrep_notify`` script failed on node shutdown. Bug fixed :bug:`1132955`.

 Server would segfault on ``INSERT DELAYED`` with :variable:`wsrep_replicate_myisam` set to ``1`` due to unchecked dereference of ``NULL`` pointer. Bug fixed :bug:`1165958`.

 When :file:`grastate.dat` file was not getting zeroed appropriately it would lead to RBR error during the IST. Bug fixed :bug:`1180791`.

 ``gcomm`` exception in PC ``validate_state_msgs()`` during cluster partitioning/re-merges" has been fixed. Bug fixed :bug:`1182367`.

 Due to the ``Provides:`` line in |Percona XtraDB Cluster| (which provides ``Percona-Server-server``), the command ``yum install Percona-Server-server`` would install |Percona XtraDB Cluster| instead of the expected |Percona Server|. Bug fixed :bug:`1201499`.

 Replication of partition tables without binlogging enabled failed, partition truncation didn't work because of lack of TO isolation there. Bug fixed :bug:`1219605`.

 Exception during group merge after partitioning event has been fixed. Bug fixed :bug:`1232747`.

 Default value for :variable:`binlog_format` is now ``ROW``. This is done so that |Percona XtraDB Cluster| is not started with wrong defaults leading to non-deterministic outcomes like crash. Bug fixed :bug:`1243228`.

 ``CREATE TABLE AS SELECT`` was not replicated, if the select result set was empty. Bug fixed :bug:`1246921`.
 
 ``INSERT`` would return deadlock instead of duplicate key on secondary unique key collision. Bug fixed :bug:`1255147`.

 Joiner node would not initialize storage engines if ``rsync`` was used for SST and the first view was non-primary. Bug fixed :bug:`1257341`.

 Table level lock conflict resolving was releasing the wrong lock. Bug fixed :bug:`1257678`.

 Resolved the ``perl`` dependencies needed for |Percona XtraDB Cluster| 5.6. Bug fixed :bug:`1258563`.

 Obsolete dependencies have been removed from |Percona XtraDB Cluster|. Bug fixed :bug:`1259256`.
 
 GCache file allocation could fail if file size was a multiple of page size. Bug fixed :bug:`1259952`.

 |Percona XtraDB Cluster| didn't validate the parameters of :variable:`wsrep_provider_options` when starting it up. Bug fixed :bug:`1260193`.

 Fixed rsync SST for compatibility with ``rsync`` version 3.1.0. Bug fixed :bug:`1261673`.

 During the installation of ``percona-xtradb-cluster-garbd-3.x`` package, *Debian* tries to start it, but as the configuration is not set, it would fail to start and leave the installation in ``iF`` state. Bug fixed :bug:`1262171`. 

 Runtime checks have been added for dynamic variables which are Galera incompatible. Bug fixed :bug:`1262188`.
 
 Node would get stuck and required restart if ``DDL`` was performed after ``FLUSH TABLES WITH READ LOCK``. Bug fixed :bug:`1265656`.

 :ref:`xtrabackup-v2 <xtrabackup_sst>` is now used as default |SST| method in :variable:`wsrep_sst_method`. Bug fixed :bug:`1268837`.

 ``FLUSH TABLES WITH READ LOCK`` behavior on the same connection was changed to conform to |MySQL| behavior. Bug fixed :bug:`1269085`.

 Read-only detection has been added in clustercheck, which can be helpful during major upgrades (this is used by ``xinetd`` for HAProxy etc.) Bug fixed :bug:`1269469`.

 Binary log directory is now being cleanup as part of the :ref:`XtraBackup SST <xtrabackup_sst>`. Bug fixed :bug:`1273368`.

 ``clustercheck`` script would mark node as down on *Debian* based systems if it was run with default values because it was looking for the ``defaults-extra-file`` in the wrong directory. Bug fixed :bug:`1276076`.

 Deadlock would happen when ``NULL`` unique key was inserted. Workaround has been implemented to support ``NULL`` keys, by using the ``md5`` sum of full row as key value. Bug fixed :bug:`1276424`.

 Variables :variable:`innodb-log-group-home-dir` and :variable:`innodb-data-home-dir` are now handled by default (ie., there is no need to set them up in :option:`sst_special_dirs`). Bug fixed :bug:`1276904`.

 Builds now use system ``Zlib`` instead of bundled one. Bug fixed :bug:`1277928`.

 Binlog events were created for the statements for non-InnoDB tables, but they were never cleaned from transaction cache, which could lead to node crash. Bug fixed :bug:`1277986`.

 Galera2 is now installed in ``/usr/lib/galera2/libgalera_smm.so`` with a  compatibility symlink to ``/usr/lib/libgalera_smm.so``. Bug fixed :bug:`1279328`.

 If transaction size exceeds the :variable:`wsrep_max_ws_size` limit, there will appear a warning message in the error log and replication is skipped. However, the transaction was committed in the master node, and cluster would be in inconsistent state. Bug fixed :bug:`1280557`.

 Updating an unique key value could cause server to hang if slave node has enabled parallel slaves. Bug fixed :bug:`1280896`.

 Fixed incorrect warnings and implemented better handling of repeated usage with same value for :variable:`wsrep_desync`. Bug fixed :bug:`1281696`.

 Using ``LOAD DATA INFILE`` in with :variable:`autocommit` set to ``0`` and :variable:`wsrep_load_data_splitting` set to ``ON`` could lead to incomplete loading of records while chunking. Bug fixed :bug:`1281810`.

 ``Garbd`` could crash on *CentOS* if variable :variable:`gmcast.listen_addr` wasn't set. Bug fixed :bug:`1283100`.

 Node couldn't be started with :variable:`wsrep_provider_options` option :variable:`debug` set to ``1``. Bug fixed :bug:`1285208`. 

 Boostrapping  with :variable:`pc.bootstrap=1` at runtime in a ``NON-PRIMARY`` state would lead to crash. Bug fixed :bug:`1286450`.

 Asynchronous replication slave thread is stopped when the node tries to apply next replication event while the node is in non-primary state. But it would then remain stopped after node successfully re-joined the cluster. Bug fixed :bug:`1288479`.

 New versions of xtrabackup SST scripts were ignoring ``--socket`` parameter passed by mysqld. Bug fixed :bug:`1289483`.

 Regression in Galera required explicitly setting :variable:`socket.ssl` to ``Yes`` even if you set up variables :variable:`socket.ssl_key` and :variable:`socket.ssl_cert`. Bug fixed :bug:`1290006`.

 The mysql-debug ``UNIV_DEBUG`` binary was missing from RPM/DEB server packages. Bug fixed :bug:`1290087`.

 XtraBackup SST would fail if `progress <www.percona.com/doc/percona-xtradb-cluster/5.5/manual/xtrabackup_sst.html#progress>`_ option was used with large number of files. Bug fixed :bug:`1294431`.

 When Query Cache was used and a node would go into non-PRIM state, queries which returned results earlier (and cached into query cache) would still return results whereas newer queries (or the ones not cached) would return ``unknown command``. Bug fixed :bug:`1296403`.

 Brute Force abort did not work with INSERTs to table with single unique key. Bug fixed :bug:`1299116`.

 Compiling on *FreeBSD* 10.0 with ``CLANG`` would result in fatal error. Bug fixed :bug:`1309507`.

 ``Use-after-free`` memory corruption in ``one_thread_per_connection_end`` has been fixed. Bug fixed :bug:`1310875`.

 Xtrabackup wouldn't copy empty ``test`` database during the SST. Bug fixed :bug:`1231088`.
 
 Xtrabackup SST failed when ``/tmp/test`` directory existed, fixed by creating unique directories in SST script. Bug fixed :bug:`1294760`.
 
 Deadlock during server shutdown has been prevented. Bug fixed :bug:`1284670`.

 Race condition during update of :variable:`wsrep_slave_threads` has been avoided. Bug fixed :bug:`1290612`.

 Added new option to the init script to restart a node bootstrapped. Bug fixed :bug:`1291024`.

 |Percona XtraDB Cluster| server package no longer conflicts with ``mysql-libs`` package from CentOS repository. Bug fixed :bug:`1278516`.
 
 Server abort due to BF-BF lock conflict in ``lock0lock.c`` has been fixed. Bug fixed :bug:`1264809`.

 Crash due to segfault in ``gcache::RingBuffer::get_new_buffer()`` has been fixed. Bug fixed :bug:`1152565`.

 Applier would fail with ``lock wait timeout exceeded`` on rsync SST donor. Bug fixed :bug:`1255501`.

 rsync SST could hang due to missing ``lsof``. Bug fixed :bug:`1313293`.

Other bugs fixed: :bug:`1273101`, :bug:`1297822`, :bug:`1289776`, :bug:`1279844`, :bug:`1279343`, :bug:`1278516`, :bug:`1278417`, :bug:`1277263`, :bug:`1276994`, :bug:`1272961`, :bug:`1272723`, :bug:`1269811`, :bug:`1269351`, :bug:`1269063`, :bug:`1262887`, :bug:`1262716`, :bug:`1261996`, :bug:`1261833`, :bug:`1261138`, :bug:`1269063`, :bug:`1256887`, :bug:`1256769`, :bug:`1255501`, and :bug:`1254633`.


|Percona XtraDB Cluster| `Errata <http://www.percona.com/doc/percona-xtradb-cluster/errata.html>`_ can be found in our documentation.
