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

.. note::
  |PXC| should work on other DEB-based distributions,
  but it is tested only on platforms listed above.

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/LATEST/>`_.
It is recommended to install |PXC| from repositories using :command:`apt`.

.. contents::
   :local:

Installing from Repositories
============================

1. Fetch the repository packages from Percona web:

.. code-block:: bash

  wget https://repo.percona.com/apt/percona-release_0.1-4.$(lsb_release -sc)_all.deb

#. Install the downloaded package with :program:`dpkg`.
   To do that, run the following command as root or with :program:`sudo`:

.. code-block:: bash

  sudo dpkg -i percona-release_0.1-4.$(lsb_release -sc)_all.deb

Once you install this package, the Percona repositories should be added.
You can check the repository configuration
in the :file:`/etc/apt/sources.list.d/percona-release.list` file.

#. Update the local cache:

.. code-block:: bash

  sudo apt-get update

#. Install the server package:

.. note::

  Make sure to remove existing |Percona XtraDB Cluster| 5.5 and |Percona
  Server| 5.5/5.6 packages before proceeding.

  
.. code-block:: bash

  sudo apt-get install percona-xtradb-cluster-56

.. note::

  Alternatively, you can install
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
    
   Garbd is packaged separately as part of debian split packaging. The garbd
   Debian package is ``percona-xtradb-cluster-garbd-3.x``. The package
   contains, garbd, daemon init script and related config files. This package
   will be installed if you install the ``percona-xtradb-cluster-full-56``
   meta package.

.. note:: 

   On *Debian Jessie (8.0)* and *Ubuntu Xenial (16.04)* in order to stop or
   reload the node bootstrapped with ``service mysql bootstrap-pxc`` command,
   you'll need to use ``service mysql stop-bootstrap`` or ``service mysql
   reload-bootstrap`` respectively. Also, status of the bootstrapped node
   should be checked with ``service mysql status-bootstrap``.
   
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
the definitions should look like this: 

.. code-block:: text

  deb http://repo.percona.com/apt jessie main testing
  deb-src http://repo.percona.com/apt jessie main testing

If you are running Ubuntu 14.04 LTS (Trusty Tahr)
and want to install the latest experimental builds,
the definitions should look like this: 

.. code-block:: text

  deb http://repo.percona.com/apt trusty main experimental
  deb-src http://repo.percona.com/apt trusty main experimental

Pinning the Packages
--------------------

If you want to pin your packages to avoid upgrades,
create a new file :file:`/etc/apt/preferences.d/00percona.pref`
and add the following lines to it: 

.. code-block:: text

  Package: *
  Pin: release o=Percona Development Team
  Pin-Priority: 1001

For more information about pinning,
refer to the official `Debian Wiki <http://wiki.debian.org/AptPreferences>`_.
