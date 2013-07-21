.. _monitoring:

========================
 Monitoring the cluster
========================

The important bit about the cluster is that each node should be monitored independently. There is no centralized node, the cluster is the set of active, connected nodes, and each can have a different view of the cluster. Further, many of these variables are relative to the node you query them from: for example, replication sent (from this node) and received (from writes on the rest of the cluster). Having data from all nodes helps tracking down the source of issues (i.e., where are the flow control messages coming from?  Where did that 100MB transaction came from?).

Manually
========

Manual cluster monitoring can be done with `myq_gadgets <https://github.com/jayjanssen/myq_gadgets/>`_.

Alerting
========

Standard |MySQL| alerting should apply here. |Percona XtraDB Cluster| specific alerting should include:

 * Cluster state of each node (:variable:`wsrep_cluster_status` != Primary)
 * Node state (:variable:`wsrep_connected`, :variable:`wsrep_ready` != ON)

Other optional alerting could be done on:

 * Excessive replication conflicts (high rate of :variable:`wsrep_local_cert_failures` and :variable:`wsrep_local_bf_aborts`)
 * Excessive Flow control messages (:variable:`wsrep_flow_control_sent`/ :variable:`wsrep_flow_control_recv`)
 * Large replication queues (:variable:`wsrep_local_recv_queue`).

Metrics
=======

Metrics collection (i.e., long-term graphing) on the cluster should be done on:

 * Queue sizes (:variable:`wsrep_local_recv_queue`, :variable:`wsrep_local_send_queue`)
 * Flow control (:variable:`wsrep_flow_control_sent`, :variable:`wsrep_flow_control_recv`)
 * Number of transactions in and out of this node (:variable:`wsrep_replicated`, :variable:`wsrep_received`)
 * Number of transactions in and out in bytes (:variable:`wsrep_replicated_bytes`, :variable:`wsrep_received_bytes`)
 * Replication conflicts (:variable:`wsrep_local_cert_failures` and :variable:`wsrep_local_bf_aborts`)

Other Reading
=============

* `Realtime stats to pay attention to in PXC and Galera <http://www.mysqlperformanceblog.com/2012/11/26/realtime-stats-to-pay-attention-to-in-percona-xtradb-cluster-and-galera/>`_
