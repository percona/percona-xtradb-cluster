.. _dochome:

==============================================
Percona XtraDB Cluster |version| Documentation
==============================================

|PXC|_ is a database clustering solution for MySQL.
It ensures high availability, prevents downtime and data loss,
and provides linear scalability for a growing environment.

Features of |PXC| include:

* **Synchronous replication**:
  Data is written to all nodes simultaneously,
  or not written at all if it fails even on a single node.

* **Multi-master replication**:
  Any node can trigger a data update.

* **True parallel replication**:
  Multiple threads on slave performing replication on row level.

* **Automatic node provisioning**:
  You simply add a node and it automatically syncs.

* **Data consistency**:
  No more unsynchronized nodes.

|PXC| is fully compatible with `MySQL Server Community Edition <MySQL>`_,
|PS|_, and MariaDB_ in the following sense:

* **Data compatibility**:
  You can use data created by any MySQL variant.

* **Application compatibility**:
  There is no or minimal application changes required.

Introduction
============

.. toctree::
   :maxdepth: 1
   :glob:

   intro
   limitation

Getting Started
===============

.. toctree::
   :maxdepth: 1
   :includehidden:
   :titlesonly:

   Overview <overview>
   install/index
   configure
   bootstrap
   add-node
   verify

Features
========

.. toctree::
   :maxdepth: 1
   :glob:

   features/highavailability
   features/multimaster-replication
   features/pxc-strict-mode

User's Manual
=============

.. toctree::
   :maxdepth: 1
   :glob:

   manual/state_snapshot_transfer
   manual/xtrabackup_sst
   manual/restarting_nodes
   manual/failover
   manual/monitoring
   manual/certification
   manual/threading_model
   manual/gcache_record-set_cache_difference
   manual/performance_schema_instrumentation

How-tos
=======

.. toctree::
   :maxdepth: 1
   :glob:

   howtos/upgrade_guide
   howtos/centos_howto
   howtos/ubuntu_howto
   howtos/garbd_howto
   howtos/singlebox
   howtos/3nodesec2
   howtos/encrypt-traffic
   howtos/haproxy
   howtos/proxysql
   howtos/virt_sandbox

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

