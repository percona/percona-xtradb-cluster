.. _pxc-strict-mode:

===============
PXC Strict Mode
===============

PXC Strict Mode is designed to avoid the use of
experimental and unsupported features in |PXC|.
It performs a number of validations at startup and during runtime.

Depending on the actual mode you select,
upon encountering a failed validation,
the server will either throw an error
(halting startup or denying the operation),
or log a warning and continue running as normal.
The following modes are available:

* ``DISABLED``: Do not perform strict mode validations
  and run as normal.

* ``PERMISSIVE``: If a vaidation fails, log a warning and continue running
  as normal.

* ``ENFORCING``: If a validation fails during startup,
  halt the server and throw an error.
  If a validation fails during runtime,
  deny the operation and throw an error.

* ``MASTER``: The same as ``ENFORCING`` except that the validation of
  :ref:`explicit table locking <explicit-table-locking>` is not performed.
  This mode can be used with clusters
  in which write operations are isolated to a single node.

By default, PXC Strict Mode is set to ``ENFORCING``,
except if the node is acting as a standalone server
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

To set the mode,
use the :variable:`pxc_strict_mode` variable in the configuration file
or the ``--pxc-strict-mode`` option during ``mysqld`` startup.

.. note::

   It is better to start the server with the necessary mode
   (the default ``ENFORCING`` is highly recommended).
   However, you can dynamically change it during runtime.
   For example, to set PXC Strict Mode to ``PERMISSIVE``,
   run the following command::

      mysql> SET pxc_strict_mode=PERMISSIVE;

.. note::

   To further ensure data consistency,
   it is important to have all nodes in the cluster
   running with the same configuration,
   including the value of ``pxc_strict_mode`` variable.

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

|PXC| currently supports replication only for tables
that use a transactional storage engine (XtraDB or InnoDB).
To ensure data consistency,
the following statements should not be allowed for tables
that use a non-transactional storage engine (MyISAM, MEMORY, CSV, etc.):

* Data manipulation statements that perform writing to table
  (for example, ``INSERT``, ``UPDATE``, ``DELETE``, etc.)

* The following administrative statements:
  ``CHECK``, ``OPTIMIZE``, ``REPAIR``, and ``ANALYZE``

* ``TRUNCATE TABLE`` and ``ALTER TABLE``

Depending on the selected mode, the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, all operations are permitted.

``PERMISSIVE``

 At startup, no validation is perfromed.

 At runtime, all operations are permitted,
 but a warning is logged when an undesirable operation
 is performed on an unsupported table.

``ENFORCING`` or ``MASTER``

 At startup, no validation is performed.

 At runtime, any undesirable operation performed on an unsupported table
 is denied and an error is logged.

.. note:: Unsupported tables can be converted to use a supported storage engine.

MyISAM replication
------------------

|PXC| provides experimental support for replication of tables
that use the MyISAM storage engine.
Due to the non-transactional nature of MyISAM,
it is not likely to ever be fully supported in |PXC|.

MyISAM replication is controlled
using the :variable:`wsrep_replicate_myisam` variable,
which is set to ``OFF`` by default.
Due to its unreliability, MyISAM replication should not be enabled
if you want to ensure data consistency.

Depending on the selected mode, the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, you can set :variable:`wsrep_replicate_myisam` to any value.

``PERMISSIVE``

 At startup, if :variable:`wsrep_replicate_myisam` is set to ``ON``,
 a warning is logged and startup continues.

 At runtime, it is permitted to change :variable:`wsrep_replicate_myisam`
 to any value, but if you set it to ``ON``, a warning is logged.

``ENFORCING`` or ``MASTER``

 At startup, if :variable:`wsrep_replicate_myisam` is set to ``ON``,
 an error is logged and startup is aborted.

 At runtime, any attempt to change :variable:`wsrep_replicate_myisam`
 to ``ON`` fails and an error is logged.

.. note:: The :variable:`wsrep_replicate_myisam` variable controls
   *replication* for MyISAM tables,
   and this validation only checks whether it is allowed.
   Undesirable operations for MyISAM tables
   are restricted using the :ref:`storage-engine` validation.

Binary log format
-----------------

|PXC| supports only the default row-based binary logging format.
Setting the |binlog_format|_ variable to anything but ``ROW`` at startup
is not allowed, because this changes the global scope,
which must be set to ROW.
Validation is performed only at runtime and against session scope.

Depending on the selected mode, the following happens:

``DISABLED``

 At runtime, you can set ``binlog_format`` to any value.

