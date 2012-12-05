.. rn:: 5.5.28-23.7

======================================
 |Percona XtraDB Cluster| 5.5.28-23.7
======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on November 15th, 2012. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.28-23.7/>`_ or from our `software repositories <http://www.percona.com/doc/percona-xtradb-cluster/installation.html#using-percona-software-repositories>`_.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

Features
========

  * |Percona XtraDB Cluster| has ported Twitter's |MySQL| ``NUMA`` patch. This patch `implements improved NUMA support <http://www.percona.com/doc/percona-server/5.5/performance/innodb_numa_support.html#innodb-numa-support>`_ as it prevents imbalanced memory allocation across NUMA nodes. 

  * Number of binlog files can be restricted when using |Percona XtraDB Cluster| with the new `max_binlog_files <http://www.percona.com/doc/percona-server/5.5/flexibility/max_binlog_files.html#max_binlog_files>`_ option.

  * New status variable :variable:`wsrep_incoming_addresses` has been introduced. It contains a comma-separated list of incoming (client) addresses in the cluster component.

  * Multiple addresses can now be specified in the ``gcomm:// URL`` when defining the address to connect to cluster. Option :variable:`wsrep_urls` is now deprecated, :variable:`wsrep_cluster_address` should be used to specify multiple addresses instead.

  * GTID can now be recovered by starting mysqld with ``--wsrep-recover`` option. This will parse GTID from log, and if the GTID is found, assign it as initial position for actual server start.

  * ``SET PASSWORD`` command replication has been implemented. The SET statement can contain other variables and several password changes, and each password change will be replicated separately as one TOI (total order isolation) call. The user does not have to be explicitly defined, current user will be detected and correct ``SET PASSWORD`` statement will be generated for TOI replication in every use case.

  * SQL statements aborted due to multi-master conflict can be automatically retried. This can be controlled with :variable:`wsrep_retry_autocommit` variable. The retrying will happen only if there is real abort for the SQL statement. If statement itself enters into commit phase and detects certification failure, there will be no retry.
 
  * This version of |Percona XtraDB Cluster| implements significant speedups and memory footprint reduction on ranged queries.

Bugs fixed 
==========

  * Some wsrep variables, like :variable:`wsrep_provider`, :variable:`wsrep_provider_options` and :variable:`wsrep_cluster_address`, could be "doubly" allocated which could cause memory leak. Bug fixed :bug:`1072839`.

  * In case variables :variable:`wsrep_node_address` and :variable:`wsrep_node_incoming_address` were not set explicitly, port number would be set to ``0``. Bug fixed :bug:`1071882`.

  * Server would hang before storage engine initialization with ``rsync`` SST and with option :variable:`wsrep_start_position` enabled. Bug fixed :bug:`1075495`.

  * SST script interface has been upgraded to use named instead of positional parameters. Bug fixed :bug:`1045026`.

  * When server was started with no wsrep_provider and regular |MySQL| replication slave was configured, could cause intermittent failure to start MySQL slave during startup. Bug fixed :bug:`1075617`.

  * If DDL is processing with RSU method and there is MDL conflict with local transaction, this conflict would remain unresolved. Bug fixed :bug:`1039514`.

  * Static SST method list has been removed. Bug fixed :bug:`1045040`.

  * DDL in multi-statement would cause a server crash. Bug fixed :bug:`1049024`.

  * Mysqld_safe doesn't restart the node automatically anymore. Bug fixed :bug:`1077632`.
 
  * Server would fail to start if the variable :variable:`wsrep_provider` was unset. Bug fixed :bug:`1077652`.

Other bug fixes: bug :bug:`1049103`, bug :bug:`1045811`, bug :bug:`1057910`, bug :bug:`1052668`, bug :bug:`1073220`, bug :bug:`1076382`.

Based on `Percona Server 5.5.28-29.1 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.28-29.1.html>`_ including all the bug fixes in it, |Percona XtraDB Cluster| 5.5.28-23.7 is now the current stable release. All of |Percona|'s software is open-source and free. 

