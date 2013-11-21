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

|Percona| provides repositories for :program:`yum` (``RPM`` packages for *Red Hat*, *CentOS* and *Amazon Linux AMI*) and :program:`apt` (:file:`.deb` packages for *Ubuntu* and *Debian*) for software such as |Percona Server|, |XtraDB|, |XtraBackup|, and *Percona Toolkit*. This makes it easy to install and update your software and its dependencies through your operating system's package manager.

This is the recommend way of installing where possible.

``YUM``-Based Systems
---------------------

Once the repository is set up, use the following commands: ::

  $ yum install Percona-XtraDB-Cluster-server-56 Percona-XtraDB-Cluster-client-56 percona-xtrabackup

More detailed example of the |Percona XtraDB Cluster| installation and configuration can be seen in :ref:`centos_howto` tutorial.

``DEB``-Based Systems
---------------------

Once the repository is set up, use the following commands: ::

  $ sudo apt-get install percona-xtradb-cluster-server-5.6 percona-xtradb-cluster-client-5.6 percona-xtrabackup

More detailed example of the |Percona XtraDB Cluster| installation and configuration can be seen in :ref:`ubuntu_howto` tutorial.

Prerequisites
=============

In order for |Percona XtraDB Cluster| to work correctly firewall has to be set up to allow connections on the following ports: 3306, 4444, 4567 and 4568. |Percona XtraDB Cluster| currently doesn't work with ``SELinux`` or ``apparmor`` so they should be disabled, otherwise individual nodes won't be able to communicate and form the cluster.

Initial configuration
=====================

In order to start using the |Percona XtraDB Cluster|, following options are needed in the |MySQL| configuration file :file:`my.cnf`: ::

  [mysqld]
  
  wsrep_provider — a path to Galera library.
  wsrep_cluster_address — Cluster connection URL containing the IPs of other nodes in the cluster
  wsrep_sst_method - method used for the state snapshot transfer 
  
  binlog_format=ROW - In order for Galera to work correctly binlog format should be ROW
  default_storage_engine=InnoDB - MyISAM storage engine has only experimental support
  innodb_locks_unsafe_for_binlog=1 - This is a recommended tuning variable for performance
  innodb_autoinc_lock_mode=2 - This changes how InnoDB autoincrement locks are managed

Additional parameters to specify: ::

  wsrep_sst_auth=user:password   

If any other :ref:`state_snapshot_transfer` method beside the :program:`rsync` is specified in the :variable:`wsrep_sst_method`, credentials for |SST| need to be specified.

Example: ::

  wsrep_provider=/usr/lib64/libgalera_smm.so
  wsrep_cluster_address=gcomm://10.11.12.206
  wsrep_slave_threads=8
  wsrep_sst_method=rsync
  binlog_format=ROW
  default_storage_engine=InnoDB
  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2

Detailed list of variables can be found in :ref:`wsrep_system_index` and :ref:`wsrep_status_index`.

