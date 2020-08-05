.. _installation:

=================================
Installing Percona XtraDB Cluster
=================================

|Percona XtraDB Cluster| supports most 64-bit Linux distributions.
Percona provides packages for popular DEB-based and RPM-based distributions:

* Debian 7 ("wheezy")
* Debian 8 ("jessie")
* Debian 9 ("stretch")
* Ubuntu 14.04 LTS (Trusty Tahr)
* Ubuntu 16.04 LTS (Xenial Xerus)
* Ubuntu 18.04 LTS (Bionic Beaver)
* Ubuntu 18.10 (Cosmic Cuttlefish)
* Red Hat Enterprise Linux / CentOS 6
* Red Hat Enterprise Linux / CentOS 7

.. note:: CentOS 5 includes ``openssl-0.9.8``
   that doesn't support TLS 1.1/1.2,
   which is a requirement as of Galera 3.19.
   This means PXC 5.6.34 does not support SSL
   encryption on CentOS 5 (while PXC 5.6.35
   and later do not support CentOS 5 at all).

   We recommend upgrading from CentOS 5 anyway,
   because it already reached EOL on March 31, 2017.

Prerequisites
=============

|PXC| requires the following ports for communication: 3306, 4444, 4567, 4568.
Make sure that these ports are not blocked by firewall
or used by other software.

|PXC| does not work with ``SELinux`` and ``AppArmor`` security modules.
Make sure these modules are disabled.

<<<<<<< HEAD
Installation Guides
===================
||||||| 66735bc03f8
.. toctree::
   :maxdepth: 1
   :titlesonly:

   installation/apt_repo 
   installation/yum_repo 

.. _installing_from_binary_tarball:

Installing |Percona Server| from a Binary Tarball
===================================================

In |Percona Server| :rn:`5.6.24-72.2` and newer, the single binary tarball was replaced with multiple tarballs depending on the *OpenSSL* library available in the distribution:

 * ssl100 - for all *Debian/Ubuntu* versions except *Debian Squeeze* (``libssl.so.1.0.0 => /usr/lib/x86_64-linux-gnu/libssl.so.1.0.0 (0x00007f2e389a5000)``);
 * ssl098 - only for *Debian Squeeze* (``libssl.so.0.9.8 => /usr/lib/libssl.so.0.9.8 (0x00007f9b30db6000)``);
 * ssl101 - for *CentOS* 6 and *CentOS* 7 (``libssl.so.10 => /usr/lib64/libssl.so.10 (0x00007facbe8c4000)``);
 * ssl098e - to be used only for *CentOS* 5 (``libssl.so.6 => /lib64/libssl.so.6 (0x00002aed5b64d000)``).

You can download the binary tarballs from the ``Linux - Generic`` `section <https://www.percona.com/downloads/Percona-Server-5.6/LATEST/binary/tarball/>`_ on the download page.

Fetch and extract the correct binary tarball. For example for *Debian Wheezy*: 

.. code-block:: bash

  $ wget http://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.24-72.2/binary/tarball/Percona-Server-5.6.24-rel72.2-Linux.x86_64.ssl100.tar.gz


.. _installing_from_source_tarball:

Installing |Percona Server| from a Source Tarball
=================================================

Fetch and extract the source tarball. For example: ::

  $ wget http://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.23-72.1/binary/tarball/Percona-Server-5.6.23-rel72.1-Linux.x86_64.tar.gz 
  $ tar xfz Percona-Server-5.6.23-rel72.1-Linux.x86_64.tar.gz

Next, follow the instructions in :ref:`compile_from_source` below.

.. _source_from_git:

Installing |Percona Server| from the Git Source Tree
====================================================

Percona uses the `Github <http://github.com/>`_ revision
control system for development. To build the latest |Percona Server|
from the source tree you will need ``git`` installed on your system.

You can now fetch the latest |Percona Server| 5.6 sources. 

.. code-block:: bash

  $ git clone https://github.com/percona/percona-server.git
  $ cd percona-server
  $ git checkout 5.6
  $ git submodule init
  $ git submodule update

