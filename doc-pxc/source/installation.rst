.. _installation:

===================================================
 Installing Percona XtraDB Cluster from Binaries
===================================================

.. toctree::
   :hidden:

Ready-to-use binaries are available from the *Percona XtraDB Cluster* `download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster/>`_, including:

 * ``RPM`` packages for *RHEL* 5 and *RHEL* 6

 * *Debian* packages

 * Generic ``.tar.gz`` packages

Using Percona Software Repositories
===================================

.. toctree::
   :maxdepth: 1

   installation/yum_repo
   installation/apt_repo

|Percona| provides repositories for :program:`yum` (``RPM`` packages for *Red Hat*, *CentOS*, *Amazon Linux AMI*, and *Fedora*) and :program:`apt` (:file:`.deb` packages for *Ubuntu* and *Debian*) for software such as |Percona Server|, |XtraDB|, |XtraBackup|, and *Percona Toolkit*. This makes it easy to install and update your software and its dependencies through your operating system's package manager.

This is the recommend way of installing where possible.


Initial configuration
=====================

In order to start using XtraDB Cluster, you need to configure my.cnf file. Following options are needed: ::

  [mysqld]
  wsrep_provider — a path to Galera library.
  wsrep_cluster_address — cluster connection URL.
  binlog_format=ROW
  default_storage_engine=InnoDB
  innodb_autoinc_lock_mode=2
  innodb_locks_unsafe_for_binlog=1

Additional parameters to tune: ::

  wsrep_slave_threads   # specifies amount of threads to apply events
  wsrep_sst_method

Example: ::

  wsrep_provider=/usr/lib64/libgalera_smm.so
  wsrep_cluster_address=gcomm://10.11.12.206
  wsrep_slave_threads=8
  wsrep_sst_method=rsync
  #wsrep_sst_method=xtrabackup - alternative way to do SST
  wsrep_cluster_name=percona_test_cluster
  binlog_format=ROW
  default_storage_engine=InnoDB
  innodb_autoinc_lock_mode=2
  innodb_locks_unsafe_for_binlog=1

Detailed list of variables can be found in :ref:`wsrep_system_index` and :ref:`wsrep_status_index`.

Install XtraBackup SST method
==============================

To use Percona XtraBackup for State Transfer method (copy snapshot of data between nodes)
you can use the regular xtrabackup package with the script what supports Galera information.
You can take *innobackupex* script from source code `innobackupex <http://bazaar.launchpad.net/~percona-dev/percona-xtrabackup/galera-info/view/head:/innobackupex>`_.

To inform node to use xtrabackup you need to specify in my.cnf: ::

  wsrep_sst_method=xtrabackup 

