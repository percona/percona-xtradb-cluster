.. _dochome:

==============================================
Percona XtraDB Cluster |version| Documentation
==============================================

|PXC|_ is a database clustering solution for MySQL.
It ensures high availability, prevents downtime and data loss,
and provides linear scalability for a growing environment.

Features of |PXC| include:

* **Synchronous replication**

  Data is written to all nodes simultaneously,
  or not written at all if it fails even on a single node.

* **Multi-master replication**

  Any node can trigger a data update.

* **True parallel replication**

  Multiple threads on slave performing replication on row level.

* **Automatic node provisioning**

  You simply add a node and it automatically syncs.

* **Data consistency**

  No more unsynchronized nodes.

* **PXC Strict Mode**

  Avoids the use of experimental and unsupported features.

* **Configuration script for ProxySQL**

  Percona provides a ProxySQL package with the ``proxysql-admin`` tool
  that automatically configures |PXC| nodes.

* **Automatic configuration of SSL encryption**

  |PXC| includes the ``pxc-encrypt-cluster-traffic`` variable
  that enables automatic configuration of SSL encrytion.

* **Optimized Performance**

  |PXC| performance is optimized to scale with a growing production workload.

  For more information, see the following blog posts:

  * `How We Made Percona XtraDB Cluster Scale
    <https://www.percona.com/blog/2017/04/19/how-we-made-percona-xtradb-cluster-scale/>`_

  * `Performance improvements in Percona XtraDB Cluster 5.7.17-29.20
    <https://www.percona.com/blog/2017/04/19/performance-improvements-percona-xtradb-cluster-5-7-17/>`_

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

PXC Security
============

.. toctree::
   :maxdepth: 1

   security/index
   security/secure-network
   security/encrypt-traffic

User\'s Manual
================================================================================

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
   management/data_at_rest_encryption

Flexibility
================================================================================

.. toctree::
   :maxdepth: 1
   
   flexibility/binlogging_replication_improvements
   flexibility/innodb_fts_improvements
   diagnostics/innodb_fragmentation_count
   performance/aio_page_requests

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

