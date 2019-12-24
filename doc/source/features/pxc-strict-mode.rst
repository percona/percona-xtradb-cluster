.. _pxc-strict-mode:

===============
PXC Strict Mode
===============

PXC Strict Mode (:variable:`pxc_strict_mode`) is designed to help control
behavior of cluster when an experimental/unsupported feature is used.  It
performs a number of validations at startup and during runtime.

Depending on the actual mode you select, upon encountering a failed validation,
the server will either throw an error (halting startup or denying the
operation), or log a warning and continue running as normal.  The following
modes are available:

* ``DISABLED``: Do not perform strict mode validations and run as normal; no
  error or warning is logged for use of experimental/unsupported feature.

* ``PERMISSIVE``: If a vaidation fails, log a warning and continue running as
  normal.

* ``ENFORCING``: If a validation fails during startup, halt the server and throw
  an error.  If a validation fails during runtime, deny the operation and throw
  an error.

* ``MASTER``: The same as ``ENFORCING`` except that explicit table locking is
  allowed: the validation of :ref:`explicit table locking
  <explicit-table-locking>` is not performed.  This mode can be used with
  clusters in which write operations are isolated to a single node.

.. important::

   Setting ``pxc_strict_mode`` from `DISABLED` or `PERMISSIVE` to either
   `ENFORCING` or `MASTER` requires that the following conditions are
   met:

   - `wsrep_replicate_myisam=OFF`
   - `binlog_format=ROW`
   - `log_output=FILE/NONE`
   - transaction isolation level != SERIALIZABLE

By default, PXC Strict Mode is set to ``ENFORCING``,
except if the node is acting as a standalone server (wsrep_provider=none)
or the node is bootstrapping, then PXC Strict Mode defaults to ``DISABLED``.

It is recommended to keep PXC Strict Mode set to ``ENFORCING``,
because in this case whenever |PXC| encounters an experimental feature
or an unsupported operation, the server will deny it.
This will force you to re-evaluate your |PXC| configuration
without risking the consistency of your data.

If you are planning to set PXC Strict Mode to anything else than ``ENFORCING``,
you should be aware of the limitations and effects
that this may have on data integrity.
For more information, see :ref:`validations`.

To set the mode, use the :variable:`pxc_strict_mode` variable in the
configuration file or the ``--pxc-strict-mode`` option during ``mysqld``
startup.

.. important::

   It is better to start the server with the necessary mode (the default
   ``ENFORCING`` is highly recommended).  However, you can dynamically change it
   during runtime.  For example, to set PXC Strict Mode to ``PERMISSIVE``, run
   the following command::

      mysql> SET GLOBAL pxc_strict_mode=PERMISSIVE;

   To further ensure data consistency, it is important to have all nodes in the
   cluster running with the same configuration, including the value of
   ``pxc_strict_mode`` variable.

.. _validations:

Validations
===========

PXC Strict Mode validations are designed to ensure optimal operation
for common cluster setups that do not require experimental features
and do not rely on operations not supported by |PXC|.

.. warning:: If an unsupported operation is performed on a node
   with ``pxc_strict_mode`` set to ``DISABLED`` or ``PERMISSIVE``,
   it will not be validated on nodes where it is replicated to,
   even if the destination node has ``pxc_strict_mode`` set to ``ENFORCING``.

This section describes the purpose and consequences of each validation.

.. contents::
   :local:

.. _storage-engine:

Storage engine
--------------

|PXC| supports only transactional storage engine (XtraDB or InnoDB) as PXC needs
capability of the storage engine to rollback any given transaction. Storage
engines, such as *MyISAM*, *MEMORY*, *CSV* are not supported.

* Data manipulation statements that perform writing to table (for example,
  ``INSERT``, ``UPDATE``, ``DELETE``, etc.)
* The following administrative statements: ``CHECK``, ``OPTIMIZE``, ``REPAIR``,
  and ``ANALYZE``
* ``TRUNCATE TABLE`` and ``ALTER TABLE``

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed
     - Allow write/DML, ALTER, TRUNCATE and ADMIN (check, optimize, repair,
       analyze) operations on all persistent tables (including one that resides
       in non-transactional storage engine).
   * - PERMISSIVE
     - No validation is perfromed.
     - All operations are permitted, but a warning is logged when an undesirable
       operation is performed on an unsupported table.
   * - ENFORCING or MASTER
     - No validation is performed.
     - write/DML, ALTER, TRUNCATE and ADMIN operations on each persistent table
       that is not implemented using a supported storage engine logs an error.