If you are going to be making changes to |Percona Server| 5.6 and wanting
to distribute the resulting work, you can generate a new source tarball
(exactly the same way as we do for release): ::

  $ cmake .
  $ make dist
=======
.. toctree::
   :maxdepth: 1
   :titlesonly:

   installation/apt_repo 
   installation/yum_repo 

.. _installing_from_binary_tarball:

Installing |Percona Server| from a Binary Tarball
===================================================

In |Percona Server| :rn:`5.6.24-72.2` and newer, the single binary tarball was replaced with multiple tarballs depending on the *OpenSSL* library available in the distribution:

 * ssl100 - for all *Debian/Ubuntu* versions except *Debian Squeeze* (``libssl.so.1.0.0 => /usr/lib/x86_64-linux-gnu/libssl.so.1.0.0 (0x00007f2e389a5000)``);
 * ssl098 - only for *Debian Squeeze* (``libssl.so.0.9.8 => /usr/lib/libssl.so.0.9.8 (0x00007f9b30db6000)``);
 * ssl101 - for *CentOS* 6 and *CentOS* 7 (``libssl.so.10 => /usr/lib64/libssl.so.10 (0x00007facbe8c4000)``);
 * ssl098e - to be used only for *CentOS* 5 (``libssl.so.6 => /lib64/libssl.so.6 (0x00002aed5b64d000)``).

.. note::

        The list is a guide. To find which libssl.so files are available on your
        system you should run the following in Ubuntu or Debian:

        .. code-block:: bash

            $ locate libssl | grep "^/usr/lib/"

        In CentOS, run the following command: 

        .. code-block:: bash

            $ ldconfig -p | grep ssl


Download the appropriate binary tarball from the ``Linux - Generic`` `section <https://www.percona.com/downloads/Percona-Server-5.6/LATEST/binary/tarball/>`_ on the download page.

Fetch and extract the correct binary
tarball. For example for *Debian Wheezy*: 

.. code-block:: bash

  $ wget http://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.24-72.2/binary/tarball/Percona-Server-5.6.24-rel72.2-Linux.x86_64.ssl100.tar.gz


.. _installing_from_source_tarball:

Installing |Percona Server| from a Source Tarball
=================================================

Fetch and extract the source tarball. For example: ::

  $ wget http://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.23-72.1/binary/tarball/Percona-Server-5.6.23-rel72.1-Linux.x86_64.tar.gz 
  $ tar xfz Percona-Server-5.6.23-rel72.1-Linux.x86_64.tar.gz

Next, follow the instructions in :ref:`compile_from_source` below.

.. _source_from_git:

Installing |Percona Server| from the Git Source Tree
====================================================

Percona uses the `Github <http://github.com/>`_ revision
control system for development. To build the latest |Percona Server|
from the source tree you will need ``git`` installed on your system.

You can now fetch the latest |Percona Server| 5.6 sources. 

.. code-block:: bash

  $ git clone https://github.com/percona/percona-server.git
  $ cd percona-server
  $ git checkout 5.6
  $ git submodule init
  $ git submodule update

If you are going to be making changes to |Percona Server| 5.6 and wanting
to distribute the resulting work, you can generate a new source tarball
(exactly the same way as we do for release): ::

  $ cmake .
  $ make dist
>>>>>>> 9d4976d

It is recommended to install |PXC| from official Percona repositories
using the corresponding tool for your system:

* :ref:`Install using apt <apt-repo>` if you are running Debian or Ubuntu
* :ref:`Install using yum <yum-repo>` if you are running Red Hat Enterprise
  Linux or CentOS

.. note::

  You can also `download packages
  <https://www.percona.com/downloads/Percona-XtraDB-Cluster-56>`_ from the
  Percona website and install them manually using :command:`dpkg` or
  :command:`rpm`.

If you want to build and run |PXC| from source, see :ref:`compile`.

If you want to run |PXC| using Docker, see :ref:`docker`.

.. toctree::
   :hidden:

   Install Using apt <installation/apt_repo>
   Install Using yum <installation/yum_repo>
   Compile from Source <installation/compile>
   Run in a Docker container <installation/docker>
