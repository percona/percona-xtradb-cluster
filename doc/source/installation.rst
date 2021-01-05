.. _installation:

=================================
Installing Percona XtraDB Cluster
=================================

Specific information on the supported platforms, products, and versions is described in `Percona Software and Platform Lifecycle <https://www.percona.com/services/policies/percona-software-platform-lifecycle#mysql>`_.

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

If you want to run |PXC| using Docker, see :ref:`docker`.

.. toctree::
   :hidden:

   Install Using apt <installation/apt_repo>
   Install Using yum <installation/yum_repo>
   Compile from Source <installation/compile>
   Run in a Docker container <installation/docker>
