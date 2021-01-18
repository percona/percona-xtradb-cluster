.. _apt:

====================================
Installing |PXC| on Debian or Ubuntu
====================================

Specific information on the supported platforms, products, and versions
is described in `Percona Software and Platform Lifecycle <https://www.percona.com/services/policies/percona-software-platform-lifecycle#mysql>`_.

The packages are available in the official Percona software repository
and on the `download page
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-80/LATEST/>`_.
It is recommended to install |PXC| from the official repository
using :command:`apt`.

Prerequisites
=============

- You need to have root access on the node where you will be installing |PXC|
  (either logged in as a user with root privileges or be able to run commands
  with :command:`sudo`).

- Make sure that the following ports are not blocked by firewall
  or used by other software. |PXC| requires them for communication.

   * 3306
   * 4444
   * 4567
   * 4568

.. seealso:: 

      For more information, see :ref:`apparmor`.

Installing from Repository
==========================

1. Configure Percona repositories as described in
   `Percona Software Repositories Documentation
   <https://www.percona.com/doc/percona-repo-config/index.html>`_.

#. Install the |PXC| server package:

   .. code-block:: bash

      $ sudo apt-get install percona-xtradb-cluster

   .. note:: Alternatively, you can install
      the ``percona-xtradb-cluster-full`` meta package,
      which contains the following additional packages:

      * ``libperconaserverclient21-dev``
      * ``libperconaserverclient21``
      * ``percona-xtradb-cluster-client``
      * ``percona-xtradb-cluster-common``
      * ``percona-xtradb-cluster-dbg``
      * ``percona-xtradb-cluster-full``
      * ``percona-xtradb-cluster-garbd-debug``
      * ``percona-xtradb-cluster-garbd``
      * ``percona-xtradb-cluster-server-debug``
      * ``percona-xtradb-cluster-server``
      * ``percona-xtradb-cluster-source``
      * ``percona-xtradb-cluster-test``
      * ``percona-xtradb-cluster``


   During the installation, you are requested to provide a password
   for the ``root`` user on the database node.

Next Steps
==========

After you install |PXC| and stop the ``mysql`` service,
configure the node according to the procedure described in :ref:`configure`.

