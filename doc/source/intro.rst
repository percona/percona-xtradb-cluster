.. _intro:

============================
About Percona XtraDB Cluster
============================

Percona XtraDB Cluster_ is a fully open-source high-availability solution for MySQL.  It
integrates Percona Server for MySQL_ and Percona XtraBackup_ with the Galera_ library to enable synchronous
multi-source replication.

A *cluster* consists of *nodes*, where each node contains the same set of data
synchronized accross nodes.  The recommended configuration is to have at least 3
nodes, but you can have 2 nodes as well.  Each node is a regular MySQL Server
instance (for example, Percona Server).  You can convert an existing MySQL
Server instance to a node and run the cluster using this node as a base.  You
can also detach any node from the cluster and use it as a regular MySQL Server
instance.

.. image:: _static/cluster-diagram1.png

.. rubric:: Benefits

* When you execute a query,
  it is executed locally on the node.
  All data is available locally, no need for remote access.

* No central management.
  You can loose any node at any point of time,
  and the cluster will continue to function without any data loss.

* Good solution for scaling a read workload.
  You can put read queries to any of the nodes.

.. rubric:: Drawbacks

* Overhead of provisioning new node. When you add a new node, it has to copy the
  full data set from one of existing nodes. If it is *100 GB*, it copies *100
  GB*.

* This can't be used as an effective write scaling solution.  There might be
  some improvements in write throughput when you run write traffic to 2 nodes
  versus all traffic to 1 node, but you can't expect a lot.  All writes still
  have to go on all nodes.

* You have several duplicates of data: for 3 nodes you have 3 duplicates.

Components
==========

Percona XtraDB Cluster_ is based on Percona Server for MySQL_ running with the XtraDB_ storage engine.
It uses the Galera_ library,
which is an implementation of the write set replication (wsrep) API
developed by `Codership Oy <http://www.galeracluster.com/>`_.
The default and recommended data transfer method is via Percona XtraBackup_.

