.. _upgrade_guide:

================================
Upgrading Percona XtraDB Cluster
================================

This guide describes the procedure for upgrading |PXC| without downtime
(*rolling upgrade*) to the |PXC| 8.0.

A "rolling upgrade" means there is no need to take down the complete
cluster during the upgrade.

The following documents contain details about relevant changes in the 8.0 series
of MySQL and |percona-server|. Make sure you deal with any incompatible features
and variables mentioned in these documents when upgrading to |PXC| 8.0.

* `Changed in Percona Server 8.0 <https://www.percona.com/doc/percona-server/8.0/changed_in_version.html>`_
* `Upgrading MySQL <http://dev.mysql.com/doc/refman/8.0/en/upgrading.html>`_
* `Upgrading from MySQL 5.7 to 8.0 <https://dev.mysql.com/doc/refman/8.0/en/upgrading-from-previous-series.html>`_

Important changes in |pxc| |version|
================================================================================

.. rubric:: Traffic encryption enabled by default

The :variable:`pxc_encrypt_cluster_traffic` variable, which enables traffic
encryption, is set to ``ON`` by default in |pxc| |version|. For more
information, see :ref:`encrypt-traffic`.

.. rubric:: Rolling upgrades to |version| from versions older than 5.7 are not supported

Therefore, if you are running |PXC| version 5.6, shut down all
nodes, then remove and re-create the cluster from scratch. Alternatively,
you can perform a `rolling upgrade from PXC 5.6 to 5.7
<https://www.percona.com/doc/percona-xtradb-cluster/5.7/howtos/upgrade_guide.html>`_.
and then follow the current procedure to upgrade from 5.7 to 8.0.

.. rubric:: *Not* recommended to mix |pxc| 5.7 nodes with |pxc| 8.0 nodes

If downtime is acceptable, shut down the cluster and upgrade all nodes to |pxc|
8.0. It is important that you make backups before attempting an upgrade.

If downtime is not possible, the rolling upgrade is supported but ensure the
traffic is controlled during the upgrade and writes are directed only to 5.7
nodes until all nodes are upgraded to |version|.

.. important:: 

   If writes are directed to a |pxc| 8.0 node in a cluster that has |pxc| 5.7,
   the cluster will be broken down. In this case, the system reports an error message
   similar to the following:

   .. code-block:: text

      [ERROR] Slave SQL: Error 'Character set '#255' is not a compiled character set and
      is not specified in the '/usr/share/percona-xtradb-cluster/charsets/Index.xml' file' on
      query. Default database: DATABASE_NAME. Query: QUERY, Error_code: 22

.. rubric:: The configuration file layout has changed in |pxc| |version|

All configuration settings are stored in the default |MySQL| configuration file:
:file:`my.cnf`:

* Path on Debian and Ubuntu: :file:`/etc/mysql/my.cnf`
* Path on Red Hat and CentOS: :file:`/etc/my.cnf`

Before you start the upgrade, move your custom settings from
:file:`/etc/mysql/percona-xtradb-cluster.conf.d/wsrep.cnf` (on Debian and
Ubuntu) or from :file:`/etc/percona-xtradb-cluster.conf.d/wsrep.cnf` (on Red Hat
and CentOS) to the new location accordingly.

.. rubric:: ``caching_sha2_password`` Is the Default Authentication Plugin

In |PXC| |version|, the default authentication plugin is
``caching_sha2_password``. The ProxySQL option
:ref:`pxc.proxysql.v2.admin-tool.syncusers` will not work if the |PXC| user is
created using ``caching_sha2_password``. Use the ``mysql_native_password``
authentication plugin in these cases.


.. contents::
   :local:

.. _upgrading.auto:

Auto-upgrade
================================================================================

|pxc| |version| introduces auto-upgrade facility that can detect when the
upgrade is needed and then run the upgrade if necessary. To make auto-upgrade
possible |pxc| maintains the :file:`wsrep_state.dat` state file that contains
the data-directory compatible server version.

.. code-block:: text

   # *wsrep_state.dat* WSREP state file (created on initialization)
   version=1.0
   wsrep_schema_version=8.0.16

The |wsrep_state_dat| file represents that the data directory is compatible
with |pxc| 8.0.16. If this directory is attached to |pxc| 8.0.18 binaries, then
the 8.0.18 binaries detect automatically that the data directory is not
compatible and should be upgraded.

Versions of |pxc| prior to the 8.0 release do not have the |wsrep_state_dat|
file. If this file is not detected, the version of the data directory is less
8.0 and |pxc| should be upgraded.

The auto-upgrade is available for upgrading from version 5.7 to 8.0 both for the
rolling upgrade (without downtime) and the offline upgrade (with downtime).

.. As part of rolling upgrade, you need to accommodate the pxc-cluster-encrypt-traffic difference.

Rolling Upgrade of a 3-Node |pxc| from 5.7 to 8.0
================================================================================

