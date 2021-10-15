.. _performance_schema_instrumentation:

=================================
Perfomance Schema Instrumentation
=================================

To improve monitoring |Percona XtraDB Cluster| has implemented an
infrastructure to expose Galera instruments (mutexes, cond-variables, files,
threads) as a part of ``PERFORMANCE_SCHEMA``.

Although mutexes and condition variables from ``wsrep`` were already part of
``PERFORMANCE_SCHEMA`` threads were not.

Mutexes, condition variables, threads, and files from Galera library also were
not part of the ``PERFORMANCE_SCHEMA``.

You can see the complete list of available instruments by running:

.. code-block:: mysql

  mysql> SELECT * FROM performance_schema.setup_instruments WHERE name LIKE '%galera%' OR name LIKE '%wsrep%';

  +----------------------------------------------------------+---------+-------+
  | NAME                                                     | ENABLED | TIMED |
  +----------------------------------------------------------+---------+-------+
  | wait/synch/mutex/sql/LOCK_wsrep_ready                    | NO      | NO    |
  | wait/synch/mutex/sql/LOCK_wsrep_sst                      | NO      | NO    |
  | wait/synch/mutex/sql/LOCK_wsrep_sst_init                 | NO      | NO    |
  ...
  | stage/wsrep/wsrep: in rollback thread                    | NO      | NO    |
  | stage/wsrep/wsrep: aborter idle                          | NO      | NO    |
  | stage/wsrep/wsrep: aborter active                        | NO      | NO    |
  +----------------------------------------------------------+---------+-------+
  73 rows in set (0.00 sec)

Some of the most important are:

* Two main actions that Galera does are ``REPLICATION`` and ``ROLLBACK``.
  Mutexes, condition variables, and threads related to this are part of
  ``PERFORMANCE_SCHEMA``.

* Galera internally uses monitor mechanism to enforce ordering of
  events. These monitor control events apply and are mainly responsible for
  the wait between different action. All such monitor mutexes and condition
  variables are covered as part of this implementation.

* There are lot of other miscellaneous action related to receiving of package
  and servicing messages. Mutexes and condition variables needed for them are
  now visible too. Threads that manage receiving and servicing are also being
  instrumented.

This feature has exposed all the important mutexes, condition variables that
lead to lock/threads/files as part of this process.

Besides exposing file it also tracks write/read bytes like stats for file.
These stats are not exposed for Galera files as Galera uses ``mmap``.

Also, there are some threads that are short-lived and created only when needed
especially for SST/IST purpose. They are also tracked but come into
``PERFORMANCE_SCHEMA`` tables only if/when they are created.

``Stage Info`` from Galera specific function which server updates to track
state of running thread is also visible in ``PERFORMANCE_SCHEMA``.

What is not exposed ?
---------------------

Galera uses customer data-structure in some cases (like STL structures).
Mutexes used for protecting these structures which are not part of mainline
Galera logic or doesn't fall in big-picture are not tracked. Same goes with
threads that are ``gcomm`` library specific.

Galera maintains a process vector inside each monitor for its internal graph
creation. This process vector is 65K in size and there are two such vectors per
monitor. That is 128K * 3 = 384K condition variables. These are not tracked to
avoid hogging ``PERFORMANCE_SCHEMA`` limits and sidelining of the main crucial
information.

.. _pxc_cluster_view:

Using ``pxc_cluster_view``
----------------------------

The ``pxc_cluster_view`` - provides a unified view of the cluster. The table is in the Performance_Schema database.

This table has the following definition:

.. sourcecode:: mysql

  DESCRIBE pxc_cluster_view;
  +-------------+--------------+------+-----+---------+-------+
  | Field       | Type         | Null | Key | Default | Extra |
  +-------------+--------------+------+-----+---------+-------+
  | HOST_NAME   | char(64)     | NO   |     | NULL    |       |
  | UUID        | char(36)     | NO   |     | NULL    |       |
  | STATUS      | char(64)     | NO   |     | NULL    |       |
  | LOCAL_INDEX | int unsigned | NO   |     | NULL    |       |
  | SEGMENT     | int unsigned | NO   |     | NULL    |       |
  +-------------+--------------+------+-----+---------+-------+
  5 rows in set (0.00 sec)

To view the table, run the following query:

.. sourcecode:: mysql

    SELECT * FROM pxc_cluster_view;
    +-----------+--------------------------------------+--------+-------------+---------+
    | HOST_NAME | UUID                                 | STATUS | LOCAL_INDEX | SEGMENT |
    +-----------+--------------------------------------+--------+-------------+---------+
    | node1     | 22b9d47e-c215-11eb-81f7-7ed65a9d253b | SYNCED |           0 |       0 |
    | node3     | 29c51cf5-c216-11eb-9101-1ba3a28e377a | SYNCED |           1 |       0 |
    | node2     | 982cdb03-c215-11eb-9865-0ae076a59c5c | SYNCED |           2 |       0 |
    +-----------+--------------------------------------+--------+-------------+---------+
    3 rows in set (0.00 sec)
