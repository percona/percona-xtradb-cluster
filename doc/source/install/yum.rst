.. _yum:

=======================================================
Installing |PXC| on Red Hat Enterprise Linux and CentOS
=======================================================

Specific information on the supported platforms, products, and versions
is described in `Percona Software and Platform Lifecycle <https://www.percona.com/services/policies/percona-software-platform-lifecycle#mysql>`_.

The packages are available in the official Percona software repository
and on the `download page
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-80/LATEST/>`_.
It is recommended to install |PXC| from the official repository
using :command:`yum`.

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

.. note:: The `SELinux <https://selinuxproject.org>`_ security module
   can constrain access to data for |PXC|.
   The best solution is to change the mode
   from ``enforcing``  to ``permissive`` by running the following command::

    setenforce 0

   This only changes the mode at runtime.
   To run SELinux in permissive mode after a reboot,
   set ``SELINUX=permissive`` in the :file:`/etc/selinux/config`
   configuration file.

Installing from Percona Repository
==================================

1. Configure Percona repositories as described in
   `Percona Software Repositories Documentation
   <https://www.percona.com/doc/percona-repo-config/index.html>`_.

#. Install the |PXC| packages:

   .. code-block:: bash


      $ sudo yum install percona-xtradb-cluster

   .. note:: Alternatively you can install
      the ``percona-xtradb-cluster-full`` meta package,
      which contains the following additional packages:

      * ``percona-xtraDB-cluster-shared``
      * ``percona-xtraDB-cluster-shared-compat``
      * ``percona-xtradb-cluster-client``
      * ``percona-xtradb-cluster-debuginfo``
      * ``percona-xtradb-cluster-devel``
      * ``percona-xtradb-cluster-garbd``
      * ``percona-xtradb-cluster-server``
      * ``percona-xtradb-cluster-test``
      
#. Start the |PXC| server:

   .. code-block:: bash

      $ sudo service mysql start

#. Copy the automatically generated temporary password
   for the superuser account:

   .. code-block:: bash

      $ sudo grep 'temporary password' /var/log/mysqld.log

#. Use this password to log in as ``root``:

   .. code-block:: sql

      $ mysql -u root -p

#. Change the password for the superuser account and log out. For example:

   .. code-block:: sql

      mysql> ALTER USER 'root'@'localhost' IDENTIFIED BY 'rootPass';
      Query OK, 0 rows affected (0.00 sec)

      mysql> exit
      Bye

#. Stop the ``mysql`` service:

   .. code-block:: bash

      $ sudo service mysql stop

Next Steps
==========

After you install |PXC| and change the superuser account password,
configure the node according to the procedure described in :ref:`configure`.

