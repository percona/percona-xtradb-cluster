.. Percona XtraDB Cluster documentation master file, created by
   sphinx-quickstart on Sat Dec  3 13:59:56 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==================================================
Welcome to Percona XtraDB Cluster's documentation!
==================================================

Percona XtraDB Cluster is High Availability and Scalability solution for MySQL Users.

Percona XtraDB Cluster provides:
 
 * Synchronous replication. Transaction either commited on all nodes or none.

 * Multi-master replication. You can write to any node.

 * Parallel applying events on slave. Real “parallel replication”.

 * Automatic node provisioning.

 * Data consistency. No more unsynchronized slaves.

Percona XtraDB Cluster is fully compatible with MySQL or Percona Server in the following meaning:

 * Data compatibility. Percona XtraDB Cluster works with databases created in MySQL / Percona Server

 * Application compatibility. There is no or minimal application changes required to start work with Percona XtraDB Cluster


Introduction
============

.. toctree::
   :maxdepth: 1
   :glob:

   intro
   resources

Installation
============

.. toctree::
   :maxdepth: 1
   :glob:

   installation
   installation/compiling_xtradb_cluster

Features
========

.. toctree::
   :maxdepth: 1
   :glob:

   features/highavailability
   features/multimaster-replication

FAQ
===

.. toctree::
   :maxdepth: 1
   :glob:

   faq


How-to
======

.. toctree::
   :maxdepth: 1
   :glob:
  
   howtos/singlebox
   howtos/3nodesec2
   howtos/haproxy
   howtos/virt_sandbox
   howtos/kewpietests

Percona XtraDB Cluster limitations
==================================

.. toctree::
   :maxdepth: 1
   :glob:

   limitation


Galera documentation
=====================

The full documentation and reference is available on
`Galera Wiki <http://www.codership.com/wiki/doku.php>`_


Reference
=========

.. toctree::
   :maxdepth: 1
   :glob:

   release-notes/release-notes_index
   bugreport
   glossary

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

