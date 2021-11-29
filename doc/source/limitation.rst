.. _limitations:

==================================
Percona XtraDB Cluster Limitations
==================================

The following limitations apply to Percona XtraDB Cluster:

Replication works only with InnoDB storage engine.
   Any writes to tables of other types are not replicated.

Unsupported queries:
   ``LOCK TABLES`` and ``UNLOCK TABLES`` is not supported in multi-source setups

   Lock functions, such as ``GET_LOCK()``, ``RELEASE_LOCK()``, and so on

Query log cannot be directed to table.
   If you enable query logging, you must forward the log to a file:

   .. code-block:: mysql

      log_output = FILE

   Use ``general_log`` and ``general_log_file`` to choose query logging
   and the log file name.

Maximum allowed transaction size is defined by the |max-ws-rows| and |max-ws-size| variables.
   ``LOAD DATA INFILE`` processing will commit every 10 000 rows.  So large
   transactions due to ``LOAD DATA`` will be split to series of small
   transactions.

Transaction issuing ``COMMIT`` may still be aborted at that stage.
   Due to cluster-level optimistic concurrency control,  there can be two
   transactions writing to the same rows and committing in separate Percona XtraDB Cluster nodes,
   and only one of the them can successfully commit. The failing one will be
   aborted. For cluster-level aborts, Percona XtraDB Cluster gives back deadlock error code: ::

   (Error: 1213 SQLSTATE: 40001  (ER_LOCK_DEADLOCK)).

XA transactions are not supported
   Due to possible rollback on commit.

Write throughput of the whole cluster is limited by the weakest node.
   If one node becomes slow, the whole cluster slows down.  If you have
   requirements for stable high performance, then it should be supported by
   corresponding hardware.

Minimal recommended size of cluster is 3 nodes.
   The 3rd node can be an arbitrator.

``enforce_storage_engine=InnoDB`` is not compatible with ``wsrep_replicate_myisam=OFF``
   :variable:`wsrep_replicate_myisam` is set to ``OFF`` by default.

Avoid ``ALTER TABLE ... IMPORT/EXPORT`` workloads when running Percona XtraDB Cluster in cluster mode.
   It can lead to node inconsistency if not executed in sync on all nodes.

All tables must have a primary key.
   This ensures that the same rows appear in the same order on different
   nodes. The ``DELETE`` statement is not supported on tables without a primary
   key.

  .. seealso::

     Galera Documentation: Tables without Primary Keys
        https://galeracluster.com/library/training/tutorials/differences.html#tables-without-primary-keys

Avoid reusing the names of persistent tables for temporary tables
   Although MySQL does allow having temporary tables named the same as
   persistent tables, this approach is not recommended.

   Galera Cluster blocks the replication of those persistent tables
   the names of which match the names of temporary tables.

   With wsrep_debug set to *1*, the error log may contain the following message:

   .. code-block:: text

      ... [Note] WSREP: TO BEGIN: -1, 0 : create table t (i int) engine=innodb
      ... [Note] WSREP: TO isolation skipped for: 1, sql: create table t (i int) engine=innodb.Only temporary tables affected.

   .. seealso:: `MySQL Documentation: Problems with temporary tables
		<https://dev.mysql.com/doc/refman/8.0/en/temporary-table-problems.html>`_

.. include:: .res/replace.opt.txt

As of version 8.0.21, an INPLACE `ALTER TABLE <https://dev.mysql.com/doc/refman/8.0/en/alter-table.html>`__  query takes an internal shared lock on the table during the execution of the query. The ``LOCK=NONE`` clause is no longer allowed for all of the INPLACE ALTER TABLE queries due to this change.

This change addresses a deadlock, which could cause a cluster node to hang in the following scenario:

* An INPLACE ``ALTER TABLE`` query in one session or being applied as Total Order Isolation (TOI) 

* A DML on the same table from another session


