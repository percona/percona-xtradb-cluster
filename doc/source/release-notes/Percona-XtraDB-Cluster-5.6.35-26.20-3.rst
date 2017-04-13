.. rn:: 5.6.35-26.20-3

=====================================
Percona XtraDB Cluster 5.6.35-26.20-3
=====================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.35-26.20-3 on April 13, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.35-26.20-3 is now the current release,
based on the following:

* `Percona Server 5.6.35-81.0 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.35-81.0.html>`_

* Galera Replication library 3.20

* wsrep API version 26

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.35-26.20-3 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.35-26.20-3>`_.

Changes
=======

* As of this release,
  |PXC| no longer supports RedHat 5 derivatives,
  including CentOS 5.

Fixed Bugs
==========

* Updated semantics for gcache page cleanup
  to trigger when either :option:`gcache.keep_pages_size`
  or :option:`gcache.keep_pages_count` exceeds the limit,
  instead of both at the same time.

* Added support for passing the XtraBackup buffer pool size
  with the ``use-memory`` option under ``[xtrabackup]``
  and the ``innodb_buffer_pool_size`` option under ``[mysqld]``
  when the ``--use-memory`` option is not passed
  with the ``inno-apply-opts`` option under ``[sst]``.

* Fixed gcache page cleanup not triggering
  when limits are exceeded.

* :jirabug:`PXC-782`: Updated ``xtrabackup-v2`` script
  to use the :option:`tmpdir` option
  (if it is set under ``[sst]``, ``[xtrabackup]`` or ``[mysqld]``,
  in that order).

* :jirabug:`PXC-784`: Fixed the ``pc.recovery`` procedure to abort
  if the :file:`gvwstate.dat` file is empty or invalid,
  and fall back to normal joining process.
  For more information, see :bug:`1669333`.

* :jirabug:`PXC-794`: Updated the :option:`sockopt` option
  to include a comma at the beginning if it is not set by the user.

* :jirabug:`PXC-797`: Blocked :option:`wsrep_desync` toggling
  while node is paused
  to avoid halting the cluster when running ``FLUSH TABLES WITH READ LOCK``.
  For more information, see :bug:`1370532`.

* Fixed several packaging and dependency issues.

