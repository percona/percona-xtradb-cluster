.. _yum-repo:

===================================
Installing |PXC| on RHEL and CentOS
===================================

Percona provides :file:`.rpm` packages for 64-bit versions
of the following distributions:

* Red Hat Enterprise Linux and CentOS 5
* Red Hat Enterprise Linux and CentOS 6
* Red Hat Enterprise Linux and CentOS 7

.. note::
  |PXC| should work on other RPM-based distributions
  (such as Oracle Linux and Amazon Linux AMI),
  but it is tested only on platforms listed above.

The packages are available in the official Percona software repositories
and on the
`download page <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/LATEST/>`_.
It is recommended to install |PXC| from repositories using :command:`yum`.

Installing from Repositories
============================

Make sure to remove existing |PXC| 5.5 packages
and any |Percona Server| packages before proceeding.

1. Configure Percona repositories as described in
   `Percona Software Repositories Documentation
   <https://www.percona.com/doc/percona-repo-config/index.html>`_.

#. Install the required |PXC| package using :command:`yum`.
   For example, to install the base package, run the following:
  
   .. code-block:: bash

      sudo yum install Percona-XtraDB-Cluster-56

   .. note:: Alternatively, you can install
      the ``Percona-XtraDB-Cluster-full-56`` meta package,
      which contains the following additional packages:

      * ``Percona-XtraDB-Cluster-devel-56``
      * ``Percona-XtraDB-Cluster-test-56``
      * ``Percona-XtraDB-Cluster-debuginfo-56``
      * ``Percona-XtraDB-Cluster-galera-3-debuginfo``
      * ``Percona-XtraDB-Cluster-shared-56``

For more information on how to bootstrap the cluster please check
:ref:`ubuntu_howto`.

.. warning:: 

   |PXC| requires the ``socat`` package,
   which can be installed from the
   `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repositories.

In CentOS, ``mysql-libs`` conflicts
with the ``Percona-XtraDB-Cluster-server-56`` package.
To avoid this, remove the ``mysql-libs`` package before installing |PXC|.
The ``Percona-Server-shared-56`` provides that dependency if required.

