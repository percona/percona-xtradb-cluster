.. _intro:

==============================
 About Percona XtraDB Cluster
==============================

*Percona XtraDB Cluster* is open-source, free |MySQL| High Availability software 

General introduction
====================

1. The Cluster consists of Nodes. Recommended configuration is to have at least 3 nodes, but you can make it running with 2 nodes as well.
2. Each Node is regular |MySQL| / |Percona Server| setup. The point is that you can convert your existing MySQL / Percona Server into Node and roll Cluster using it as a base. Or otherwise – you can detach Node from Cluster and use it as just a regular server.
3. Each Node contains the full copy of data. That defines XtraDB Cluster behavior in many ways. And obviously there are benefits and drawbacks.

.. image:: _static/cluster-diagram1.png

Benefits of such approach:
 * When you execute a query, it is executed locally on the node. All data is available locally, no need for remote access.
 * No central management. You can loose any node at any point of time, and the cluster will continue to function.
 * Good solution for scaling a read workload. You can put read queries to any of the nodes.

Drawbacks:
 * Overhead of joining new node. The new node has to copy full dataset from one of existing nodes. If it is 100GB, it copies 100GB.
 * This can’t be used as an effective write scaling solution. There might be some improvements in write throughput when you run write traffic to 2 nodes vs all traffic to 1 node, but you can't expect a lot. All writes still have to go on all nodes.
 * You have several duplicates of data, for 3 nodes – 3 duplicates.

What is core difference Percona XtraDB Cluster from MySQL Replication ?
=======================================================================

Let's take look into the well known CAP theorem for Distributed systems.
Characteristics of Distributed systems:

 C - Consistency (all your data is consistent on all nodes),

 A - Availability  (your system is AVAILABLE to handle requests in case of  failure of one or several nodes ),

 P - Partitioning  tolerance (in case of inter-node connection failure, each node is still available to handle requests).


CAP theorem says that each Distributed system can have only two out of these three.

MySQL replication has: Availability and Partitioning tolerance.

Percona XtraDB Cluster has: Consistency and Availability.

That is MySQL replication does not guarantee Consistency of your data, while Percona XtraDB Cluster provides data Consistency. (And yes, Percona XtraDB Cluster looses Partitioning tolerance property).

Components
==========

|Percona XtraDB Cluster| is based on `Percona Server with XtraDB <http://www.percona.com/software/percona-server/>`_
and includes `Write Set Replication patches <https://launchpad.net/codership-mysql>`_.
It uses  the  `Galera library <https://launchpad.net/galera>`_, version 3.x, 
a generic Synchronous Multi-Master replication plugin for transactional applications. 

Galera library is developed by `Codership Oy <http://www.codership.com/>`_.

Galera 3.x supports such new features as:
 * Incremental State Transfer (|IST|), especially useful for WAN deployments,
 * RSU, Rolling Schema Update. Schema change does not block operations against table.

