.. rn:: 5.5.27

=====================================
 |Percona XtraDB Cluster| 5.5.27-23.6
=====================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on August 27th, 2012. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.27-23.6/>`_ or from our `software repositories <http://www.percona.com/doc/percona-xtradb-cluster/installation.html#using-percona-software-repositories>`_.

This is an General Availability release. We did our best to eliminate bugs and problems during alpha and beta testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

Features
========

  * |Percona XtraDB Cluster| supports tunable buffer size for fast index creation in |InnoDB|. This value was calculated based on the merge block size (which was hardcoded to 1 MB) and the minimum index record size. By adding the session variable `innodb_merge_sort_block_size <http://www.percona.com/doc/percona-server/5.5/management/innodb_fast_index_creation.html#innodb_merge_sort_block_size>`_ block size that is used in the merge sort can now be adjusted for better performance.

  * |Percona XtraDB Cluster| has implemented ability to have a MySQL `Utility user <http://www.percona.com/doc/percona-server/5.5/management/utility_user.html#psaas-utility-user>`_ who has system access to do administrative tasks but limited access to user schema. This feature is especially useful to those operating MySQL As A Service.

  * New  `Expanded Program Option Modifiers <http://www.percona.com/doc/percona-server/5.5/management/expanded_program_option_modifiers.html#expanded-option-modifiers>`_ have been added to allow access control to system variables.

  * New table `INNODB_UNDO_LOGS <http://www.percona.com/doc/percona-server/5.5/diagnostics/misc_info_schema_tables.html#INNODB_UNDO_LOGS>`_ has been added to allow access to undo segment information. Each row represents an individual undo segment and contains information about which rollback segment the undo segment is currently owned by, which transaction is currently using the undo segment, and other size and type information for the undo segment. This information is 'live' and calculated for each query of the table.

  * |Percona XtraDB Cluster| now has improved ``clustercheck.sh`` script which makes a proxy (ie HAProxy) capable of monitoring Percona XtraDB Cluster nodes properly.

Bugs fixed 
==========

  * Increasing the number of slave threads doesn't require provider reloading anymore. Bug fixed :bug:`852094`. 

  * No other SQL statement except INSERT...SELECT would replicate if /*!99997*/ comment was present. Bug fixed :bug:`1023911`.
 
  * Fixed the bug that caused DDL command hanging when RSU was enabled and server was under high rate SQL write load. Bug fixed :bug:`1026181`.

  * Cascading foreign key constraint could cause a slave crash. Bug fixed :bug:`1013978`.

  * Fixed bug when donor accepts query, which then blocks. Bug fixed :bug:`1002714`.

Based on `Percona Server 5.5.27-28.0 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.27-28.0.html>`_ including all the bug fixes in it, and the latest `Galera 2.x replicator <https://code.launchpad.net/~codership/galera/2.x>`_,  |Percona XtraDB Cluster| 5.5.27-23.6 is now the current stable release. All of |Percona|'s software is open-source and free. 

