.. rn:: 5.6.41-28.28

===================================
Percona XtraDB Cluster 5.6.41-28.28
===================================

Percona is glad to announce the release of
Percona XtraDB Cluster 5.6.41-28.28 on September 18, 2018.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.41-28.28 is now the current release,
based on the following:

* `Percona Server 5.6.41 <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.41-84.1.html>`_

* Codership WSREP API release 5.6.41

* Codership Galera library 3.24

All Percona software is open-source and free.

Fixed Bugs
==========

* :jirabug:`PXC-1017`: Memcached API is now disabled if node is acting as a
  cluster node, because InnoDB Memcached access is not replicated by Galera.

* :jirabug:`PXC-2164`: SST script compatibility with SELinux was improved by
  forcing it to look for port associated with the said process only.

* :jirabug:`PXC-2155`: Temporary folders created during SST execution are now
  deleted on cleanup. 

* :jirabug:`PXC-2199`: TOI replication protocol was fixed to prevent unexpected
  GTID generation caused by the ``DROP TRIGGER IF EXISTS`` statement logged by
  MySQL as a successful one due to its ``IF EXISTS`` clause.