The major upgrade involves upgrading the |pxc| from the previous major version
5.7 to the new major version 8.0.

To upgrade the cluster, follow these steps for each node:

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Remove existing |PXC| and Percona XtraBackup packages, then install
   |PXC| version 8.0 packages.  For more information, see
   :ref:`install`.

   For example, if you have Percona software repositories configured,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: bash

        $ sudo yum remove percona-xtrabackup* Percona-XtraDB-Cluster*
        $ sudo yum install percona-xtradb-cluster

   * On Debian or Ubuntu:

     .. code-block:: bash

        $ sudo apt-get remove percona-xtrabackup* percona-xtradb-cluster*
        $ sudo apt-get install percona-xtradb-cluster

#. In case of Debian or Ubuntu, the ``mysql`` service starts automatically after
   the installation. Stop the service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Start the node outside the cluster (in standalone mode)
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      $ sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

   .. note::

      To prevent any users from accessing this node while performing
      work on it, you may add `--skip-networking
      <https://dev.mysql.com/doc/refman/8.0/en/server-options.html#option_mysqld_skip-networking>`_
      to the startup options and use a local socket to connect, or
      alternatively you may want to divert any incoming traffic from
      your application to other operational nodes.

#. When the upgrade is done, stop the ``mysqld`` process.
   You can either run ``sudo kill`` on the ``mysqld`` process ID,
   or ``sudo mysqladmin shutdown`` with the MySQL root user credentials.

   .. note:: On CentOS, the :file:`my.cnf` configuration file
      is renamed to :file:`my.cnf.rpmsave`.
      Make sure to rename it back
      before joining the upgraded node back to the cluster.

#. Now you can join the upgraded node back to the cluster.

   In most cases, starting the ``mysql`` service
   should run the node with your previous configuration::

   .. code-block:: bash

      $ sudo service mysql start

   For more information, see :ref:`add-node`.

   .. note:: As of version |version|,
      |PXC| runs with :ref:`pxc-strict-mode` enabled by default.
      This will deny any unsupported operations and may halt the server
      upon encountering a failed validation.

      If you are not sure, it is recommended to first start the node
      with the :variable:`pxc_strict_mode` variable set to ``PERMISSIVE``
      in the in the |MySQL| configuration file, :file:`my.cnf`.

      After you check the log for any experimental or unsupported features
      and fix any encountered incompatibilities,
      you can set the variable back to ``ENFORCING`` at run time::

       mysql> SET pxc_strict_mode=ENFORCING;

      Also switch back to ``ENFORCING`` may be done by restarting the node
      with updated :file:`my.cnf`.

#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

It is important that on rejoining, the node should synchronize using
:term:`IST`. For this, it is best not to leave the cluster node being
upgraded offline for an extended period. More on this below.

When performing any upgrade (major or minor), :term:`SST` could
be initiated by the joiner node after the upgrade if the server
was offline for some time. After :term:`SST` completes, the data
directory structure needs to be upgraded (using mysql_upgrade)
once more time to ensure compatibility with the newer version
of binaries.

.. note::

   In case of :term:`SST` synchronization, the error log contains
   statements like

   .. code-block:: text

      Check if state gap can be serviced using IST
      ... State gap can't be serviced using IST. Switching to SST
      
   instead of "Receiving IST: ..." lines appropriate to :term:`IST`
   synchronization.

Major Upgrade Scenarios
================================================================================

Upgrading |pxc| from 5.7 to 8.0 may have slightly different strategies depending
on the configuration and workload on your |pxc| cluster.

.. contents::
   :local:

.. _upgrading-rolling-no-active-read-only-parallel-workload:

Scenario: No active parallel workload or with read-only workload
--------------------------------------------------------------------------------

If there is no active parallel workload or the cluster has read-only workload
while upgrading the nodes, complete the following procedure for each node in the
cluster:

#. Shutdown one of the node 5.7 cluster nodes.
#. Remove 5.7 PXC packages without removing the data-directory.
#. Install |pxc| |version| packages.
#. Restart the mysqld service.

.. important::

   Before upgrading, make sure your application can work with a reduced cluster
   size.  Also, during the upgrade, the cluster will operate with an even number
   of nodes - the cluster may run into the split-brain problem.

This upgrade flow auto-detects the presence of the 5.7 data directory and
triggers
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

The situation was different for the previous and penultimate nodes since the
donation of the configuration changes is not supported by protocol
version 3 that they used.

With :term:`IST` involved on joining the last node, the smart IST flow is triggered
to take care of the upgrade even before MySQL starts to look at the
data directory.

.. important::

   It is not recommended to restart the last node without upgrading it.

Scenario: Upgrading with active parallel read-write workload
--------------------------------------------------------------------------------

