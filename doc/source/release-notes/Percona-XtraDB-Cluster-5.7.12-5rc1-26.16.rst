.. rn:: 5.7.12-5rc1-26.16

==========================================
|Percona XtraDB Cluster| 5.7.12-5rc1-26.16
==========================================

Percona is glad to announce the release of
|PXC| 5.7.12-5rc1-26.16 on August 9, 2016.
Binaries are available from the
`downloads area
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST>`_
or from our :ref:`software repositories <install>`.

|PXC| 5.7.12-5rc1-26.16 is based on the following:

* `Percona Server 5.7.12 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.12.html>`_

* `Galera Replicator 3.16 <https://github.com/percona/galera/tree/rel-3.16>`_

New Features
============

* **PXC Strict Mode**:
  Use the :variable:`pxc_strict_mode` variable in the configuration file
  or the ``--pxc-strict-mode`` option during ``mysqld`` startup.
  For more information, see :ref:`pxc-strict-mode`.

* **Galera instruments exposed in Performance Schema**:
  This includes mutexes, condition variables, file instances, and threads.

Bug Fixes
=========

* Fixed error messages.

* Fixed the failure of SST via ``mysqldump`` with ``gtid_mode=ON``.

* Added check for TOI that ensures node readiness to process DDL+DML
  before starting the execution.

* Removed protection against repeated calls of ``wsrep->pause()``
  on the same node to allow parallel RSU operation.

* Changed ``wsrep_row_upd_check_foreign_constraints``
  to ensure that ``fk-reference-table`` is open before marking it open.

* Fixed error when running ``SHOW STATUS`` during group state update.

* Corrected the return code of ``sst_flush_tables()`` function
  to return a non-negative error code and thus pass assertion.

* Fixed memory leak and stale pointer due to stats not freeing
  when toggling the :variable:`wsrep_provider` variable.

* Fixed failure of ``ROLLBACK`` to register ``wsrep_handler``

* Fixed failure of symmetric encryption during SST.

Other Changes
=============

* Added support for sending the keyring when performing encrypted SST.
  For more information, see :ref:`encrypt-traffic`.

* Changed the code of ``THD_PROC_INFO``
  to reflect what the thread is currently doing.

* Using XtraBackup as the SST method
  now requires Percona XtraBackup 2.4.4 or later.

* Improved rollback process to ensure that when a transaction
  is rolled back, any statements open by the transaction are also rolled back.

* Removed the ``sst_special_dirs`` variable.

* Disabled switching of ``slave_preserve_commit_order`` to ``ON``
  when running PXC in cluster mode, as it conflicts with existing
  multi-master commit ordering resolution algorithm in Galera.

* Changed the default :file:`my.cnf` configuration.

* Other low-level fixes and improvements for better stability.

