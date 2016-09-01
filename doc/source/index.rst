.. _dochome:

==============================================
Percona XtraDB Cluster |version| Documentation
==============================================

|Percona XtraDB Cluster| is a high-availability solution for MySQL.
It ensures the highest possible uptime,
prevents data loss,
and provides linear scalability for a growing environment.

Features of |Percona XtraDB Cluster| include:

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

|PXC| is fully compatible with `MySQL Server Community Edition
<https://www.mysql.com>`_, `Percona Server
<https://www.percona.com/software/mysql-database/percona-server>`_, and
`MariaDB <https://www.mariadb.com>`_ in the following sense:

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

Installation
============

.. toctree::
   :maxdepth: 2
   :includehidden:
   :titlesonly:

   install/index
   install/upgrade_guide

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

   manual/bootstrap
   manual/state_snapshot_transfer
   manual/xtrabackup_sst
   manual/restarting_nodes
   manual/failover
   manual/monitoring
   manual/certification
   manual/threading_model
   manual/gcache_record-set_cache_difference

How-tos
=======

.. toctree::
   :maxdepth: 1
   :glob:

   howtos/centos_howto
   howtos/ubuntu_howto
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

