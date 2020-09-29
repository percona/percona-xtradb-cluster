.. _upgrade_guide:

================================
Upgrading Percona XtraDB Cluster
================================

This guide describes the procedure for upgrading |PXC| without downtime
(*rolling upgrade*) to the |PXC| 8.0. A "rolling upgrade" means
there is no need to take down the complete cluster during the upgrade.

.. important::

    There is no downgrade path.
    
    If you must revert a upgrade between 8.0 versions, take an 8.0 backup,
    using mysqldump from the upgraded cluster and restore the backup to the earlier 8.0
    version.

The following documents contain details about relevant changes in the 8.0 series
of MySQL and |percona-server|. Make sure you deal with any incompatible features
and variables mentioned in these documents when upgrading to |PXC| 8.0.

* `Changed in Percona Server for MySQL 8.0
  <https://www.percona.com/doc/percona-server/8.0/changed_in_version.html>`_
* `Upgrading MySQL
  <http://dev.mysql.com/doc/refman/8.0/en/upgrading.html>`_
* `Upgrading from MySQL 5.7 to 8.0
  <https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html>`_

.. contents:: In this section
   :depth: 1
   :local:

.. _upgrade-guide-changed:

Important changes in |pxc| |version|
================================================================================

.. contents:: 
   :local:

 _upgrade-guide-changed-traffic-encryption:

Traffic encryption is enabled by default
--------------------------------------------------------------------------------

The :variable:`pxc_encrypt_cluster_traffic` variable, which enables traffic
encryption, is set to ``ON`` by default in |pxc| |version|. 

All nodes have the same SSL certificates. The 5.7 traffic is, by default, not
encrypted. If you attempt to connect an "out of a box" 8.0 node, this results
in an error:

.. code-block:: text

   ... [ERROR] ... [Galera] handshake with remote endpoint ... 
   This error is often caused by SSL issues. ...

.. seealso:: sections :ref:`encrypt-traffic`, :ref:`configure`



Rolling upgrades to |version| from versions older than 5.7 are not supported
--------------------------------------------------------------------------------

If you are running |PXC| version 5.6, shut down all
nodes, then remove and re-create the cluster from scratch. Alternatively,
you can perform a `rolling upgrade from PXC 5.6 to 5.7 <https://www.percona.com/doc/percona-xtradb-cluster/5.7/howtos/upgrade_guide.html>`_.
and then follow the current procedure to upgrade from 5.7 to 8.0.

Not recommended to mix |pxc| 5.7 nodes with |pxc| 8.0 nodes
--------------------------------------------------------------------------------

If downtime is acceptable, shut down the cluster and upgrade all nodes
to |pxc| 8.0. It is important to make backups before attempting an upgrade.

The rolling upgrade is supported, but you must ensure the traffic is controlled
during the upgrade process and that writes are directed only to 5.7 nodes
until all nodes are upgraded to 8.0.

.. _upgrade-guide-changed-strict-mode:

|strict-mode| is enabled by default
--------------------------------------------------------------------------------

|PXC| in |version| runs with |strict-mode| enabled by default. This option denies
any unsupported operations and may halt the server if :ref:`a strict mode
validation fails <validations>`.

It is recommended to first start the node with
the :variable:`pxc_strict_mode` variable set to ``PERMISSIVE`` in the |MySQL|
configuration file (on Debian and Ubuntu |file.debian-conf|; on CentOS and Red
Hat |file.centos-conf|).

After a time, check the log for any experimental or unsupported features and fix any
encountered incompatibilities. You can then set the variable to ``ENFORCING`` at
run time:

.. code-block:: mysql

   mysql> SET GLOBAL pxc_strict_mode=ENFORCING;

Restarting the node with the updated configuration file
(on Debian and Ubuntu |file.debian-conf|; on
CentOS and Red Hat |file.centos-conf|) switches the variable to ``ENFORCING``.

The configuration file layout has changed in |pxc| |version|
--------------------------------------------------------------------------------

All configuration settings are stored in the default |MySQL| configuration file:

* Path on Debian and Ubuntu: |file.debian-conf|
* Path on Red Hat and CentOS: |file.centos-conf|

Before you start the upgrade, move your custom settings from
:file:`/etc/mysql/percona-xtradb-cluster.conf.d/wsrep.cnf` (on Debian and
Ubuntu) or from :file:`/etc/percona-xtradb-cluster.conf.d/wsrep.cnf` (on Red Hat
and CentOS) to the appropriate location.


``caching_sha2_password`` is the default authentication plugin
--------------------------------------------------------------------------------

In |PXC| |version|, the default authentication plugin is
``caching_sha2_password``. The ProxySQL option
:ref:`pxc.proxysql.v2.admin-tool.syncusers` will not work if the |PXC| user is
created using ``caching_sha2_password``. Use the ``mysql_native_password``
authentication plugin in these cases.

Be sure you are running on the latest 5.7 version before you upgrade to 8.0.

|mysql-upgrade| is now run automatically as part of :term:`SST`. You do not have
to run it manually when upgrading your system from an older version.

Rolling Upgrade of a 3-Node |pxc| from 5.7 to 8.0
=================================================

