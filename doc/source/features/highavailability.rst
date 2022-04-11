High Availability
=================

In a basic setup with 3 nodes, Percona XtraDB Cluster will continue to function
if you take any of the nodes down.
At any point in time, you can shut down any node to perform maintenance
or make configuration changes.
Even in unplanned situations
(like a node crashing or if it becomes unavailable over the network),
the Percona XtraDB Cluster will continue to work
and you'll be able to run queries on working nodes.

If there were changes to data while a node was down,
there are two options that the node may use when it joins the cluster again:

* **State Snapshot Transfer** (SST) is when all data is copied
  from one node to another.

  SST is usually used when a new node joins the cluster
  and receives all data from an existing node.
  Percona XtraDB Cluster uses :program:`xtrabackup` for SST.

  SST using ``xtrabackup`` does not require the :command:`READ LOCK` command
  for the entire syncing process, only for syncing |.FRM| files
  (the same as with a regular backup).

* **Incremental State Transfer** (IST) is when only incremental changes
  are copied from one node to another.

  Even without locking your cluster in read-only state, SST may be intrusive
  and disrupt normal operation of your services.
  IST lets you avoid that.
  If a node goes down for a short period of time,
  it can fetch only those changes that happened while it was down.
  IST is implemeted using a caching mechanism on nodes.
  Each node contains a cache, ring-buffer (the size is configurable)
  of last N changes, and the node is able to transfer part of this cache.
  Obviously, IST can be done only if the amount of changes needed to transfer
  is less than N. If it exceeds N, then the joining node has to perform SST.

You can monitor the current state of a node using the following command:

.. code-block:: mysql

   SHOW STATUS LIKE 'wsrep_local_state_comment';

When a node is in ``Synced (6)`` state, it is part of the cluster
and ready to handle traffic.

