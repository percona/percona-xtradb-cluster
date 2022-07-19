.. _tarball:

======================================================
Installing Percona XtraDB Cluster from Binary Tarball
======================================================

Percona provides generic tarballs with all required files and binaries
for manual installation.

You can download the appropriate tarball package from
https://www.percona.com/downloads/Percona-XtraDB-Cluster-80

.. rubric:: Version updates


Starting with *Percona XtraDB Cluster* 8.0.20-11, the **Linux - Generic**
section lists only full or minimal tar files. Each tarball file replaces the
multiple tar file listing used in earlier versions and supports all
distributions.

.. important::

   Starting with *Percona XtraDB Cluster* 8.0.21, Percona does not provide a
   tarball for RHEL 6/CentOS 6 (glibc2.12).

The version number in the tarball name must be substituted with
the appropriate version number for your system. To indicate that such a
substitution is needed in statements, we use ``<version-number>``.

.. tabularcolumns:: |p{0.10\linewidth}|p{0.30\linewidth}|p{0.40\linewidth}|

.. list-table::
   :header-rows: 1

   * - Name
     - Type
     - Description
   * - |full|
     - Full
     - Contains binary files, libraries, test files, and debug symbols
   * - |min|
     - Minimal
     - Contains binary files and libraries but does not include test files, or
       debug symbols


For installations before |Percona XtraDB Cluster| 8.0.20-11, the **Linux - Generic** section contains multiple tarballs based on the operating system names:

.. code-block:: text

    Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.bionic.tar.gz
    Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.buster.tar.gz
    ...

For example, you can use ``curl`` as follows:

.. code-block:: bash

   $ curl -O https://downloads.percona.com/downloads/Percona-XtraDB-Cluster-LATEST/Percona-XtraDB-Cluster-8.0.27/binary/tarball/Percona-XtraDB-Cluster_8.0.27-18.1_Linux.x86_64.glibc2.17-minimal.tar.gz


Check your system to make sure the packages that the PXC
version requires are installed.

.. rubric:: For Debian or Ubuntu:

.. code-block:: bash

   $ sudo apt-get install -y \
   socat libdbd-mysql-perl \
   libaio1 libc6 libcurl3 libev4 libgcc1 libgcrypt20 \
   libgpg-error0 libssl1.1 libstdc++6 zlib1g libatomic1

.. rubric:: For Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

   $ sudo yum install -y openssl socat  \
   procps-ng chkconfig procps-ng coreutils shadow-utils \


.. |full| replace:: Percona-XtraDB-Cluster_<version-number>-Linux.x86_64.glibc2.17.tar.gz

.. |min|
 replace:: Percona-XtraDB-Cluster_<version-number>-Linux.x86_64.glibc2.17.minimal.tar.gz
