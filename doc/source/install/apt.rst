.. _apt:

=====================================
Installing |PXC| on Debian and Ubuntu
=====================================

Percona provides :file:`.deb` packages for 64-bit versions
of the following distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Ubuntu 12.04 LTS (Precise Pangolin)
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 15.10 (Wily Werewolf)
* Ubuntu 16.04 (Xenial Xerus)

.. note:: |PXC| should work on other DEB-based distributions,
   but it is tested only on platforms listed above.

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST/>`_.
It is recommended to intall |PXC| from repositories using :command:`apt`.

.. contents::
   :local:

Installing from Repositories
============================

1. Fetch the repository packages from Percona web:

   .. prompt:: bash

      wget https://repo.percona.com/apt/percona-release_0.1-3.$(lsb_release -sc)_all.deb

2. Install the downloaded package with :program:`dpkg`.
   To do that, run the following command as root or with :program:`sudo`:

   .. prompt:: bash

      sudo dpkg -i percona-release_0.1-3.$(lsb_release -sc)_all.deb

   Once you install this package, the Percona repositories should be added.
   You can check the repository configuration
   in the :file:`/etc/apt/sources.list.d/percona-release.list` file.

3. Enable testing repository.

   This is a beta release, which can only be installed from the testing repo.
   For more information, see :ref:`apt-testing-repo`.

4. Update the local cache:

   .. prompt:: bash

      sudo apt-get update

5. Install the server package:

   .. prompt:: bash

      sudo apt-get install percona-xtradb-cluster-57

   .. note:: Alternatively, you can install
      the ``percona-xtradb-cluster-full-57`` meta package,
      which contains the following additional packages:

      * ``percona-xtradb-cluster-test-5.7``
      * ``percona-xtradb-cluster-5.7-dbg``
      * ``percona-xtradb-cluster-garbd-3.x``
      * ``percona-xtradb-cluster-galera-3.x-dbg``
      * ``percona-xtradb-cluster-garbd-3.x-dbg``
      * ``libmysqlclient18``

.. _apt-testing-repo:

Testing and Experimental Repositories
-------------------------------------

Percona offers pre-release builds from the testing repo,
and early-stage development builds from the experimental repo.
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

