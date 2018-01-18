.. rn:: 5.7.20-29.24

=====================================
Percona XtraDB Cluster 5.7.20-29.24
=====================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.7.20-29.24 on January 26, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

.. note:: Due to new package dependency,
   Ubuntu/Debian users should use ``apt-get dist-upgrade``, ``apt upgrade``,
   or ``apt-get install percona-xtradb-cluster-57`` to upgrade.

Percona XtraDB Cluster 5.7.20-29.24 is now the current release,
based on the following:

* `Percona Server 5.7.20-18 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.20-18.html>`_

* Galera Replication library 3.22

* Galera/Codership WSREP API Release 5.7.20

All Percona software is open-source and free.

NEW FEATURES:
=============

* Ubuntu 17.10 Artful Aardvark is now supported.

* :jirabug:`PXC-737`: freezing gcache purge was implemented to facilitate node
  joining through IST, avoiding time consuming SST process.

* :jirabug:`PXC-822`: a usability improvement was made to timeout error
  messages, the name of the configuration variable which caused the
  timeout was added to the message.

* :jirabug:`PXC-866`: a new variable :variable:`wsrep_last_applied`, in
  addition to :variable:`wsrep_last_committed` one, was introduced to clearly
  separate last committed and last applied transaction numbers.

* :jirabug:`PXC-868`: on the Joiner, during SST, :variable:`tmpdir` variable
  under ``[sst]`` section can be used to specify temporary SST files storage
  different from the default ``datadir/.sst`` one.


Fixed Bugs
==========

* :jirabug:`PXC-889`: fixed an issue where a node with an invalid value for
  :variable:`wsrep_provider` was allowed to start up and operate in standalone
  mode, which could lead to data inconsistency. The node will now abort in
  this case. Bug fixed :bug:`1728774`

* :jirabug:`PXC-806`: fixed an abort caused by an early read of the
  ``query_id``, ensuring valid  ids are assigned to subsequent transactions.

* :jirabug:`PXC-850`: ensured that a node, because of data inconsistency,
  isolates itself before leaving the cluster, thus allowing pending nodes
  to re-evaluate the quorum. Bug fixed :bug:`1704404`

* :jirabug:`PXC-867`: ``wsrep_sst_rsync`` script was overwriting
  :variable:`wsrep_debug` configuration setting making it not to be taken
  into account.

* :jirabug:`PXC-873`: fixed formatting issue in the error message appearing
  when SST is not possible due to a timeout. Bug fixed :bug:`1720094`

* :jirabug:`PXC-874`: PXC acting as async slave reported unhandled transaction
  errors, namely "Rolling back unfinished transaction".

* :jirabug:`PXC-875`: fixed an issue where toggling :variable:`wsrep_provider`
  off and on failed to reset some internal variables and resulted in PXC
  logging an "Unsupported protocol downgrade" warning. Bug fixed :bug:`1379204`

* :jirabug:`PXC-877`: fixed PXC hang caused by an internal deadlock.

* :jirabug:`PXC-878`: thread failed to mark exit from the InnoDB server
  concurrency and therefore never got un-register in InnoDB concurrency system.

* :jirabug:`PXC-879`: fixed a bug where a ``LOAD DATA`` command used
  with GTIDs was executed on one node, but the other nodes would receive less
  rows than the first one. Bug fixed :bug:`1741818`

* :jirabug:`PXC-880`: insert to table without primary key was possible with
  insertable view if :variable:`pxc_strict_mode` variable was set to ENFORCING.
  Bug fixed :bug:`1722493`

* :jirabug:`PXC-883`: fixed ``ROLLBACK TO SAVEPOINT`` incorrect operation on
  slaves by avoiding useless wsrep plugin register for a savepoint rollback.
  Bug fixed :bug:`1700593`

* :jirabug:`PXC-885`: fixed IST hang when ``keyring_file_data`` is set.
  Bug fixed :bug:`1728688`

* :jirabug:`PXC-887`: gcache page files were unnecessarily created due to
  an error in projecting gcache free size when configured to recover on
  restart.

* :jirabug:`PXC-895`: fixed transaction loss after recovery by avoiding
  interruption of the binlog recovery based on wsrep saved position.
  Bug fixed :bug:1734113

* :jirabug:`PXC-897`: fixed empty :variable:`gtid_executed` variable after
  recovering the position of a node with ``--wsrep_recover``.

* :jirabug:`PXC-906`: fixed certification failure in the case of a node
  restarting at the same time when frequent ``TRUNCATE TABLE`` commands and
  DML writes occur simultaneously on other nodes. Bug fixed :bug:`1737731`

* :jirabug:`PXC-909`: qpress package was turned into a dependency from
  suggested/recommended one on Debian 9.

* :jirabug:`PXC-903` and :jirabug:`PXC-910`: init.d/systemctl scripts on Debian
  9 were updated to avoid starting wsrep-recover if there was no crash, and to
  fix an infinite loop at mysqladmin ping fail because of nonexistent ping
  user.

* :jirabug:`PXC-915`: suppressing DDL/TOI replication in case of
  :variable:`sql_log_bin` zero value didn't work when DDL statement was
  modifying an existing table, resulting in an error.

