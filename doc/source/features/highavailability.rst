High Availability
=================

In a basic setup with 3 nodes, |PXC| continues to function
if you take any of the nodes down.
At any point in time, you can shut down any node to perform maintenance
or make configuration changes.
Even in unplanned situations
(like a node crashing or if it becomes unavailable over the network),
the |PXC| continues to work,
and you are able to run queries on working nodes.

If there were changes to data while a node was down,
there are two options that the node may use when it joins the cluster again:

* **State Snapshot Transfer** (SST) is when all data is copied
  from one node to another.

  |PXC| uses :program:`xtrabackup` for SST.

  SST using ``xtrabackup`` does not require the :command:`READ LOCK` command
  for the entire syncing process, only for syncing |.FRM| files
  (the same as with a regular backup).

* **Incremental State Transfer** (IST) is when only incremental changes
  are copied from one node to another.

  Even without locking your cluster in read-only state, SST may be intrusive
  and disrupt the normal operation of your services.
  IST lets you avoid that.
  If a node goes down for a short period,
  it can fetch only those changes that happened while it was down.
  IST is implemented using a caching mechanism on nodes.
  Each node contains a cache, ring-buffer (the size is configurable)
  of last N changes, and the node can transfer part of this cache.
  IST can be done if the amount of changes needed to transfer
  is less than N. If the amount exceeds N, then the joining node must perform SST.

You can monitor the current state of a node using the following command:

.. code-block:: mysql

   SHOW STATUS LIKE 'wsrep_local_state_comment';

When a node is in ``Synced (6)`` state, it is part of the cluster
and ready to handle traffic.

