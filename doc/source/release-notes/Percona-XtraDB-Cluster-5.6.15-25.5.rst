.. rn:: 5.6.15-25.5

======================================
 |Percona XtraDB Cluster| 5.6.15-25.5
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6 on March 20th 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.15-25.5/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.15-63.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.15-63.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.4 <https://launchpad.net/galera/+milestone/25.3.4>`_ and on `Codership wsrep API 25.5 <https://launchpad.net/codership-mysql/+milestone/5.6.16-25.5>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.15-25.5 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.15-25.5>`_ at Launchpad.

New Features 
============

 wsrep patch did not allow server to start with query cache enabled. This restriction and check have been removed now and query cache can be fully enabled from configuration file.

 New SST options have been implemented: :option:`inno-backup-opts`, :option:`inno-apply-opts`, :option:`inno-move-opts` which pass options to backup, apply and move stages of innobackupex.

 The joiner would wait and not fall back to choosing other potential donor nodes (not listed in :variable:`wsrep_sst_donor`) by their state. This happened even when comma was added at the end. This fixes it for that particular case.

 Initial configurable timeout, of 100 seconds, to receive a first packet via SST has been implemented, so that if donor dies somewhere in between, joiner doesn't hang. Timeout can be configured with the :option:`sst-initial-timeout` variable.

Bugs fixed 
==========

 Replication of partition tables without binlogging enabled failed, partition truncation didn't work because of lack of TO isolation there. Bug fixed :bug:`1219605`.

 Using ``LOAD DATA INFILE`` in with :variable:`autocommit` set to ``0`` and :variable:`wsrep_load_data_splitting` set to ``ON`` could lead to incomplete loading of records while chunking. Bug fixed :bug:`1281810`.

 ``Garbd`` could crash on *CentOS* if variable :variable:`gmcast.listen_addr` wasn't set. Bug fixed :bug:`1283100`.

 Node couldn't be started with :variable:`wsrep_provider_options` option :variable:`debug` set to ``1``. Bug fixed :bug:`1285208`. 

 Boostrapping a Node in a ``NON-PRIMARY`` state would lead to crash. Bug fixed :bug:`1286450`.

 New versions of xtrabackup SST scripts were ignoring ``--socket`` parameter passed by mysqld. Bug fixed :bug:`1289483`.

 Regression in Galera required explicitly setting :variable:`socket.ssl` to ``Yes`` even if you set up variables :variable:`socket.ssl_key` and :variable:`socket.ssl_cert`. Bug fixed :bug:`1290006`.

 Fixed the ``clang`` build issues that were happening during the Galera build. Bug fixed :bug:`1290462`.

 Better diagnostic error message has been implemented when :variable:`wsrep_max_ws_size` limit has been succeeded. Bug fixed :bug:`1280557`.

 Fixed incorrect warnings and implemented better handling of repeated usage with same value for :variable:`wsrep_desync`. Bug fixed :bug:`1281696`.

 Fixed the issue with :variable:`wsrep_slave_threads` wherein if the number of slave threads was changed before closing threads from an earlier change, it could increase the total number of threads beyond value specified in :variable:`wsrep_slave_threads`.

 A regression in mutex handling caused dynamic update of :variable:`wsrep_log_conflicts` to hang the server. Bug fixed :bug:`1293624`.

 Presence of :file:`/tmp/test` directory and an empty test database caused |Percona Xtrabackup| to fail, causing SST to fail. This is an |Percona XtraBackup| issue. But, this has been fixed in PXC's xtrabackup SST separately by using unique temporary directories with |Percona Xtrabackup|. Bug fixed :bug:`1294760`.

 After installing the ``auth_socket`` plugin any local user might get root access to the server. If you're using this plugin upgrade is advised. This is a regression, introduced in |Percona Server| :rn:`5.6.11-60.3`. Bug fixed :bug:`1289599`

Other bug fixes: :bug:`1287098`, :bug:`1289776`, :bug:`1279343`, :bug:`1259649`, :bug:`1292533`, :bug:`1272982`, :bug:`1284670`, and :bug:`1264809`.

We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

