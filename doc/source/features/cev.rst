.. _cev:

====================================================
Cluster Error Voting 
====================================================

Implemented in |PXC| 8.0.21, Cluster Error Voting (CEV) is a protocol that allows a cluster to handle replication problems with data inconsistency. The protocol is active in a cluster with any number of nodes. Data inconsistency is a failure of data integrity, such as the cluster replicating unplanned data changes. Therefore, detecting data inconsistency is always after the fact. 

For example, if a node fails to apply a transaction in a five-node cluster, this node initiates a vote within the cluster. The cluster removes any nodes in the minority and continues to function. A DBA manually fixes the issue, and the minority nodes rejoin the cluster.

In the source-replica topology, replication is one-way. Table updates on the source are replicated to the replica table, and replica updates are not replicated back to the source. If the clients use reads from the replica nodes for writes to the source, this action can cause inconsistency. 

In the multi-master topology, a node can be both source and replica; the client interacts with any node. Thus, if one part of the cluster has inconsistent data, this data spreads throughout.

Reasons for Inconsistency
--------------------------

There are many ways a cluster node can have inconsistent data. They are the following: 

* Missing data in State Snapshot Transfer

* Incorrect cluster variable settings

* Issue in the client code

:ref:`state_snapshot_transfer` (SST) is a full data copy from one node (donor) to the joining node (joiner). The SST script may miss a transaction on the donor, and this transaction is not replicated to the joiner and create an inconsistency. This inconsistency may cause either a node to fail to start or be unnoticed for a long time. 

Incorrect cluster variable settings or incorrect replication filtering can also cause inconsistency. The following actions are examples:

* Changing the ``wsrep_on`` variable to ``OFF``. As a result, workloads executed in that session are not replicated to other nodes.

* Setting the session variable ``sql_log_bin`` to ``OFF`` disables binary logging for one node. This ``OFF`` setting also prevents binary log transactions from being assigned global transaction identifiers (GTID). If replication uses GTIDs, and the variable is reset to ``ON``, any transactions without GTIDs are lost.

* Replication filters let you configure the replicas to skip selected events. If these filters are misused, they may cause inconsistency. For example, the ``binlog-do-db`` option configures a source to filter transactions that match the criteria from being written to the binary log. As a result the replicas do not see those transactions.

An issue in the client code or scripts can cause data inconsistency. 

Before implementing :ref:`cev`, the node with an issue during a transaction reports the error in the log and exits the cluster, the other nodes in the cluster report that node as ``out of the view``. 


Cluster Error Voting
-----------------------------------

Using the CEV

With CEV, a node that encounters a data inconsistency, the node returns an error message instead of immediately exiting the cluster. Then, the sender initiates voting. Each node informs the cluster about the operation's success or failure. 

Voting can cause the following actions:

* A simple majority with the same result continues. The node or nodes with different results perform an emergency shutdown. 

* If there is no majority, then success wins. 

* If none of the nodes have a success event, then another node is selected to continue.

.. note::

    For DDL transactions, the same process occurs except, the source can initiate a vote if the DDL fails.

You can control the voting with the following variables:

.. list-table::
    :widths: 20 40
    :header-rows: 1

    * - Variable Name
      - Control description
    * - ``wsrep_ignore_apply_errors``
      - Controls if errors are reported or ignored. 
    * - ``gcs.vote_policy``
      - Controls how much each vote weighs. If the value is ``0``, a simple majority wins because each vote is either 0 or 1. If the value is >= 1, then each success vote is >= that value. A success vote would win even if the number of nodes is in the minority. 

The DBA refers to the logs to investigate what has happened and fix the inconsistency. For example, :ref:`restarting-nodes` adds the node back to the cluster.

.. seealso:: 

    `Galera replication - how to recover a PXC cluster <https://www.percona.com/blog/2014/09/01/galera-replication-how-to-recover-a-pxc-cluster/>`__




