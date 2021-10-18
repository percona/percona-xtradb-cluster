.. _pxc-crash-recovery:

================================================================================
Crash Recovery
================================================================================

Unlike the standard MySQL replication, a |pxc| cluster acts like one logical
entity, which controls the status and consistency of each node as well as the
status of the whole cluster. This allows maintaining the data integrity more
efficiently than with traditional asynchronous replication without losing safe
writes on multiple nodes at the same time.

However, there are scenarios where the database service can stop with no node
being able to serve requests.

.. contents::
   :local:

.. _pxc-crash-recovery-scenario-1:

Scenario: Node A is gracefully stopped
================================================================================

In a three node cluster (node A, Node B, node C), one node (node A, for example)
is gracefully stopped: for the purpose of maintenance, configuration change,
etc.

In this case, the other nodes receive a "good bye" message from the stopped node
and the cluster size is reduced; some properties like quorum calculation or auto
increment are automatically changed. As soon as node A is started again, it
joins the cluster based on its :variable:`wsrep_cluster_address` variable in
:file:`my.cnf`.

If the writeset cache (:variable:`gcache.size`) on nodes B and/or C still
has all the transactions executed while node A was down, joining is possible via
:term:`IST`. If :term:`IST` is impossible due to missing transactions in donor’s
gcache, the fallback decision is made by the donor and :term:`SST` is started
automatically.

.. _pxc-crash-recovery-scenario-2:

Scenario: Two nodes are gracefully stopped
================================================================================

Similar to :ref:`pxc-crash-recovery-scenario-1`, the cluster size is reduced to
1 --- even the single remaining node C forms the primary component and is able to
serve client requests. To get the nodes back into the cluster, you just need
to start them.

However, when a new node joins the cluster, node C will be switched to the
"Donor/Desynced" state as it has to provide the state transfer at least to the
first joining node. It is still possible to read/write to it during that
process, but it may be much slower, which depends on how large amount of data
should be sent during the state transfer. Also, some load balancers may consider
the donor node as not operational and remove it from the pool. So, it is best to
avoid the situation when only one node is up.

If you restart node A and then node B, you may want to make sure note B does not
use node A as the state transfer donor: node A may not have all the needed
writesets in its gcache. Specify node C node as the donor in your configuration
file and start the `mysql` service:

.. code-block:: bash

   $ systemctl start mysql

.. seealso::

   Galera Documentation: wsrep_sst_donor option
      https://galeracluster.com/library/documentation/mysql-wsrep-options.html#wsrep-sst-donor

.. _pxc-crash-recovery-scenario-3:

Scenario: All three nodes are gracefully stopped
================================================================================

The cluster is completely stopped and the problem is to initialize it again. It
is important that a PXC node writes its last executed position to the
:file:`grastate.dat` file.

By comparing the seqno number in this file, you can see which is the most
advanced node (most likely the last stopped). The cluster must be bootstrapped
using this node, otherwise nodes that had a more advanced position will have to
perform the full :term:`SST` to join the cluster initialized from the less
advanced one. As a result, some transactions will be lost). To bootstrap the
first node, invoke the startup script like this:

.. code-block:: bash

   $ systemctl start mysql@bootstrap.service

.. note::

   Even though you bootstrap from the most advanced node, the other
   nodes have a lower sequence number. They will still have to join via the full :term:`SST`
   because the *Galera Cache* is not retained on restart.

   For this reason, it is recommended to stop writes to the cluster *before* its
   full shutdown, so that all nodes can stop at the same position. See also
   :variable:`pc.recovery`.

.. _pxc-crash-recovery-scenario-4:

Scenario: One node disappears from the cluster
================================================================================

This is the case when one node becomes unavailable due to power outage, hardware
failure, kernel panic, mysqld crash, :command:`kill -9` on mysqld pid, etc.

Two remaining nodes notice the connection to node A is down and start trying to
re-connect to it. After several timeouts, node A is removed from the
cluster. The quorum is saved (2 out of 3 nodes are up), so no service disruption
happens. After it is restarted, node A joins automatically (as described in
:ref:`pxc-crash-recovery-scenario-1`).

.. _pxc-crash-recovery-scenario-5:

Scenario: Two nodes disappear from the cluster
================================================================================

Two nodes are not available and the remaining node (node C) is not able to form
the quorum alone. The cluster has to switch to a non-primary mode, where MySQL
refuses to serve any SQL queries. In this state, the :command:`mysqld` process
on node C is still running and can be connected to but any statement related to
data fails with an error

.. code-block:: mysql

   > SELECT * FROM test.sbtest1;
   ERROR 1047 (08S01): WSREP has not yet prepared node for application use

Reads are possible until node C decides that it cannot access node A and
node B. New writes are forbidden.

As soon as the other nodes become available, the cluster is formed again
automatically. If node B and node C were just network-severed from
node A, but they can still reach each other, they will keep functioning as
they still form the quorum.

If node A and node B crashed, you need to enable the primary component on
node C manually, before you can bring up node A and node B. The command to do
this is:

