.. rn:: 5.5.23

=====================================
 |Percona XtraDB Cluster| 5.5.23-23.5
=====================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on May 14, 2012. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.23-23.5/>`_ or from our `software repositories <http://www.percona.com/doc/percona-xtradb-cluster/installation.html#using-percona-software-repositories>`_.

This is an General Availability release. We did our best to eliminate bugs and problems during alpha and beta testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.

List of changes:
  * Fixes merged from upstream (`Codership-mysql <http://www.codership.com/content/wsrep-patch-235-mysql-5523-released>`_).
  * Support for MyISAM, now changes to MyISAM tables are replicated to other nodes.
  * Improvements to XtraBackup SST methods, better error handling.
  * New SST wsrep_sst_method=skip, useful when you start all nodes from the same sources (i.e. backup).
  * Ability to pass list of IP addresses for a new node, it will connect to the first available.

Based on `Percona Server 5.5.23-25.3 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.23-25.3.html>`_ including all the bug fixes in it, and the latest `Galera 2.x replicator <https://code.launchpad.net/~codership/galera/2.x>`_,  |Percona XtraDB Cluster| 5.5.23-25.3 is now the current stable release. All of |Percona|'s software is open-source and free. 

