.. _upgrade_guide:

================================
Upgrading Percona XtraDB Cluster
================================

This guide describes the procedure for upgrading |PXC| without downtime
(*rolling upgrade*) to the latest 5.7 version. A "rolling upgrade" means
there is no need to take down the complete cluster during the upgrade.

Both major upgrades (from 5.6 to 5.7 version) and minor ones (from 5.7.x
to 5.7.y) can be done in this way. Rolling upgrades to 5.7 from versions
older than 5.6 are not supported. Therefore if you are running |PXC| version
5.5, it is recommended to shut down all nodes, then remove and re-create
the cluster from scratch. Alternatively, you can perform a
`rolling upgrade from PXC 5.5 to 5.6 <https://www.percona.com/doc/percona-xtradb-cluster/5.6/upgrading_guide_55_56.html>`_,
and then follow the current procedure to upgrade from 5.6 to 5.7.

The following documents contain details about relevant changes
in the 5.7 series of MySQL and Percona Server.
Make sure you deal with any incompatible features and variables
mentioned in these documents when upgrading to |PXC| 5.7.

* `Changed in Percona Server 5.7 <https://www.percona.com/doc/percona-server/5.7/changed_in_57.html>`_

* `Upgrading MySQL <http://dev.mysql.com/doc/refman/5.7/en/upgrading.html>`_

* `Upgrading from MySQL 5.6 to 5.7 <http://dev.mysql.com/doc/refman/5.7/en/upgrading-from-previous-series.html>`_

.. contents::
   :local:

Major upgrade
-------------

To upgrade the cluster, follow these steps for each node:

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Remove existing |PXC| and Percona XtraBackup packages,
   then install |PXC| version 5.7 packages.
   For more information, see :ref:`install`.

   For example, if you have Percona software repositories configured,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: bash

        $ sudo yum remove percona-xtrabackup* Percona-XtraDB-Cluster*
        $ sudo yum install Percona-XtraDB-Cluster-57

   * On Debian or Ubuntu:

     .. code-block:: bash

        $ sudo apt remove percona-xtrabackup* percona-xtradb-cluster*
        $ sudo apt install percona-xtradb-cluster-57

#. In case of Debian or Ubuntu,
   the ``mysql`` service starts automatically after install.
   Stop the service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Start the node outside the cluster (in standalone mode)
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

   .. note:: As of |PXC| 5.7.6, the ``--skip-grant-tables`` option
      is not required.

   .. note:: To prevent any users from accessing this node while performing
      work on it, you may add `--skip-networking <https://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_skip_networking>`_
      to the startup options and use a local socket to connect, or
      alternatively you may want to divert any incoming traffic from your
      application to other operational nodes.

#. Open another session and run ``mysql_upgrade``.

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

   .. note:: As of version 5.7,
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

   .. note:: In case of :term:`SST` synchronization, the error log
             contains statements like "Check if state gap can be
             serviced using IST ... State gap can't be serviced
             using IST. Switching to SST" instead of "Receiving
             IST: ..." lines appropriate to  :term:`IST`
             synchronization.

Minor upgrade
-------------

To upgrade the cluster, follow these steps for each node:

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Upgrade |PXC| and Percona XtraBackup packages.
   For more information, see :ref:`install`.

   For example, if you have Percona software repositories configured,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: bash

        $ sudo yum update Percona-XtraDB-Cluster-57

   * On Debian or Ubuntu:

     .. code-block:: bash

        $ sudo apt install --only-upgrade percona-xtradb-cluster-57

#. In case of Debian or Ubuntu,
   the ``mysql`` service starts automatically after install.
   Stop the service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Start the node outside the cluster (in standalone mode)
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

   .. note:: As of |PXC| 5.7.6, the ``--skip-grant-tables`` option
      is not required.

   .. note:: To prevent any users from accessing this node while performing
      work on it, you may add `--skip-networking <https://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_skip_networking>`_
      to the startup options and use a local socket to connect, or
      alternatively you may want to divert any incoming traffic from your
      application to other operational nodes.

#. Open another session and run ``mysql_upgrade``.

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

   .. note:: As of version 5.7,
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

   .. note:: In case of :term:`SST` synchronization, the error log
             contains statements like "Check if state gap can be
             serviced using IST ... State gap can't be serviced
             using IST. Switching to SST" instead of "Receiving
             IST: ..." lines appropriate to  :term:`IST`
             synchronization.

The following additional steps should be made to upgrade the data
directory structure after :term:`SST` (after the normal major or
minor upgrade steps):

1. shutdown the node that rejoined the cluster using :term:`SST`:

   .. code-block:: bash

      $ sudo service mysql stop

#. restart the node in standalone mode by setting the
   :variable:`wsrep_provider` variable to ``none``, e.g.:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

#. run ``mysql-upgrade``

#. restart the node in cluster mode (e.g by executing ``sudo service mysql
   start`` and make sure the cluster joins back using :term:`IST`.
