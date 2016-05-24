.. _yum:

=======================================================
Installing |PXC| on Red Hat Enterprise Linux and CentOS
=======================================================

Percona provides :file:`.rpm` packages for 64-bit versions
of the following distributions:

* Red Hat Enterprise Linux / CentOS 6
* Red Hat Enterprise Linux / CentOS 7

.. note:: |PXC| should work on other RPM-based distributions
   (for example, Amazon Linux AMI and Oracle Linux),
   but it is tested only on platforms listed above.

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/LATEST/>`_.
It is recommended to intall |PXC| from the official repository
using :command:`yum`.

.. contents::
   :local:

Prerequisites
=============

In CentOS, remove the ``mysql-libs`` package before installing |PXC|,
to avoid possible conflicts.
The |PXC| package will correctly resolve this dependency.

Use the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repository to install
the ``socat`` package before installing |PXC|.

Installing from Percona Repository
==================================

1. Install the Percona repository package:
   
   .. prompt:: bash

      sudo yum install http://www.percona.com/downloads/percona-release/redhat/0.1-3/percona-release-0.1-3.noarch.rpm

   Confirm installation and you should see the following if successful: ::

      Installed:
        percona-release.noarch 0:0.1-3                                      

      Complete!

   .. note:: Red Hat Enterprise Linux and CentOS 5 do not support installing packages directly from the remote location. Download the Percona repository package first and install it manually using :program:`rpm`:

      .. prompt:: bash

         wget http://www.percona.com/downloads/percona-release/redhat/0.1-3/percona-release-0.1-3.noarch.rpm
         rpm -ivH percona-release-0.1-3.noarch.rpm

2. Enable testing repository.

   This is a beta release, which can only be installed from the testing repo.
   For more information, see :ref:`yum-testing-repo`.

3. Check that the packages are available:
   
   .. prompt:: bash

      yum list | grep Percona-XtraDB-Cluster

   You should see output similar to the following:

   .. code-block:: none

      Percona-XtraDB-Cluster-57.x86_64           1:5.7.11-25.14.2.el7        percona-release-x86_64
      Percona-XtraDB-Cluster-57-debuginfo.x86_64 1:5.7.11-25.14.2.el7        percona-release-x86_64

4. Install the |PXC| packages:

   .. prompt:: bash

      sudo yum install Percona-XtraDB-Cluster-57

   .. note:: Alternatively, you can install
      the ``Percona-XtraDB-Cluster-full-57`` meta package,
      which contains the following additional packages:

      * ``Percona-XtraDB-Cluster-devel-57``
      * ``Percona-XtraDB-Cluster-test-57``
      * ``Percona-XtraDB-Cluster-debuginfo-57``
      * ``Percona-XtraDB-Cluster-galera-3-debuginfo``
      * ``Percona-XtraDB-Cluster-shared-57``

.. _yum-testing-repo:

Testing and Experimental Repositories
-------------------------------------

Percona offers pre-release builds from the testing repo,
and early-stage development builds from the experimental repo.
You can enable either one in the Percona repository
configuration file :file:`/etc/yum.repos.d/percona-release.repo`.
There are three sections in this file,
for configuring corresponding repositories:

* stable release
* testing
* experimental

The latter two repositories are disabled by default.

If you want to install the latest testing builds,
set ``enabled=1`` for the following entries: ::

  [percona-testing-$basearch]
  [percona-testing-noarch]

If you want to install the latest experimental builds,
set ``enabled=1`` for the following entries: ::

  [percona-experimental-$basearch]
  [percona-experimental-noarch]

