.. _strict-mode:

===========
Strict Mode
===========

This mode is designed to avoid problems with improper configuration of |PXC|.
It performs a number of validations against essential features of |PXC|.
Some validations are performed only at startup,
while others remain enabled during runtime.

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

By default, *enforcing* strict mode is enabled,
except if the node is acting as a standalone server
or the node is bootstrapping, then strict mode is disabled.

To select the mode,
use the :variable:`wsrep_strict_mode` variable in the configuration file
or the ``--wsrep-strict-mode`` option during ``mysqld`` startup.

.. note::

   Although it is recommended to start the server with the necessary mode,
   you can dynamically change it during runtime.
   For example, to enable permissive strict mode::

      mysql> SET wsrep_strict_mode=PERMISSIVE;

Validations
===========

Strict mode validations are designed to ensure optimal operation of |PXC|
for the majority of setups.
This section describes the purpose and consequences of each validation.

.. contents::
   :local:

Storage engine
--------------

|PXC| currently supports replication only for tables
that use a transactional storage engine (XtraDB or InnoDB).
When you enable strict mode,
non-system persistent tables are checked during runtime.
If a write or alter operation is performed on an unsupported table,
one of the following happens:

* In case of *enforcing* or *maser* mode,
  an error is thrown and the operation is denied.

* In case of *permissive* mode,
  a warning is logged and the operation is permitted.

Due to the time that may be required to scan all tables,
this validation is not performed during startup.

.. note:: Unsupported tables can be converted to use a supported storage engine.

MyISAM replication
------------------

|PXC| provides experimental support for replication of tables
that use the MyISAM storage engine.
Due to the non-transactional nature of MyISAM,
it is not likely to ever be fully supported.

When you start the server with strict mode enabled,
it checks the value of the :variable:`wsrep_replicate_myisam` variable.
By default, this variable is set to ``OFF``.
However, if MyISAM replication is enabled, one of the following happens:

* In case of *enforcing* or *master* mode,
  an error is thrown and server startup is halted.

* In case of *permissive* mode,
  a warning is logged and server startup continues.

.. note:: This validation is not performed during runtime,
   because the :variable:`wsrep_replicate_myisam` variable
   cannot be set dynamically.

Binary log format
-----------------

The binary logging format must be set to ``ROW``.
This is the default format.

When you enable strict mode,
it checks the value of the |binlog_format| variable
both at server startup and during runtime.

In case of *enforcing* or *master* mode:

* If the ``binlog_format`` variable is set to anything other than ``ROW``
  at startup, an error will be thrown and server startup is halted.
* If you attempt to change the value of the ``binlog_format`` variable,
  an error will be thrown.

In case of *permissive* mode:

* If the ``binlog_format`` variable is set to anything other than ``ROW``
  at startup, a warning will be logged and server startup continues.
* If you attempt to change the value of the ``binlog_format`` variable,
  a warning will be logged.

.. |binlog_format| replace:: ``binlog_format``
.. _binlog_format: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_format

Tables without primary keys
---------------------------

|PXC| cannot properly propagate ``DELETE`` operations on tables
that do not have primary keys defined.
When you enable strict mode,
non-system persistent tables are checked during runtime.
If a write operation is performed on a table without a primary key,
one of the following happens:

* In case of *enforcing* or *maser* mode,
  an error is thrown and the operation is denied.

* In case of *permissive* mode,
  a warning is logged and the operation is permitted.

Due to the time that may be required to scan all tables,
this validation is not performed during startup.

Log output
----------

|PXC| does not support tables in the MySQL database
as the destination for log output.
By default, log entries are written to file.

When you enable strict mode,
it checks the value of the |log_output| variable
both at server startup and during runtime.

In case of *enforcing* or *master* mode:

* If the ``log_output`` variable is set only to ``TABLE``
  at startup, an error will be thrown and server startup is halted.
* If you attempt to change the value of the ``log_output`` variable to ``TABLE``
  during runtime, an error will be thrown.

In case of *permissive* mode:

* If the ``log_output`` variable is set only to ``TABLE``
  at startup, a warning will be logged and server startup continues.
