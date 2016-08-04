.. _upgrade_from_5.5:

=================================================
Upgrading Percona XtraDB Cluster from version 5.5
=================================================

|PXC| versions 5.6 and later have incompatible changes compared to version 5.5.
If possible, you should stop all nodes, upgrade them, bootstrap the first node,
and start the others as normal.
However, to upgrade the cluster from |PXC| version 5.5 without downtime,
you need to use special configuration options
that would allow replication between 5.5 nodes and nodes with later versions.
These compatibility options affect performance,
so only use them during the upgrade.

This guide describes the procedure for a rolling upgrade (without downtime)
of a cluster running |PXC| version 5.5 to the latest |PXC| version 5.7.
The procedure will work similarly for upgrading to 5.6,
but it is recommended to use the latest available version.

.. note:: Replication between 5.6 and 5.7 is fully compatible.
   You can have nodes running both versions in the cluster at the same time
   without any compatibility options required.
   For information, see :ref:`upgrading_from_5.6`.

To avoid automatic SST while upgrading nodes,
set the :variable:`wsrep_sst_method` variable to ``skip``.
You can perform SST manually during the ugrade if necessary:

* If DONOR is 5.5 and JOINER is 5.6/5.7, perform ``mysql_upgrade`` on JOINER

* If JOINER is 5.5 and DONOR is 5.6/5.7, perform full package upgrade on JOINER

However, it is recommended to have large gcache and avoid SST altogether
until you upgrade all the nodes in the cluster.

To upgrade the cluster from |PXC| version 5.5 without downtime:

.. contents::
   :local:

Stage 1. Upgrade a single node
==============================

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` process:

   .. prompt:: bash

      sudo service mysql stop

#. Remove version 5.5 packages and install version 5.7.
   For more information, see :ref:`install`.

   For example, if you are using Percona software repositories,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: none

        sudo yum remove 'Percona*'
        sudo yum install Percona-XtraDB-Cluster-57

   * On Debian or Ubuntu:

     .. code-block:: none

        sudo apt-get remove percona-*
        sudo apt-get install percona-xtradb-cluster-57

#. Remove or replace variables in the MySQL configuration file :file:`my.cnf`
   that are not compatible with Percona Server 5.6 and later versions.
   For more information,
   see `Changed in Percona Server 5.6 <http://www.percona.com/doc/percona-server/5.6/changed_in_56.html>`_

#. Use the following settings in :file:`my.cnf`
   for compatibility with |PXC| nodes running version 5.5:

   * Append ``socket.checksum=1`` to :variable:`wsrep_provider_options`.
     Required for compatibility with Galera library version 2.

   * Set |log_bin_use_v1_row_events|_ to ``1``

   * Set |avoid_temporal_upgrade|_ to ``ON``

   * Set |gtid_mode|_ to ``OFF``

   * Set |binlog_checksum|_ to ``NONE``

   * Set |read_only|_ to ``ON``

     .. note:: ``read_only=ON`` is required only if there are tables with
        ``timestamp``, ``datetime``, or ``time`` data types.
        For more information about how these data types affect replication,
        see `Replication compatibility guide <https://dev.mysql.com/doc/refman/5.6/en/replication-compatibility.html>`_.
        Any DDLs during migration are not recommended for the same reason.

     .. note:: ``read_only`` does not apply to root connections
        (as per MySQL specification).

     .. note:: To ensure that a node with ``read-only=ON``
        is not written to during the upgrade,
        ``clustercheck`` (usually used with ``xinetd`` and HAProxy)
        distributed with |PXC| has been modified to return ``503``
        when the node is read-only,
        so that HAProxy doesn't send writes to it.
        For more information, see the ``clustercheck`` script.
        You can also opt for read-write splitting at load-balancer/proxy level,
        or at application level.

   For example, here is what you might add to :file:`my.cnf`:

   .. code-block:: none

      wsrep_provider_options="socket.checksum=1"
      log_bin_use_v1_row_events=1
      avoid_temporal_upgrade=ON
      gtid_mode=0
      binlog_checksum=NONE
      read_only=ON

   .. note:: If there are no other 5.5 nodes in the cluster,
      the listed compatibility options are not required.

.. |log_bin_use_v1_row_events| replace:: ``log_bin_use_v1_row_events``
.. _log_bin_use_v1_row_events: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_log_bin_use_v1_row_events

.. |avoid_temporal_upgrade| replace:: ``avoid_temporal_upgrade``
.. _avoid_temporal_upgrade: http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_avoid_temporal_upgrade

.. |gtid_mode| replace:: ``gtid_mode``
.. _gtid_mode: http://dev.mysql.com/doc/refman/5.7/en/replication-options-gtids.html#sysvar_gtid_mode

.. |binlog_checksum| replace:: ``binlog_checksum``
.. _binlog_checksum: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_checksum

.. |read_only| replace:: ``read_only``
.. _read_only: http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_read_only

#. Back up :file:`grastate.dat`.

#. Start the node outside of the cluster
   by setting the ``--wsrep-provider=none`` option. For example:

   .. prompt:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

   This will ensure that other nodes are not affected by the upgrade.

#. Open another session and run ``mysql_upgrade``.

#. When the upgrade is done, stop the ``mysqld`` process.

#. Start the node as normal:

   .. pormpt:: bash

      sudo service mysql start

   If you configured the compatibility options correclty,
   the upgraded node should synchronize with other nodes in the cluster.

Stage 2. Upgrade other nodes one by one
======================================

Follow the same procedure for all other nodes in the cluster one by one.
When upgrading the last node, do not provide compatibility options for it.

Stage 3. (Optional) Restart nodes without coompatibility options
================================================================

Compatibility options that you added when upgrading each node
are necessary only while you have nodes with old and new versions.
Once you upgrade all nodes to newest |PXC| version,
they are not required any more.

* Remove ``socket.checksum=1`` from :variable:`wsrep_provider_options`.
  This affects performance.
  It defaults to ``socket.checksum=2``,
  which uses hardware accelerated CRC32 checksumming.

* Remove |read_only|_ or set it to ``OFF`` (which is default).
  This prevented writes from nodes with old |PXC| versions.
  With all nodes upgraded, you need to enable writes to all nodes.

  .. note:: If you set up read-write splitting at load-balancer or proxy level
     to prevent writes to upgraded nodes from those with old |PXC| versions,
     re-configure it to allow writes to all nodes.

* You can remove the other options, leave them, or set to other values:

  * |log_bin_use_v1_row_events|_
  * |avoid_temporal_upgrade|_
  * |gtid_mode|_
  * |binlog_checksum|_

.. rubric:: References

.. target-notes::

