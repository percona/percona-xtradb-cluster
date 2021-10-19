.. _threading_model:

======================================
Percona XtraDB Cluster threading model
======================================

Percona XtraDB Cluster creates a set of threads to service its operations,
which are not related to existing MySQL threads.
There are three main groups of threads:

.. contents::
   :depth: 1

Applier threads
===============

Applier threads apply write-sets that the node receives from other nodes.
Write messages are directed through ``gcv_recv_thread``.

The number of applier threads is controlled
using the :variable:`wsrep_slave_threads` variable.
The default value is ``1``,
which means at least one wsrep applier thread exists to process the request.

Applier threads wait for an event, and once it gets the event,
it applies it using normal replica apply routine path,
and relays the log info apply path with wsrep-customization.
These threads are similar to replica worker threads (but not exactly the same).

Coordination is achieved using *Apply and Commit Monitor*.
A transaction passes through two important states: ``APPLY`` and ``COMMIT``.
Every transaction registers itself with an apply monitor,
where its apply order is defined.
So all transactions with apply order sequence number (``seqno``)
of less than this transaction's sequence number,
are applied before applying this transaction.
The same is done for commit as well (``last_left >= trx_.depends_seqno()``).

Rollback thread
===============

There is only one rollback thread to perform rollbacks in case of conflicts.

* Transactions executed in parallel can conflict and may need to roll back.

* Applier transactions always take priority over local transactions.
  This is natural, as applier transactions have been accepted by the cluster,
  and some of the nodes may have already applied them. Local conflicting
  transactions still have a window to rollback.

All the transactions that need to be rolled back
are added to the rollback queue, and the rollback thread is notified.
The rollback thread then iterates over the queue
and performs rollback operations.

If a transaction is active on a node,
and a node receives a transaction write-set from the cluster group
that conflicts with the local active transaction,
then such local transactions are always treated
as a victim transaction to roll back.

Transactions can be in a commit state
or an execution stage when the conflict arises.
Local transactions in the execution stage are forcibly killed
so that the waiting applier transaction is allowed to proceed.
Local transactions in the commit stage fail with a certification error.

Other threads
=============

Service thread
--------------

This thread is created during boot-up and used to perform auxiliary services.
It has two main functions:

* It releases the GCache buffer
  after the cached write-set is purged up to the said level.

* It notifies the cluster group
  that the respective node has committed a transaction up to this level.
  Each node maintains some basic status info about other nodes in the cluster.
  On receiving the message, the information is updated in this local metadata.

Receiving thread
----------------

The ``gcs_recv_thread`` thread is the first one to see all the messages
received in a group.

It will try to assign actions against each message it receives.
It adds these messages to a central FIFO queue,
which are then processed by the Applier threads.
Messages can include different operations like state change,
configuration update, flow-control, and so on.

One important action is processing a write-set,
which actually is applying transactions to database objects.

Gcomm connection thread
-----------------------

The gcomm connection thread ``GCommConn::run_fn``
is used to co-ordinate the low-level group communication activity.
Think of it as a black box meant for communication.

Action-based threads
--------------------

Besides the above, some threads are created on a needed basis.
SST creates threads for donor and joiner
(which eventually forks out a child process to host the needed SST script),
IST creates receiver and async sender threads,
PageStore creates a background thread for removing the files that were created.

If the checksum is enabled and the replicated write-set is big enough,
the checksum is done as part of a separate thread.

