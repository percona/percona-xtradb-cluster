.. _install:

=================================
Installing Percona XtraDB Cluster 
=================================

|PXC| supports most 64-bit Linux distributions.
Percona provides packages for popular DEB-based and RPM-based distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Ubuntu 12.04 LTS (Precise Pangolin)
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 15.10 (Wily Werewolf)
* Ubuntu 16.04 (Xenial Xerus)
* Red Hat Enterprise Linux / CentOS 6
* Red Hat Enterprise Linux / CentOS 7

Prerequisites
=============

|PXC| requires the following ports for communication: 3306, 4444, 4567, 4568.
Make sure that these ports are not blocked by firewall
or used by other software.

|PXC| does not work with ``SELinux`` and ``AppArmor`` security modules.
Make sure these modules are disabled.

There are several MySQL configuration options
that are mandatory for running |PXC|.
These include the path to the Galera wsrep library,
connection URL for other nodes in the cluster,
and method used for state snapshot transfer (SST).
Make sure that the binlog format is set to ``ROW``,
the default storage engine is InnoDB (MyISAM has only experimental support),
and set the mode of InnoDB autoincrement locks to ``2``.
The following is an example of the :file:`my.cnf` file:

.. code-block:: none

   [mysqld]

   wsrep_provider=/usr/lib64/libgalera_smm.so
   wsrep_cluster_address=gcomm://10.11.12.206
   wsrep_slave_threads=8
   wsrep_sst_method=rsync
   binlog_format=ROW
   default_storage_engine=InnoDB
   innodb_autoinc_lock_mode=2

.. note:: If the SST method is not ``rsync``, specify SST credentials using
   ``wsrep_sst_auth=<user>:<password>``

Installation Guides
===================

It is recommended to install |PXC| from official Percona repositories
using the corresponding tool for your system:

* :ref:`Install using apt <apt>` if you are running Debian or Ubuntu
* :ref:`Install using yum <yum>` if you are running Red Hat Enterprise Linux
  or CentOS

.. note:: You can also `download packages <https://www.percona.com/downloads/Percona-XtraDB-Cluster-57>`_ from the Percona website and install them manually using :command:`dpkg` or :command:`rpm`.

If you want to build and run |PXC| from source, see :ref:`compile`.

.. toctree::
   :hidden:

   Install Using apt <apt>
   Install Using yum <yum>
   Compile from Source <compile>