The major upgrade involves upgrading the |pxc| from the previous major version
5.7 to the new major version 8.0. Be aware of the following issues:

* Ensure your application works with a reduced cluster size

* During the upgrade, the cluster has an even number of nodes, which can cause
  split brain. Be prepared to handle this situation if needed.
  
In this scenario, follow these steps for each node:

1. Ensure that all nodes are synchronized.

#. Shutdown one of the 5.7 nodes.

#. Remove the PXC 5.7 packages. Do not remove the data-directory.

#. Install the PXC 8.0 packages. For more information, see :ref:`install`.

#. Back up :file:`grastate.dat`. If the file is corrupted or zeroed out due to
   network issues in the upgrade process you may need to restore it.

#. |pxc| |version| creates an internal SST user used for data transfer. You
   should remove ``wsrep_sst_auth`` from :file:`my.cnf` before starting the
   node with 8.0 packages.
   
   .. seealso::
    
        `wsrep_sst_auth_Removal <https://www.percona.com/blog/2019/10/03/percona-xtradb-cluster-8-0-new-feature-wsrep_sst_auth-removal/>`_

#. Restart the ``mysqld`` service outside the cluster in standalone mode by
   setting the :variable:`wsrep_provider` variable to ``none``.
   
      .. code-block:: bash
      
           $ sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'
           
   .. note::

      To prevent any users from accessing this node while performing
      work on it, you may add `--skip-networking
      <https://dev.mysql.com/doc/refman/8.0/en/server-options.html#option_mysqld_skip-networking>`_
      to the startup options and use a local socket to connect, or
      alternatively you may divert any incoming traffic from
      your application to other operational nodes.
      
#. When the upgrade is complete, stop the ``mysqld`` process. You can either run
   ``sudo kill`` on the ``mysqld`` process ID or ``sudo mysqladmin shutdown``
   with the MySQL root user credentials.

#. Start the cluster node with the 8.0 packages installed. |pxc| upgrades the
   data directory as part of the startup process or a state transfer (IST/SST).
   
   In most cases, starting the ``mysql`` service should run the node with your
   previous configuration. For more information, see :ref:`add-node`.

   .. code-block: bash

      $ sudo service mysql start

   .. note:

      On CentOS, the |file.centos-conf| configuration file is renamed to
      :file:`my.cnf.rpmsave`.  Make sure to rename it back before
      joining the upgraded node back to the cluster.

   |strict-mode| is enabled by default, which may result in denying any
   unsupported operations and may halt the server. For more information, see    :ref:`upgrade-guide-changed-strict-mode`.

   |opt.encrypt-cluster-traffic| is enabled by default. You must configure
   each node accordingly and avoid joining a cluster with unencrypted cluster
   traffic: all nodes in your cluster must have traffic encryption enabled. For
   more information, see :ref:`upgrade-guide-changed-traffic-encryption`.
       
#. Repeat the steps for the other nodes. The last node upgrade may take
   time to join since the node invites IST for configuration changes.

   .. note::
   
      Starting with ``Galera`` 4, configuration changes
      are cached to gcache and donated as part of IST/SST to help the
      certification queue on the JOINING node. The other nodes, already
      using protocol version 4, donate the configuration changes when the
      JOINER node is booted.
  

Major Upgrade Scenarios
================================================================================

Upgrading |pxc| from 5.7 to 8.0 may have slightly different strategies depending
on the configuration and workload on your |pxc| cluster.

Note that the new default value of |opt.encrypt-cluster-traffic| (set to *ON*
versus *OFF* in |pxc| 5.7) requires additional care. You cannot join a 5.7 node
to a |pxc| 8.0 cluster unless the node has traffic encryption enabled. A
cluster cannot have some nodes with traffic encryption enabled and some nodes
with traffic encryption disabled. For more information, see
:ref:`upgrade-guide-changed-traffic-encryption`.

   .. warning:

      *Joining a 5.7 node to a |pxc| |version| cluster is not supported.*

.. contents::
   :local:

.. _upgrading-rolling-no-active-read-only-parallel-workload:

Scenario: No active parallel workload or with read-only workload
--------------------------------------------------------------------------------

If there is no active parallel workload or the cluster has read-only workload
while upgrading the nodes, use the following procedure.

#. Shutdown one of the node 5.7 cluster nodes.
#. Remove 5.7 PXC packages without removing the data-directory.
#. Install |pxc| |version| packages.
#. Restart the mysqld service.

This upgrade flow auto-detects the presence of the 5.7 data directory and trigger
the upgrade as part of the node bootup process. The data directory is upgraded
to be compatible with |pxc| |version|. Then the node joins the cluster and
enters synced state. The 3-node cluster is restored with 2 nodes running |pxc|
5.7 and 1 node running |pxc| |version|.

.. note::

   Since :term:`SST` is not involved, :term:`SST` based auto-upgrade flow is not
   started.

|pxc| |version| uses Galera 4 while |pxc| 5.7 uses Galera-3. The cluster will
continue to use the protocol version 3 used in Galera 3 effectively limiting
some of the functionality. With all nodes upgraded to version |version|,
protocol version 4 is applied.

