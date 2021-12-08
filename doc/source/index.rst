.. _dochome:

==============================================
Percona XtraDB Cluster |version| Documentation
==============================================

Percona XtraDB Cluster is a database clustering solution for MySQL.
It ensures high availability, prevents downtime and data loss,
and provides linear scalability for a growing environment.

Features of Percona XtraDB Cluster include:

`Synchronous replication <Database replication>`
   Data is written to all nodes simultaneously,
   or not written at all if it fails even on a single node.

`Multi-source replication`
   Any node can trigger a data update.

`True parallel replication <Database replication>`
   Multiple threads on slave performing replication on row level.

Automatic node provisioning**
   You simply add a `node` and it automatically syncs.

Data consistency
   Percona XtraDB Cluster ensures that data is automatically synchronized on all
   `nodes <Node>` in your cluster.

:ref:`pxc-strict-mode`
   Avoids the use of experimental and unsupported features.


Configuration script for ProxySQL
  Percona provides a ProxySQL package with the ``proxysql-admin``
  tool that automatically configures Percona XtraDB Cluster nodes.

  .. seealso:: :ref:`load_balancing_with_proxysql`

Automatic configuration of SSL encryption
  Percona XtraDB Cluster includes the ``pxc-encrypt-cluster-traffic`` variable
  that enables automatic configuration of SSL encryption.

Optimized Performance
  Percona XtraDB Cluster performance is optimized to scale with a growing production workload.

  For more information, see the following blog posts:

  * `How We Made Percona XtraDB Cluster Scale
    <https://www.percona.com/blog/2017/04/19/how-we-made-percona-xtradb-cluster-scale/>`_

  * `Performance improvements in Percona XtraDB Cluster 5.7.17-29.20
    <https://www.percona.com/blog/2017/04/19/performance-improvements-percona-xtradb-cluster-5-7-17/>`_

Percona XtraDB Cluster is fully compatible with `MySQL Server Community Edition
<MySQL>`_, |PS|_, and MariaDB_. It provides a robust application
compatibility: there is no or minimal application changes required.

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
   performance/aio_page_requests

Diagnostics
============================================================================

.. toctree::
   :maxdepth: 1

   diagnostics/innodb_fragmentation_count
   diagnostics/libcoredumper
   diagnostics/stacktrace
   

How-tos
=======

.. toctree::
   :maxdepth: 1
   :glob:

   howtos/upgrade_guide
   howtos/crash-recovery
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
