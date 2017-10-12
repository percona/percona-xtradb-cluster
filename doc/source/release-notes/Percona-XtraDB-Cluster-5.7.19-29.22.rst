.. rn:: 5.7.19-29.22

===================================
Percona XtraDB Cluster 5.7.19-29.22
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.7.19-29.22 on September 22, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster 5.7.19-29.22 is now the current release,
based on the following:

* `Percona Server 5.7.19-17 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.19-17.html>`_

* Galera Replication library 3.22

* wsrep API version 29

All Percona software is open-source and free.

Upgrade Instructions
====================

After you upgrade each node to |PXC| 5.7.19-29.22,
run the following command on one of the nodes::

 $ mysql -uroot -p < /usr/share/mysql/pxc_cluster_view.sql

Then restart all nodes, one at a time::

 $ sudo service mysql restart

New Features
============

* Introduced the ``pxc_cluster_view`` table
  to get a unified view of the cluster.
  This table is exposed through the performance schema.

  .. code-block:: text
  
     mysql> select * from pxc_cluster_view;
     -----------------------------------------------------------------------------
     HOST_NAME  UUID                                  STATUS  LOCAL_INDEX  SEGMENT
     -----------------------------------------------------------------------------
     n1         b25bfd59-93ad-11e7-99c7-7b26c63037a2  DONOR   0            0
     n2         be7eae92-93ad-11e7-88d8-92f8234d6ce2  JOINER  1            0
     -----------------------------------------------------------------------------
     2 rows in set (0.01 sec)

* :jirabug:`PXC-803`: Added support for new features in |PXB| 2.4.7:

  * :option:`wsrep_debug` enables debug logging

  * :option:`encrypt_threads` specifies the number of threads
    that XtraBackup should use for encrypting data (when ``encrypt=1``).
    This value is passed using the ``--encrypt-threads`` option in XtraBackup.

  * :option:`backup_threads` specifies the number of threads
    that XtraBackup should use to create backups.
    See the ``--parallel`` option in XtraBackup.

Improvements
============

* :jirabug:`PXC-835`: Limited ``wsrep_node_name`` to 64 bytes.

* :jirabug:`PXC-846`: Improved logging to report reason of IST failure.

* :jirabug:`PXC-851`: Added version compatibility check during SST
  with XtraBackup:

  * If donor is 5.6 and joiner is 5.7:
    A warning is printed to perform ``mysql_upgrade``.

  * If donor is 5.7 and joiner is 5.6:
    An error is printed and SST is rejected.

Fixed Bugs
==========

* :jirabug:`PXC-825`: Fixed script for SST with XtraBackup
  (``wsrep_sst_xtrabackup-v2``) to include the ``--defaults-group-suffix``
  when logging to syslog.
  For more information, see :bug:`1559498`.

* :jirabug:`PXC-826`: Fixed multi-source replication to PXC node slave.
  For more information, see :bug:`1676464`.

* :jirabug:`PXC-827`: Fixed handling of different binlog names
  between donor and joiner nodes when GTID is enabled.
  For more information, see :bug:`1690398`.

* :jirabug:`PXC-830`: Rejected the ``RESET MASTER`` operation
  when wsrep provider is enabled and ``gtid_mode`` is set to ``ON``.
  For more information, see :bug:`1249284`.

* :jirabug:`PXC-833`: Fixed connection failure handling during SST
  by making the donor retry connection to joiner every second
  for a maximum of 30 retries.
  For more information, see :bug:`1696273`.

* :jirabug:`PXC-839`: Fixed GTID inconsistency when setting ``gtid_next``.

* :jirabug:`PXC-840`: Fixed typo in alias for ``systemd`` configuration.

* :jirabug:`PXC-841`: Added check to avoid replication of DDL
  if ``sql_log_bin`` is disabled.
  For more information, see :bug:`1706820`.

* :jirabug:`PXC-842`: Fixed deadlocks during Load Data Infile (LDI)
  with ``log-bin`` disabled
  by ensuring that a new transaction (of 10 000 rows)
  starts only after the previous one is committed by both wsrep and InnoDB.
  For more information, see :bug:`1706514`.

* :jirabug:`PXC-843`: Fixed situation where the joiner hangs
  after SST has failed
  by dropping all transactions in the receive queue.
  For more information, see :bug:`1707633`.

* :jirabug:`PXC-853`: Fixed cluster recovery by enabling ``wsrep_ready``
  whenever nodes become PRIMARY.

* :jirabug:`PXC-862`: Fixed script for SST with XtraBackup
  (``wsrep_sst_xtrabackup-v2``) to use the ``ssl-dhparams`` value
  from the configuration file.
 
.. note:: As part of fix for :jirabug:`PXC-827`,
   version communication was added to the SST protocol.
   As a result, newer version of PXC (as of 5.7.19 and later)
   cannot act as donor when joining an older version PXC node (prior to 5.7.19).
   It will work fine vice versa:
   old node can act as donor when joining nodes with new version.

