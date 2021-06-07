.. _limitations:

==================================
Percona XtraDB Cluster Limitations
==================================

The following limitations apply to |PXC|:

* Replication works only with |InnoDB| storage engine.
  Any writes to tables of other types, including system (``mysql.*``) tables,
  are not replicated.
  However, ``DDL`` statements are replicated in the statement level,
  and changes to ``mysql.*`` tables are replicated that way.
  So you can safely issue ``CREATE USER...``,
  but issuing ``INSERT INTO mysql.user...`` will not be replicated.
  You can enable experimental |MyISAM| replication support
  using the :variable:`wsrep_replicate_myisam` variable.

* Unsupported queries:

  * ``LOCK TABLES`` and ``UNLOCK TABLES`` is not supported
    in multi-source setups

  * Lock functions, such as ``GET_LOCK()``, ``RELEASE_LOCK()``, and so on

  .. seealso::
  
     |MySQL| Documentation:
         - `LOCK TABLES AND UNLOCK TABLES statements <https://dev.mysql.com/doc/refman/8.0/en/lock-tables.html>`_
         - `Locking functions <https://dev.mysql.com/doc/refman/5.7/en/locking-functions.html>`_
         
  * Query log cannot be directed to table.
         
  If you enable query logging, you must forward the log to a file: ::

    log_output = FILE

  Use ``general_log`` and ``general_log_file`` to choose query logging
  and the log file name.

* Maximum allowed transaction size is defined by the
  :variable:`wsrep_max_ws_rows` and :variable:`wsrep_max_ws_size` variables.
  ``LOAD DATA INFILE`` processing will commit every 10,000 rows.
  So large transactions due to ``LOAD DATA``
  will be split to series of small transactions.

* Due to cluster-level optimistic concurrency control, a
  transaction issuing a ``COMMIT`` may still be aborted at that stage.
  There can be two transactions writing to the same rows
  and committing in separate |PXC| :term:`nodes <Node>`,
  and only one of the them can successfully commit.
  The failing one will be aborted.
  For cluster-level aborts, |PXC| gives back deadlock error code: ::

   (Error: 1213 SQLSTATE: 40001  (ER_LOCK_DEADLOCK)).

* `XA transactions <https://dev.mysql.com/doc/refman/5.7/en/xa.html>`_ are not supported due to possible rollback on commit.

* The write throughput of the whole cluster is limited by the weakest :term:`node`.  If
  one node becomes slow, the whole cluster slows down.  If you have requirements
  for stable high performance, then it should be supported by corresponding
  hardware.

* The minimal recommended size of cluster is three nodes.  The third node can be an
  `arbitrator <https://galeracluster.com/library/documentation/arbitrator.html>`_.

* `InnoDB fake changes feature <https://www.percona.com/doc/percona-server/5.5/management/innodb_fake_changes.html>`_ is not supported. This feature has been removed.

* ``enforce_storage_engine=InnoDB`` is not compatible with
  ``wsrep_replicate_myisam=OFF`` (default).
  
  .. seealso:: `Percona Server for MySQL documentation: enforcing storage engine <https://www.percona.com/doc/percona-server/5.7/management/enforce_engine.html>`_

* When running |PXC| in cluster mode,
  avoid ``ALTER TABLE ... IMPORT/EXPORT`` workloads.
  It can lead to :term:`node` inconsistency if not executed in sync on all nodes.

* All tables must have the primary key. This ensures that the same rows appear
  in the same order on different :term:`nodes <Node>`. The ``DELETE`` statement is not supported on
  tables without a primary key.

*    |Percona Server| 5.7 data at rest encryption is similar to the `MySQL 5.7 data-at-rest encryption <https://dev.mysql.com/doc/refman/5.7/en/innodb-data-encryption.html>`_. Review the available encryption features for `Percona Server for MySQL 5.7 <https://www.percona.com/doc/percona-server/5.7/security/data-at-rest-encryption.html>`__. |Percona Server| 8.0 provides more encryption features and options which are not available in this version. 

  .. seealso::

     Galera Documentation: Tables without Primary Keys
        http://galeracluster.com/documentation-webpages/limitations.html#tables-without-primary-keys

* As of version 5.7.32-13.47, an INPLACE `ALTER TABLE <https://dev.mysql.com/doc/refman/5.7/en/alter-table.html>`__  query takes an internal shared lock on the table during the execution of the query. The ``LOCK=NONE`` clause is no longer allowed for all of the INPLACE ALTER TABLE queries due to this change.

  This change addresses a deadlock, which could cause a cluster node to hang in the following scenario:

  * An INPLACE ``ALTER TABLE`` query in one session or being applied as Total Order Isolation (TOI) 

  * A DML on the same table from another session