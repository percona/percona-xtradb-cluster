.. _upgrade_guide:

================================
Upgrading Percona XtraDB Cluster
================================

This guide describes the procedure for upgrading |PXC| version 5.6
to the latest version 5.7 without downtime (*rolling upgrade*).
This means that replication between nodes will continue during the upgrade.

Rolling upgrade from earlier versions to 5.7 is not supported.
If you are running |PXC| version 5.5, it is recommended to shut down all nodes,
then remove and re-create the cluster from scratch.
Alternatively, you can perform a `rolling upgrade from PXC 5.5 to 5.6 <https://www.percona.com/doc/percona-xtradb-cluster/5.6/upgrading_guide_55_56.html>`_,
and then follow the current procedure to upgrade from 5.6 to 5.7.

The following documents contain details about relevant changes
in the 5.7 series of MySQL and Percona Server.
Make sure you deal with any incompatible features and variables
mentioned in these documents when upgrading to |PXC| 5.7.

* `Changed in Percona Server 5.7 <https://www.percona.com/doc/percona-server/5.7/changed_in_57.html>`_

* `Upgrading MySQL <http://dev.mysql.com/doc/refman/5.7/en/upgrading.html>`_

* `Upgrading from MySQL 5.6 to 5.7 <http://dev.mysql.com/doc/refman/5.7/en/upgrading-from-previous-series.html>`_

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

        $ sudo apt-get remove percona-xtrabackup* percona-xtradb-cluster*
        $ sudo apt-get install percona-xtradb-cluster-57

#. In case of Debian or Ubuntu,
   the ``mysql`` service starts automatically after install.
   Stop the service:

   .. code-block:: bash

      $ sudo service mysql stop

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue.

#. Start the node outside the cluster
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

#. Open another session and run ``mysql_upgrade``.

#. When the upgrade is done, stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

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
      with the :variable:`pxc_strict_mode` variable set to ``PERMISSIVE``::

       $ sudo mysqld --pxc-strict-mode=PERMISSIVE

      After you check the log for any experimental or unsupported features
      and fix any encountered incompatibilities,
      you can set the variable back to ``ENFORCING`` at run time::

       mysql> SET pxc_strict_mode=ENFORCING;

#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

