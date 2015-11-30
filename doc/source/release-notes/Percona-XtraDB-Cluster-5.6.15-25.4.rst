.. rn:: 5.6.15-25.4

======================================
 |Percona XtraDB Cluster| 5.6.15-25.4
======================================

Percona is glad to announce the new release of |Percona XtraDB Cluster| 5.6 on February 20th 2014. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.15-25.4/>`_ or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.6.15-63.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.15-63.0.html>`_ including all the bug fixes in it, `Galera Replicator 3.3 <https://launchpad.net/galera/+milestone/25.3.3>`_ and on `Codership wsrep API 5.6.15-25.2 <https://launchpad.net/codership-mysql/+milestone/5.6.15-25.2>`_ is now the current **General Availability** release. All of |Percona|'s software is open-source and free, all the details of the release can be found in the `5.6.15-25.4 milestone <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.15-25.4>`_ at Launchpad.

Bugs fixed 
==========

 Parallel Applying was not functioning which was evident from the :variable:`wsrep_cert_deps_distance` being ``1.0`` at all times. Bug fixed :bug:`1277703`.

 Binlog events were created for the statements for non-InnoDB tables, but they were never cleaned from transaction cache, which could lead to node crash. Bug fixed :bug:`1277986`.

 |Percona XtraDB Cluster| didn't validate the parameters of :variable:`wsrep_provider_options` when starting it up. Bug fixed :bug:`1260193`.

 ``clustercheck`` script would mark node as down on *Debian* based systems if it was run with default values because it was looking for the ``defaults-extra-file`` in the wrong directory. Bug fixed :bug:`1276076`.

 Deadlock would happen when ``NULL`` unique key was inserted. Workaround has been implemented to support ``NULL`` keys, by using the ``md5`` sum of full row as key value. Bug fixed :bug:`1276424`.

 Variables :variable:`innodb-log-group-home-dir` and :variable:`innodb-data-home-dir` are now handled by default (ie., there is no need to set them up in :option:`sst_special_dirs`). Bug fixed :bug:`1276904`.

 Builds now use system ``Zlib`` instead of bundled one. Bug fixed :bug:`1277928`.

 If transaction size exceeds the :variable:`wsrep_max_ws_size` limit, there will appear a warning message in the error log and replication is skipped. However, the transaction was committed in the master node, and cluster would be in inconsistent state. Bug fixed :bug:`1280557`.

 :variable:`wsrep_load_data_splitting` defaults to ``OFF`` now, using it turned ``ON`` with :variable:`autocommit` set to ``0`` is not recommended.

 Other bugs fixed: :bug:`1279844`.

We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

|Percona XtraDB Cluster| `Errata <http://www.percona.com/doc/percona-xtradb-cluster/5.6/errata.html>`_ can be found in our documentation.
