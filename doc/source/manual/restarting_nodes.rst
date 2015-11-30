.. _restarting-nodes:

==============================
 Restarting the cluster nodes
==============================

Restarting a cluster node is as simple as shutting down and restarting standard mysql. The node should gracefully leave the cluster (and the total vote count for :term:`quorum` should decrement). When it rejoins, the node should receive an :term:`IST` of changes since it left so it can get back in sync. If the set of changes needed for IST are not found in the :file:`gcache` file on any other node in the entire cluster, then an :term:`SST` will be performed instead. Therefore, restarting cluster nodes for rolling configuration changes or software upgrades should be fairly trivial to accomplish from the cluster's perspective.  

.. note::
 
  If a configuration change is done and mysql restarted and that change happens to contain a misspelling or some other mistake that prevents mysqld from loading, Galera will generally decide to drop its state and an SST will be forced for that node.

