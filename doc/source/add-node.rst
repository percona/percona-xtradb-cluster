.. _add-node:

=======================
Adding Nodes to Cluster
=======================

New nodes that are :ref:`properly configured <configure>` are provisioned
automatically.  When you start a node with the address of at least one other
running node in the wsrep_cluster_address variable, this node automatically
joins and synchronizes with the cluster.

.. note::

   Any existing data and configuration will be overwritten to match
   the data and configuration of the DONOR node.  Do not join several
   nodes at the same time to avoid overhead due to large amounts of
   traffic when a new node joins.

Percona XtraDB Cluster uses Percona XtraBackup_ for :term:`State Snapshot Transfer <SST>` and the
wsrep_sst_method variable is always set to ``xtrabackup-v2``.


Starting the Second Node
========================

Start the second node using the following command:

.. code-block:: bash

   [root@pxc2 ~]# systemctl start mysql

After the server starts, it receives :term:`SST` automatically.

To check the status of the second node, run the following:

.. include:: .res/code-block/sql/show-status-like-wsrep.txt

The output of ``SHOW STATUS`` shows that the new node has been successfully
added to the cluster.  The cluster size is now 2 nodes, it is the primary
component, and it is fully connected and ready to receive write-set replication.

If the state of the second node is ``Synced`` as in the previous example, then
the node received full :term:`SST` is synchronized with the cluster, and you can
proceed to add the next node.

.. note::

   If the state of the node is ``Joiner``, it means that SST hasn't finished.
   Do not add new nodes until all others are in ``Synced`` state.

Starting the Third Node
=======================

To add the third node, start it as usual:

.. code-block:: bash

   [root@pxc3 ~]# systemctl start mysql

To check the status of the third node, run the following:

.. code-block:: sql

   mysql@pxc3> show status like 'wsrep%';
   +----------------------------+--------------------------------------+
   | Variable_name              | Value                                |
   +----------------------------+--------------------------------------+
   | wsrep_local_state_uuid     | c2883338-834d-11e2-0800-03c9c68e41ec |
   | ...                        | ...                                  |
   | wsrep_local_state          | 4                                    |
   | wsrep_local_state_comment  | Synced                               |
   | ...                        | ...                                  |
   | wsrep_cluster_size         | 3                                    |
   | wsrep_cluster_status       | Primary                              |
   | wsrep_connected            | ON                                   |
   | ...                        | ...                                  |
   | wsrep_ready                | ON                                   |
   +----------------------------+--------------------------------------+
   40 rows in set (0.01 sec)

Previous output shows that the new node has been successfully added to the
cluster.  Cluster size is now 3 nodes, it is the primary component, and it is
fully connected and ready to receive write-set replication.

Next Steps
==========

When you add all nodes to the cluster, you can :ref:`verify replication
<verify>` by running queries and manipulating data on nodes to see if these
changes are synchronized accross the cluster.

.. wsrep_sst_method replace:: :variable:`wsrep_sst_method`
.. wsrep_cluster_address replace:: :variable:`wsrep_cluster_address`
