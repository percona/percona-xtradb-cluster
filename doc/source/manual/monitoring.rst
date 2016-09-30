.. _monitoring:

======================
Monitoring the cluster
======================

Each node can have a different view of the cluster.
There is no centralized node to monitor.
To track down the source of issues,
you have to monitor each node independently.

Values of many variables depend on the node from which you are querying.
For example, replication sent from a node
and writes received by all other nodes.

Having data from all nodes can help you understand
where flow messages are coming from,
which node sends excessively large transactions,
and so on.

Manual Monitoring
=================

Manual cluster monitoring can be performed using
`myq-tools <https://github.com/jayjanssen/myq-tools/>`_.

Alerting
========

Besides standard MySQL alerting,
you should use at least the following triggers specific to |PXC|:

 * Cluster state of each node

   * :variable:`wsrep_cluster_status` != Primary

 * Node state

   * :variable:`wsrep_connected` != ``ON``
   * :variable:`wsrep_ready` != ``ON``

For additional alerting, consider the following:

 * Excessive replication conflicts can be identtified using the
   :variable:`wsrep_local_cert_failures` and :variable:`wsrep_local_bf_aborts`
   variables

 * Excessive flow control messages can be identified using the
   :variable:`wsrep_flow_control_sent` and :variable:`wsrep_flow_control_recv`
   variables

 * Large replication queues can be identified using the
   :variable:`wsrep_local_recv_queue`.

Metrics
=======

Cluster metrics collection for long-term graphing should be done
at least for the following:

 * Queue sizes:
   :variable:`wsrep_local_recv_queue` and :variable:`wsrep_local_send_queue`

 * Flow control:
   :variable:`wsrep_flow_control_sent` and :variable:`wsrep_flow_control_recv`

 * Number of transactions for a node:
   :variable:`wsrep_replicated` and :variable:`wsrep_received`

 * Number of transactions in bytes:
   :variable:`wsrep_replicated_bytes` and :variable:`wsrep_received_bytes`

 * Replication conflicts:
   :variable:`wsrep_local_cert_failures` and :variable:`wsrep_local_bf_aborts`

Other Reading
=============

* `Realtime stats to pay attention to in PXC and Galera <http://www.mysqlperformanceblog.com/2012/11/26/realtime-stats-to-pay-attention-to-in-percona-xtradb-cluster-and-galera/>`_

