.. _certification:

=======================================
Certification in Percona XtraDB Cluster
=======================================

|Percona XtraDB Cluster| replicates actions executed on one node to all other
nodes in the cluster and make it fast enough to appear as it if is
synchronous (aka virtually synchronous).

There are two main types of actions: DDL and DML. DDL actions are executed
using Total Order Isolation (let's ignore Rolling Schema Upgrade for now) and
DML using normal Galera replication protocol.

.. note::

  This manual page assumes the reader is aware of Total Order Isolation and
  MySQL replication protocol.

DML (``INSERT``/``UPDATE``/``DELETE``) operations effectively change the state
of the database, and all such operations are recorded in |XtraDB| by
registering a unique object identifier (aka key) for each change (an update
or a new addition).

* A transaction can change “n” different data objects. Each such object change
  is recorded in |XtraDB| using a so-call ``append_key`` operation. The
  ``append_key`` operation registers the key of the data object that has
  undergone a change by the transaction. The key for rows can be represented in
  three parts as ``db_name``, ``table_name``, and ``pk_columns_for_table`` (if
  ``pk`` is absent, a hash of the complete row is calculated). In short there
  is quick and short meta information that this transaction has
  touched/modified following rows. This information is passed on as part of the
  write-set for certification to all the nodes of a cluster while the
  transaction is in the commit phase.

* For a transaction to commit it has to pass XtraDB/Galera certification,
  ensuring that transactions don't conflict with any other changes posted on
  the cluster group/channel. Certification will add the keys modified by given
  the transaction to its own central certification vector (CCV), represented by
  ``cert_index_ng``. If the said key is already part of the vector, then
  conflict resolution checks are triggered.

* Conflict resolution traces reference the transaction (that last modified
  this item in cluster group). If this reference transaction is from some other
  node, that suggests the same data was modified by the other node and changes
  of that node have been certified by the local node that is executing the
  check. In such cases, the transaction that arrived later fails to certify.

Changes made to DB objects are bin-logged. This is the same as how |MySQL|
does it for replication with its Master-Slave ecosystem, except that a packet
of changes from a given transaction is created and named as a write-set.

Once the client/user issues a ``COMMIT``, |Percona XtraDB Cluster| will run a
commit hook. Commit hooks ensure following:

* Flush the binary logs.

* Check if the transaction needs replication (not needed for read-only
  transactions like ``SELECT``).

* If a transaction needs a replication, then it invokes a pre_commit hook in
  the Galera ecosystem. During this pre-commit hook, a write-set is written in
  the group channel by a “replicate” operation. All nodes (including the one
  that executed the transaction) subscribes to this group-channel and reads
  the write-set.

* ``gcs_recv_thread`` is first to receive the packet, which is then processed
  through different action handlers.

* Each packet read from the group-channel is assigned an ``id``, which is a
  locally maintained counter by each node in sync with the group. When any new
  node joins the group/cluster, a seed-id for it is initialized to the current
  active id from group/cluster. (There is an inherent assumption/protocol
  enforcement that all nodes read the packet from a channel in same order, and
  that way even though each packet doesn't carry ``id`` information it is
  inherently established using the local maintained ``id`` value).

.. code-block:: bash

  /*  Common situation -
  * increment and assign act_id only for totally ordered actions
  * and only in PRIM (skip messages while in state exchange) */
    rcvd->id = ++group->act_id_;

  [This is an amazing way to solve the problem of the id co-ordination in
  multiple master system, otherwise a node will have to first get an id from
  central system or through a separate agreed protocol and then use it for the
  packet there-by doubling the round-trip time].

What happens if two nodes get ready with their packet at same time?

* Both nodes will be allowed to put the packet on the channel. That means the
  channel will see packets from different nodes queued one-behind-another.

* It is interesting to understand what happens if two nodes modify same set of
  rows. For example:

  .. code-block:: bash

    create -> insert (1,2,3,4)....nodes are in sync till this point.
    node-1: update i = i + 10;
    node-2: update i = i + 100;

    Let's associate transaction-id (trx-id) for an update transaction that
    is executed on node-1 and node-2 in parallel (The real algorithm is bit
    more involved (with uuid + seqno) but conceptually the same so for ease
    we're using trx_id here)

    node-1:
      update action: trx-id=n1x
    node-2:
      update action: trx-id=n2x

Both node packets are added to the channel but the transactions are
conflicting. Let's see which one succeeds. The protocol says: FIRST WRITE WINS.
So in this case, whoever is first to write to the channel will get certified.
Let's say node-2 is first to write the packet and then node-1 makes
immediately after it.

.. note::
  each node subscribes to all packages including its own package. See below
  for details.

Node-2:
  - Will see its own packet and will process it.
  - Then it will see node-1 packet that it tries to certify but fails.

Node-1:
  - Will see node-2 packet and will process it. (Note: InnoDB allows isolation
    and so node-1 can process node-2 packets independent of node-1 transaction
    changes)
  - Then it will see the node-1 packet that it tries to certify but fails.
    (Note even though the packet originated from node-1 it will under-go
    certification to catch cases like thes. This is beauty of listening to own
    events that make consistent processing path even if events are locally
    generated)

The certification protocol will be described using the example from above. As
discussed above, the central certification vector (CCV) is updated to reflect
reference transaction.

Node-2:
  - node-2 sees its own packet for certification, adds it to its local CCV and
    performs certification checks. Once these checks pass it updates the
    reference transaction by setting it to ``n2x``
  - node-2 then gets node-1 packet for certification. Said key is already
    present in CCV with a reference transaction set it to ``n2x``, whereas
    write-set proposes setting it to ``n1x``. This causes a conflict, which in
    turn causes the node-1 originated transaction to fail the certification
    test.

This helps point out a certification failure and the node-1 packet is rejected.

Node-1:
  - node-1 sees node-2 packet for certification, which is then processed, the
    local CCV is updated and the reference transaction is set to ``n2x``
  - Using the same case explained above, node-1 certification also rejects the
    node-1 packet.

This suggests that the node doesn't need to wait for certification to complete,
but just needs to ensure that the packet is written to the channel. The applier
transaction will always win and the local conflicting transaction will be
rolled back.

What happens if one of the nodes has local changes that are not synced with
group?

.. code-block:: bash

  create (id primary key) -> insert (1), (2), (3), (4);
  node-1: wsrep_on=0; insert (5); wsrep_on=1
  node-2: insert(5).
  insert(5) will generate a write-set that will then be replicated to node-1.
  node-1 will try to apply it but will fail with duplicate-key-error, as 5
  already exist.

  XtraDB will flag this as an error, which would eventually cause node-1 to
  shutdown.

With all that in place, how is GTID incremented if all the packets are
processed by all nodes (including ones that are rejected due to certification)?
GTID is incremented only when the transaction passes certification and is ready
for commit. That way errant-packets don't cause GTID to increment. Also, they
don't confuse the group packet ``id`` quoted above with GTID. Without
errant-packets, you may end up seeing these two counters going hand-in-hand,
but they are no way related.
