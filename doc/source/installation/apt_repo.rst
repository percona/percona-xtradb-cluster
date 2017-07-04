.. _apt-repo:

=====================================
Installing |PXC| on Debian and Ubuntu
=====================================

Percona provides :file:`.deb` packages for 64-bit versions
of the following distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Ubuntu 12.04 LTS (Precise Pangolin)
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 16.04 LTS (Xenial Xerus)
* Ubuntu 16.10 (Yakkety Yak)
* Ubuntu 17.04 (Zesty Zapus)

.. note::
  |PXC| should work on other DEB-based distributions,
  but it is tested only on platforms listed above.

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/LATEST/>`_.
It is recommended to install |PXC| from repositories using :command:`apt`.

Installing from Repositories
============================

Make sure to remove existing |PXC| 5.5 packages
and any |Percona Server| packages before proceeding.

1. Configure Percona repositories as described in
   `Percona Software Repositories Documentation
   <https://www.percona.com/doc/percona-repo-config/index.html>`_.

#. Install the required |PXC| package using :command:`apt-get`.
   For example, to install the base package, run the following:
  
   .. code-block:: bash

      sudo apt-get install percona-xtradb-cluster-56

   .. note:: Alternatively, you can install
      the ``percona-xtradb-cluster-full-56`` meta package,
      which contains the following additional packages:

      * ``percona-xtradb-cluster-5.6-dbg``
      * ``percona-xtradb-cluster-galera-3.x-dbg`` 
      * ``percona-xtradb-cluster-galera3-dbg``
      * ``percona-xtradb-cluster-garbd-3``
      * ``percona-xtradb-cluster-garbd-3.x``
      * ``percona-xtradb-cluster-garbd-3.x-dbg``
      * ``percona-xtradb-cluster-server-debug-5.6``
      * ``percona-xtradb-cluster-test-5.6``

For more information on how to bootstrap the cluster please check
:ref:`ubuntu_howto`.

.. note:: 
    
   Garbd is packaged separately as part of Debian split packaging.
   The ``garbd`` package in Debian is ``percona-xtradb-cluster-garbd-3.x``.
   The package contains ``garbd``, daemon init script and related config files.
   This package will be installed,
   if you install the ``percona-xtradb-cluster-full-56`` meta package.

.. note:: 

   On *Debian Jessie (8.0)* and *Ubuntu Xenial (16.04)*,
   to stop or reload the node bootstrapped with ``service mysql bootstrap-pxc``,
   you'll need to use ``service mysql stop-bootstrap``
   or ``service mysql reload-bootstrap`` respectively.
   To check the status of the bootstrapped node,
   run ``service mysql status-bootstrap``.

