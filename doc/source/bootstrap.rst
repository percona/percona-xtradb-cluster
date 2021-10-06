.. _bootstrap:

================================================================================
Bootstrapping the First Node
================================================================================

After you :ref:`configure all PXC nodes <configure>`, initialize the cluster by
bootstrapping the first node.  The initial node must contain all the data that
you want to be replicated to other nodes.

Bootstrapping implies starting the first node without any known cluster
addresses: if the wsrep_cluster_address variable is empty, Percona XtraDB Cluster assumes that
this is the first node and initializes the cluster.

Instead of changing the configuration, start the first node using the following
command:

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
it is the primary component, the node is in the ``Synced`` state,
it is fully connected and ready for write-set replication.

.. include:: .res/text/admonition-automatic-user.txt

.. seealso::

   Percona Blog Post: Percona XtraBackup |version| New Feature: wsrep_sst_auth removal
      https://www.percona.com/blog/2019/10/03/percona-xtradb-cluster-8-0-new-feature-wsrep_sst_auth-removal/

Next Steps
==========

After initializing the cluster, you can :ref:`add other nodes <add-node>`.

.. wsrep_cluster_address replace:: :variable:`wsrep_cluster_address`

.. include:: .res/replace.opt.txt
.. include:: .res/replace.txt
