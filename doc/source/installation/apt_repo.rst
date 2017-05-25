.. _apt-repo:

=================================
Percona :program:`apt` Repository
=================================

Supported Architectures
=======================

 * x86_64 (also known as amd64)
 * x86

Supported Releases
==================

Debian
------

 * 7.0 wheezy

Ubuntu
------

 * 12.04LTS precise
 * 14.04LTS trusty

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-55/LATEST/>`_.
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

.. code-block:: bash

  sudo apt-get install percona-xtradb-cluster-55

.. note::

  For Ubuntu 14.04 (Trusty) ``percona-xtradb-cluster-galera-2.x`` will have to
  be specified with the meta package so the installation command should look like:

  .. code-block:: bash

    $ sudo apt-get install percona-xtradb-cluster-55 percona-xtradb-cluster-galera-2.x

.. note::

  Alternatively, you can install ``percona-xtradb-cluster-full-55``
  meta-package which will install the following additional packages:

  * ``percona-xtradb-cluster-test-5.5``,
  * ``percona-xtradb-cluster-5.5-dbg``,
  * ``percona-xtradb-cluster-garbd-2.x``,
  * ``percona-xtradb-cluster-galera-2x-dbg``,
  * ``percona-xtradb-cluster-garbd-2.x-dbg`` and
  * ``libmysqlclient18``

For more information on how to bootstrap the cluster please check
:ref:`ubuntu_howto`.

.. note::

  Garbd is packaged separately as part of Debian split packaging. The garbd
  debian package is ``percona-xtradb-cluster-garbd-2.x``. The package contains,
  garbd, daemon init script and related config files. This package will be
  installed if you install the ``percona-xtradb-cluster-full-55`` meta package.

Percona `apt` Testing repository
================================

Percona offers fresh beta builds from the testing repository. To enable it add
the following lines to your  :file:`/etc/apt/sources.list` , replacing
``VERSION`` with the name of your distribution: ::

  deb http://repo.percona.com/apt VERSION main testing
  deb-src http://repo.percona.com/apt VERSION main testing

Apt-Pinning the packages
========================

In some cases you might need to "pin" the selected packages to avoid the
upgrades from the distribution repositories. You'll need to make a new file
:file:`/etc/apt/preferences.d/00percona.pref` and add the following lines in
it: ::

  Package: *
  Pin: release o=Percona Development Team
  Pin-Priority: 1001

For more information about the pinning you can check the official `debian wiki <http://wiki.debian.org/AptPreferences>`_.
