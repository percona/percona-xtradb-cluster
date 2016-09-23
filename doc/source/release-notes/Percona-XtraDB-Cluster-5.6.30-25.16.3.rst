.. rn:: 5.6.30-25.16.3

=====================================
Percona XtraDB Cluster 5.6.30-25.16.3
=====================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.30-25.16.3 on September 21, 2016.
Binaries are available from the
`downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.30-25.16.3/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.30-25.16.3 is now the current release,
based on the following:

* `Percona Server 5.6.30-76.3
  <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.30-76.3.html>`_

* Galera Replication library 3.16

* wsrep API version 25

Bug Fixed
=========

 Limiting ``ld_preload`` libraries to be loaded from specific directories in
 ``mysqld_safe`` didn't work correctly for relative paths. Bug fixed
 :bug:`1624247`.

 Fixed possible privilege escalation that could be used when running ``REPAIR
 TABLE`` on a ``MyISAM`` table. Bug fixed :bug:`1624397`.

 The general query log and slow query log cannot be written to files ending in
 :file:`.ini` and :file:`.cnf` anymore. Bug fixed :bug:`1624400`.

 Implemented restrictions on symlinked files (:file:`error_log`,
 :file:`pid_file`) that can't be used with ``mysqld_safe``. Bug fixed
 :bug:`1624449`.

Other bugs fixed: :bug:`1553938`.
