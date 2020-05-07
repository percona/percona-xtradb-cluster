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

.. important::

   After installing |PXC| the ``mysql`` service is *stopped* but *enabled* so
   that it may start the next time the system is restarted. The service starts
   if the the grastate.dat file exists and the value of ``seqno`` is not **-1**.

   .. seealso::

      More information about Galera state information in :ref:`wsrep_file_index`
         :ref:`grastat.dat <galera-grastate-dat>`

Installation Alternatives
=========================

Percona also provides a generic tarball with all required files and binaries
for manual installation:

* :ref:`tarball`

If you want to build |PXC| from source, see :ref:`compile`.

If you want to run |PXC| using Docker, see :ref:`pxc.docker-container.running`.

.. _pxc-install-version-numbering-conventions:

Product version numbering
================================================================================

The version number in |pxc| releases contains the following components:

- The version of |PS| that the given |pxc| release is based on
- The sequence number which represents the |pxc| built.

For example, version number *8.0.18-9.3* means that this is the third |pxc|
build based on |PS| 8.0.18-9.

.. toctree::
   :hidden:

   Install on Debian or Ubuntu <apt>
   Install on Red Hat or CentOS <yum>
   Install from Binary Tarball <tarball>
   Compile from Source <compile>
   Run in a Docker container <docker>

