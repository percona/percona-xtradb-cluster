.. _upgrade_from_5.6:

=================================================
Upgrading Percona XtraDB Cluster from version 5.6
=================================================

You can upgrade |PXC| version 5.6 to the latest version 5.7 without downtime.
For each node, stop the MySQL server, upgrade to the new version,
and start the node as normal.
Replication between all nodes will continue as normal.

1. Make sure that all nodes are synchronized.

#. Stop the ``mysql`` process:

   .. prompt:: bash

      sudo service mysql stop

#. Remove existing packages and install version 5.7.
   For more information, see :ref:`install`.

   For example, if you have Percona software repositories configured,
   you might use the following commands:

   * On CentOS or RHEL:

     .. code-block:: none

        sudo yum remove 'Percona*'
        sudo yum install Percona-XtraDB-Cluster-57

   * On Debian or Ubuntu:

     .. code-block:: none

        sudo apt-get remove percona-*
        sudo apt-get install percona-xtradb-cluster-57

#. Back up :file:`grastate.dat`.

#. Start the node:

   .. pormpt:: bash

      sudo service mysql start

#. Open another session and run ``mysql_upgrade``.

#. When the upgrade is done, restart the node:

   .. prompt:: bash

      sudo service mysql restart

#. Repeat this procedure for the next node in the cluster
   until you upgrade all nodes.

