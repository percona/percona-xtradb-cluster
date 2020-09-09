.. _wsrep_status_index:

===============================
Index of wsrep status variables
===============================

.. variable:: wsrep_apply_oooe

This variable shows parallelization efficiency, how often writesets have been
applied out of order.

.. seealso:: `Galera status variable: wsrep_apply_oooe
	     <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-apply-oooe>`_

.. variable::   wsrep_apply_oool
		
This variable shows how often a writeset with a higher sequence number was
applied before one with a lower sequence number.

.. seealso:: `Galera status variable: wsrep_apply_oool
	     <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-apply-oool>`_

.. variable:: wsrep_apply_window

Average distance between highest and lowest concurrently applied sequence
numbers.

.. seealso:: `Galera status variable: wsrep_apply_window
	     <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-apply-window>`_

.. variable:: wsrep_causal_reads_

Shows the number of writesets processed while the variable
:variable:`wsrep_causal_reads` was set to ``ON``.

.. seealso:: `MySQL wsrep options: wsrep_causal_reads
             <https://galeracluster.com/library/documentation/mysql-wsrep-options.html#wsrep-causal-reads>`_

.. variable:: wsrep_cert_bucket_count

Shows the number of cells in the certification index
hash-table.

.. variable:: wsrep_cert_deps_distance

Average distance between highest and lowest sequence number that can be
possibly applied in parallel.

.. seealso:: `Galera status variable: wsrep_cert_deps_distance
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cert-deps-distance>`_

.. variable:: wsrep_cert_index_size

Number of entries in the certification index.

.. seealso:: `Galera status variable: wsrep_cert_index_size
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cert-index-size>`_

.. variable:: wsrep_cert_interval

Average number of write-sets received while a transaction replicates.

.. seealso:: `Galera status variable: wsrep_cert_interval
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cert-interval>`_

.. variable:: wsrep_cluster_conf_id

The number of cluster membership changes that have taken place.

.. seealso:: `Galera status variable: wsrep_cluster_conf_id
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cluster-conf-id>`_

.. variable:: wsrep_cluster_size

Current number of nodes in the cluster.

.. seealso:: `Galera status variable: wsrep_cluster_size
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cluster-size>`_

.. variable:: wsrep_cluster_state_uuid

This variable contains :term:`UUID` state of the cluster. When this value is
the same as the one in :variable:`wsrep_local_state_uuid`, node is synced with
the cluster.

.. seealso:: `Galera status variable: wsrep_cluster_state_uuid
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cluster-state-uuid>`_

.. variable:: wsrep_cluster_status

Status of the cluster component. Possible values are:

  * ``Primary``
  * ``Non-Primary``
  * ``Disconnected``

.. seealso:: `Galera status variable: wsrep_cluster_status
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-cluster-status>`_

.. variable:: wsrep_commit_oooe

This variable shows how often a transaction was committed out of order.

.. seealso:: `Galera status variable: wsrep_commit_oooe
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-commit-oooe>`_

.. variable:: wsrep_commit_oool

This variable currently has no meaning.

.. seealso:: `Galera status variable: wsrep_commit_oool
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-commit-oool>`_

.. variable:: wsrep_commit_window

Average distance between highest and lowest concurrently committed sequence
number.

.. seealso:: `Galera status variable: wsrep_commit_window
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-commit-oool>`_

.. variable:: wsrep_connected

This variable shows if the node is connected to the cluster. If the value is
``OFF``, the node has not yet connected to any of the cluster components. This
may be due to misconfiguration.

.. seealso:: `Galera status variable: wsrep_connected
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-connected>`_

.. variable:: wsrep_evs_delayed

Comma separated list of nodes that are considered delayed. The node format is
``<uuid>:<address>:<count>``, where ``<count>`` is the number of entries on
delayed list for that node.

.. seealso:: `Galera status variable: wsrep_evs_delayed
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-evs-delayed>`_

