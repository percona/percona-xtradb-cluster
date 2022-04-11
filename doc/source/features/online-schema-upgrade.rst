.. _online-schema-upgrade:

===================================================
Online Schema Upgrade
===================================================

Database schemas must change as applications change. For a cluster, the schema upgrade must occur while the system is online. A synchronous cluster requires all active nodes have the same data. Schema updates are performed using Data Definition Language (DDL) statements, such as ``ALTER TABLE <table_name> DROP COLUMN <column_name>``. 

The DDL statements are non-transactional, so these statements use up-front locking to avoid the chance of deadlocks and cannot be rolled back. We recommend that you test your schema changes, especially if you must run an ``ALTER`` statement on large tables. Verify the backups before updating the schemas in the production environment. A failure in a schema change can cause your cluster to drop nodes and lose data. 

*Percona XtraDB Cluster* supports the following methods for making online schema changes:

.. list-table::
    :widths: 20 30 60
    :header-rows: 1

    * - Method Name
      - Reason for use
      - Description
    * - `TOI` or Total Order Isolation
    
      - Consistency is important. Other transactions are blocked while the cluster processes the DDL statements.
        
      - This is the default method for the `wsrep-OSU-method <https://galeracluster.com/library/documentation/mysql-wsrep-options.html#wsrep-osu-method>`__ variable. The isolation of the DDL statement guarantees consistency.
        
        The DDL replication uses a Statement format. Each node processes the replicated DDL statement at same position in the replication stream. All other writes must wait until the DDL statement is executed.
        
        While a DDL statement is running, any long-running transactions in progress and using the same resource receive a deadlock error at commit and are rolled back. 
        
        The `pt-online-schema-change <https://www.percona.com/doc/percona-toolkit/LATEST/pt-online-schema-change.html>`__ in the *Percona* Toolkit can alter the table without using locks. There are limitations: only InnoDB tables can be altered, and the ``wsrep_OSU_method`` must be ``TOI``.

      
    * - `RSU` or Rolling Schema Upgrade
    
      - This method guarantees high availability during the schema upgrades.

      - The node desynchronizes with the cluster and disables flow control during the execution of the DDL statement. The rest of the cluster is not affected. After the statement execution, the node applies delayed events and synchronizes with the cluster.

        Although the cluster is active, during the process some nodes have the newer schema and some nodes have the older schema. The RSU method is a manual operation.

        For this method, the ``gcache`` must be large enough to store the data for the duration of the DDL change.

    * - `NBO` or Non-Blocking Operation

      - This method is used when consistency is important and uses a more efficient locking strategy.
      
      - Implemented in *Percona XtraDB Cluster* 8.0.25-15.1. The Non-Blocking Operation feature is in **Tech Preview** mode.

        This method is similar to ``TOI``. DDL operations acquire an exclusive metadata lock on the table or schema at a late stage of the operation when updating the table or schema definition. Attempting a State Snapshot Transfer (SST) fails during the NBO operation.
        
        This mode uses a more efficient locking strategy and avoids the ``TOI`` issue of long-running DDL statements blocking other updates in the cluster.
