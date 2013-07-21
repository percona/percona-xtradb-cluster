High Availability
=================

In a basic setup with 3 nodes, the |Percona XtraDB Cluster| will continue to function if you take any of the nodes down.
At any point in time you can shutdown any Node to perform maintenance or make 
configuration changes. Even in unplanned situations like Node crash or if it 
becomes unavailable over the network, the Cluster will continue to work and you'll be able 
to run queries on working nodes.

In case there were changes to data while node was down, there are two options that Node may use when it joins the cluster: State Snapshot Transfer:  (SST) and Incremental State Transfer (IST).

* SST is the full copy of data from one node to another. It's used when a new node joins the cluster, it has to transfer data from existing node. There are three methods of SST available in Percona XtraDB Cluster: :program:`mysqldump`, :program:`rsync` and :program:`xtrabackup`. The downside of `mysqldump` and `rsync` is that your cluster becomes *READ-ONLY* while data is being copied from one node to another (SST applies :command:`FLUSH TABLES WITH READ LOCK` command). Xtrabackup SST does not require :command:`READ LOCK` for the entire syncing process, only for syncing |.FRM| files (the same as with regular backup).

* Even with that, SST may be intrusive, that`s why there is IST mechanism. If you put your node down for a short period of time and then start it, the node is able to fetch only those changes made during the period it was down. This is done using caching mechanism on nodes. Each node contains a cache, ring-buffer, (the size is configurable) of last N changes, and the node is able to transfer part of this cache. Obviously, IST can be done only if the amount of changes needed to transfer is less than N. If it exceeds N, then the joining node has to perform SST.

You can monitor current state of Node by using

.. code-block:: mysql

   SHOW STATUS LIKE 'wsrep_local_state_comment';

When it is `Synced (6)`, the node is ready to handle traffic.