.. variable:: wsrep_evs_evict_list

List of UUIDs of the evicted nodes.

.. seealso:: `Galera status variable: wsrep_evs_evict_list
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-evs-evict-list>`_

.. variable:: wsrep_evs_repl_latency

This status variable provides information regarding group communication
replication latency. This latency is measured in seconds from when a message is
sent out to when a message is received.

The format of the output is ``<min>/<avg>/<max>/<std_dev>/<sample_size>``.

.. seealso:: `Galera status variable: wsrep_evs_repl_latency
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-evs-repl-latency>`_

.. variable:: wsrep_evs_state

Internal EVS protocol state.

.. seealso:: `Galera status variable: wsrep_evs_state
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-evs-state>`_

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

.. seealso:: `Galera status variable: wsrep_flow_control_paused
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-flow-control-paused>`_

.. variable:: wsrep_flow_control_paused_ns

Total time spent in a paused state measured in nanoseconds.

.. seealso:: `Galera status variable: wsrep_flow_control_paused_ns
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-flow-control-paused-ns>`_

.. variable:: wsrep_flow_control_recv

Number of ``FC_PAUSE`` events received since the last status query. Unlike most
status variables, the counter for this one does not reset every time you run the
query.

.. seealso:: `Galera status variable: wsrep_flow_control_recv
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-flow-control-recv>`_

.. variable:: wsrep_flow_control_sent

Number of ``FC_PAUSE`` events sent since the last status query. Unlike most
status variables, the counter for this one does not reset every time you run the
query.

.. seealso:: `Galera status variable: wsrep_flow_control_sent
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-flow-control-sent>`_

.. variable:: wsrep_flow_control_status

This variable shows whether a node has flow control enabled for normal traffic.
It does not indicate the status of flow control during SST.

.. variable:: wsrep_gcache_pool_size

This variable shows the size of the page pool and dynamic memory allocated for
GCache (in bytes).

.. variable:: wsrep_gcomm_uuid

This status variable exposes UUIDs in :file:`gvwstate.dat`, which are Galera
view IDs (thus unrelated to cluster state UUIDs). This UUID is unique for each
node. You will need to know this value when using manual eviction feature.

.. seealso:: `Galera status variable: wsrep_gcomm_uuid
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-gcomm-uuid>`_

.. variable:: wsrep_incoming_addresses

Shows the comma-separated list of incoming node addresses in the cluster.

.. seealso:: `Galera status variable: wsrep_incoming_addresses
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-incoming-addresses>`_

.. variable:: wsrep_ist_receive_status

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

Number of local transactions that were aborted by replica transactions while
being executed.

.. seealso:: `Galera status variable: wsrep_local_bf_aborts
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-bf-aborts>`_

.. variable:: wsrep_local_cached_downto

The lowest sequence number in GCache. This information can be helpful with
determining IST and SST. If the value is ``0``, then it means there are no
writesets in GCache (usual for a single node).

.. seealso:: `Galera status variable: wsrep_local_cached_downto
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-cached-downto>`_

.. variable:: wsrep_local_cert_failures

Number of writesets that failed the certification test.

.. seealso:: `Galera status variable: wsrep_local_cert_failures
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-cert-failures>`_

.. variable:: wsrep_local_commits

Number of writesets commited on the node.

.. seealso:: `Galera status variable: wsrep_local_commits
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-commits>`_

.. variable:: wsrep_local_index

Node's index in the cluster.

.. seealso:: `Galera status variable: wsrep_local_index
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-index>`_

.. variable:: wsrep_local_recv_queue

Current length of the receive queue (that is, the number of writesets waiting
to be applied).

.. seealso:: `Galera status variable: wsrep_local_recv_queue
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-recv-queue>`_

.. variable:: wsrep_local_recv_queue_avg

Average length of the receive queue since the last status query. When this
number is bigger than ``0`` this means node can't apply writesets as fast as
they are received. This could be a sign that the node is overloaded and it may
cause replication throttling.

