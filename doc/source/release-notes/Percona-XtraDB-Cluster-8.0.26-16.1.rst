.. _PXC-8.0.26-16.1:

================================================================================
*Percona XtraDB Cluster* 8.0.26-16.1
================================================================================

:Date: January 17, 2022
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_

Percona XtraDB Cluster (PXC) supports critical business applications in your public, private, or hybrid cloud environment. Our free, open source, enterprise-grade solution includes the high availability and security features your business requires to meet your customer expectations and business goals.

Release Highlights
================================================================================

The following are a number of the notable fixes for MySQL 8.0.26, provided by Oracle, and included in this release:

* The `TLSv1 and TLSv1.1 <https://tools.ietf.org/id/draft-ietf-tls-oldversions-deprecate-02.html>`__ connection protocols are deprecated.
* Identifiers with specific terms, such as "master" or "slave" are deprecated and replaced. See the `Functionality Added or Changed section in the 8.0.26 Release Notes <https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-26.html#mysqld-8-0-26-feature>`__ for a list of updated identifiers. The following terms have been changed:
    - The identifier ``master`` is changed to ``source``
    - The identifier ``slave`` is changed to ``replica``
    - The identifier ``multithreaded slave`` (``mts``) is changed to ``multithreaded applier`` (``mta``)
* When using semisynchronous replication, either the old version or the new version of system variables and status variables are available. You cannot have both versions installed on an instance. The old system variables are available when you use the old version, but the new ones are not. The new system variables are available when you use the new version, but the old values are not. 

  In an upgrade from an earlier version to 8.0.26, enable the ``rpl_semi_sync_source`` plugin and the ``rpl_semi_sync_replica`` plugin after the upgrade has been completed. Enabling these plugins before all of the nodes are upgraded may cause data inconsistency between the nodes.

  For the source, the ``rpl_semi_sync_master`` plugin (``seminsync_master.so`` library) is the old version and the ``rpl_semi_sync_source`` plugin(``semisync_source.so`` library) is the new version.

  For the client, the ``rpl_semi_sync_slave`` plugin (``semisync_slave.so`` library) is the old version and the ``rpl_semi_sync_replica`` plugin (``semisync_replica.so`` library) is the new version

For more information, see the `MySQL 8.0.26 Release Notes <https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-26.html>`__.


Bugs Fixed
================================================================================

* :jirabug:`PXC-3824`: An incorrect directive in Systemd Unit File (Thanks to Jim Lohiser for reporting this issue)
* :jirabug:`PXC-3706`: A fix for a race condition in group commit queue (Thanks to Kevin Sauter for reporting this issue)
* :jirabug:`PXC-3739`: The ``FLUSH TABLES FOR EXPORT`` lock is released when the session ends.
* :jirabug:`PXC-3628`: The server allowed altering the storage engine to ``MyISAM`` for mysql.wsrep_* tables.
* :jirabug:`PXC-3731`: A fix for when the user deletes data from the source but does not want that data deleted from the replica. The ``sql_log_bin=0`` command had no effect and the deleted rows were replicated and written into the binary log.
* :jirabug:`PXC-3857`: The following system variables are renamed. The old variables are deprecated and may be removed in a future version.
    - ``wsrep_slave_threads`` renamed as ``wsrep_applier_threads``
    - ``wsrep_slave_FK_checks`` renamed as ``wsrep_applier_FK_checks``
    - ``wsrep_slave_UK_checks`` renamed as ``wsrep_applier_UK_checks``
    - ``wsrep_restart_slave`` renamed as ``wsrep_restart_replica``



Known Issues (unfixed problems that you should be aware of)
================================================================================

* :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tried to join an SSL-enabled cluster
* :jirabug:`PXC-3093`: A completed SST Transfer is incorrectly logged by garbd. The timing is incorrect.


