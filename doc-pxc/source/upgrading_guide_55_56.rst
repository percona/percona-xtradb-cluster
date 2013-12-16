.. _upgrading_guide:

==================================================================
 Percona XtraDB Cluster In-Place Upgrading Guide: From 5.5 to 5.6
==================================================================

.. warning::

   * This upgrade guide ensures that a replication from 5.6 nodes to 5.5 ones is avoided by making them read-only. This is due to an existing bug :bug:`1251137`.
   * Also, some variables (possibly deprecated) in PS 5.5 may have been removed in PS 5.6 (hence in PXC 5.6), please check that the variable is still valid before upgrade.
   * Also, make sure to avoid SST during upgrade since a SST between nodes with 5.5 and 5.6 may not work (especially, if 5.5 is donor and 5.6 is joiner, mysql_upgrade will be required on joiner; vice-versa, package upgrade will be required on joiner).

Upgrading cluster involves two major stages:

I) Upgrade a single node
II) Upgrade the whole cluster while not breaking replication
 
Following is the upgrade process on CentOS 6.4
==============================================
 
**Step #1** Make sure all the nodes in cluster are upgraded to the latest 5.5 version and are in synced state.
 
Stage I 
--------
Assuming we are going to upgrade node A, (and other nodes B and C are on 5.5)
 
**Step #2** On node A stop the mysql process and remove the old packages: ::

    # service mysql stop
    # yum remove 'Percona*'
 
**Step #3** Install the new packages: ::

    # yum install Percona-XtraDB-Cluster-server-56 Percona-XtraDB-Cluster-client-56 percona-xtrabackup
 
**Step #4** Fix the variables in the |MySQL| configuration file :file:`my.cnf` which are not compatible with |Percona Server| 5.6. Detailed list can be checked in `Changed in Percona Server 5.6 <http://www.percona.com/doc/percona-server/5.6/changed_in_56.html>`_ documentation. Add the following to :file:`my.cnf` for compatibility with 5.5 replication for the duration of upgrade, add 'socket.checksum=1' to the :variable:`wsrep_provider_options` variable, and set the following options: ::

    wsrep_provider_options="socket.checksum=1"
    log_bin_use_v1_row_events=1
    gtid_mode=0
    binlog_checksum=NONE
    wsrep-slave-threads=1
    read_only=ON

**Step #5** Next, start the node with the variable :variable:`wsrep_provider` set to ``none``: ::

    # mysqld --skip-grant-tables --user=mysql --wsrep-provider='none' 
 
This is to ensure that other hosts are not affected by this upgrade.
 
**Step #6** While Step #5 is running, in the background or in another session run: ::

    # mysql_upgrade
 
**Step #7** Step #5 must complete successfully, upon which, process started in Step 5) can be stopped/killed.
 
 
**Step #8** Step #5) should not fail (if it fails check for any bad variables in the configuration file), otherwise :file:`grastate.dat` can potentially get zeroed and the node will try to perform State Snapshot Transfer from a 5.5 node. ('Potentially' since with --wsrep-provider='none' it shouldn't). Also backing up :file:`grastate.dat` is recommended prior to Step #5 for the same purpose.

**Step #9** If all the steps above have completed successfully node can be started with: ::
  
    # service mysql start 
 
**Step #10** At this point, other nodes (B, C) should acknowledge that this node is up and synced! 

Stage II
---------
 
**Step #11** After this has been set up all 5.5 nodes can be upgraded, one-by-one, as described in the Stage I. 

  a) After all nodes in the cluster are upgraded to 5.6, option :variable:`read_only` should be set to ``OFF``. 

  b) Nodes should be restarted with compatibility options added earlier removed/updated for optimal performance (though cluster will continue run with those options).
 
Following is the upgrade process on Ubuntu 12.04 (precise)
==========================================================

**Step #1** Make sure all the nodes in cluster are upgraded to the latest 5.5 version and are in synced state.

Stage I 
--------
Assuming we are going to upgrade node A, (and other nodes B and C are on 5.5)

**Step #2** On node A stop the mysql process and remove the old packages: ::

    # /etc/init.d/mysql stop
    # apt-get remove percona-xtradb-cluster-server-5.5 percona-xtradb-cluster-galera-2.x percona-xtradb-cluster-common-5.5 percona-xtradb-cluster-client-5.5

**Step #3** Fix the variables in the |MySQL| configuration file :file:`my.cnf` which are not compatible with |Percona Server| 5.6. Detailed list can be checked in `Changed in Percona Server 5.6 <http://www.percona.com/doc/percona-server/5.6/changed_in_56.html>`_ documentation. Add the following to :file:`my.cnf` for compatibility with 5.5 replication for the duration of upgrade, add 'socket.checksum=1' to the :variable:`wsrep_provider_options` variable and set :variable:`wsrep_provider` set to ``none`` ::

    wsrep_provider_options="socket.checksum=1"
    wsrep_provider=none
    log_bin_use_v1_row_events=1
    gtid_mode=0
    binlog_checksum=NONE
    wsrep-slave-threads=1

**Step #4** Install the new packages: ::

    # apt-get install percona-xtradb-cluster-server-5.6 percona-xtradb-cluster-client-5.6 percona-xtrabackup percona-xtradb-cluster-galera-3.x

**Step #5** After node has been started you'll need to run ``mysql_upgrade``: ::

    # mysql_upgrade

**Step #6** Step #5) should not fail (if it fails check for any bad variables in the configuration file), otherwise :file:`grastate.dat` can potentially get zeroed and the node will try to perform State Snapshot Transfer from a 5.5 node. ('Potentially' since with --wsrep-provider='none' it shouldn't). Also backing up :file:`grastate.dat` is recommended prior to Step #5 for the same purpose.


**Step #7** If all the steps above have completed successfully,  set the :variable:`wsrep_provider` to the location of the Galera library, and node can be started with: ::
  
    # service mysql start 

**Step #8** At this point, other nodes (B, C) should acknowledge that this node is up and synced!

Stage II
---------

**Step #9**   After this has been set up all 5.5 nodes can be upgraded, one-by-one, as described in the Stage I. 

  a) After all nodes in the cluster are upgraded to 5.6, option :variable:`read_only` should be set to ``OFF``. 

  b) Nodes should be restarted with compatibility options added earlier removed/updated for optimal performance (though cluster will continue run with those options).


 
