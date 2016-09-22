.. rn:: 5.5.41-25.11.1

=======================================
|Percona XtraDB Cluster| 5.5.41-25.11.1
=======================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on
September 22nd, 2016. Binaries are available from `downloads area
<http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.41-25.11.1/>`_
or from our :doc:`software repositories </installation>`.

Based on `Percona Server 5.5.41-37.0
<http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.41-37.0.html>`_
including all the bug fixes in it, Galera Replicator 2.11 and on wsrep API
25.11, |Percona XtraDB Cluster| `5.5.41-25.11.1i
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.5.41-25.11>`_ is
now the current stable release. All of |Percona|'s software is open-source and
free.

This is an General Availability release. We did our best to eliminate bugs and
problems during the testing release, but this is a software, so bugs are
expected. If you encounter them, please report them to our `bug tracking system
<https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

Bugs fixed
==========

 Due to security reasons ``ld_preload`` libraries can now only be loaded from
 the system directories (``/usr/lib64``, ``/usr/lib``) and the MySQL
 installation base directory. This fix also addresses issue with where limiting
 didn't work correctly for relative paths. Bug fixed :bug:`1624247`.

 Fixed possible privilege escalation that could be used when running ``REPAIR
 TABLE`` on a ``MyISAM`` table. Bug fixed :bug:`1624397`.

 The general query log and slow query log cannot be written to files ending in
 :file:`.ini` and :file:`.cnf` anymore. Bug fixed :bug:`1624400`.

 Implemented restrictions on symlinked files (:file:`error_log`,
 :file:`pid_file`) that can't be used with ``mysqld_safe``. Bug fixed
 :bug:`1624449`.

Other bugs fixed: :bug:`1553938`.