.. hint::

   The protocol version is stored in the ``protocol_version` column of the
   ``wsrep_cluster`` table.

   .. code-block:: sql

      mysql> USE mysql;
      ...
      mysql> SELECT protocol_version from wsrep_cluster;
      +------------------+
      | protocol_version |
      +------------------+
      |                4 |
      +------------------+
      1 row in set (0.00 sec)

As soon as the last 5.7 node shuts down, the configuration of the remaining
two nodes is updated to use protocol version 4. A new upgraded node will then join
using protocol version 4 and the whole cluster will maintain
protocol version 4 enabling the support for additional Galera 4 facilities.

It may take longer to join the last upgraded node since it will invite
:term:`IST` to obtain the configuration changes.

.. note::

   Starting from Galera 4, the configuration changes are cached to ``gcache``
   and the configuration changes are donated as part of :term:`IST` or
   :term:`SST` to help build the certification queue on the JOINING node. As
   other nodes (say n2 and n3), already using protocol version 4, donate the
   configuration changes when the JOINER node is booted.

The situation was different for the previous and second-to-last nodes since the
donation of the configuration changes is not supported by protocol
version 3 that they used.

With :term:`IST` involved on joining the last node, the smart IST flow is triggered
to take care of the upgrade even before MySQL starts to look at the
data directory.

.. important::

   *It is not recommended to restart the last node without upgrading it.*5.7
   

.. Scenario: Upgrading with active parallel read-write workload
   --------------------------------------------------------------------------------

.. The upgrade process is similar to that described in
   :ref:`upgrading-rolling-no-active-read-only-parallel-workload`: since
   the cluster state keeps changing while each node is taken down and
   upgraded, :term:`IST` or :term:`SST` is triggered when rejoining the
   node that you upgrading. In |version|, |pxc| clears the configuration
   of each replica node (via `RESET SLAVE ALL <https://dev.mysql.com/doc/refman/8.0/en/reset-slave.html>`_).

.. important:

   It is important that the joining node have enough replica threads to catch up
   IST write-sets and cluster write-sets.

.. Scenario: Adding 8.0 node to a 5.7 cluster
   --------------------------------------------------------------------------------

   Instead of upgrading a node in a 5.7 cluster to |pxc| 8.0, you are booting a
   fresh 8.0 node and try joining it as an additional node.

   Suppose the 5.7 cluster is already active and has processed write-sets. The new
   8.0 node joins the cluster and gets a dump of the cluster through :term:`SST`
   and remains part of the cluster.

   Since the underline protocol version negotiated in both cases is based on Galera 3,
   the 8.0 node will fail to service Galera 4 features. As soon as all 5.7
   nodes leave the cluster, the 8.0 nodes re-negotiates using protocol version 4
   and gets a proper local index and other properties assigned.

   .. warning:

      *Joining a 5.7 node to a |pxc| |version| cluster is not supported.*

.. Scenario: Upgrading a node which is an async replication replica
   --------------------------------------------------------------------------------

   In this case, you need to configure the source-replica replication. Make one of the
   PXC nodes as an async replica. Upgrade the |pxc| async replica node and check the
   replication status.

Scenario: Upgrading from |pxc| 5.6 to |pxc| 8.0
--------------------------------------------------------------------------------

First, upgrade |pxc| from 5.6 to the latest version of |pxc| 5.7. Then proceed
with the upgrade using the procedure described in
:ref:`upgrading-rolling-no-active-read-only-parallel-workload`.

Minor upgrade from a PXC 8.0 lower version to a PXC 8.0 higher version
================================================================================

This upgrade follows the same upgrade steps but there is no step for package
removal.

To upgrade the cluster from a PXC 8.0 lower version to a PXC 8.0 higher version,
follow these steps for each node:

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Upgrade |PXC| and Percona XtraBackup packages.
   For more information, see :ref:`install`.

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Now, start the cluster node with 8.0 packages installed, |pxc| will upgrade
   the data directory as needed - either as part of the startup process or a
   state transfer (IST/SST).

   In most cases, starting the ``mysql`` service should run the node with your
   previous configuration. For more information, see :ref:`add-node`.

   .. code-block:: bash

      $ sudo service mysql start

   .. note::

      On CentOS, the |file.centos-conf| configuration file is renamed to
      :file:`my.cnf.rpmsave`.  Make sure to rename it back before
      joining the upgraded node back to the cluster.

   |strict-mode| is enabled by default, which may result in denying any
   unsupported operations and may halt the server. For more information, see
   :ref:`upgrade-guide-changed-strict-mode`.

   |opt.encrypt-cluster-traffic| is enabled by default. You need to configure
   each node accordingly and avoid joining a cluster with unencrypted cluster
   traffic. For more information, see
   :ref:`upgrade-guide-changed-traffic-encryption`.
   
#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

.. include:: ../.res/replace.txt
.. include:: ../.res/replace.opt.txt
.. include:: ../.res/replace.ref.txt
.. include:: ../.res/replace.file.txt
.. include:: ../.res/replace.program.txt
