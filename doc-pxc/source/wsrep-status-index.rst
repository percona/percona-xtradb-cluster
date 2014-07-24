.. _wsrep_status_index:

=================================
 Index of wsrep status variables
=================================

.. variable:: wsrep_local_state_uuid
  
This variable contains :term:`UUID` state stored on the node.

.. variable:: wsrep_protocol_version
  
Version of the wsrep protocol used. 

.. variable:: wsrep_last_committed
  
Sequence number of the last committed transaction. 

.. variable:: wsrep_replicated
  
Total number of writesets sent to other nodes.

.. variable:: wsrep_replicated_bytes
  
Total size (in bytes) of writesets sent to other nodes.

.. variable:: wsrep_repl_keys

.. variable:: wsrep_repl_keys_bytes

.. variable:: wsrep_repl_data_bytes

.. variable:: wsrep_repl_other_bytes

.. variable:: wsrep_received
  
Total number of writesets received from other nodes. 

.. variable:: wsrep_received_bytes
  
Total size (in bytes) of writesets received from other nodes.

.. variable:: wsrep_local_commits
  
Number of writesets commited on the node.

.. variable:: wsrep_local_cert_failures
  
Number of writesets that failed the certification test.

.. variable:: wsrep_local_replays
  
Number of transaction replays due to "asymmetric lock granularity".

.. variable:: wsrep_local_send_queue
  
Current length of the send queue. Show the number of writesets waiting to be sent. 

.. variable:: wsrep_local_send_queue_avg
  
Average length of the send queue since the last status query. When cluster experiences network throughput issues or replication throttling this value will be significantly bigger than ``0``.

.. variable:: wsrep_local_recv_queue
  
Current length of the receive queue. Show the number of writesets waiting to be applied. 

.. variable:: wsrep_local_recv_queue_avg
  
Average length of the receive queue since the last status query. When this number is bigger than ``0`` this means node can't apply writesets as fast as they're received. This could be sign that node is overloaded and it will cause the replication throttling. 

.. variable:: wsrep_local_cached_downto

This variable shows the lowest sequence number in gcache. This information can be helpful with determining IST and/or SST. If the value is 18446744073709551615, then it means there are no writesets in cached in gcache (usual for a single node).

.. variable:: wsrep_flow_control_paused_ns

.. variable:: wsrep_flow_control_paused
  
Time since the last status query that replication was paused due to flow control.

.. variable:: wsrep_flow_control_sent
  
Number of ``FC_PAUSE`` events sent since the last status query.

.. variable:: wsrep_flow_control_recv
  
Number of ``FC_PAUSE`` events sent and received since the last status query.

.. variable:: wsrep_cert_deps_distance
  
Average distance between highest and lowest sequence number that can be possibly applied in parallel.

.. variable:: wsrep_apply_oooe
  
This variable shows parallelization efficiency, how often writests have been applied out-of-order. 

.. variable:: wsrep_apply_oool

This variable shows how often was writeset with higher sequence number applied before the one with lower sequence number.
  
.. variable:: wsrep_apply_window
  
Average distance between highest and lowest concurrently applied sequence number.

.. variable:: wsrep_commit_oooe
  
This variable shows how often a transaction has been applied out of order.

.. variable:: wsrep_commit_oool
  
This variable currently isn't being used.

.. variable:: wsrep_commit_window
  
Average distance between highest and lowest concurrently committed sequence number.

.. variable:: wsrep_local_state
  
This variable shows internal Galera state number. Possible values are:
 * 1 - Joining (requesting/receiving State Transfer) - node is joining the cluster
 * 2 - Donor/Desynced - node is the donor to the node joining the cluster
 * 3 - Joined - node has joined the cluster
 * 4 - Synced - node is synced with the cluster

.. variable:: wsrep_local_state_comment
  
Description of the :variable:`wsrep_local_state` variable.

.. variable:: wsrep_cert_index_size
  
.. variable:: wsrep_causal_reads_

Shows the number of writesets processed while the variable :variable:`wsrep_causal_reads` was set to ``ON``.

.. variable:: wsrep_incoming_addresses

Shows the comma-separated list of incoming node addresses in the cluster.

.. variable:: wsrep_evs_repl_latency

This status variable provides the information regarding the group communication replication latency. This latency is measured from the time point when a message is sent out to the time point when a message is received.
  
.. variable:: wsrep_cluster_conf_id

Number of cluster membership changes happened.
  
.. variable:: wsrep_cluster_size
  
Current number of nodes in the cluster. 

.. variable:: wsrep_cluster_state_uuid
  
This variable contains :term:`UUID` state of the cluster. When this value is the same as the one in :variable:`wsrep_local_state_uuid` node is synced with the cluster.

.. variable:: wsrep_cluster_status

Status of the cluster component. Possible values are:
  * ``Primary`` -
  * ``Non-Primary`` -
  * ``Disconnected`` -
  
.. variable:: wsrep_connected
  
.. variable:: wsrep_local_bf_aborts
  
Number of local transactions that were aborted by slave transactions while being executed.

.. variable:: wsrep_local_index
  
Node index in the cluster 

.. variable:: wsrep_provider_name
  
Name of the wsrep provider (usually ``Galera``).

.. variable:: wsrep_provider_vendor
  
Name of the wsrep provider vendor (usually ``Codership Oy``)

.. variable:: wsrep_provider_version
  
Current version of the wsrep provider.

.. variable:: wsrep_ready
  
This variable shows if node is ready to accept queries. If status is ``OFF`` almost all the queries will fail with ``ERROR 1047 (08S01) Unknown Command`` error (unless :variable:`wsrep_on` variable is set to ``0``)