``PERMISSIVE``

 At runtime, it is permitted to change ``binlog_format``
 to any value, but if you set it to anything other than ``ROW``,
 a warning is logged.

``ENFORCING`` or ``MASTER``

 At runtime, any attempt to change ``binlog_format``
 to anything other than ``ROW`` fails and an error is logged.

.. |binlog_format| replace:: ``binlog_format``
.. _binlog_format: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_format

Tables without primary keys
---------------------------

|PXC| cannot properly propagate certain write operations
to tables that do not have primary keys defined.
Undesirable operations include data manipulation statements
that perform writing to table (especially ``DELETE``).

Depending on the selected mode, the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, all operations are permitted.

``PERMISSIVE``

 At startup, no validation is perfromed.

 At runtime, all operations are permitted,
 but a warning is logged when an undesirable operation
 is performed on a table without an explicit primary key defined.

``ENFORCING`` or ``MASTER``

 At startup, no validation is performed.

 At runtime, any undesirable operation
 performed on a table without an explicit primary key
 is denied and an error is logged.

Log output
----------

|PXC| does not support tables in the MySQL database
as the destination for log output.
By default, log entries are written to file.
This validation checks the value of the |log_output|_ variable.

Depending on the selected mode, the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, you can set ``log_output`` to any value.

``PERMISSIVE``

 At startup, if ``log_output`` is set only to ``TABLE``,
 a warning is logged and startup continues.

 At runtime, it is permitted to change ``log_output``
 to any value, but if you set it only to ``TABLE``,
 a warning is logged.

``ENFORCING`` or ``MASTER``

 At startup, if ``log_output`` is set only to ``TABLE``,
 an error is logged and startup is aborted.

 At runtime, any attempt to change ``log_output`` only to ``TABLE`` fails
 and an error is logged.

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

``DISABLED`` or ``MASTER``

 At startup, no validation is performed.

 At runtime, all operations are permitted.

``PERMISSIVE``

 At startup, no validation is performed.

 At runtime, all operations are permitted,
 but a warning is logged when an undesirable operation is performed.

``ENFORCING``

 At startup, no validation is performed.

 At runtime, any undesirable operation is denied and an error is logged.

Auto-increment lock mode
------------------------

The lock mode for generating auto-increment values must be *interleaved*
to ensure that each node generates a unique (but non-sequential) identifier.

This validation checks the value of the |innodb_autoinc_lock_mode|_ variable.
By default, the variable is set to ``1`` (*consecutive* lock mode),
but it should be set to ``2`` (*interleaved* lock mode).

Depending on the strict mode selected,
the following happens:

``DISABLED``

 At startup, no validation is performed.

``PERMISSIVE``

 At startup, if ``innodb_autoinc_lock_mode`` is not set to ``2``,
 a warning is logged and startup continues.

``ENFORCING`` or ``MASTER``

 At startup, if ``innodb_autoinc_lock_mode`` is not set to ``2``,
 an error is logged and startup is aborted.

.. note:: This validation is not performed during runtime,
   because the ``innodb_autoinc_lock_mode`` variable
   cannot be set dynamically.

.. |innodb_autoinc_lock_mode| replace:: ``innodb_autoinc_lock_mode``
.. _innodb_autoinc_lock_mode: http://dev.mysql.com/doc/refman/5.7/en/innodb-parameters.html#sysvar_innodb_autoinc_lock_mode

Combining schema and data changes in a single statement
-------------------------------------------------------

|PXC| does not support ``CREATE TABLE ... AS SELECT`` (CTAS) statements,
because they combine both schema and data changes.

Depending on the strict mode selected,
the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, all operations are permitted.

``PERMISSIVE``

 At startup, no validation is perfromed.

 At runtime, all operations are permitted,
 but a warning is logged when a CTAS operation is performed.

``ENFORCING``

 At startup, no validation is performed.

 At runtime, any CTAS operation is denied and an error is logged.

.. note:: CTAS operations for temporary tables are permitted
   even in strict mode.

Discarding and Importing Tablespaces
------------------------------------

``DISCARD TABLESPACE`` and ``IMPORT TABLESPACE``
are not replicated using TOI.
This can lead to data inconsistency if executed on only one node.

Depending on the strict mode selected,
the following happens:

``DISABLED``

 At startup, no validation is performed.

 At runtime, all operations are permitted.

``PERMISSIVE``

 At startup, no validation is perfromed.

 At runtime, all operations are permitted,
 but a warning is logged when you discard or import a tablespace.

``ENFORCING``

 At startup, no validation is performed.

 At runtime, discarding or importing a tablespace is denied
 and an error is logged.

.. rubric:: References

.. target-notes::