In order to balance backward compatibility and existing application, |PXC|
allows table created with a non-transactional storage engine to co-exist but
DML, ALTER, TRUNCATE and ADMIN (ANALYZE, CHECK, etc....) operations on such
tables are blocked in ENFORCING mode.

ALTERing unsupported table from non-transactional storage engine to a
transactional storage engine is allowed.

As operation checks are applicable only to persistent tables, temporary tables
are not replicated by Galera Cluster so *pxc-strict-mode* enforcement is not
applicable to temporary table. Same is applicable to performance-schema.  (Check
is also not applied for system tables located in the `mysql` database.)

MyISAM replication
------------------

|PXC| provides experimental support for replication of tables that use the
MyISAM storage engine.  Due to the non-transactional nature of MyISAM, it is not
likely to ever be fully supported in |PXC|.

MyISAM replication is controlled using the :variable:`wsrep_replicate_myisam`
variable, which is set to ``OFF`` by default.  Due to its unreliability, MyISAM
replication should not be enabled if you want to ensure data consistency.

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed.
     - You can set :variable:`wsrep_replicate_myisam` to any value.
   * - PERMISSIVE
     - If :variable:`wsrep_replicate_myisam` is set to ``ON``, a warning is
       logged and startup continues.
     - It is permitted to change :variable:`wsrep_replicate_myisam` to any
       value, but if you set it to ``ON``, a warning is logged.
   * - ENFORCING or MASTER
     - If :variable:`wsrep_replicate_myisam` is set to ``ON``, an error is
       logged and startup is aborted.
     - Any attempt to change :variable:`wsrep_replicate_myisam` to ``ON`` fails
       and an error is logged. Setting it to OFF is welcome operation and
       doesn't result in an error.

.. note::

   The :variable:`wsrep_replicate_myisam` variable controls *replication* for
   MyISAM tables, and this validation only checks whether it is allowed.
   Undesirable operations for MyISAM tables are restricted using the
   :ref:`storage-engine` validation.

Binary log format
-----------------

|PXC| supports only the default row-based binary logging format.  Setting the
|binlog_format|_ variable to anything but ``ROW`` at startup is not allowed,
because this changes the global scope, which must be set to ROW.  Validation is
performed only at runtime and against session scope.

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No check is enforced.
     - You can set ``binlog_format`` to any value.
   * - PERMISSIVE
     - No check is enforced.
     - It is permitted to change ``binlog_format`` to any value, but if you set
       it to anything other than ``ROW``, a warning is logged.
   * - ENFORCING or MASTER
     - No check is enforced.
     - At runtime, any attempt to change ``binlog_format`` to anything other
       than ``ROW`` fails and an error is logged. Setting it to ROW is welcome
       operation and doesn't result in an error.

.. note::

   Setting *binlog_format* at global level to STATEMENT/MIXED is not allowed under
   any mode and even during startup (startup setting are global setting)

.. |binlog_format| replace:: ``binlog_format``
.. _binlog_format: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_format

Tables without the primary key
---------------------------

|PXC| cannot properly propagate certain write operations to tables that do not
have primary keys defined.  Undesirable operations include data manipulation
statements that perform writing to table (especially ``DELETE``).

.. note::

   This type of validation is not applicable to temporary tables, system tables
   (tables located in `mysql` database), and performance-schema.

Depending on the selected mode, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed.
     - All operations are permitted.
   * - PERMISSIVE
     - No validation is perfromed.
     - All operations are permitted, but a warning is logged when an undesirable
       operation is performed on a table without an explicit primary key
       defined.
   * - ENFORCING or MASTER
     - No validation is performed.
     - Any undesirable operation performed on a table without an
       explicit primary key is denied and an error is logged.

Log output
----------

|PXC| does not support tables in the MySQL database as the destination for log
output.  By default, log entries are written to a file.  This validation checks
the value of the |log_output|_ variable.