.. code-block:: mysql

   > SET GLOBAL wsrep_provider_options='pc.bootstrap=true';

This approach only works if the other nodes are down before doing that!
Otherwise, you end up with two clusters having different data.


.. admonition:: Cross Reference

   :ref:`add-node`

.. _pxc-crash-recovery-scenario-6:

Scenario: All nodes went down without a proper shutdown procedure
================================================================================

This scenario is possible in case of a datacenter power failure or when hitting
a MySQL or Galera bug. Also, it may happen as a result of data consistency being
compromised where the cluster detects that each node has different data. The
:file:`grastate.dat` file is not updated and does not contain a valid sequence
number (seqno). It may look like this:

.. code-block:: text

   $ cat /var/lib/mysql/grastate.dat
   # GALERA saved state
   version: 2.1
   uuid: 220dcdcb-1629-11e4-add3-aec059ad3734
   seqno: -1
   safe_to_bootstrap: 0

In this case, you cannot be sure that all nodes are consistent with each
other. We cannot use `safe_to_bootstrap` variable to determine the node that has
the last transaction committed as it is set to **0** for each node. An attempt
to bootstrap from such a node will fail unless you start ``mysqld`` with the
:option:`--wsrep-recover` parameter:

.. code-block:: bash

   $ mysqld --wsrep-recover

Search the output for the line that reports the recovered position after the
node UUID (**1122** in this case):

.. code-block:: text

   ...
   ... [Note] WSREP: Recovered position: 220dcdcb-1629-11e4-add3-aec059ad3734:1122
   ...

The node where the recovered position is marked by the greatest number is the
best bootstrap candidate. In its :file:`grastate.dat` file, set the
`safe_to_bootstrap` variable to **1**. Then, bootstrap from this node.

.. note::

   After a shutdown, you can boostrap from the node which is marked as safe in the
   :file:`grastate.dat` file.

   .. code-block:: text

      ...
      safe_to_bootstrap: 1
      ...

.. seealso::

   Galera Documentation
      `Introducing the Safe-To-Bootstrap feature in Galera Cluster
      <https://galeracluster.com/2016/11/introducing-the-safe-to-bootstrap-feature-in-galera-cluster/>`_

-----

In recent Galera versions, the option :variable:`pc.recovery` (enabled by
default) saves the cluster state into a file named :file:`gvwstate.dat` on each
member node. As the name of this option suggests (pc – primary component), it
saves only a cluster being in the PRIMARY state. An example content of : file
may look like this:

.. code-block:: bash

   cat /var/lib/mysql/gvwstate.dat
   my_uuid: 76de8ad9-2aac-11e4-8089-d27fd06893b9
   #vwbeg
   view_id: 3 6c821ecc-2aac-11e4-85a5-56fe513c651f 3
   bootstrap: 0
   member: 6c821ecc-2aac-11e4-85a5-56fe513c651f 0
   member: 6d80ec1b-2aac-11e4-8d1e-b2b2f6caf018 0
   member: 76de8ad9-2aac-11e4-8089-d27fd06893b9 0
   #vwend

We can see a three node cluster with all members being up. Thanks to this new
feature, the nodes will try to restore the primary component once all the
members start to see each other. This makes the PXC cluster automatically
recover from being powered down without any manual intervention! In the logs we
will see:

.. _pxc-crash-recovery-scenario-7:

Scenario: The cluster loses its primary state due to split brain
================================================================================

For the purpose of this example, let’s assume we have a cluster that consists of
an even number of nodes: six, for example. Three of them are in one location
while the other three are in another location and they lose network
connectivity.  It is best practice to avoid such topology: if you cannot have an
odd number of real nodes, you can use an additional arbitrator (garbd) node or
set a higher pc.weight to some nodes. But when the split brain happens any way,
none of the separated groups can maintain the quorum: all nodes must stop
serving requests and both parts of the cluster will be continuously trying to
re-connect.

If you want to restore the service even
before the network link is restored, you can make one of the groups primary
again using the same command as described in :ref:`pxc-crash-recovery-scenario-5`

.. code-block:: mysql

    > SET GLOBAL wsrep_provider_options='pc.bootstrap=true';

After this, you are able to work on the manually restored part of the cluster,
and the other half should be able to automatically re-join using :term:`IST` as
soon as the network link is restored.

.. warning::

   If you set the bootstrap option on both the separated parts, you will end up
   with two living cluster instances, with data likely diverging away from each
   other. Restoring a network link in this case will not make them re-join until
   the nodes are restarted and members specified in configuration
   file are connected again.

   Then, as the Galera replication model truly cares about data consistency:
   once the inconsistency is detected, nodes that cannot execute row change
   statement due to a data difference – an emergency shutdown will be performed and the only
   way to bring the nodes back to the cluster is via the full :term:`SST`

.. admonition:: Based on material from **Percona Database Performance Blog**

   This article is based on the blog post *Galera replication - how to recover a PXC cluster by *Przemysław Malkowski*:
      https://www.percona.com/blog/2014/09/01/galera-replication-how-to-recover-a-pxc-cluster/

.. include:: ../.res/replace.txt
