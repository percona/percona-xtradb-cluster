.. _failover:

==================
 Cluster Failover
==================

Cluster membership is determined simply by which nodes are connected to the rest of the cluster; there is no configuration setting explicitly defining the list of all possible cluster nodes. Therefore, every time a node joins the cluster, the total size of the cluster is increased and when a node leaves (gracefully) the size is decreased.

The size of the cluster is used to determine the required votes to achieve :term:`quorum`. A quorum vote is done when a node or nodes are suspected to no longer be part of the cluster (they do not respond). This no response timeout is the :option:`evs.suspect_timeout` setting in the :variable:`wsrep_provider_options` (default 5 sec), and when a node goes down ungracefully, write operations will be blocked on the cluster for slightly longer than that timeout.

Once the node (or nodes) is determined to be disconnected, then the remaining nodes cast a quorum vote and if a majority remain from the total nodes connected from before the disconnect, then that partition remains up. In the case of a network partition, some nodes will be alive and active on each side of the network disconnect. In this case, only the quorum will continue, the partition(s) without quorum will go to the non-Primary state.

Because of this, it's not possible to safely have automatic failover in a 2 node cluster, because the failure of one node will cause the remaining node to go non-Primary. Further, cluster with an even number of nodes (say two nodes in two different switches) have some possibility of a split brain condition when if network connectivity is lost between the two partitions, neither would retain quorum, and so both would go to Non-Primary.
Therefore: for automatic failover, the "rule of 3s" is recommended. It applies at various levels of infrastructure, depending on how far cluster is spread out to avoid single points of failure. For example:

 * A cluster on a single switch should have 3 nodes 
 * A cluster spanning switches should be spread evenly across at least 3 switches
 * A cluster spanning networks should be span at least 3 networks
 * A cluster spanning data centers should span at least 3 data centers

This is all to prevent split brain situations from preventing automatic failover from working. 

Using an arbitrator
===================

In the case where the expense of adding the third node/switch/datacenter/etc. above is prohibitively high, using an arbitrator node may be a viable alternative.  
An arbitrator is a voting member of the cluster which does receive and can relay replication, but it does not persist any data and does not run mysqld, it is a separate daemon.  
Placing even a single arbitrator in a 3rd location can add split brain protection to a cluster that is spread only across two nodes/locations.

Recovering a Non-Primary cluster
================================

It is important to note that the rule of 3s only applies for automatic failover. In the event of a 2 node cluster (or in the event of some other outage that leaves a minority of nodes active), the failure of one will cause the other to shift to non-Primary and refuse operations. However, that is a recoverable state via a manual command: :: 

  SET GLOBAL wsrep_provider_options='pc.bootstrap=true';

This will tell the node (and all nodes still connected to its partition) that it can become a Primary cluster. However, this is only safe to do when you are sure there exists no other partition operating in Primary as well, or else |Percona XtraDB Cluster| will allow those two partitions to diverge (and now you have two databases that are impossible to remerge automatically).  
For example, if there are two data centers where one is primary and one is for Disaster Recovery, with even number of nodes in each. When an extra arbitrator node is run only in the Primary data center, the following High Availability features will be available:

 * Auto-failover of any single node or nodes within the Primary or Secondary data center
 * Failure of the secondary data center would not cause Primary to go down (because of the arbitrator)
 * Failure of the primary data center would leave the secondary in a non-Primary state. 
 * If a disaster recovery failover has been executed. In this case you could simply tell the secondary data center to bootstrap itself with a single command, but disaster recovery failover remains in your control.  

Other Reading
=============

* `PXC - Failure Scenarios with only 2 nodes <http://www.mysqlperformanceblog.com/2012/07/25/percona-xtradb-cluster-failure-scenarios-with-only-2-nodes/>`_
