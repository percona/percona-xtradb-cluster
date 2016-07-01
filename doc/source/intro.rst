.. _intro:

============================
About Percona XtraDB Cluster
============================

|PXC| is a fully open-source high-availability solution for MySQL.

A *cluster* consists of *nodes*,
where each node contains the same set of data synchronized accross nodes.
The recommended configuration is to have at least 3 nodes,
but you can have 2 nodes as well.
Each node is a regular MySQL Server instance
(for example, Percona Server).
You can convert an existing MySQL Server instance to a node
and run the cluster using this node as a base.
You can also detach any node from the cluster
and use it as a regular MySQL Server instance.

.. image:: _static/cluster-diagram1.png

Benefits:

* When you execute a query,
  it is executed locally on the node.
  All data is available locally, no need for remote access.

* No central management.
  You can loose any node at any point of time,
  and the cluster will continue to function without any data loss.

* Good solution for scaling a read workload.
  You can put read queries to any of the nodes.

Drawbacks:

* Overhead of provisioning new node.
  When you add a new node,
  it has to copy the full data set from one of existing nodes.
  If it is 100GB, it copies 100GB.

* This can't be used as an effective write scaling solution.
  There might be some improvements in write throughput
  when you run write traffic to 2 nodes versus all traffic to 1 node,
  but you can't expect a lot.
  All writes still have to go on all nodes.

* You have several duplicates of the data, for 3 nodes you have 3 duplicates.

Components
==========

|PXC| is based on `Percona Server`_ running with the `XtraDB`_ storage engine.
It uses the `Galera library <https://github.com/percona/galera>`_,
which is an implementation of the write set replication (wsrep) API
developed by `Codership Oy <http://www.galeracluster.com/>`_.

