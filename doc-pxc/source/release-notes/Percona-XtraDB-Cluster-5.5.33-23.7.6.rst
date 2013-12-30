.. rn:: 5.5.33-23.7.6

========================================
 |Percona XtraDB Cluster| 5.5.33-23.7.6
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on September 18th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.33-23.7.6/>`_ or from our :doc:`software repositories </installation>`.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

New Features
============
 
 Default :ref:`state_snapshot_transfer` method, defined in :variable:`wsrep_sst_method` has been changed from ``mysqldump`` to ``rsync``.

 New :variable:`wsrep_reject_queries` has been implemented that can be used to reject queries for that node. This can be useful if someone wants to manually run maintenance on the node like ``mysqldump`` without need to change the settings on the load balancer. 

 Variable :variable:`wsrep_sst_donor` has been extended to accept multiple hostnames that are preferred as :ref:`state_snapshot_transfer` donors. This can be helpful in case other nodes on the list go down or it can be used as a whitelist during automated provisioning.

 Desync functionality has now been exposed to the client. This can be done either via ``/*! WSREP_DESYNC */`` comment on the query or by setting the global :variable:`wsrep_desync` variable to ``1``.

Improvements to :ref:`XtraBackup SST <xtrabackup_sst>`
------------------------------------------------------

.. note::
    There are several changes to Xtrabackup SST from 5.5.33 onwards. Make sure to check :ref:`XtraBackup SST <xtrabackup_sst>` for details, and also `incompatibilities <http://www.percona.com/doc/percona-xtradb-cluster/errata.html#incompatibilities>`_ for any issues.

 |Percona XtraDB Cluster| has implemented progress indicator for :ref:`XtraBackup SST <xtrabackup_sst>`.

 |Percona XtraDB Cluster| has implemented new rate limiting, :option:`rlimit`, option for :ref:`XtraBackup SST <xtrabackup_sst>` that can be used to avoid saturating the donor node.

 |Percona XtraDB Cluster| has added new :ref:`XtraBackup SST <xtrabackup_sst>` :option:`time` option that can be used to see how much time different stages of :ref:`state_snapshot_transfer` are taking. 

 |Percona XtraDB Cluster| has implemented additional :ref:`XtraBackup SST <xtrabackup_sst>` encryption option. Beside standard |Percona XtraBackup| encryption, new ``OpenSSL`` based encryption can be specified in the :option:`encrypt` option.

 :ref:`XtraBackup SST <xtrabackup_sst>` now works in two stages. This was implemented to avoid issues like bug bug:`1193240`.

