.. rn:: 5.5.29-23.7.1

========================================
 |Percona XtraDB Cluster| 5.5.29-23.7.1
========================================

Percona is glad to announce the release of |Percona XtraDB Cluster| on January 30th, 2013. Binaries are available from `downloads area <http://www.percona.com/downloads/Percona-XtraDB-Cluster/5.5.29-23.7.1/>`_ or from our `software repositories <http://www.percona.com/doc/percona-xtradb-cluster/installation.html#using-percona-software-repositories>`_.

This is an General Availability release. We did our best to eliminate bugs and problems during the testing release, but this is a software, so bugs are expected. If you encounter them, please report them to our `bug tracking system <https://bugs.launchpad.net/percona-xtradb-cluster/+filebug>`_.


Bugs fixed 
==========

  In some cases when node is recovered variable :variable:`threads_running` would become huge. Bug fixed :bug:`1040108` (*Teemu Ollakka*).

  Variable :variable:`wsrep_defaults_file` would be set up to the value in the last configuration file read. Bug fixed by keeping the value found in the top configuration file. Bug fixed :bug:`1079892` (*Alex Yurchenko*).

  Variable :variable:`wsrep_node_name` was initialized before the ``glob_hostname``, which lead to empty value for :variable:`wsrep_node_name` if it wasn't set explicitly. Bug fixed :bug:`1081389` (*Alex Yurchenko*). 

  Running ``FLUSH TABLES WITH READ LOCK`` when slave applier needed to abort transaction that is on the way caused the deadlock situation. Resolved by grabbing Global Read Lock before pausing wsrep provider. Bug fixed :bug:`1083162` (*Teemu Ollakka*).

  |Percona XtraDB Cluster| would crash when processing a delete for a table with foreign key constraint. Bug fixed :bug:`1078346` (*Seppo Jaakola*).

  When variable :variable:`innodb_support_xa` was set to ``0``, wsrep position wasn't stored into the InnoDB tablespace. Bug fixed :bug:`1084199` (*Teemu Ollakka*).

  Using |XtraBackup| for State Snapshot Transfer would fail due to ``mktemp`` error. Bug fixed :bug:`1080829` (*Alex Yurchenko*).

  |XtraBackup| donor would run |XtraBackup| indefinitely if the xtrabackup ``--tmpdir`` was on ``tmpfs``. Bug fixed :bug:`1086978` (*Alex Yurchenko*).

  In some cases non-uniform foreign key reference could cause a slave crash. Fixed by using primary key of the child table when appending exclusive key for cascading delete operation. Bug fixed :bug:`1089490` (*Seppo Jaakola*).

  |Percona XtraDB Cluster| would crash when :variable:`binlog_format` was set to ``STATEMENT``. This was fixed by introducing the warning message. Bug fixed :bug:`1088400` (*Seppo Jaakola*).

  An explicitly set :variable:`wsrep_node_incoming_address` may make "``SHOW STATUS LIKE 'wsrep_incoming_addresses';``" return the address without the port number. Bug fixed :bug:`1082406` (*Alex Yurchenko*).

  |Percona XtraDB Cluster| would crash if the node's own address would be specified in the :variable:`wsrep_cluster_address` variable. Bug fixed :bug:`1099413`. (*Alexey Yurchenko*)

  When installing from yum repository, ``Percona-XtraDB-Cluster-server`` and ``Percona-XtraDB-Cluster-client`` would conflict with ``mysql`` and ``mysql-server`` packages. Bug fixed :bug:`1087506` (*Ignacio Nin*).

Other bug fixes: bug fixed :bug:`1037165`, bug fixed :bug:`812059`.

Based on `Percona Server 5.5.29-29.4 <http://www.percona.com/doc/percona-server/5.5/release-notes/Percona-Server-5.5.29-29.4.html>`_ including all the bug fixes in it, |Percona XtraDB Cluster| 5.5.29-23.7.1 is now the current stable release. All of |Percona|'s software is open-source and free. 

