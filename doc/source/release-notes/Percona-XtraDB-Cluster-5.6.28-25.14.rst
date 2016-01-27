.. rn:: 5.6.28-25.14

===================================
Percona XtraDB Cluster 5.6.28-25.14 
===================================

Percona is glad to announce the release of |Percona XtraDB Cluster| 5.6.28-25.14 on January 29, 2016. Binaries are available from the `downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.28-25.14/>`_ or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.28-25.14 is now the current release, based on the following:

* `Percona Server 5.6.28-76.1 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.28-76.1.html>`_

* `Galera Replicator 3.14 <https://github.com/codership/galera/issues?q=milestone%3A25.3.14>`_

* `Codership wsrep API 25.13 <https://github.com/codership/mysql-wsrep/issues?q=milestone%3A5.6.28-25.13>`_

All Percona software is open-source and free. Details of this release can be found in the `5.6.28-25.14 milestone on Launchpad <https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.28-25.14>`_.

For more information about relevant Codership releases, see `this announcement <http://galeracluster.com/2016/01/announcing-galera-cluster-5-5-42-and-5-6-25-with-galera-3-12-2-2/>`_.

Bugs Fixed
==========

* :bug:`1494399`: Fixed issue caused by replication of events on certain system tables (for example, ``mysql.slave_master_info``, ``mysql.slave_relay_log_info``). Replication in the Galera eco-system is now avoided when bin-logging is disabled for said tables.

  .. note:: As part of this fix, when bin-logging is enabled, replication in the Galera eco-system will happen only if ``BINLOG_FORMAT`` is set to either ``ROW`` or ``STATEMENT``. The recommended format is ``ROW``, while ``STATEMENT`` is required only for the ``pt-table-checksum`` tool to operate correctly. If ``BINLOG_FORMAT`` is set to ``MIXED``, replication of events in the Galera eco-system tables will not happen even with bin-logging enabled for those tables.

* :bug:`1522385`: Fixed GTID holes caused by skipped replication. A slave might ignore an event replicated from master, if the same event has already been executed on the slave. Such events are now propagated in the form of special GTID events to maintain consistency.

* :bug:`1532857`: The installer now creates a :file:`/var/lib/galera/` directory (assigned to user ``nobody``), which can be used by ``garbd`` in the event it is started from a directory that ``garbd`` cannot write to.

Known Issues
============

* :bug:`1531842`: Two instances of ``garbd`` cannot be started from the same working directory. This happens because each instance creates a state file (:file:`gvwstate.dat`) in the current working directory by default. Although ``garbd`` is configured to use the ``base_dir`` variable, it was not registered due to a bug. Until ``garbd`` is fixed, you should start each instance from a separate working directory.

