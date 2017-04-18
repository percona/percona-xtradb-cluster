.. _installation:

=================================
Installing Percona XtraDB Cluster
=================================

|Percona XtraDB Cluster| supports most 64-bit Linux distributions.
Percona provides packages for popular DEB-based and RPM-based distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Ubuntu 12.04 LTS (Precise Pangolin)
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 16.04 LTS (Xenial Xerus)
* Ubuntu 16.10 (Yakkety Yak)
* Red Hat Enterprise Linux / CentOS 5
* Red Hat Enterprise Linux / CentOS 6
* Red Hat Enterprise Linux / CentOS 7

.. note:: CentOS 5 includes ``openssl-0.9.8``
   that doesn't support TLS 1.1/1.2,
   which is a requirement as of Galera 3.19.
   This means PXC 5.6.34 and later versions
   do not support SSL encryption on CentOS 5.

   We recommend upgrading from CentOS 5 anyway,
   because it already reached EOL on March 31, 2017.

Prerequisites
=============

|PXC| requires the following ports for communication: 3306, 4444, 4567, 4568.
Make sure that these ports are not blocked by firewall
or used by other software.

|PXC| does not work with ``SELinux`` and ``AppArmor`` security modules.
Make sure these modules are disabled.

Installation Guides
===================

It is recommended to install |PXC| from official Percona repositories
using the corresponding tool for your system:

* :ref:`Install using apt <apt-repo>` if you are running Debian or Ubuntu
* :ref:`Install using yum <yum-repo>` if you are running Red Hat Enterprise
  Linux or CentOS

.. note::

  You can also `download packages
  <https://www.percona.com/downloads/Percona-XtraDB-Cluster-56>`_ from the
  Percona website and install them manually using :command:`dpkg` or
  :command:`rpm`.

If you want to build and run |PXC| from source, see :ref:`compile`.

.. toctree::
   :hidden:

   Install Using apt <installation/apt_repo>
   Install Using yum <installation/yum_repo>
   Compile from Source <installation/compile>
