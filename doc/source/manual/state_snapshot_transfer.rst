.. _state_snapshot_transfer:

=======================
State Snapshot Transfer
=======================

State Snapshot Transfer (SST) is a full data copy from one node (donor)
to the joining node (joiner).
It's used when a new node joins the cluster.
In order to be synchronized with the cluster,
the new node has to receive data from a node
that is already part of the cluster.

Percona XtraDB Cluster enables :abbr:`SST (State Snapshot Transfer)` via :program:`xtrabackup`.

Xtrabackup SST uses `backup locks
<http://www.percona.com/doc/percona-server/8.0/management/backup_locks.html>`_,
which means the Galera provider is not paused at all as with :abbr:`FTWRL
(Flush Tables with Read Lock)` earlier.
The SST method can be configured
using the :variable:`wsrep_sst_method` variable.

.. note:: If the :variable:`gcs.sync_donor` variable is set to ``Yes``
   (default is ``No``), the whole cluster will get blocked
   if the donor is blocked by SST.

Choosing the SST Donor
======================

If there are no nodes available
that can safely perform incremental state transfer (IST),
the cluster defaults to SST.

If there are nodes available that can perform IST,
the cluster prefers a local node over remote nodes to serve as the donor.

If there are no local nodes available that can perform IST,
the cluster chooses a remote node to serve as the donor.

If there are several local and remote nodes that can perform IST,
the cluster chooses the node with the highest ``seqno`` to serve as the donor.

Using Percona Xtrabackup
========================

The default SST method is ``xtrabackup-v2`` which uses |Percona XtraBackup|.
This is the least blocking method that leverages `backup locks
<http://www.percona.com/doc/percona-server/8.0/management/backup_locks.html>`_.
XtraBackup is run locally on the donor node.

The :term:`datadir` needs to be specified in the server configuration file
:file:`my.cnf`, otherwise the transfer process will fail.

Detailed information on this method
is provided in :ref:`xtrabackup_sst` documentation.

SST for tables with tablespaces that are not in the data directory
==================================================================

For example:

.. code-block:: sql

   CREATE TABLE t1 (c1 INT PRIMARY KEY) DATA DIRECTORY = '/alternative/directory';

.. rubric:: SST using Percona XtraBackup

XtraBackup will restore the table to the same location on the joiner node.  If
the target directory does not exist, it will be created.  If the target file
already exists, an error will be returned, because XtraBackup cannot clear
tablespaces not in the data directory.

Other Reading
=============

* `SST Methods for MySQL <http://galeracluster.com/documentation-webpages/statetransfer.html#state-snapshot-transfer-sst>`_
* :ref:`Xtrabackup SST configuration<xtrabackup_sst>`
