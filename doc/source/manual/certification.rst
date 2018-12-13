.. _certification:

=======================================
Certification in Percona XtraDB Cluster
=======================================

|PXC| replicates actions executed on one node
to all other nodes in the cluster,
and makes it fast enough to appear
as if it is synchronous (*virtually synchronous*).

The following types of actions exist:

* DDL actions are executed using *Total Order Isolation* (TOI).
  We can ignore *Rolling Schema Upgrades* (ROI).

* DML actions are executed using normal Galera replication protocol.

.. note::

  This manual page assumes the reader is aware of TOI
  and MySQL replication protocol.

DML (``INSERT``, ``UPDATE``, and ``DELETE``) operations
effectively change the state of the database,
and all such operations are recorded in |XtraDB|
by registering a unique object identifier (key)
for each change (an update or a new addition).

* A transaction can change an arbitrary number of different data objects.
  Each such object change is recorded in |XtraDB|
  using an ``append_key`` operation.
  An ``append_key`` operation registers the key of the data object
  that has undergone change by the transaction.
  The key for rows can be represented in three parts as
  ``db_name``, ``table_name``, and ``pk_columns_for_table``
  (if ``pk`` is absent, a hash of the complete row is calculated).

  This ensures that there is quick and short meta information
  about the rows that this transaction has touched or modified.
  This information is passed on as part of the write-set for certification
  to all the nodes in the cluster while the transaction is in the commit phase.

* For a transaction to commit, it has to pass XtraDB/Galera certification,
  ensuring that transactions don't conflict with any other changes
  posted on the cluster group/channel.
  Certification will add the keys modified by a given transaction
  to its own central certification vector (CCV),
  represented by ``cert_index_ng``.
  If the said key is already part of the vector,
  then conflict resolution checks are triggered.

* Conflict resolution traces the reference transaction
  (that last modified this item in the cluster group).
  If this reference transaction is from some other node,
  that suggests the same data was modified by the other node,
  and changes of that node have been certified by the local node
  that is executing the check.
  In such cases, the transaction that arrived later fails to certify.

Changes made to database objects are bin-logged.
This is similar to how |MySQL| does it for replication
with its Master-Slave ecosystem,
except that a packet of changes from a given transaction
is created and named as a write-set.

Once the client/user issues a ``COMMIT``,
|PXC| will run a commit hook.
Commit hooks ensure the following:

* Flush the binary logs.

* Check if the transaction needs replication
  (not needed for read-only transactions like ``SELECT``).

* If a transaction needs replication,
  then it invokes a pre-commit hook in the Galera ecosystem.
  During this pre-commit hook,
  a write-set is written in the group channel by a *replicate* operation.
  All nodes (including the one that executed the transaction)
  subscribe to this group-channel and read the write-set.

* ``gcs_recv_thread`` is the first to receive the packet,
  which is then processed through different action handlers.

* Each packet read from the group-channel is assigned an ``id``,
  which is a locally maintained counter by each node in sync with the group.
  When any new node joins the group/cluster,
  a seed-id for it is initialized to the current active id from group/cluster.

  There is an inherent assumption/protocol enforcement
  that all nodes read the packet from a channel in the same order,
  and that way even though each packet doesn't carry ``id`` information,
  it is inherently established using the locally maintained ``id`` value.

Common Situation
================

The following example shows what happens in a common situation.
``act_id`` is incremented and assigned only for totally ordered actions,
and only in primary state (skip messages while in state exchange). ::

   rcvd->id = ++group->act_id_;

.. note:: This is an amazing way to solve the problem
   of the id coordination in multi-master systems.
   Otherwise a node will have to first get an id from central system
   or through a separate agreed protocol,
   and then use it for the packet, thereby doubling the round-trip time.

Conflicts
=========

The following happens if two nodes get ready with their packet at same time:

* Both nodes will be allowed to put the packet on the channel.
  That means the channel will see packets
  from different nodes queued one behind another.

* The following example shows what happens
  if two nodes modify same set of rows.
  Nodes are in sync until this point::

    create -> insert (1,2,3,4)

  * Node 1: ``update i = i + 10;``
  * Node 2: ``update i = i + 100;``

  Let's associate transaction ID (``trx-id``) for an update transaction
  that is executed on Node 1 and Node 2 in parallel.
  Although the real algorithm is more involved (with ``uuid`` + ``seqno``),
  it is conceptually the same, so we are using ``trx_id``.

  * Node 1: ``update action: trx-id=n1x``
  * Node 2: ``update action: trx-id=n2x``

  Both node packets are added to the channel,
  but the transactions are conflicting.
  The protocol says: FIRST WRITE WINS.

  So in this case, whoever is first to write to the channel will get certified.
  Let's say Node 2 is first to write the packet,
  and then Node 1 makes changes immediately after it.

  .. note:: Each node subscribes to all packages, including its own package.

  * Node 2 will see its own packet and will process it.
    Then it will see the packet from Node 1, try to certify it, and fail.

  * Node 1 will see the packet from Node 2 and will process it.

    .. note:: InnoDB allows isolation, so Node 1 can process packets from Node 2
       independent of Node 1 transaction changes

    Then Node 1 will see its own packet, try to certify it, and fail.

    .. note:: Even though the packet originated from Node 1,
       it will undergo certification to catch cases like these.

Resolving Certification Conflicts
=================================

The certification protocol can be described using the previous example.
The central certification vector (CCV) is updated
to reflect reference transaction.

* Node 2 sees its own packet for certification,
  adds it to its local CCV and performs certification checks.
  Once these checks pass, it updates the reference transaction
  by setting it to ``n2x``.

  Node 2 then gets the packet from Node 1 for certification.
  The packet key is already present in CCV,
  with the reference transaction set it to ``n2x``,
  whereas write-set proposes setting it to ``n1x``.
  This causes a conflict, which in turn causes the transaction from Node 1
  to fail the certification test.

* Node 1 sees the packet from Node 2 for certification,
  which is then processed, the local CCV is updated,
  and the reference transaction is set to ``n2x``.

  Using the same case as explained above, Node 1 certification
  also rejects the packet from Node 1.

This suggests that the node doesn't need to wait for certification to complete,
but just needs to ensure that the packet is written to the channel.
The applier transaction will always win
and the local conflicting transaction will be rolled back.

The following example shows what happens
if one of the nodes has local changes that are not synced with the group: ::

  create (id primary key) -> insert (1), (2), (3), (4);
  node-1: wsrep_on=0; insert (5); wsrep_on=1
  node-2: insert(5).

The ``insert(5)`` statement will generate a write-set
that will then be replicated to Node 1.
Node 1 will try to apply it but will fail with ``duplicate-key-error``,
because 5 already exist.

XtraDB will flag this as an error,
which would eventually cause Node 1 to shutdown.

Incrementing GTID
=================

GTID is incremented only when the transaction passes certification,
and is ready for commit.
That way errant packets don't cause GTID to increment.

Also, group packet ``id`` is not confused with GTID.
Without errant packets,
it may seem that these two counters are the same,
but they are not related.