Depending on the selected mode, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed.
     - You can set ``log_output`` to any value.
   * - PERMISSIVE
     - If ``log_output`` is set only to ``TABLE``, a warning is logged and
       startup continues.
     - It is permitted to change ``log_output`` to any value, but if you set it
       only to ``TABLE``, a warning is logged. Setting it to FILE/NONE is
       welcome operation and doesn't result in a warning.
   * - ENFORCING or MASTER
     - If ``log_output`` is set only to ``TABLE``, an error is logged and
       startup is aborted.
     - Any attempt to change ``log_output`` only to ``TABLE`` fails and an error
       is logged. Setting it to FILE/NONE is welcome operation and doesn't
       result in a warning.

.. |log_output| replace:: ``log_output``
.. _log_output: http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_log_output

.. _explicit-table-locking:

Explicit table locking
----------------------

|PXC| has only experimental support for explicit table locking operations,
The following undesirable operations lead to explicit table locking
and are covered by this validation:

* ``LOCK TABLES``
* ``GET_LOCK()`` and ``RELEASE_LOCK()``
* ``FLUSH TABLES <tables> WITH READ LOCK``
* Setting the ``SERIALIZABLE`` transaction level

Depending on the selected mode, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED or MASTER
     - No validation is performed.
     - All operations are permitted.
   * - PERMISSIVE
     - No validation is performed.
     - All operations are permitted, but a warning is logged when an
       undesirable operation is performed.
   * - ENFORCING
     - No validation is performed.
     - Any undesirable operation is denied and an error is logged.

Auto-increment lock mode
------------------------

The lock mode for generating auto-increment values must be *interleaved*
to ensure that each node generates a unique (but non-sequential) identifier.

This validation checks the value of the |innodb_autoinc_lock_mode|_ variable.
By default, the variable is set to ``1`` (*consecutive* lock mode),
but it should be set to ``2`` (*interleaved* lock mode).

.. note::

   This validation is not performed during runtime, because the
   ``innodb_autoinc_lock_mode`` variable cannot be set dynamically.

Depending on the strict mode selected, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - no validation is performed.
     - This option is not dynamic, no validation is required during runtime.
   * - PERMISSIVE
     - If ``innodb_autoinc_lock_mode`` is not set to ``2``,
       a warning is logged and startup continues.
     - This option is not dynamic, no validation is required during runtime.
   * - ENFORCING or MASTER
     - If ``innodb_autoinc_lock_mode`` is not set to ``2``, an error is logged
       and startup is aborted.
     - This option is not dynamic, no validation is required during runtime.
       
.. |innodb_autoinc_lock_mode| replace:: ``innodb_autoinc_lock_mode``
.. _innodb_autoinc_lock_mode: http://dev.mysql.com/doc/refman/5.7/en/innodb-parameters.html#sysvar_innodb_autoinc_lock_mode

Combining schema and data changes in a single statement
-------------------------------------------------------

|PXC| does not support ``CREATE TABLE ... AS SELECT`` (CTAS) statements, because
they combine both schema and data changes.

Depending on the strict mode selected, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed.
     - All operations are permitted.
   * - PERMISSIVE
     - No validation is perfromed.
     - All operations are permitted, but a warning is logged when a CTAS
       operation is performed.
   * - ENFORCING
     - No validation is performed.
     - Any CTAS operation is denied and an error is logged.

.. note::

   CTAS operations for temporary tables are permitted even in strict mode.

Discarding and Importing Tablespaces
------------------------------------

``DISCARD TABLESPACE`` and ``IMPORT TABLESPACE`` are not replicated using
:term:`TOI`.  This can lead to data inconsistency if executed on only one node.

Depending on the strict mode selected, the following happens:

.. list-table::
   :header-rows: 1

   * - Strict Mode
     - At startup
     - At runtime
   * - DISABLED
     - No validation is performed
     - All operations are permitted
   * - PERMISSIVE
     - No validation is perfromed.
     - All operations are permitted, but a warning is logged when you discard or
       import a tablespace.
   * - ENFORCING
     - No validation is performed
     - Discarding or importing a tablespace is denied and an error is logged.

XA transactions
--------------------------------------------------------------------------------

XA transaction are not supported by PXC/Galera and with new MySQL-5.7 semantics
using xa statement causes reuse of XID which is being used by Galera causing
conflicts.  XA statements are completely blocked irrespective of the value of
:variable:`pxc-strict-mode`.


.. rubric:: References

.. target-notes::
