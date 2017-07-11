.. _bootstrap:

============================
Bootstrapping the First Node
============================

After you :ref:`configure all PXC nodes <configure>`,
initialize the cluster by bootstrapping the first node.
The initial node should be the one that contains all your data,
which you want to be replicated to other nodes.

Bootstrapping implies starting the node without any known cluster addresses.
If the :variable:`wsrep_cluster_address` variable is empty,
|PXC| assumes that this is the first node and initializes the cluster.

Instead of changing the configuration,
start the first node using the following command:

.. code-block:: bash

   [root@pxc1 ~]# /etc/init.d/mysql bootstrap-pxc

.. note::

   On RHEL or CentOS 7, use the following bootstrap command:

   .. code-block:: bash

      [root@pxc1 ~]# systemctl start mysql@bootstrap.service

When you start the node using the previous command,
it runs in bootstrap mode with ``wsrep_cluster_address=gcomm://``.
This tells the node to initialize the cluster
with :variable:`wsrep_cluster_conf_id` set to ``1``.
After you :ref:`add other nodes <add-node>` to the cluster,
you can then restart this node as normal,
and it will use standard configuration again.

To make sure that the cluster has been initialized, run the following:

.. code-block:: sql

   mysql@pxc1> show status like 'wsrep%';
   +----------------------------+--------------------------------------+
   | Variable_name              | Value                                |
   +----------------------------+--------------------------------------+
   | wsrep_local_state_uuid     | c2883338-834d-11e2-0800-03c9c68e41ec |
   | ...                        | ...                                  |
   | wsrep_local_state          | 4                                    |
   | wsrep_local_state_comment  | Synced                               |
   | ...                        | ...                                  |
   | wsrep_cluster_size         | 1                                    |
   | wsrep_cluster_status       | Primary                              |
   | wsrep_connected            | ON                                   |
   | ...                        | ...                                  |
   | wsrep_ready                | ON                                   |
   +----------------------------+--------------------------------------+
   40 rows in set (0.01 sec)

The previous output shows that the cluster size is 1 node,
it is the primary component, the node is in ``Synced`` state,
it is fully connected and ready for write-set replication.

Before :ref:`adding other nodes <add-node>` to your new cluster,
create a user for :term:`SST` and provide necessary privileges for it.
The credentials must match those specified when :ref:`configure`.

.. code-block:: sql

   mysql@pxc1> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 'passw0rd';
   mysql@pxc1> GRANT RELOAD, LOCK TABLES, PROCESS, REPLICATION CLIENT ON *.* TO
     'sstuser'@'localhost';
   mysql@pxc1> FLUSH PRIVILEGES;

For more information, see `Privileges for Percona XtraBackup
<https://www.percona.com/doc/percona-xtrabackup/2.4/using_xtrabackup/privileges.html>`_.

Next Steps
==========

After initializing the cluster, you can :ref:`add other nodes <add-node>`.