The upgrade prosess is similar to that described in
:ref:`upgrading-rolling-no-active-read-only-parallel-workload`: since the
cluster state keeps changing while each node is taken down and upgraded,
:term:`IST` or :term:`SST` is triggered when rejoining the node that you
upgrading. In |version|, |pxc| clears the configuration of each slave node (via
RESET SLAVE).

.. important::

   It is important that the joining node have enough slave threads to catch up
   IST write-sets and cluster write-sets.


Scenario: Adding 8.0 node to a 5.7 cluster
--------------------------------------------------------------------------------

Instead of upgrading a node in a 5.7 cluster to |pxc| 8.0, you are booting a
fresh 8.0 node and try joining it as an additional node.

Suppose the 5.7 cluster is already active and has processed write-sets. The new
8.0 node joins the cluster and gets a dump of the cluster through :term:`SST`
and remains part of the cluster.

Since the underline protocol version negotiated in both cases is based on Galera 3,
the 8.0 node will fail to service galera 4 features. As soon as all 5.7
nodes leave the cluster, the 8.0 nodes re-negotiates using protocol version 4
and gets a proper local index and other properties assigned.

.. warning::

   Joining a 5.7 node to a |pxc| |version| cluster is not supported.

Scenario: Upgrading a node which is an async replication slave
--------------------------------------------------------------------------------

In this case you need to configure the master-slave replication. Make one of the
PXC nodes as an async slave. Upgrade the |pxc| async slave node and check the
replication status.

Scenario: Upgrading from |pxc| 5.6 to |pxc| 8.0
--------------------------------------------------------------------------------

First, upgrade |pxc| from 5.6 to the latest version of |pxc| 5.7. Then proceed
with the upgrade using the procedure described in
:ref:`upgrading-rolling-no-active-read-only-parallel-workload`.


Minor upgrade
================================================================================

To upgrade the cluster, follow these steps for each node:

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Upgrade |PXC| and Percona XtraBackup packages.
   For more information, see :ref:`install`.

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Start the node outside the cluster (in standalone mode)
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

   .. note::

      To prevent any users from accessing this node while performing
      work on it, you may add `--skip-networking <https://dev.mysql.com/doc/refman/8.0/en/server-options.html#option_mysqld_skip-networking>`_
      to the startup options and use a local socket to connect, or
      alternatively you may want to divert any incoming traffic from your
      application to other operational nodes.

#. When the upgrade is done, stop the ``mysqld`` process.
   You can either run ``sudo kill`` on the ``mysqld`` process ID,
   or ``sudo mysqladmin shutdown`` with the MySQL root user credentials.

   .. note:: On CentOS, the :file:`my.cnf` configuration file
      is renamed to :file:`my.cnf.rpmsave`.
      Make sure to rename it back
      before joining the upgraded node back to the cluster.

#. Now you can join the upgraded node back to the cluster.

   In most cases, starting the ``mysql`` service
   should run the node with your previous configuration::

    $ sudo service mysql start

   For more information, see :ref:`add-node`.

   .. note:: 

      |PXC| runs with :ref:`pxc-strict-mode` enabled by default.
      This will deny any unsupported operations and may halt the server
      upon encountering a failed validation.

      If you are not sure, it is recommended to first start the node
      with the :variable:`pxc_strict_mode` variable set to ``PERMISSIVE``
      in the in the |MySQL| configuration file, :file:`my.cnf`.

      After you check the log for any experimental or unsupported features
      and fix any encountered incompatibilities,
      you can set the variable back to ``ENFORCING`` at run time::

       mysql> SET pxc_strict_mode=ENFORCING;

      Also switch back to ``ENFORCING`` may be done by restarting the node
      with updated :file:`my.cnf`.

#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

Dealing with IST/SST synchronization while upgrading
----------------------------------------------------

It is important that on rejoining, the node should synchronize using
:term:`IST`. For this, it is best not to leave the cluster node being
upgraded offline for an extended period. More on this below.

When performing any upgrade (major or minor), :term:`SST` could
be initiated by the joiner node after the upgrade if the server
was offline for some time. After :term:`SST` completes, the data
directory structure needs to be upgraded (using mysql_upgrade)
once more time to ensure compatibility with the newer version
of binaries.

.. note::

   In case of :term:`SST` synchronization, the error log contains
   statements like "Check if state gap can be serviced using IST
   ... State gap can't be serviced using IST. Switching to SST"
   instead of "Receiving IST: ..." lines appropriate to :term:`IST`
   synchronization.

The following additional steps should be made to upgrade the data
directory structure after :term:`SST` (after the normal major or
minor upgrade steps):

1. Shutdown the node that rejoined the cluster using :term:`SST`:

   .. code-block:: bash

      $ sudo service mysql stop

#. Restart the node in standalone mode by setting the
   :variable:`wsrep_provider` variable to ``none``, e.g.:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

#. Restart the node in cluster mode (e.g by executing ``sudo service mysql
   start`` and make sure the cluster joins back using :term:`IST`.

.. include:: ../.res/replace.txt
