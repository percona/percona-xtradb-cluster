.. _upgrading_guide:

==================================================================
 Percona XtraDB Cluster In-Place Upgrading Guide: From 5.5 to 5.6
==================================================================

.. warning::
   * Some variables (possibly deprecated) in PS 5.5 may have been removed in PS 5.6 (hence in PXC 5.6), please check that the variable is still valid before upgrade.
   * Also, make sure to avoid SST during upgrade since a SST between nodes with 5.5 and 5.6 may not work as expected (especially, if 5.5 is donor and 5.6 is joiner, mysql_upgrade will be required on joiner; vice-versa, package upgrade will be required on joiner). You can also avoid automatic SST for the duration of upgrade by setting ```wsrep-sst-method``` to 'skip' for the duration of upgrade. (Note that ```wsrep-sst-method``` is a dynamic variable.) Having a large enough gcache helps here. Also, setting it to ``skip`` ensures that you can handle SST manually if and when required.

Upgrading cluster involves following major stages:

I) Upgrade a single node.
II) Upgrade the whole cluster while not breaking replication.
III) [Optional] Restarting nodes with non-compat options.
 
.. note::
    Following upgrade process is for a **rolling** upgrade, ie. an upgrade without downtime for the cluster. If you intend to allow for downtime - bring down all nodes, upgrade them, bootstrap and start nodes - then you can just follow Stage I sans the compatibility variables part. Make sure to bootstrap the first node in the cluster after upgrade.

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

    # yum install Percona-XtraDB-Cluster-56

.. note::
    For more details on installation, refer to :ref:`installation` guide. You may also want to install Percona-XtraDB-Cluster-full-56 which installs other ancillary packages like '-shared-56', '-test-56', debuginfos and so on.
 
**Step #4** Fix the variables in the |MySQL| configuration file :file:`my.cnf` which are not compatible with |Percona Server| 5.6. Detailed list can be checked in `Changed in Percona Server 5.6 <http://www.percona.com/doc/percona-server/5.6/changed_in_56.html>`_ documentation.  In case you are not sure after this, you can also do following: ::

    # mysqld --user=mysql --wsrep-provider='none' 

If there are any invalid variables, it will print it there without affect galera grastate or any other things.

.. note::
    It may also be worthwhile to backup the grastate.dat to use it if it gets zeroed (or sequence number to -1) accidentally (or due to network issues) since this can avoid SST.

**Step #5** Add the following to :file:`my.cnf` for compatibility with 5.5 replication for the duration of upgrade, and set the following options: ::

    # Required for compatibility with galera-2
    # Append socket.checksum=1 to other options if others are in wsrep_provider_options. Eg.: "gmcast.listen_addr=tcp://127.0.0.1:15010; socket.checksum=1"
    wsrep_provider_options="socket.checksum=1"
    # Required for replication compatibility
    log_bin_use_v1_row_events=1
    gtid_mode=0
    binlog_checksum=NONE
    # Required under certain conditions
    read_only=ON

**Step #5.1** "read_only=ON" is required only when the tables you have contain timestamp/datetime/time data types as those data types are incompatible across replication from higher version to lower. This is currently a limitation of mysql itself. Also, refer to `Replication compatibility guide <https://dev.mysql.com/doc/refman/5.6/en/replication-compatibility.html>`_. Any DDLs during migration are not recommended for the same reason.

.. note::
    ``read_only`` does not apply to root connections (as per mysql specifications).

**Step #5.2** To ensure 5.6 read-only nodes are not written to during migration, clustercheck (usually used with xinetd and HAProxy) distributed with PXC has been modified to return 503 when the node is read-only so that HAProxy doesn't send writes to it. Refer to clustercheck script for more details. Instead, you can also opt for read-write splitting at load-balancer/proxy level or at application level.

.. note::
    On the last 5.5 node to upgrade to 5.6, the compatibility options of Step #5 are not required since all other nodes will already be upgrade and their compat. options are compatible with a 5.6 node without them.

**Step #6** Next, start the node with the variable :variable:`wsrep_provider` set to ``none``: ::

    # mysqld --skip-grant-tables --user=mysql --wsrep-provider='none' 
 
This is to ensure that other hosts are not affected by this upgrade (hence provider is none here).
 
**Step #7** While Step #5 is running, in the background or in another session run: ::

    # mysql_upgrade
 
    Other options like socket, user, pass may need to provided here if not defined in my.cnf.

**Step #8** Step #7 must complete successfully, upon which, process started in Step #6 can be stopped/killed.
 

**Step #9** If all the steps above have completed successfully node can be started with: ::
  
    # service mysql start 

.. note::
    If this is the first node of cluster, then replace start with ``bootstrap-pxc``. This shouldn't apply to rolling upgrade in general (since other nodes are up during this) but only for downtime-based upgrades (where you bring up nodes one by one).
 
**Step #10** At this point, other nodes (B, C) should acknowledge that this node is up and synced! 

Stage II
---------
 
**Step #11** After this has been set up all 5.5 nodes can be upgraded, one-by-one, as described in the Stage I. 

  a) If :variable:`read_only` was turned on in Step #5.1, then after all nodes in the cluster are upgraded to 5.6 or equivalently, after the last 5.5 has been take down for upgrade, option :variable:`read_only` can be set to ``OFF`` (since this is a dynamic variable, it can done without restart).

  b) If read-write splitting was done in applications and/or in load-balancer then in previous step, instead of ``read_only``, writes need to be directed to 5.6 nodes.

Stage III [Optional]
--------------------

