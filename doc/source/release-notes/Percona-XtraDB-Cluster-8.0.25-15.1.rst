.. _PXC-8.0.25-15.1:

================================================================================
*Percona XtraDB Cluster* 8.0.25-15.1
================================================================================

:Date: November 22, 2021
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_.

Percona XtraDB Cluster 8.0.25-15.1 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.25-15 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.25-15.html>`__ for more details on these changes.

Percona XtraDB Cluster (PXC) supports critical business applications in your public, private, or hybrid cloud environment. Our free, open source, enterprise-grade solution includes the high availability and security features your business requires to meet your customer expectations and business goals.

Release Highlights
=================================================

A Non-Blocking Operation method for online schema changes in Percona XtraDB Cluster. This mode is similar to the Total Order Isolation (TOI) mode, whereas a data definition language (DDL) statement (for example, ``ALTER``) is executed on all nodes in sync. The difference is that in the NBO mode, the DDL statement acquires a metadata lock that locks the table or schema at a late stage of the operation, which is a more efficient locking strategy.

Note that the NBO mode is a **Tech Preview** feature. We do not recommend that you use this mode in a production environment. For more information, see :ref:`nbo`.

The following list are some of the notable fixes for MySQL 8.0.26, provided by Oracle, and included in this release:

* :mysqlbug:`103980`: Fixes the allocation of too much memory when dropping a database with many partitioned tables.
* :mysqlbug:`103743`: Fixes a slow startup when the tables are encrypted.

For more information, see the `MySQL 8.0.26 Release Notes <https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-26.html>`__.



New Features
=====================================================

* :jirabug:`PXC-3265` Implements the Non-Blocking Operations (NBO) mode for an Online schema upgrade.


Bugs Fixed
=====================================================

* :jirabug:`PXC-3275`: Fix the documented APT package list to match the packages listed in the Repo. (Thanks to user Hubertus Krogmann for reporting this issue)
* :jirabug:`PXC-3387`: Performing an intermediate commit does not call wsrep commit hooks.
* :jirabug:`PXC-3449`: Fix for missing dependencies which were carried out in replication writesets caused Galera to fail.
* :jirabug:`PXC-3589`: Documentation: Updates in :ref:`limitations` that the ``LOCK=NONE`` clause is no longer allowed in an INPLACE ALTER TABLE statement. (Thanks to user Brendan Byrd for reporting this issue)
* :jirabug:`PXC-3611`: Fix that deletes any keyring.backup file if it exists for SST operation.
* :jirabug:`PXC-3608`: Fix a concurrency issue that caused a server exit when attempting to read a foreign key.
* :jirabug:`PXC-3637`: Changes the service start sequence to allow more time for mounting local or remote directories with large amounts of data. (Thanks to user Eric Gonyea for reporting this issue)
* :jirabug:`PXC-3679`: Fix for SST failures after the update of socat to '1.7.4.0'.
* :jirabug:`PXC-3706`: Fix adds a wait to ``wsrep_after_commit`` until the first thread in a group commit queue is available. 
* :jirabug:`PXC-3729`: Fix for conflicts when multiple applier threads execute certified transactions and are in High-Priority transaction mode. 
* :jirabug:`PXC-3731`: Fix for incorrect writes to the binary log when ``sql_log_bin=0``.
* :jirabug:`PXC-3733`: Fix to clean the WSREP transaction state if a transaction is requested to be re-prepared.
