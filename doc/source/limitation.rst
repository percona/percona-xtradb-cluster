.. _limitations:

==================================
Percona XtraDB Cluster Limitations
==================================

The following limitations apply to |PXC|:

* Replication works only with |InnoDB| storage engine.
  Any writes to tables of other types, including system (``mysql.*``) tables,
  are not replicated.
  However, ``DDL`` statements are replicated in statement level,
  and changes to ``mysql.*`` tables will get replicated that way.
  So you can safely issue ``CREATE USER...``,
  but issuing ``INSERT INTO mysql.user...`` will not be replicated.
  You can enable experimental |MyISAM| replication support
  using the :variable:`wsrep_replicate_myisam` variable.

* Unsupported queries:

  * ``LOCK TABLES`` and ``UNLOCK TABLES`` is not supported
    in multi-master setups

  * Lock functions, such as ``GET_LOCK()``, ``RELEASE_LOCK()``, and so on

* Query log cannot be directed to table.
  If you enable query logging, you must forward the log to a file: ::

    log_output = FILE

  Use ``general_log`` and ``general_log_file`` to choose query logging
  and the log file name.

* Maximum allowed transaction size is defined by the
  :variable:`wsrep_max_ws_rows` and :variable:`wsrep_max_ws_size` variables.
  ``LOAD DATA INFILE`` processing will commit every 10 000 rows.
  So large transactions due to ``LOAD DATA``
  will be split to series of small transactions.

* Due to cluster-level optimistic concurrency control,
  transaction issuing ``COMMIT`` may still be aborted at that stage.
  There can be two transactions writing to the same rows
  and committing in separate |PXC| nodes,
  and only one of the them can successfully commit.
  The failing one will be aborted.
  For cluster-level aborts, |PXC| gives back deadlock error code: ::

   (Error: 1213 SQLSTATE: 40001  (ER_LOCK_DEADLOCK)).

* XA transactions are not supported due to possible rollback on commit.

* The write throughput of the whole cluster is limited by the weakest node.  If
  one node becomes slow, the whole cluster slows down.  If you have requirements
  for stable high performance, then it should be supported by corresponding
  hardware.

* The minimal recommended size of cluster is 3 nodes.  The 3rd node can be an
  arbitrator.

* InnoDB fake changes feature is not supported.

* ``enforce_storage_engine=InnoDB`` is not compatible with
  ``wsrep_replicate_myisam=OFF`` (default).

* When running |PXC| in cluster mode,
  avoid ``ALTER TABLE ... IMPORT/EXPORT`` workloads.
  It can lead to node inconsistency if not executed in sync on all nodes.

* All tables must have the primary key. This ensures that the same rows appear
  in the same order on different nodes. The ``DELETE`` statement is not supported on
  tables without a primary key.

  .. seealso::

     Galera Documentation: Tables without Primary Keys
        http://galeracluster.com/documentation-webpages/limitations.html#tables-without-primary-keys