**Step #12** This step is required to turn off the options added in #Step 5. Note, that this step is not required immediately after upgrade and can be done at a latter stage. The aim here is to turn off the compatibility options for performance reasons (only socket.checksum=1 fits this). This requires restart of each node. Hence, following can be removed/commented-out::

    # Remove socket.checksum=1 from other options if others are in wsrep_provider_options. Eg.: "gmcast.listen_addr=tcp://127.0.0.1:15010"
    # Removing this makes socket.checksum=2 which uses hardware accelerated CRC32 checksumming.
    wsrep_provider_options="socket.checksum=1"

    # Options added for replication compatibility, being removed here.
    # You can keep some of these if you wish.

    log_bin_use_v1_row_events=1

    # You can keep if you are not adding async-slaves.
    # Apropos, you may need to enable this if you are adding async-slaves, refer to MySQL 5.6 gtid_mode documentation for more details on this variable.
    gtid_mode=0

    # Galera already has full writeset checksumming, so 
    # you can keep this if async-slaves are not there and
    # binlogging is not turned on.
    binlog_checksum=NONE

    # Remove it from cnf even though it was turned off at runtime in Step #11.
    read_only=ON

 
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

    # Required for compatibility with galera-2
    # Append socket.checksum=1 to other options if others are in wsrep_provider_options. Eg.: "gmcast.listen_addr=tcp://127.0.0.1:15010; socket.checksum=1"
    wsrep_provider_options="socket.checksum=1"
    # Required for replication compatibility
    log_bin_use_v1_row_events=1
    gtid_mode=0
    binlog_checksum=NONE
    # Required under certain conditions
    read_only=ON
    wsrep_provider=none

**Step #3.1** "read_only=ON" is required only when the tables you have contain timestamp/datetime/time data types as those data types are incompatible across 
replication from higher version to lower. This is currently a limitation of mysql itself. Also, refer to `Replication compatibility guide <https://dev.mysql.c
om/doc/refman/5.6/en/replication-compatibility.html>`_. Any DDLs during migration are not recommended for the same reason.

.. note::
    ``read_only`` does not apply to root connections (as per mysql specifications).

**Step #3.2** To ensure 5.6 read-only nodes are not written to during migration, clustercheck (usually used with xinetd and HAProxy) distributed with PXC has 
been modified to return 503 when the node is read-only so that HAProxy doesn't send writes to it. Refer to clustercheck script for more details. Instead, you 
can also opt for read-write splitting at load-balancer/proxy level or at application level.

.. note::
    It may also be worthwhile to backup the grastate.dat to use it if it gets zeroed (or sequence number to -1) accidentally (or due to network issues).

.. note::
    On the last 5.5 node to upgrade to 5.6, the compatibility options of Step #3 are not required since all other nodes will already be upgrade and their configuration options are compatible with a 5.6 node without them.

**Step #4** Install the new packages: ::

    # apt-get install percona-xtradb-cluster-56

.. note::
    For more details on installation, refer to :ref:`installation` guide. You may also want to install percona-xtradb-cluster-full-56 which installs other ancillary packages like '-shared-56', '-test-56', debuginfos and so on.

**Step #5** After node has been started you'll need to run ``mysql_upgrade``: ::

    # mysql_upgrade

    Other options like socket, user, pass may need to provided here if not defined in my.cnf.

**Step #6** If all the steps above have completed successfully, shutdown the server,  set the :variable:`wsrep_provider` to the location of the Galera library (from 'none' to something like /usr/lib/libgalera_smm.so) in my.cnf, and node can be started with: ::
  
    # service mysql start 

.. note::
    If this is the first node of cluster, then replace start with ``bootstrap-pxc``. This shouldn't apply to rolling upgrade in general (since other nodes are up during this) but only for downtime-based upgrades (where you bring up nodes one by one).

**Step #7** At this point, other nodes (B, C) should acknowledge that this node is up and synced!

Stage II
---------

**Step #8**   After this has been set up all 5.5 nodes can be upgraded, one-by-one, as described in the Stage I. 

  a) If :variable:`read_only` was turned on in Step #3.1, then after all nodes in the cluster are upgraded to 5.6 or equivalently, after the last 5.5 has been take down for upgrade, option :variable:`read_only` can be set to ``OFF`` (since this is a dynamic variable, it can done without restart).

  b) If read-write splitting was done in applications and/or in load-balancer then in previous step, instead of ``read_only``, writes need to be directed to 5.6 nodes.

Stage III [Optional]
--------------------

**Step #9** This step is required to turn off the options added in #Step 3. Note, that this step is not required immediately after upgrade and can be done at a latter stage. The aim here is to turn off the compatibility options for performance reasons (only socket.checksum=1 fits this). This requires restart of each node. Hence, following can be removed/commented-out::

    # Remove socket.checksum=1 from other options if others are in wsrep_provider_options. Eg.: "gmcast.listen_addr=tcp://127.0.0.1:15010"
    # Removing this makes socket.checksum=2 which uses hardware accelerated CRC32 checksumming.
    wsrep_provider_options="socket.checksum=1"

    # Options added for replication compatibility, being removed here.
    # You can keep some of these if you wish.

    log_bin_use_v1_row_events=1

    # You can keep if you are not adding async-slaves.
    # Apropos, you may need to enable this if you are adding async-slaves, refer to MySQL 5.6 gtid_mode documentation for more details on this variable.
    gtid_mode=0

    # Galera already has full writeset checksumming, so 
    # you can keep this if async-slaves are not there and
    # binlogging is not turned on.
    binlog_checksum=NONE

    # Remove it from cnf even though it was turned off at runtime in Step #8.
    read_only=ON
