.. rn:: 5.5.29-23.7.2

========================================
 |Percona XtraDB Cluster| 5.5.29-23.7.2
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on February 12th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.29-23.7.2/>`_ or from our `software repositories <http://www.percona.com/doc/percona-xtradb-cluster/installation.html#using-percona-software-repositories>`_.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.


Bugs fixed 
==========
 
 ``DML`` operations on temporary tables would try to append the key for provider library, which could cause a memory leak. Bug fixed for |MyISAM| temporary tables, |InnoDB| temporary tables still have this issue, probably caused by upstream bug :mysqlbug:`67259`. Bug fixed :bug:`1112514` (*Seppo Jaakola*).

 Bug fix for bug :bug:`1078346` introduced a regression. Foreign key checks were skipped in the parent table, which would cause the foreign key constraint errors. Bug fixed :bug:`1117175` (*Seppo Jaakola*).

Based on `Percona Server 5.5.29-29.4 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.29-29.4.html>`_ including all the bug fixes in it, |Percona XtraDB Cluster| 5.5.29-23.7.2 is now the current stable release. All of |Percona|'s software is open-source and free. 

