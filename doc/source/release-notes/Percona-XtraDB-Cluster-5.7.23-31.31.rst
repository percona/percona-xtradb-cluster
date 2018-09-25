.. rn:: 5.7.23-31.31

=====================================
Percona XtraDB Cluster |release|
=====================================

Percona is glad to announce the release of
Percona XtraDB Cluster |release| on September 26, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster |release| is now the current release,
based on the following:

* `Percona Server for MySQL 5.7.23 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.23-23.html>`_
* Galera Replication library 3.24
* Galera/Codership WSREP API Release 5.7.23

Deprecated
==========

The following variables are deprecated starting from this release:

  * :variable:`wsrep_convert_lock_to_trx`

This variable, which defines whether locking sessions should be converted to
transactions, is deprecated in Percona XtraDB Cluster |release| because it is
rarely used in practice.

Fixed Bugs
==========

* :jirabug:`PXC-1017`: Memcached access to InnoDB was not replicated by Galera
* :jirabug:`PXC-2164`: The SST script prevented SELinux from being enabled
* :jirabug:`PXC-2155`: ``wsrep_sst_xtrabackup-v2`` did not delete all folders on cleanup
* :jirabug:`PXC-2160`: In some cases, the MySQL version was not detected correctly with the ``Xtrabackup-v2`` method of SST (State Snapshot Transfer).
* :jirabug:`PXC-2199`: When the ``DROP TRIGGER IF EXISTS`` statement was run for a not existing trigger, the node GTID was incremented instead of the cluster GTID.
* :jirabug:`PXC-2209`: compression dictionary was not replicated in PXC

Other bugs fixed:

* :jirabug:`PXC-2202`: "Disconnected Cluster Node should still allow itself to be shut down"
* :jirabug:`PXC-2165`: "Failing to set wsrep_node_address or wsrep_sst_receive_address can cause SST to fail"
* :jirabug:`PXC-2213`: "NULL/VOID DDL can commit out-of-order"

.. |release| replace:: 5.7.23-31.31
