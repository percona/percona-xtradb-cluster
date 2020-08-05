.. _yum-repo:

===================================
Installing |PXC| on RHEL and CentOS
===================================

Percona provides :file:`.rpm` packages for 64-bit versions
of the following distributions:

<<<<<<< HEAD
* Red Hat Enterprise Linux and CentOS 6
* Red Hat Enterprise Linux and CentOS 7
||||||| 66735bc03f8
The easiest way to install the *Percona Yum* repository is to install an *RPM* that configures :program:`yum` and installs the `Percona GPG key <https://www.percona.com/downloads/RPM-GPG-KEY-percona>`_.

Supported Releases:

 * *CentOS* 6 and *RHEL* 6 (Current Stable) [#f1]_

 * *CentOS* 7 and *RHEL* 7

 * *Amazon Linux AMI* (works the same as *CentOS* 6)

The *CentOS* repositories should work well with *Red Hat Enterprise Linux* too, provided that :program:`yum` is installed on the server.

Supported Platforms:

 * x86
 * x86_64 (also known as ``amd64``)

What's in each RPM package?
===========================

Each of the |Percona Server| RPM packages have a particular purpose.

The ``Percona-Server-server-56`` package contains the server itself (the ``mysqld`` binary).

The ``Percona-Server-56-debuginfo`` package contains debug symbols for the server.

The ``Percona-Server-client-56`` package contains the command line client.

The ``Percona-Server-devel-56`` package contains the header files needed to compile software using the client library.

The ``Percona-Server-shared-56`` package includes the client shared library.

The ``Percona-Server-shared-compat`` package includes shared libraries for software compiled against old versions of the client library. Following libraries are included in this package: ``libmysqlclient.so.12``, ``libmysqlclient.so.14``, ``libmysqlclient.so.15``, and ``libmysqlclient.so.16``.

The ``Percona-Server-test-56`` package includes the test suite for |Percona Server|.

Installing |Percona Server| from Percona ``yum`` repository
===========================================================

1. Install the Percona repository

   You can install Percona yum repository by running the following command as a ``root`` user or with sudo:

   .. code-block:: bash

      $ yum install https://repo.percona.com/yum/percona-release-latest.noarch.rpm


   .. admonition:: Output example


      .. code-block:: guess


	 Retrieving https://repo.percona.com/yum/percona-release-latest.noarch.rpm
	 Preparing...                ########################################### [100%]
         1:percona-release        ########################################### [100%]


   To install |Percona Server| with SELinux policies, you also need the :program:`Percona-Server-selinux-*.noarch.rpm` package:

   .. code-block:: bash

      $ yum install http://repo.percona.com/centos/7/RPMS/x86_64/Percona-Server-selinux-56-5.6.42-rel84.2.el7.noarch.rpm


2. Testing the repository

   Make sure packages are now available from the repository, by executing the following command:

   .. code-block:: bash

     yum list | grep percona

   You should see output similar to the following:

   .. code-block:: bash

     ...
     Percona-Server-56-debuginfo.x86_64          5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-client-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-devel-56.x86_64              5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-server-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-shared-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-test-56.x86_64               5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-shared-compat.x86_64         5.1.68-rel14.6.551.rhel6     percona-release-x86_64
     ...

3. Install the packages

   You can now install |Percona Server| by running:

   .. code-block:: bash

     yum install Percona-Server-server-56

Percona `yum` Testing repository
--------------------------------

Percona offers pre-release builds from our testing repository. To subscribe to the testing repository, you'll need to enable the testing repository in :file:`/etc/yum.repos.d/percona-release.repo`. To do so, set both ``percona-testing-$basearch`` and ``percona-testing-noarch`` to ``enabled = 1`` (Note that there are 3 sections in this file: release, testing and experimental - in this case it is the second section that requires updating). **NOTE:** You'll need to install the Percona repository first (ref above) if this hasn't been done already.


.. _standalone_rpm:

Installing |Percona Server| using downloaded rpm packages
=========================================================

1. Download the packages of the desired series for your architecture from the `download page <http://www.percona.com/downloads/Percona-Server-5.6/>`_. The easiest way is to download bundle which contains all the packages. Following example will download |Percona Server| 5.6.25-73.1 release packages for *CentOS* 6:

   .. code-block:: bash

     wget https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.25-73.1/binary/redhat/6/x86_64/Percona-Server-5.6.25-73.1-r07b797f-el6-x86_64-bundle.tar

2. You should then unpack the bundle to get the packages:

   .. code-block:: bash

     tar xvf Percona-Server-5.6.25-73.1-r07b797f-el6-x86_64-bundle.tar

   After you unpack the bundle you should see the following packages:

   .. code-block:: bash

     $ ls *.rpm

     Percona-Server-56-debuginfo-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-client-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-devel-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-server-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-shared-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-test-56-5.6.25-rel73.1.el6.x86_64.rpm


3. Now you can install |Percona Server| by running:

   .. code-block:: bash

     rpm -ivh Percona-Server-server-56-5.6.25-rel73.1.el6.x86_64.rpm \
     Percona-Server-client-56-5.6.25-rel73.1.el6.x86_64.rpm \
     Percona-Server-shared-56-5.6.25-rel73.1.el6.x86_64.rpm

This will install only packages required to run the |Percona Server|. To install all the packages (for debugging, testing, etc.) you should run:

.. code-block:: bash

   $ rpm -ivh *.rpm
=======
The easiest way to install the *Percona Yum* repository is to install an *RPM* that configures :program:`yum` and installs the `Percona GPG key <https://www.percona.com/downloads/RPM-GPG-KEY-percona>`_.

Specific information on the supported platforms, products, and versions are described in `Percona Software and Platform Lifecycle <https://www.percona.com/services/policies/percona-software-platform-lifecycle#mysql>`_.

What's in each RPM package?
===========================

Each of the |Percona Server| RPM packages have a particular purpose.

The ``Percona-Server-server-56`` package contains the server itself (the ``mysqld`` binary).

The ``Percona-Server-56-debuginfo`` package contains debug symbols for the server.

The ``Percona-Server-client-56`` package contains the command line client.

The ``Percona-Server-devel-56`` package contains the header files needed to compile software using the client library.

The ``Percona-Server-shared-56`` package includes the client shared library.

The ``Percona-Server-shared-compat`` package includes shared libraries for software compiled against old versions of the client library. Following libraries are included in this package: ``libmysqlclient.so.12``, ``libmysqlclient.so.14``, ``libmysqlclient.so.15``, and ``libmysqlclient.so.16``.

The ``Percona-Server-test-56`` package includes the test suite for |Percona Server|.

Installing |Percona Server| from Percona ``yum`` repository
===========================================================

1. Install the Percona repository

   You can install Percona yum repository by running the following command as a ``root`` user or with sudo:

   .. code-block:: bash

      $ yum install https://repo.percona.com/yum/percona-release-latest.noarch.rpm


   .. admonition:: Output example


      .. code-block:: guess


	 Retrieving https://repo.percona.com/yum/percona-release-latest.noarch.rpm
	 Preparing...                ########################################### [100%]
         1:percona-release        ########################################### [100%]


   To install |Percona Server| with SELinux policies, you also need the :program:`Percona-Server-selinux-*.noarch.rpm` package:

   .. code-block:: bash

      $ yum install http://repo.percona.com/centos/7/RPMS/x86_64/Percona-Server-selinux-56-5.6.42-rel84.2.el7.noarch.rpm


2. Testing the repository

   Make sure packages are now available from the repository, by executing the following command:

   .. code-block:: bash

     yum list | grep percona

   You should see output similar to the following:

   .. code-block:: bash

     ...
     Percona-Server-56-debuginfo.x86_64          5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-client-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-devel-56.x86_64              5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-server-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-shared-56.x86_64             5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-test-56.x86_64               5.6.25-rel73.1.el6           @percona-release-x86_64
     Percona-Server-shared-compat.x86_64         5.1.68-rel14.6.551.rhel6     percona-release-x86_64
     ...

3. Install the packages

   You can now install |Percona Server| by running:

   .. code-block:: bash

     yum install Percona-Server-server-56

Percona `yum` Testing repository
--------------------------------

Percona offers pre-release builds from our testing repository. To subscribe to the testing repository, you'll need to enable the testing repository in :file:`/etc/yum.repos.d/percona-release.repo`. To do so, set both ``percona-testing-$basearch`` and ``percona-testing-noarch`` to ``enabled = 1`` (Note that there are 3 sections in this file: release, testing and experimental - in this case it is the second section that requires updating). **NOTE:** You'll need to install the Percona repository first (ref above) if this hasn't been done already.


.. _standalone_rpm:

Installing |Percona Server| using downloaded rpm packages
=========================================================

1. Download the packages of the desired series for your architecture from the `download page <http://www.percona.com/downloads/Percona-Server-5.6/>`_. The easiest way is to download bundle which contains all the packages. Following example will download |Percona Server| 5.6.25-73.1 release packages for *CentOS* 6:

   .. code-block:: bash

     wget https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.25-73.1/binary/redhat/6/x86_64/Percona-Server-5.6.25-73.1-r07b797f-el6-x86_64-bundle.tar

2. You should then unpack the bundle to get the packages:

   .. code-block:: bash

     tar xvf Percona-Server-5.6.25-73.1-r07b797f-el6-x86_64-bundle.tar

   After you unpack the bundle you should see the following packages:

   .. code-block:: bash

     $ ls *.rpm

     Percona-Server-56-debuginfo-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-client-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-devel-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-server-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-shared-56-5.6.25-rel73.1.el6.x86_64.rpm
     Percona-Server-test-56-5.6.25-rel73.1.el6.x86_64.rpm


3. Now you can install |Percona Server| by running:

   .. code-block:: bash

     rpm -ivh Percona-Server-server-56-5.6.25-rel73.1.el6.x86_64.rpm \
     Percona-Server-client-56-5.6.25-rel73.1.el6.x86_64.rpm \
     Percona-Server-shared-56-5.6.25-rel73.1.el6.x86_64.rpm

This will install only packages required to run the |Percona Server|. To install all the packages (for debugging, testing, etc.) you should run:

.. code-block:: bash

   $ rpm -ivh *.rpm
>>>>>>> 9d4976d

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

<<<<<<< HEAD
In CentOS, ``mysql-libs`` conflicts
with the ``Percona-XtraDB-Cluster-server-56`` package.
To avoid this, remove the ``mysql-libs`` package before installing |PXC|.
The ``Percona-Server-shared-56`` provides that dependency if required.
||||||| 66735bc03f8
.. rubric:: Footnotes
=======
>>>>>>> 9d4976d