.. seealso:: `Galera status variable: wsrep_local_recv_queue_avg
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-recv-queue-avg>`_

.. variable:: wsrep_local_replays

Number of transaction replays due to *asymmetric lock granularity*.

.. seealso:: `Galera status variable: wsrep_local_replays
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-replays>`_

.. variable:: wsrep_local_send_queue

Current length of the send queue (that is, the number of writesets waiting to
be sent).

.. seealso:: `Galera status variable: wsrep_local_send_queue
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-send-queue>`_

.. variable:: wsrep_local_send_queue_avg

Average length of the send queue since the last status query. When cluster
experiences network throughput issues or replication throttling, this value
will be significantly bigger than ``0``.

.. seealso:: `Galera status variable: wsrep_local_send_queue_avg
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-send-queue-avg>`_

.. variable:: wsrep_local_state

Internal Galera cluster FSM state number

.. seealso:: `Galera status variable: wsrep_local_state <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-state>`_

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

.. seealso:: `Galera status variable: wsrep_local_state_comment
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-state-comment>`_

.. variable:: wsrep_local_state_uuid

The :term:`UUID` of the state stored on the node.

.. seealso:: `Galera status variable: wsrep_local_state_uuid
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-local-state-uuid>`_

.. variable:: wsrep_monitor_status

The status of the local monitor (local and replicating actions), apply monitor
(apply actions of write-set), and commit monitor (commit actions of write
sets). In the value of this variable, each monitor (L: Local, A: Apply, C:
Commit) is represented as a *last_entered*, and *last_left* pair:

.. code-block:: text

   wsrep_monitor_status (L/A/C)	[ ( 7, 5), (2, 2), ( 2, 2) ]

last_entered
   Shows which transaction or write-set has recently entered the queue
last_left
   Shows which last transaction or write-set has been executed and left the queue

According to the Galera protocol, transactions can be applied in parallel but
must be committed in a given order. This rule implies that there can be multiple
transactions in the *apply* state at a given point of time but transactions are
*committed* sequentially.

.. seealso::

   Galera Documentation: Database replication
      https://galeracluster.com/library/documentation/tech-desc-introduction.html

.. variable:: wsrep_protocol_version

Version of the wsrep protocol used.

.. seealso:: `Galera status variable: wsrep_protocol_version
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-protocol-version>`_

.. variable:: wsrep_provider_name

Name of the wsrep provider (usually ``Galera``).

.. seealso:: `Galera status variable: wsrep_provider_name
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-provider-name>`_

.. variable:: wsrep_provider_vendor

Name of the wsrep provider vendor (usually ``Codership Oy``)

.. seealso:: `Galera status variable: wsrep_provider_vendor
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-provider-vendor>`_

.. variable:: wsrep_provider_version

Current version of the wsrep provider.

.. seealso:: `Galera status variable: wsrep_provider_version
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-provider-version>`_

.. variable:: wsrep_ready

This variable shows if node is ready to accept queries. If status is ``OFF``,
almost all queries will fail with ``ERROR 1047 (08S01) Unknown Command`` error
(unless the :variable:`wsrep_on` variable is set to ``0``).

.. seealso:: `Galera status variable: wsrep_ready
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-ready>`_

.. variable:: wsrep_received

Total number of writesets received from other nodes.

.. seealso:: `Galera status variable: wsrep_received
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-received>`_

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

.. seealso:: `Galera status variable: wsrep_replicated
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-replicated>`_

.. variable:: wsrep_replicated_bytes

Total size of replicated writesets. To compute the actual size of bytes sent
over network to cluster peers, multiply the value of this variable by the number
of cluster peers in the given :variable:`network segment <gmcast.segment>`.

.. seealso:: `Galera status variable: wsrep_replicated_bytes
             <https://galeracluster.com/library/documentation/galera-status-variables.html#wsrep-replicated-bytes>`_

