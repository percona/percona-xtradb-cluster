.. _limitations:

====================================
 Percona XtraDB Cluster Limitations
====================================

There are some limitations which you should be aware of. Some of them will be eliminated later as product is improved and some are design limitations.

 - Currently replication works only with |InnoDB| storage engine. Any writes to tables of other types, including system (mysql.*) tables, are not replicated. However, ``DDL`` statements are replicated in statement level, and changes to mysql.* tables will get replicated that way. So, you can safely issue: ``CREATE USER...``, but issuing: ``INSERT INTO mysql.user...``, will not be replicated. You can enable experimental |MyISAM| replication support with :variable:`wsrep_replicate_myisam`.

 - Unsupported queries:

    * ``LOCK/UNLOCK TABLES`` cannot be supported in multi-master setups.
    * lock functions (GET_LOCK(), RELEASE_LOCK()... )

 - Query log cannot be directed to table. If you enable query logging, you must forward the log to a file: log_output = FILE. Use general_log and general_log_file to choose query logging and the log file name.

 - Maximum allowed transaction size is defined by :variable:`wsrep_max_ws_rows` and :variable:`wsrep_max_ws_size` variables. ``LOAD DATA INFILE`` processing will commit every 10K rows. So large transactions due to ``LOAD DATA`` will be split to series of small transactions.

 - Due to cluster level optimistic concurrency control, transaction issuing ``COMMIT`` may still be aborted at that stage. There can be two transactions writing to same rows and committing in separate |Percona XtraDB Cluster| nodes, and only one of the them can successfully commit. The failing one will be aborted. For cluster level aborts, |Percona XtraDB Cluster| gives back deadlock error code: ::

   (Error: 1213 SQLSTATE: 40001  (ER_LOCK_DEADLOCK)).

 - XA transactions can not be supported due to possible rollback on commit.

 - The write throughput of the whole cluster is limited by weakest node. If one node becomes slow, whole cluster is slow. If you have requirements for stable high performance, then it should be supported by corresponding hardware.

 - The minimal recommended size of cluster is 3 nodes.

 - To enable Query Cache, node should be started with :variable:`query_cache_type` set to ``1`` and :variable:`query_cache_size` set to ``0``. After the node has been successfully started :variable:`query_cache_size` can be changed to desired value during the runtime.
