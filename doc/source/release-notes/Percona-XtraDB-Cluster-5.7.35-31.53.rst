.. _PXC-5.7.35-31.53:

================================================================================
*Percona XtraDB Cluster* 5.7.35-31.53
================================================================================

:Date: November 18, 2021
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.7/install/index.html>`_

Percona XtraDB Cluster (PXC) supports critical business applications in your public, private, or hybrid cloud environment. Our free, open source, enterprise-grade solution includes the high availability and security features your business requires to meet your customer expectations and business goals.

Release Highlights
================================================================================

The following are some of the notable fixes for MySQL 5.7.35, provided by Oracle, and included in this release:

* :mysqlbug:`104373`: Fixes failure of ``OPTIMIZE TABLE`` command writing to the binary log and replicated to replicas.

* :mysqlbug:`104451`: Fixes which event turns on the ``LOG_EVENT_THREAD_SPECIFIC_F`` flag.

For more information, see the `MySQL 5.7.35 Release Notes <https://dev.mysql.com/doc/relnotes/mysql/5.7/en/news-5-7-35.html>`__

The following are the notable fixes for Galera Cluster, provided by Codership, and included in this release:

* :wsrepbug:`381`: Disables binary log purging when the ``mysqld`` starts with ``--wsrep-recover`` option.
* :mdevbug:`25551`: Disables tables without a primary key from the parallel applying of write sets.



Bugs Fixed
================================================================================

* :jirabug:`PXC-3589`: Documentation: Updates in :ref:`limitations` that the ``LOCK=NONE`` clause is no longer allowed in an INPLACE ALTER TABLE statement. (Thanks to user Brendan Byrd for reporting this issue)
* :jirabug:`PXC-3637`: Changes the service start sequence to allow more time for mounting local or remote directories with large amounts of data. (Thanks to user Eric Gonyea for reporting this issue)
* :jirabug:`PXC-3741`: Fix when a network issue causes the Incremental State Transfer (IST) receiver to stall.


