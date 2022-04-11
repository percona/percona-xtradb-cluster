.. _failover:

================
Cluster Failover
================

Cluster membership is determined simply by
which nodes are connected to the rest of the cluster;
there is no configuration setting
explicitly defining the list of all possible cluster nodes.
Therefore, every time a node joins the cluster,
the total size of the cluster is increased
and when a node leaves (gracefully) the size is decreased.

The size of the cluster is used to determine
the required votes to achieve :term:`quorum`.
A quorum vote is done when a node or nodes are suspected
to no longer be part of the cluster (they do not respond).
This no response timeout is the :option:`evs.suspect_timeout` setting
in the :variable:`wsrep_provider_options` (default 5 sec),
and when a node goes down ungracefully,
write operations will be blocked on the cluster
for slightly longer than that timeout.

Once a node (or nodes) is determined to be disconnected,
then the remaining nodes cast a quorum vote,
and if the majority of nodes from before the disconnect
are still still connected, then that partition remains up.
In the case of a network partition,
some nodes will be alive and active on each side of the network disconnect.
In this case, only the quorum will continue.
The partition(s) without quorum will change to non-primary state.

As a consequence,
it's not possible to have safe automatic failover in a 2 node cluster,
because failure of one node
will cause the remaining node to become non-primary.
Moreover, any cluster with an even number of nodes
(say two nodes in two different switches)
have some possibility of a *split brain* situation,
when neither partition is able to retain quorum
if connection between them is lost,
and so they both become non-primary.

Therefore, for automatic failover, the *rule of 3s* is recommended.
It applies at various levels of your infrastructure,
depending on how far the cluster is spread out
to avoid single points of failure. For example:

 * A cluster on a single switch should have 3 nodes
 * A cluster spanning switches should be spread evenly
   across at least 3 switches
 * A cluster spanning networks should span at least 3 networks
 * A cluster spanning data centers should span at least 3 data centers

These rules will prevent split brain situations
and ensure automatic failover works correctly.

Using an arbitrator
===================

If it is too expensive to add a third node, switch, network, or datacenter,
you should use an arbitrator.
An arbitrator is a voting member of the cluster
that can receive and relay replication,
but it does not persist any data,
and runs its own daemon instead of ``mysqld``.  
Placing even a single arbitrator in a 3rd location
can add split brain protection to a cluster
that is spread across only two nodes/locations.

Recovering a Non-Primary cluster
================================

It is important to note that the *rule of 3s* applies
only to automatic failover.
In the event of a 2-node cluster
(or in the event of some other outage that leaves a minority of nodes active),
the failure of one node will cause the other to become non-primary
and refuse operations.
However, you can recover the node from non-primary state
using the following command:

.. code-block:: mysql

  SET GLOBAL wsrep_provider_options='pc.bootstrap=true';

This will tell the node (and all nodes still connected to its partition)
that it can become a primary cluster.
However, this is only safe to do when you are sure there is no other partition
operating in primary as well,
or else Percona XtraDB Cluster will allow those two partitions to diverge
(and you will end up with two databases
that are impossible to re-merge automatically).  

For example, assume there are two data centers,
where one is primary and one is for disaster recovery,
with an even number of nodes in each.
When an extra arbitrator node is run only in the primary data center,
the following high availability features will be available:

* Auto-failover of any single node or nodes
  within the primary or secondary data center

* Failure of the secondary data center would not cause the primary to go down
  (because of the arbitrator)

* Failure of the primary data center would leave the secondary
  in a non-primary state.

* If a disaster-recovery failover has been executed,
  you can tell the secondary data center to bootstrap itself
  with a single command,
  but disaster-recovery failover remains in your control.  

Other Reading
=============

* `PXC - Failure Scenarios with only 2 nodes <http://www.mysqlperformanceblog.com/2012/07/25/percona-xtradb-cluster-failure-scenarios-with-only-2-nodes/>`_