* If you attempt to change the value of the ``log_output`` variable to ``TABLE``
  during runtime, a warning will be logged.

.. |log_output| replace:: ``log_output``
.. _log_output: http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_log_output

.. _explicit-table-locking:

Explicit table locking
----------------------

Normally used with non-transactional storage engines (for example, MyISAM),
explicit table locking is not supported by |PXC|.
There are no startup validations for this feature.  

The following features are covered by this validation:

* Use of ``LOCK`` or ``UNLOCK`` on any persistent table
* Use of ``GET_LOCK()`` and ``RELEASE_LOCK()`` functions
* ``FLUSH TABLES {table list} WITH READ LOCK``
* ``SELECT ... LOCK IN SHARE MODE``
* ``SERIALIZABLE`` transactions

The following locking features are not covered by this validation:

* Global ``FLUSH TABLES WITH READ LOCK``
* ``SELECT ... FOR UPDATE``

In case of *enforcing* mode, any attempt to use the covered features
on a persistent table is denied, an error is displayed and logged.

In case of *master* mode, any attempt to use the covered features
on any persistent table is permitted,
no errors or warnings are displayed or logged.

In case of *permissive* mode, any attempt to use ``LOCK`` or ``UNLOCK``
on any persistent table is permitted and a warning is logged.

XA transactions
---------------

|PXC| does not support distributed XA transactions.
When strict mode is enabled and it detects an XA command during runtime,
one of the following happens:

* In case of *enforcing* or *master* mode,
  an error is thrown and the command is denied.

* In case of *permissive* mode,
  a warning is logged and the command is permitted.

There are no stratup validations for this feature.

InnoDB doublewrite buffering
----------------------------

Without doublewrite buffering,
data corruption cannot be promptly detected and fixed
if a crash occurs in the middle of a page write.
Data integrity is of paramount importance for proper operation of |PXC|.

When you start the server with strict mode enabled,
it checks the value of the |innodb_doublewrite|_ variable.
By default, this variable is set to ``ON``.
However, if InnoDB doublewrite buffering is disabled,
one of the following happens:

* In case of *enforcing* or *master* mode,
  an error is thrown and server startup is halted.

* In case of *permissive* mode,
  a warning is logged and server startup continues.

.. note:: This validation is not performed during runtime,
   because the ``innodb_doublewrite`` variable
   cannot be set dynamically.

.. |innodb_doublewrite| replace:: ``innodb_doublewrite``
.. _innodb_doublewrite: http://dev.mysql.com/doc/refman/5.7/en/innodb-parameters.html#sysvar_innodb_doublewrite

Auto-increment lock mode
------------------------

The lock mode for generating auto-increment values must be *interleaved*
to ensure that each node generates a unique (but non-sequential) identifier.

When you start the server with strict mode enabled,
it checks the value of the |innodb_autoinc_lock_mode|_ variable.
By default, this variable is set to ``1`` (*consecutive* lock mode).
If the validation detects that the value is not ``2`` (*interleaved* lock mode)
at startup, one of the following happens:

* In case of *enforcing* or *master* mode,
  an error is thrown and server startup is halted.

* In case of *permissive* mode,
  a warning is logged and server startup continues.

.. note:: This validation is not performed during runtime,
   because the ``innodb_autoinc_lock_mode`` variable
   cannot be set dynamically.

.. |innodb_autoinc_lock_mode| replace:: ``innodb_autoinc_lock_mode``
.. _innodb_autoinc_lock_mode: http://dev.mysql.com/doc/refman/5.7/en/innodb-parameters.html#sysvar_innodb_autoinc_lock_mode

Combining schema and data changes in a single statement
-------------------------------------------------------

|PXC| does not support ``CREATE TABLE ... AS SELECT`` (CTAS) statements,
because they combine both schema and data changes.
When strict mode is enabled and it detects a CTAS query during runtime,
one of the following happens:

* In case of *enforcing* or *master* mode,
  an error is thrown and the query is denied.

* In case of *permissive* mode,
  a warning is logged and the query is permitted.

There are no stratup validations for this feature.

.. rubric:: References

.. target-notes::
