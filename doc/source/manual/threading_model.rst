.. _threading_model:

======================================
Percona XtraDB Cluster threading model
======================================

|Percona XtraDB Cluster| (PXC) creates a different set of threads to service
its operations. These threads are in addition to existing |MySQL| threads.
There are three main groups of threads:

Applier thread(s)
=================

Applier threads are meant to apply write-sets that the node receives from other
nodes (through cluster). (write-message is directed through ``gcv_recv_thread``
.)

The number of applier threads is controlled by using the
:variable:`wsrep_slave_threads` configuration. (The default value is ``1``, so
at least one wsrep applier thread exists to process the request.)

Applier threads wait for an event, and once it gets the event, it applies it
using the normal slave apply routine path (and relays the log info apply path
with wsrep-customization). In short these threads are kind of similar to slave
worker threads (but not exactly the same).

Coordination is achieved using Apply and Commit Monitor. A transaction passes
through two important states: ``APPLY -> COMMIT``. Every transaction registers
itself with an apply monitor, where its apply order gets defined. So all
transactions with apply-order sequence number (``seqno``) that is less than
this transaction should be applied before applying this transaction. The same
is done for commit as well (``last_left >= trx_.depends_seqno()``).

Rollback thread
===============

Why do we need a rollback thread (there is only one rollback thread)?

* Transactions executed in parallel can conflict and may need to rollback.
  A rollback thread helps achieve this.

* Applier transactions always take priority over local transactions.
  This is natural, as applier transactions have been accepted by the cluster,
  and some of the nodes may have already applied them.
  Local conflicting transactions still has a window to rollback.

All the transactions that need to be rolled back are added to the rollback
queue, and the rollback thread is notified. The rollback thread will then
iterate over the queue and perform rollback operations.

If a transaction is active on a node, and a node receives a
transaction-write-set from cluster-group that conflicts with the local active
transaction, then such local transactions are always treated as a victim
transaction to rollback. Transactions can be in a commit state or an execution
stage when the conflict arises. Local transactions in the execution stage are
forcibly killed so that the waiting applier transaction is allowed to proceed.
Local transactions in the commit stage fail with a certification error.

Other threads
=============

Service thread
--------------

This thread is created during boot-up and used to perform auxiliary services.
It has two main functions:

* It releases the GCache buffer after the cached write-set is purged up to the
  said level.

* It notifies the cluster group that the respective node has committed a
  transaction up to this level. Each node maintains some basic status info
  about other nodes in the cluster. On receiving the message, the information
  is updated in this local metadata.

``gcs_recv_thread``
-------------------

This thread is the first one to see all the messages received in a group.

It will try to assign actions against each message it receives. It adds these
messages to a central FIFO queue, which are then processed by the Applier
thread(s). Messages can include different operation like state change,
configuration update, flow-control, etc. One important action is about
processing write-set, which actually is applying transactions to database
objects.

gcomm connection thread
-----------------------

There is also a gcomm connection thread ``GCommConn::run_fn`` which is used to
co-ordinate the low-level group communication activity. Think of it as a
black-box meant for communication.

Action-based threads
--------------------

Besides the above, some threads are created on a needed basis. SST creates
threads for donor and joiner (which eventually forks out a child process to
host the needed SST script), IST creates receiver and async sender threads,
PageStore creates a background thread for removing the files that were created.
If the checksum is enabled and the replicated write-set is big enough, the
checksum is done as part of a separate thread.
