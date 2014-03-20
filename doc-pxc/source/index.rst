.. Percona XtraDB Cluster documentation master file, created by
   sphinx-quickstart on Sat Dec  3 13:59:56 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

==========================================
 Percona XtraDB Cluster 5.6 Documentation
==========================================

|Percona XtraDB Cluster| is High Availability and Scalability solution for MySQL Users.

|Percona XtraDB Cluster| provides:
 
 * Synchronous replication. Transaction either committed on all nodes or none.

 * Multi-master replication. You can write to any node.

 * Parallel applying events on slave. Real "parallel replication".

 * Automatic node provisioning.

 * Data consistency. No more unsynchronized slaves.

|Percona XtraDB Cluster| is fully compatible with |MySQL| or |Percona Server| in the following meaning:

 * Data compatibility. |Percona XtraDB Cluster| works with databases created in |MySQL| / |Percona Server|.

 * Application compatibility. There is no or minimal application changes required to start work with |Percona XtraDB Cluster|.


Introduction
============

.. toctree::
   :maxdepth: 1
   :glob:

   intro
   limitation
   errata

Installation
============

.. toctree::
   :maxdepth: 1
   :glob:

   installation
   installation/compiling_xtradb_cluster
   upgrading_guide_55_56

Features
========

.. toctree::
   :maxdepth: 1
   :glob:

   features/highavailability
   features/multimaster-replication

User's Manual
=============

.. toctree::
   :maxdepth: 1
   :glob:

   manual/bootstrap
   manual/state_snapshot_transfer
   manual/xtrabackup_sst
   manual/restarting_nodes
   manual/failover
   manual/monitoring

How-tos
=======

.. toctree::
   :maxdepth: 1
   :glob:
  
   howtos/cenots_howto
   howtos/ubuntu_howto
   howtos/singlebox
   howtos/3nodesec2
   howtos/haproxy
   howtos/virt_sandbox
   howtos/bugreport

Reference
=========

.. toctree::
   :maxdepth: 1
   :glob:

   release-notes/release-notes_index
   wsrep-status-index
   wsrep-system-index
   wsrep-provider-index
   wsrep-files-index
   faq
   glossary

* :ref:`genindex`
* :ref:`search`

