.. _dochome:

==============================================
Percona XtraDB Cluster |version| Documentation
==============================================

|PXC|_ is a database clustering solution for MySQL.  It ensures high
availability, prevents downtime and data loss, and provides linear scalability
for a growing environment.

.. rubric:: Features of |PXC|

.. list-table::
   :header-rows: 1

   * - Feature
     - Details
   * - Synchronous replication**
     - Data is written to all nodes simultaneously, or not written at all in
       case of a failure even on a single node
   * - Multi-source replication
     - Any node can trigger a data update.
   * - True parallel replication
     - Multiple threads on replica performing replication on row level
   * - Automatic node provisioning
     - You simply add a node and it automatically syncs.
   * - Data consistency
     - No more unsynchronized nodes.
   * - PXC Strict Mode
     - Avoids the use of tech preview features and unsupported features
   * - Configuration script for ProxySQL
     - |PXC| includes the ``proxysql-admin`` tool that automatically configures
       |PXC| nodes using ProxySQL.
   * - Automatic configuration of SSL encryption
     - |PXC| includes the ``pxc-encrypt-cluster-traffic`` variable
       that enables automatic configuration of SSL encryption
   * - Optimized Performance
     - |PXC| performance is optimized to scale with a growing production
       workload


|PXC| |version| is fully compatible with |MySQL| Server Community Edition
|version| and |ps-last|.

.. seealso::

   Overview of changes in the most recent |pxc| release
      :ref:`upgrade-guide-changed`

   |MySQL| Community Edition
      https://www.mysql.com/products/community/
   |percona-server|
      https://www.percona.com/doc/percona-server/LATEST/index.html
   How We Made Percona XtraDB Cluster Scale
       https://www.percona.com/blog/2017/04/19/how-we-made-percona-xtradb-cluster-scale
      
Data compatibility
   You can use data created by any MySQL variant.
Application compatibility
   There is no or minimal application changes required.

Introduction
============

.. toctree::
   :maxdepth: 1

   intro
   limitation

Getting Started
===============

.. toctree::
   :maxdepth: 1
   :includehidden:
   :titlesonly:

   overview
   install/index
   configure
   bootstrap
   add-node
   verify

Features
========

.. toctree::
   :maxdepth: 1

   features/highavailability
   features/multisource-replication
   features/pxc-strict-mode
   features/online-schema-upgrade
   features/nbo

PXC Security
============

.. toctree::
   :maxdepth: 1

   security/index
   security/secure-network
   security/encrypt-traffic
   security/apparmor
   security/selinux

User\'s Manual
================================================================================

.. toctree::
   :maxdepth: 1

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

How-tos
=======

.. toctree::
   :maxdepth: 1

   howtos/upgrade_guide
   howtos/crash-recovery
   howtos/centos_howto
   howtos/ubuntu_howto
   howtos/garbd_howto
   howtos/singlebox
   howtos/3nodesec2
   howtos/haproxy
   howtos/proxysql
   howtos/proxysql-v2
   howtos/virt_sandbox

Reference
=========

.. toctree::
   :maxdepth: 1

   release-notes/release-notes_index
   wsrep-status-index
   wsrep-system-index
   wsrep-provider-index
   wsrep-files-index
   faq
   glossary

* :ref:`genindex`
* :ref:`search`

.. include:: .res/replace.txt
