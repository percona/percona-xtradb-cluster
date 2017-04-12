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

.. note:: |PXC| should work on other DEB-based distributions,
   but it is tested only on platforms listed above.

The packages are available in the official Percona software repository
and on the `download page
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST/>`_.
It is recommended to install |PXC| from the official repository
using :command:`apt`.

.. contents::
   :local:

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

1. Fetch the package for configuring Percona software repository:

   .. code-block:: bash

      $ wget https://repo.percona.com/apt/percona-release_0.1-4.$(lsb_release -sc)_all.deb

#. Install the downloaded package with :program:`dpkg`:

   .. code-block:: bash

      $ sudo dpkg -i percona-release_0.1-4.$(lsb_release -sc)_all.deb

   Once you install this package, the Percona repository should be added.
   You can check the repository configuration
   in the :file:`/etc/apt/sources.list.d/percona-release.list` file.

#. Update the local cache:

   .. code-block:: bash

      $ sudo apt-get update

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

.. _apt-testing-repo:

Testing and Experimental Repositories
-------------------------------------

Percona offers pre-release builds from the testing repository,
and early-stage development builds from the experimental repository.
To enable them, add either ``testing`` or ``experimental``
at the end of the Percona repository definition in your repository file
(by default, :file:`/etc/apt/sources.list.d/percona-release.list`).

For example, if you are running Debian 8 ("jessie")
and want to install the latest testing builds,
the definitions should look like this: ::

  deb http://repo.percona.com/apt jessie main testing
  deb-src http://repo.percona.com/apt jessie main testing

If you are running Ubuntu 14.04 LTS (Trusty Tahr)
and want to install the latest experimental builds,
the definitions should look like this: ::

  deb http://repo.percona.com/apt trusty main experimental
  deb-src http://repo.percona.com/apt trusty main experimental

Pinning the Packages
--------------------

If you want to pin your packages to avoid upgrades,
create a new file :file:`/etc/apt/preferences.d/00percona.pref`
and add the following lines to it: ::

  Package: *
  Pin: release o=Percona Development Team
  Pin-Priority: 1001

For more information about pinning,
refer to the official `Debian Wiki <http://wiki.debian.org/AptPreferences>`_.

Next Steps
==========

After you install |PXC| and stop the ``mysql`` service,
configure the node according to the procedure described in :ref:`configure`.

