.. rn:: 5.7.24-31.33

=====================================
Percona XtraDB Cluster |release|
=====================================

Percona is glad to announce the release of
Percona XtraDB Cluster |release| on January 4, 2019.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster |release| is now the current release,
based on the following:

* `Percona Server for MySQL 5.7.24 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.24-26.html>`_
* Galera Replication library 3.25
* Galera/Codership WSREP API Release 5.7.24

Deprecated
==========

The following variables are deprecated starting from this release:

* :variable:`wsrep_preordered` was used to turn on transparent handling of
  preordered replication events applied locally first before being replicated
  to other nodes in the cluster. It is not needed anymore due to the `carried
  out performance fix <https://jira.percona.com/browse/PXC-2128>`_ eliminating
  the lag in asynchronous replication channel and cluster replication.

* :variable:`innodb_disallow_writes` usage to make |InnoDB| avoid writes during
  |abbr.sst| was deprecated in favor of the :variable:`innodb_read_only`
  variable.

* :variable:`wsrep_drupal_282555_workaround` avoided the duplicate value
  creation caused by buggy auto-increment logic, but the correspondent bug is
  already fixed.

* session-level variable :variable:`binlog_format=STATEMENT` was enabled
  only for ``pt-table-checksum``, which would be addressed in following
  releases of the *Percona Toolkit*.

Fixed Bugs
==========

* :jirabug:`PXC-2220`: Starting two instances of |PXC| on the same node could
  cause writing transactions to a page store instead of a galera.cache ring
  buffer, resulting in huge memory consumption because of retaining already
  applied write-sets.

* :jirabug:`PXC-2230`: :variable:`gcs.fc_limit=0` not allowed as dynamic
  setting to avoid generating flow control on every message was still possible
  in ``my.cnf`` due to the inconsistent check.

* :jirabug:`PXC-2238`: setting :variable:`read_only=1` caused race condition.

* :jirabug:`PXC-1131`: ``mysqld-systemd`` threw an error at |MySQL| restart in
  case of non-existing error-log in Centos/RHEL7.

* :jirabug:`PXC-2269`: being not dynamic, the
  :variable:`pxc_encrypt_cluster_traffic` variable was erroneously allowed to
  be changed by a ``SET GLOBAL`` statement.

* :jirabug:`PXC-2275`: checking :variable:`wsrep_node_address` value in the
  ``wsrep_sst_common`` command line parser caused parsing the wrong variable.

.. |release| replace:: 5.7.24-31.33
.. |abbr.sst| replace:: :abbr:`SST (State Snapshot Transfer)`
