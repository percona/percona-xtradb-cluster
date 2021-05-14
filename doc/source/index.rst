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
<<<<<<< HEAD
||||||| 6f7822ffd0f
   :glob:

   management/udf_percona_toolkit
   management/kill_idle_trx
   management/changed_page_tracking
   management/innodb_expanded_fast_index_creation
   management/backup_locks
   management/audit_log_plugin
   management/start_transaction_with_consistent_snapshot
   management/extended_show_grants
   management/utility_user

Security Improvements
================================================================================

.. toctree::
  :maxdepth: 1
  :glob:

  security/pam_plugin
  security/simple-ldap
  security/simple-ldap-variables
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
=======
   :glob:

   management/udf_percona_toolkit
   management/kill_idle_trx
   management/changed_page_tracking
   management/enforce_engine
   management/pam_plugin
   management/innodb_expanded_fast_index_creation
   management/backup_locks
   management/audit_log_plugin
   management/start_transaction_with_consistent_snapshot
   management/extended_show_grants
   management/utility_user

Security Improvements
================================================================================

.. toctree::
  :maxdepth: 1
  :glob:

  security/pam_plugin
  security/simple-ldap
  security/simple-ldap-variables
  security/selinux
  security/apparmor
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
  security/ssl-improvement
  security/data-masking

Diagnostics Improvements
================================================================================
>>>>>>> Percona-Server-8.0.23-14

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

<<<<<<< HEAD
How-tos
=======
||||||| 6f7822ffd0f
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
   tokudb/tokudb_faq
   tokudb/removing_tokudb

Percona MyRocks
================================================================================
>>>>>>> Percona-Server-8.0.23-14

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
