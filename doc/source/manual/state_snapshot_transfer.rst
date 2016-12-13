.. _state_snapshot_transfer:

=========================
 State Snapshot Transfer
=========================

State Snapshot Transfer is a full data copy from one node (donor) to the joining node (joiner). It's used when a new node joins the cluster. In order to be synchronized with the cluster, new node has to transfer data from the node that is already part of the cluster.  
There are three methods of SST available in Percona XtraDB Cluster: :program:`mysqldump`, :program:`rsync` and :program:`xtrabackup`. The downside of `mysqldump` and `rsync` is that the donor node becomes *READ-ONLY* while data is being copied from one node to another. Xtrabackup SST, on the other hand, uses `backup locks <http://www.percona.com/doc/percona-server/5.6/management/backup_locks.html>`_, which means galera  provider is not paused at all as with FTWRL (Flush Tables with Read Lock) earlier. State snapshot transfer method can be configured with the :variable:`wsrep_sst_method` variable.

For more information, see :variable:`wsrep_desync`.

.. note:: 

 If the variable :variable:`gcs.sync_donor` is set to ``Yes`` (default ``No``), whole cluster will get blocked if the donor is blocked by the State Snapshot Transfer and not just the donor node.

Choosing the SST Donor
======================

If there are no nodes available that can safely perform an incremental state transfer, the cluster defaults to a state snapshot transfer. If there are nodes available that can safely perform an incremental state transfer, the cluster prefers a local node over remote nodes to serve as the donor. If there are no local nodes available that can safely perform an incremental state transfer, the cluster chooses a remote node to serve as the donor. Where there are several local or remote nodes available that can safely perform an incremental state transfer, the cluster chooses the node with the highest ``seqno`` to serve as the donor.

Using *Percona Xtrabackup*
==========================

This is the default SST method (version 2 of it: xtrabackup-v2). This is the least blocking method as it uses `backup locks <http://www.percona.com/doc/percona-server/5.6/management/backup_locks.html>`_. |XtraBackup| is run locally on the donor node, so it's important that the correct user credentials are set up on the donor node. In order for PXC to perform the SST using the |XtraBackup|, credentials for connecting to the donor node need to be set up in the variable :variable:`wsrep_sst_auth`. Beside the credentials, one more important thing is that the :term:`datadir` needs to be specified in the server configuration file :file:`my.cnf`, otherwise the transfer process will fail.

More information about the required credentials can be found in the |XtraBackup| `manual <http://www.percona.com/doc/percona-xtrabackup/innobackupex/privileges.html#permissions-and-privileges-needed>`_. Easy way to test if the credentials will work is to run the |innobackupex| on the donor node with the username and password specified in the variable :variable:`wsrep_sst_auth`. For example, if the value of the :variable:`wsrep_sst_auth` is ``root:Passw0rd`` |innobackupex| command should look like: :: 

  innobackupex --user=root --password=Passw0rd /tmp/
 
Detailed information on this method are provided in :ref:`xtrabackup_sst` documentation.

Using ``mysqldump``
===================

This method uses the standard :program:`mysqldump` to dump all the databases from the donor node and import them to the joining node. For this method to work :variable:`wsrep_sst_auth` needs to be set up with the root credentials. This method is the slowest one and it also performs the global lock while doing the |SST| which will block writes to the donor node.

Script used for this method can be found in :file:`/usr/bin/wsrep_sst_mysqldump` and it's provided with the |Percona XtraDB Cluster| binary packages.

Using ``rsync``
===============

This method uses :program:`rsync` to copy files from donor to the joining node. In some cases this can be faster than using the |XtraBackup| but requires the global data lock which will block writes to the donor node. This method doesn't require username/password credentials to be set up in the variable :variable:`wsrep_sst_auth`.

Script used for this method can be found in :file:`/usr/bin/wsrep_sst_rsync` and it's provided with the |Percona XtraDB Cluster| binary packages.

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

* `SST Methods for MySQL <http://galeracluster.com/documentation-webpages/statetransfer.html#state-snapshot-transfer-sst>`_
* :ref:`Xtrabackup SST configuration<xtrabackup_sst>`
