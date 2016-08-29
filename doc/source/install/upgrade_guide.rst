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

#. Stop the ``mysql`` process:

   .. code-block:: bash

      sudo service mysql stop

#. Remove existing packages and install version 5.7.
   For more information, see :ref:`install`.

   For example, if you have Percona software repositories configured,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: bash

        sudo yum remove 'Percona*'
        sudo yum install Percona-XtraDB-Cluster-57

   * On Debian or Ubuntu:

     .. code-block:: bash

        sudo apt-get remove percona-*
        sudo apt-get install percona-xtradb-cluster-57

#. Back up :file:`grastate.dat`, so that you can restore it
   if it is corrupted or zeroed out due to network issue, for example.

#. Start the node outside the cluster
   by setting the :variable:`wsrep_provider` variable to ``none``.
   For example:

   .. code-block:: bash

      sudo mysqld --skip-grant-tables --user=mysql --wsrep-provider='none'

#. Open another session and run ``mysql_upgrade``.

#. When the upgrade is done, stop the ``mysqld`` process.

   .. note:: On CentOS, the :file:`my.cnf` configuration file
      is renamed to :file:`my.cnf.rpmsave`.
      Make sure to rename it back
      before joining the upgraded node back to the cluster.

#. Start the node as normal:

   .. code-block:: bash

      sudo service mysql start

#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

