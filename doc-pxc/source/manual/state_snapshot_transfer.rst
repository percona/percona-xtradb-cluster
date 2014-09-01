.. _state_snapshot_transfer:

=========================
 State Snapshot Transfer
=========================

State Snapshot Transfer is a full data copy from one node (donor) to the joining node (joiner). It's used when a new node joins the cluster. In order to be synchronized with the cluster, new node has to transfer data from the node that is already part of the cluster.  
There are three methods of SST available in Percona XtraDB Cluster: :program:`mysqldump`, :program:`rsync` and :program:`xtrabackup`. The downside of `mysqldump` and `rsync` is that the donor node becomes *READ-ONLY* while data is being copied from one node to another (SST applies :command:`FLUSH TABLES WITH READ LOCK` command). Xtrabackup SST does not require :command:`READ LOCK` for the entire syncing process, only for syncing the |MySQL| system tables and writing the information about the binlog, galera and slave information (same as the regular XtraBackup backup). State snapshot transfer method can be configured with the :variable:`wsrep_sst_method` variable.

.. note:: 

 If the variable :variable:`gcs.sync_donor` is set to ``Yes`` (default ``No``), whole cluster will get blocked if the donor is blocked by the State Snapshot Transfer and not just the donor node.

Choosing the SST Donor
======================

If there are no nodes available that can safely perform an incremental state transfer, the cluster defaults to a state snapshot transfer. If there are nodes available that can safely perform an incremental state transfer, the cluster prefers a local node over remote nodes to serve as the donor. If there are no local nodes available that can safely perform an incremental state transfer, the cluster chooses a remote node to serve as the donor. Where there are several local or remote nodes available that can safely perform an incremental state transfer, the cluster chooses the node with the highest ``seqno`` to serve as the donor.

Using *Percona Xtrabackup*
==========================

This is the default SST method (version 2 of it: xtrabackup-v2). This is the least blocking method as it locks the tables only to copy the |MyISAM| system tables. |XtraBackup| is run locally on the donor node, so it's important that the correct user credentials are set up on the donor node. In order for PXC to perform the SST using the |XtraBackup|, credentials for connecting to the donor node need to be set up in the variable :variable:`wsrep_sst_auth`. Beside the credentials, one more important thing is that the :term:`datadir` needs to be specified in the server configuration file :file:`my.cnf`, otherwise the transfer process will fail.

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

Other Reading
=============

* `SST Methods for MySQL <http://galeracluster.com/documentation-webpages/statetransfer.html#state-snapshot-transfer-sst>`_
* :ref:`Xtrabackup SST configuration<xtrabackup_sst>`
