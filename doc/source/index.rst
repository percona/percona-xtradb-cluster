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
   * - Multi-master replication
     - Any node can trigger a data update.
   * - True parallel replication
     - Multiple threads on slave performing replication on row level
   * - Automatic node provisioning
     - You simply add a node and it automatically syncs.
   * - Data consistency
     - No more unsynchronized nodes.
   * - PXC Strict Mode
     - Avoids the use of experimental and unsupported features
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

   |MySQL| Community Edition
      https://www.mysql.com/products/community/
   |percona-server|
      https://www.percona.com/doc/percona-server/LATEST/index.html
   How We Made Percona XtraDB Cluster Scale
       https://www.percona.com/blog/2017/04/19/how-we-made-percona-xtradb-cluster-scale
      
..   Performance improvements in Percona XtraDB Cluster 5.7.17-29.20
..   https://www.percona.com/blog/2017/04/19/performance-improvements-percona-xtradb-cluster-5-7-17


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

<<<<<<< HEAD
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
||||||| merged common ancestors
Security Improvements
================================================================================

.. toctree::
  :maxdepth: 1
  :glob:

  security/pam_plugin
  security/data-at-rest-encryption
  security/vault
  security/using-keyring-plugin
  security/rotating-master-key
  security/encrypting-tables
  security/encrypting-tablespaces
  security/encrypting-system-tablespace
  security/encrypting-temporary-files
  security/encrypting-binlogs
  security/encrypting-redo-log
  security/encrypting-undo-tablespace
  security/encrypting-threads
  security/encrypting-doublewrite-buffers
  security/verifying-encryption
  security/data-scrubbing
  security/ssl-improvement
  security/data-masking

Diagnostics Improvements
================================================================================

.. toctree::
   :maxdepth: 1
   :glob:

   diagnostics/user_stats
   diagnostics/slow_extended
   diagnostics/innodb_show_status
   diagnostics/show_engines
   diagnostics/process_list
   diagnostics/misc_info_schema_tables
   diagnostics/thread_based_profiling
   diagnostics/innodb_fragmentation_count

TokuDB
================================================================================
=======
Security Improvements
================================================================================

.. toctree::
  :maxdepth: 1
  :glob:

  security/pam_plugin
  security/simple-ldap
  security/data-at-rest-encryption
  security/vault
  security/using-keyring-plugin
  security/rotating-master-key
  security/encrypting-tables
  security/encrypting-tablespaces
  security/encrypting-system-tablespace
  security/encrypting-temporary-files
  security/encrypting-binlogs
  security/encrypting-redo-log
  security/encrypting-undo-tablespace
  security/encrypting-threads
  security/encrypting-doublewrite-buffers
  security/verifying-encryption
  security/data-scrubbing
  security/ssl-improvement
  security/data-masking

Diagnostics Improvements
================================================================================

.. toctree::
   :maxdepth: 1
   :glob:

   diagnostics/user_stats
   diagnostics/slow_extended
   diagnostics/innodb_show_status
   diagnostics/show_engines
   diagnostics/process_list
   diagnostics/misc_info_schema_tables
   diagnostics/thread_based_profiling
   diagnostics/innodb_fragmentation_count

TokuDB
================================================================================
>>>>>>> Percona-Server-8.0.19-10

<<<<<<< HEAD
How-tos
=======
||||||| merged common ancestors
.. toctree::
   :maxdepth: 1
   :glob:

   tokudb/tokudb_intro
   tokudb/tokudb_installation
   tokudb/using_tokudb
   tokudb/fast_updates
   tokudb/tokudb_files_and_file_types
   tokudb/tokudb_file_management
   tokudb/tokudb_background_analyze_table
   tokudb/tokudb_variables
   tokudb/tokudb_status_variables
   tokudb/tokudb_troubleshooting
   tokudb/tokudb_performance_schema
   tokudb/toku_backup
   tokudb/tokudb_faq
   tokudb/removing_tokudb

Percona MyRocks
================================================================================
=======
.. toctree::
   :maxdepth: 1
   :glob:

   tokudb/tokudb_intro
   tokudb/tokudb_installation
   tokudb/using_tokudb
   tokudb/fast_updates
   tokudb/tokudb_files_and_file_types
   tokudb/tokudb_file_management
   tokudb/tokudb_background_analyze_table
   tokudb/tokudb_variables
   tokudb/tokudb_status_variables
   tokudb/tokudb_fractal_tree_indexing
   tokudb/tokudb_troubleshooting
   tokudb/tokudb_performance_schema
   tokudb/toku_backup
   tokudb/tokudb_faq
   tokudb/removing_tokudb

Percona MyRocks
================================================================================
>>>>>>> Percona-Server-8.0.19-10

.. toctree::
   :maxdepth: 1

   howtos/upgrade_guide
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
