.. rn:: 5.6.37-26.21

===================================
Percona XtraDB Cluster 5.6.37-26.21
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.37-26.21 on September 20, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.37-26.21 is now the current release,
based on the following:

* `Percona Server 5.6.37-82.2 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.37-82.2.html>`_

* Galera Replication library 3.21

* wsrep API version 26

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.37-26.21 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.37-26.21>`_.

Improvements
============

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

* :jirabug:`PXC-841`: Added check to avoid replication of DDL
  if ``sql_log_bin`` is disabled.
  For more information, see :bug:`1706820`.

* :jirabug:`PXC-853`: Fixed cluster recovery by enabling ``wsrep_ready``
  whenever nodes become PRIMARY.

* :jirabug:`PXC-862`: Fixed script for SST with XtraBackup
  (``wsrep_sst_xtrabackup-v2``) to use the ``ssl-dhparams`` value
  from the configuration file.

.. note:: As part of fix for :jirabug:`PXC-827`,
   version communication was added to the SST protocol.
   As a result, newer version of PXC (as of 5.6.37 and later)
   cannot act as donor when joining an older version PXC node (prior to 5.6.37).
   It will work fine vice versa:
   old node can act as donor when joining nodes with new version.

