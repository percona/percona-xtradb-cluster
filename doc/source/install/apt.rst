.. _apt:

====================================
Installing |PXC| on Debian or Ubuntu
====================================

Percona provides :file:`.deb` packages for 64-bit versions
of the following distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Ubuntu 12.04 LTS (Precise Pangolin)
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 16.04 LTS (Xenial Xerus)
* Ubuntu 16.10 (Yakkety Yak)
* Ubuntu 17.04 (Zesty Zapus)

.. note:: |PXC| should work on other DEB-based distributions,
   but it is tested only on platforms listed above.

The packages are available in the official Percona software repository
and on the `download page
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST/>`_.
It is recommended to install |PXC| from the official repository
using :command:`apt`.

Prerequisites
=============

.. note:: You need to have root access on the node
   where you will be installing |PXC|
   (either logged in as a user with root privileges
   or be able to run commands with :command:`sudo`).

.. note:: Make sure that the following ports are not blocked by firewall
   or used by other software. |PXC| requires them for communication.

   * 3306
   * 4444
   * 4567
   * 4568

.. note:: If you previously had MySQL installed on the server,
   there might be an `AppArmor <https://help.ubuntu.com/community/AppArmor>`_
   profile which will prevent |PXC| nodes from communicating with each other.
   The best solution is to remove the ``apparmor`` package entirely:

   .. code-block:: bash

      $ sudo apt-get remove apparmor

   If you need to have AppArmor enabled due to security policies
   or for other reasons,
   it is possible to disable or extend the MySQL profile.

Installing from Repository
==========================

1. Configure Percona repositories as described in
   `Percona Software Repositories Documentation
   <https://www.percona.com/doc/percona-repo-config/index.html>`_.

#. Install the |PXC| server package:

   .. code-block:: bash

      $ sudo apt-get install percona-xtradb-cluster-57

   .. note:: Alternatively, you can install
      the ``percona-xtradb-cluster-full-57`` meta package,
      which contains the following additional packages:

      * ``percona-xtradb-cluster-test-5.7``
      * ``percona-xtradb-cluster-5.7-dbg``
      * ``percona-xtradb-cluster-garbd-3.x``
      * ``percona-xtradb-cluster-galera-3.x-dbg``
      * ``percona-xtradb-cluster-garbd-3.x-dbg``
      * ``libmysqlclient18``

   During installation, you will be prompted to provide a password
   for the ``root`` user on the database node.

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

   .. note:: All Debian-based distributions start services
      as soon as the corresponding package is installed.
      Before starting a |PXC| node, it needs to be properly configured.
      For more information, see :ref:`configure`.

Next Steps
==========

After you install |PXC| and stop the ``mysql`` service,
configure the node according to the procedure described in :ref:`configure`.

