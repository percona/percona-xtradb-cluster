.. _install:

=================================
Installing Percona XtraDB Cluster
=================================

Install |PXC| on all hosts that you are planning to use as cluster nodes
and ensure that you have root access to the MySQL server on each one.

It is recommended to install |PXC| from official Percona software repositories
using the corresponding package manager for your system:

* :ref:`Debian or Ubuntu <apt>`

* :ref:`Red Hat or CentOS <yum>`

Installation Alternatives
=========================

Percona also provides a generic tarball with all required files and binaries
for manual installation:

* :ref:`tarball`

If you want to build |PXC| from source, see :ref:`compile`.

If you want to run |PXC| using Docker, see :ref:`pxc.docker-container.running`.

.. toctree::
   :hidden:

   Install on Debian or Ubuntu <apt>
   Install on Red Hat or CentOS <yum>
   Install from Binary Tarball <tarball>
   Compile from Source <compile>
   Run in a Docker container <docker>

