.. _wsrep_status_index:

===============================
Index of wsrep status variables
===============================

.. variable:: wsrep_apply_oooe

This variable shows parallelization efficiency, how often writests have been
applied out of order.

.. variable:: wsrep_apply_oool

This variable shows how often a writeset with a higher sequence number was
applied before one with a lower sequence number.

.. variable:: wsrep_apply_window

Average distance between highest and lowest concurrently applied sequence
numbers.

.. variable:: wsrep_causal_reads_

Shows the number of writesets processed while the variable
:variable:`wsrep_causal_reads` was set to ``ON``.

.. variable:: wsrep_cert_bucket_count

This variable, shows the number of cells in the certification index
hash-table.

.. variable:: wsrep_cert_deps_distance

Average distance between highest and lowest sequence number that can be
possibly applied in parallel.

.. variable:: wsrep_cert_index_size

Number of entries in the certification index.

.. variable:: wsrep_cert_interval

Average number of write-sets received while a transaction replicates.

.. variable:: wsrep_cluster_conf_id

Number of cluster membership changes that have taken place.

.. variable:: wsrep_cluster_size

Current number of nodes in the cluster.

.. variable:: wsrep_cluster_state_uuid

This variable contains :term:`UUID` state of the cluster. When this value is
the same as the one in :variable:`wsrep_local_state_uuid`, node is synced with
the cluster.

.. variable:: wsrep_cluster_status

Status of the cluster component. Possible values are:

  * ``Primary``
  * ``Non-Primary``
  * ``Disconnected``

.. variable:: wsrep_commit_oooe

This variable shows how often a transaction was committed out of order.

.. variable:: wsrep_commit_oool

This variable currently has no meaning.

.. variable:: wsrep_commit_window

Average distance between highest and lowest concurrently committed sequence
number.

.. variable:: wsrep_connected

This variable shows if the node is connected to the cluster. If the value is
``OFF``, the node has not yet connected to any of the cluster components. This
may be due to misconfiguration.

.. variable:: wsrep_evs_delayed

Comma separated list of nodes that are considered delayed. The node format is
``<uuid>:<address>:<count>``, where ``<count>`` is the number of entries on
delayed list for that node.

.. variable:: wsrep_evs_evict_list

List of UUIDs of the evicted nodes.

.. variable:: wsrep_evs_repl_latency

This status variable provides information regarding group communication
replication latency. This latency is measured in seconds from when a message is
sent out to when a message is received.

The format of the output is ``<min>/<avg>/<max>/<std_dev>/<sample_size>``.

.. variable:: wsrep_evs_state

Internal EVS protocol state.

.. variable:: wsrep_flow_control_interval

This variable shows the lower and upper limits for Galera flow control.
The upper limit is the maximum allowed number of requests in the queue.
If the queue reaches the upper limit, new requests are denied.
As existing requests get processed, the queue decreases,
and once it reaches the lower limit, new requests will be allowed again.

.. variable:: wsrep_flow_control_interval_high

Shows the upper limit for flow control to trigger.

.. variable:: wsrep_flow_control_interval_low

Shows the lower limit for flow control to stop.

.. variable:: wsrep_flow_control_paused

Time since the last status query that was paused due to flow control.

.. variable:: wsrep_flow_control_paused_ns

Total time spent in a paused state measured in nanoseconds.

.. variable:: wsrep_flow_control_recv

Number of ``FC_PAUSE`` events received since the last status query.

.. variable:: wsrep_flow_control_sent

Number of ``FC_PAUSE`` events sent since the last status query.

.. variable:: wsrep_flow_control_status

   :Version: Introduced in 5.7.17-29.20

This variable shows whether a node has flow control enabled for normal traffic.
It does not indicate the status of flow control during SST.

.. variable:: wsrep_gcache_pool_size

This variable shows the size of the page pool and dynamic memory allocated for
GCache (in bytes).

.. variable:: wsrep_gcomm_uuid

This status variable exposes UUIDs in :file:`gvwstate.dat`, which are Galera
view IDs (thus unrelated to cluster state UUIDs). This UUID is unique for each
node. You will need to know this value when using manual eviction feature.

