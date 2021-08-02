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

There are three methods of SST available in |PXC|:

* :program:`mysqldump`
* :program:`rsync`
* :program:`xtrabackup`

The downside of ``mysqldump`` and ``rsync`` is that
the donor node becomes **READ-ONLY** while data is being copied.
Xtrabackup SST, on the other hand, uses `backup locks
<http://www.percona.com/doc/percona-server/5.7/management/backup_locks.html>`_,
which means the Galera provider is not paused at all as with FTWRL
(Flush Tables with Read Lock) earlier.
The SST method can be configured
using the :variable:`wsrep_sst_method` variable.

.. note:: If the :variable:`gcs.sync_donor` variable is set to ``Yes``
   (default is ``No``), the whole cluster will get blocked
   if the donor is blocked by SST.

Choosing the SST Donor
======================

If there are no nodes available
that can safely perform incremental state transfer (|IST|),
the cluster defaults to |SST|.

If there are nodes available that can perform |IST|,
the cluster prefers a local node over remote nodes to serve as the donor.

If there are no local nodes available that can perform |IST|,
the cluster chooses a remote node to serve as the donor.

If there are several local and remote nodes that can perform |IST|,
the cluster chooses the node with the highest ``seqno`` to serve as the donor.

Using Percona Xtrabackup
========================

The default SST method is ``xtrabackup-v2`` which uses |Percona XtraBackup|.
This is the least blocking method that leverages `backup locks
<http://www.percona.com/doc/percona-server/5.7/management/backup_locks.html>`_.
XtraBackup is run locally on the donor node,
so it's important that the correct user credentials
are set up on the donor node.
In order for |PXC| to perform SST using XtraBackup,
credentials for connecting to the donor node need to be set up
in the :variable:`wsrep_sst_auth` variable.
Besides the credentials, the :term:`datadir` needs to be specified
in the server configuration file :file:`my.cnf`,
otherwise the transfer process will fail.

For more information about the required credentials,
see the `XtraBackup manual <https://www.percona.com/doc/percona-xtrabackup/2.4/using_xtrabackup/privileges.html#permissions-and-privileges-needed>`_.

To test if the credentials will work,
run |innobackupex| on the donor node with the username and password
specified in the :variable:`wsrep_sst_auth` variable.
For example, if the value of :variable:`wsrep_sst_auth` is ``root:Passw0rd``,
the |innobackupex| command should look like this:

.. code-block:: bash

  innobackupex --user=root --password=Passw0rd /tmp/

Detailed information on this method
is provided in :ref:`xtrabackup_sst` documentation.

Using mysqldump
===============

This method uses the standard :program:`mysqldump` utility
to dump all the databases from the donor node
and import them to the joining node.
For this method to work, the :variable:`wsrep_sst_auth` variable
needs to be set up with the root credentials.
This method is the slowest and it performs a global lock during |SST|,
which blocks writes to the donor node.

The script used for this method is :file:`/usr/bin/wsrep_sst_mysqldump`
and it is included in the |PXC| binary packages.

Using ``rsync``
===============

This method uses :program:`rsync` to copy files from donor to the joining node.
In some cases, this can be faster than using XtraBackup,
but it requires a global data lock, which will block writes to the donor node.
This method doesn't require root credentials to be set up
in the :variable:`wsrep_sst_auth` variable.

The script used for this method is :file:`/usr/bin/wsrep_sst_rsync`
and it is included in the |PXC| binary packages.

SST for tables with tablespaces that are not in the data directory
==================================================================

For example:

.. code-block:: sql

   CREATE TABLE t1 (c1 INT PRIMARY KEY) DATA DIRECTORY = '/alternative/directory';

The result depends on the SST method:

* SST using ``rsync``

  SST will report success, however the table's data will not be copied over,
  since ``rsync`` just copies the files.
  You will not be able to access the table on the joiner node:

  .. code-block:: sql

     mysql> select * from t1;
     ERROR 1812 (HY000): Tablespace is missing for table `sbtest`.`t1`.

* SST using ``mysqldump``

  Works as expected.
  If the file does not exist, it will be created.
  Otherwise it will attempt to use the file
  (if the file doesn't have the expected format, an error is returned).

* SST using Percona XtraBackup

  XtraBackup will restore the table to the same location on the joiner node.
  If the target directory does not exist, it will be created.
  If the target file already exists, an error will be returned,
  because XtraBackup cannot clear tablespaces not in the data directory.

Other Reading
=============

* `SST Methods for MySQL <https://galeracluster.com/library/documentation/sst.html#state-snapshot-transfers>`__
* :ref:`Xtrabackup SST configuration<xtrabackup_sst>`
