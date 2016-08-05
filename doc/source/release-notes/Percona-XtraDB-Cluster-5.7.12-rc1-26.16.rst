.. rn:: 5.7.12-rc1-26.16

=========================================
|Percona XtraDB Cluster| 5.7.12-rc1-26.16 
=========================================

Percona is glad to announce the release of
|PXC| 5.7.12-rc1-26.16 on Audust 8, 2016.
Binaries are available from the
`downloads area
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST>`_
or from our :ref:`software repositories <install>`.

|PXC| 5.7.12-rc1-26.16 is based on the following:

* `Percona Server 5.7.12 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.12.html>`_

* `Galera Replicator 3.16 <https://github.com/percona/galera/tree/rel-3.16>`_

New Features
============

* [PXC-602] **PXC Strict Mode**:
  Use the :variable:`pxc_strict_mode` variable in the configuration file
  or the ``--pxc-strict-mode`` option during ``mysqld`` startup.
  For more information, see :ref:`pxc-strict-mode`.

* [PXC-626] **Galera instruments exposed in Performance Schema**:
  This includes mutexes, condition variables, file instances, and thread.

Bug Fixes
=========

* [PXC-233] Fixed error messages.

* [PXC-558] Fixed the failure of SST via ``mysqldump`` with ``gtid_mode=ON``.

* [PXC-570] Added check for TOI that ensures node readiness to process DDL+DML
  before starting the execution.

* [PXC-585] Removed protection against repeated calls of ``wsrep->pause()``
  on the same node to allow parallel RSU operation.

* [PXC-592] Changed ``wsrep_row_upd_check_foreign_constraints``
  to ensure that ``fk-reference-table`` is open before marking it open.

* [PXC-601] Fixed error when running ``SHOW STATUS`` during group state update.

* [PXC-605] Changed the ``sst_flush_tables()`` function
  to return a non-negative error code and thus pass assertion.

* [PXC-613] Fixed memory leak due to stats not freeing
  when toggling the :variable:`wsrep_provider` variable.

* [PXC-624] Fixed ``galera_bf_abort`` failures by ensuring
  that nodes form a cluster.

* [PXC-625] Fixed failure of ``ROLLBACK`` to register ``wsrep_handler``

* [PXC-636] Fixed failure of symmetric encryption during SST.

---

* [PXC-630] [PXC-641] [PXC-647] [PXC-617] [PXC-650] very low-level?

* [PXC-587] [PXC-614] [PXC-616] [PXC-632] [PXC-639] [PXC-658] Fixed as part of pxc_strict_mode?

* [PXC-635] Error message fix. Need to mention?

Other Changes
==================

* [PXC-564] Added support for sending the keyring when performing encrypted SST.
  For more information, see :ref:`encrypt-traffic`.

* [PXC-571] Changed the code of ``THD_PROC_INFO``
  to reflect what the thread is currently doing.

* [PXC-612] Using XtraBackup as the SST method
  now requires Percona XtraBackup 2.4.3 or later.

* [PXC-628] [PXC-651] Improved rollback process to ensure that when a transaction
  is rolled back, any statements open by the transaction are also rolled back.

* [PXC-648] Removed the ``sst_special_dirs`` variable.

* [PXC-659] Disabled switching of ``slave_preserve_commit_order`` to ``ON``.

* [PQA-115] Changed the default :file:`my.cnf` configuration.

---

* [BLD-464] 455-458, 460, 461 should I mention BLD issues?

* [PXC-267] [PXC-268] Do these change anything for users?