.. variable:: wsrep_incoming_addresses

Shows the comma-separated list of incoming node addresses in the cluster.

.. variable:: wsrep_ist_receive_status

   :Version: Introduced in 5.7.17-29.20

Displays the progress of IST for joiner node.
If IST is not running, the value is blank.
If IST is running, the value is the percentage of transfer completed.

.. variable:: wsrep_ist_receive_seqno_end

The sequence number of the last transaction in IST.

.. variable:: wsrep_ist_receive_seqno_current

The sequence number of the current transaction in IST.

.. variable:: wsrep_ist_receive_seqno_start

The sequence number of the first transaction in IST.

.. variable:: wsrep_last_applied

Sequence number of the last applied transaction.

.. variable:: wsrep_last_committed

Sequence number of the last committed transaction.

.. variable:: wsrep_local_bf_aborts

Number of local transactions that were aborted by slave transactions while
being executed.

.. variable:: wsrep_local_cached_downto

The lowest sequence number in GCache. This information can be helpful with
determining IST and SST. If the value is ``0``, then it means there are no
writesets in GCache (usual for a single node).

.. variable:: wsrep_local_cert_failures

Number of writesets that failed the certification test.

.. variable:: wsrep_local_commits

Number of writesets commited on the node.

.. variable:: wsrep_local_index

Node's index in the cluster.

.. variable:: wsrep_local_recv_queue

Current length of the receive queue (that is, the number of writesets waiting
to be applied).

.. variable:: wsrep_local_recv_queue_avg

Average length of the receive queue since the last status query. When this
number is bigger than ``0`` this means node can't apply writesets as fast as
they are received. This could be a sign that the node is overloaded and it may
cause replication throttling.

.. variable:: wsrep_local_replays

Number of transaction replays due to *asymmetric lock granularity*.

.. variable:: wsrep_local_send_queue

Current length of the send queue (that is, the number of writesets waiting to
be sent).

.. variable:: wsrep_local_send_queue_avg

Average length of the send queue since the last status query. When cluster
experiences network throughput issues or replication throttling, this value
will be significantly bigger than ``0``.

.. variable:: wsrep_local_state

.. variable:: wsrep_local_state_comment

Internal number and the corresponding human-readable comment of the node's
state. Possible values are:

===== ================ ======================================================
 Num   Comment          Description
===== ================ ======================================================
 1     Joining          Node is joining the cluster
 2     Donor/Desynced   Node is the donor to the node joining the cluster
 3     Joined           Node has joined the cluster
 4     Synced           Node is synced with the cluster
===== ================ ======================================================

.. variable:: wsrep_local_state_uuid

The :term:`UUID` of the state stored on the node.

.. variable:: wsrep_protocol_version

Version of the wsrep protocol used.

.. variable:: wsrep_provider_name

Name of the wsrep provider (usually ``Galera``).

.. variable:: wsrep_provider_vendor

Name of the wsrep provider vendor (usually ``Codership Oy``)

.. variable:: wsrep_provider_version

Current version of the wsrep provider.

.. variable:: wsrep_ready

This variable shows if node is ready to accept queries. If status is ``OFF``,
almost all queries will fail with ``ERROR 1047 (08S01) Unknown Command`` error
(unless the :variable:`wsrep_on` variable is set to ``0``).

.. variable:: wsrep_received

Total number of writesets received from other nodes.

.. variable:: wsrep_received_bytes

Total size (in bytes) of writesets received from other nodes.

.. variable:: wsrep_repl_data_bytes

Total size (in bytes) of data replicated.

.. variable:: wsrep_repl_keys

Total number of keys replicated.

.. variable:: wsrep_repl_keys_bytes

Total size (in bytes) of keys replicated.

.. variable:: wsrep_repl_other_bytes

Total size of other bits replicated.

.. variable:: wsrep_replicated

Total number of writesets sent to other nodes.

.. variable:: wsrep_replicated_bytes

Total size (in bytes) of writesets sent to other nodes.