Bugs fixed 
==========
 
 When multiple slave threads were configured, if there was a query on different transaction that inserts a row, and a query on another transaction within the same thread id that depends on the first row (FK constraint), sometimes the second transaction would be executed first causing the Foreign Key violation. Bug fixed :bug:`1217653`.

 When variable :variable:`wsrep_provider` was set to none it would cause cluster to hang. Bug fixed :bug:`1208493`.

 |Percona XtraDB Cluster| would crash with message: ``Error "no such a transition EXECUTING -> COMMITTED"`` on the master node. This bug was fixed only for some cases. Bug fixed :bug:`1123233`.

 Running DDL commands while variable :variable:`wsrep_OSU_method` was set to ``TOI - Total Order Isolation`` could lead to server deadlock. Bug fixed :bug:`1212955`.

 Stopping mysql process with ``inet`` script didn't work if ``PID`` file was provided as ``pid_file`` option in the :file:`my.cnf` configuration file. Bug fixed :bug:`1208865`.

 When :option:`read-only` variable was set to ``ON``, |Percona XtraDB Cluster| would block ``SELECT`` statements as well. Bug fixed :bug:`1091099`.

 In geo-DR setup using ``garbd``, performance would degrade with node count when cross data center link was down. Bug fixed :bug:`1182960`.

 :option:`wsrep_recover` was being run even if it wasn't used. Script now checks if :file:`grastate.dat` file has non-zero ``uuid`` and -1 ``seqno`` before it decides to start with :option:`wsrep_recover` option. Bug fixed :bug:`1193781`.

 ``PID`` detection in the ``init`` script wasn't working correctly if the ``PID`` file was specified with the relative path. Bug fixed :bug:`1194998`.

 :ref:`state_snapshot_transfer` authentication password was displayed in the ``ps`` output. Bug fixed :bug:`1200727`.

 Fixed the packaging issue caused by shared-compat linking. Bug fixed :bug:`1201393`.

 Fixed the platform dependent code in wsrep which was required to make the code portable to ``MacOS X`` and ``FreeBSD``. Bug fixed :bug:`1201893`.

 |Percona XtraDB Cluster| donor node would get stuck during the :ref:`state_snapshot_transfer` when the `threadpool plugin <http://www.percona.com/doc/percona-server/5.5/performance/threadpool.html>`_ was used. Bug fixed :bug:`1206565`.

 ``pyclustercheck`` script did not work correctly with HAProxy. Bug fixed :bug:`1210239`.

 ``pyclustercheck`` script didn't work as expected when available node was a donor. Bug fixed :bug:`1211249`.
 
 New bootstrap method ``bootstrap-pxc`` which was introduced in :rn:`5.5.31-23.7.5` didn't check if mysqld process was already running before starting the new process. Bug fixed :bug:`1211505`.

 When table was created with ``LIKE`` and the source table was temporary table, create statement would be replicated to the slave nodes where it couldn't be applied because the temporary table wasn't present on the slave nodes. This would cause other nodes to crash if there were later ``DML`` commands for this table. Bug fixed :bug:`1212247`.
 
 Non-unique indexes in a parent table (as referenced by some foreign key constraint), would be included in write set population. i.e. key values in *all* non-unique indexes will be appended in the write set's key set. This could cause excessive multi-master conflicts, especially if the parent table has non-unique indexes with low selectivity. Bug fixed :bug:`1216805`.

 Added information to |InnoDB| status if the transaction is waiting on ``TOI``. Bug fixed :bug:`1219856`.

 Binary build was linked against ``libssl.so.10`` and ``libcrypto.so.10`` which was making it hard to run on ``SUSE Linux Enterprise Server 11``. Bug fixed :bug:`1213855`.

 ``GU_AVPHYS_SIZE`` would report more available memory than could be addressed on 32-bit systems. Bug fixed :bug:`1204241`.

Other bug fixes: bug fixed :bug:`1210638`, bug fixed :bug:`1222777`, bug fixed :bug:`1216904`, bug fixed :bug:`1205467`, bug fixed :bug:`1196898`, bug fixed :bug:`1195355`, bug fixed :bug:`1049599`, bug fixed :bug:`1191395`, bug fixed :bug:`1017526`, bug fixed :bug:`1213073`, bug fixed :bug:`1171759`, bug fixed :bug:`1210618`, bug fixed :bug:`1190756`.

Known Issues
============

For Debian/Ubuntu users: |Percona XtraDB Cluster| 5.5.33-23.7.6 includes a new dependency, the ``socat`` package. If the ``socat`` is not previously installed, ``percona-xtradb-cluster-server-5.5`` may be held back. In order to upgrade, you need to either install ``socat`` before running the ``apt-get upgrade`` or by writing the following command: ``apt-get install percona-xtradb-cluster-server-5.5``. For *Ubuntu* users the ``socat`` package is in the universe repository, so the repository will have to be enabled in order to install the package.

Based on `Percona Server 5.5.33-31.1 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.33-31.1.html>`_ including all the bug fixes in it, `Galera Replicator <https://launchpad.net/galera/+milestone/23.2.7>`_ and on `Codership wsrep API 5.5.33-23.7.6 <https://launchpad.net/codership-mysql/+milestone/5.5.33-23.7.6>`_, |Percona XtraDB Cluster| `5.5.33-23.7.6 <https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.33-23.7.6>`_ is now the current stable release. All of |Percona|'s software is open-source and free. 

Percona XtraDB Cluster `Errata <http://www.percona.com/doc/percona-xtradb-cluster/errata.html>`_ can be found in our documentation.
