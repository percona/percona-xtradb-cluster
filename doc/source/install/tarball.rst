.. _tarball:

====================================
Installing |PXC| from Binary Tarball
====================================

Percona provides generic tarballs with all required files and binaries
for manual installation.

You can download the appropriate tarball package from
https://www.percona.com/downloads/Percona-XtraDB-Cluster-80

The **Linux - Generic** section has tarballs for each supported platform. The
code name of the platform is part of the file name. For example, files ending in
*buster.tar.gz* are built for |debian| buster, files ending in *el8.tar.gz* are
built for CentOS 8:

.. admonition:: Examples of tarball names for |pxc| 8.0.18-9.3

   .. list-table::
      :header-rows: 1

      * - Platform
	- Tarball
      *	- CentOS 7
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.el7.tar.gz
      * - CentOS 8
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.el8.tar.gz
      * - Debian 10 (Buster)
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.buster.tar.gz
      * - Debian 9 (Stretch)
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.stretch.tar.gz
      * - Ubuntu 16.4 (Xenial Xerus: xenial)
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.xenial.tar.gz
      * - Ubuntu 18.04 (Bionic Beaver: bionic)
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.bionic.tar.gz
      * - Ubuntu 20.04 (Focal Fossa: focal)
	- Percona-XtraDB-Cluster_8.0.18-9.3_Linux.x86_64.focal.tar.gz


To download a tarball, you can use ``curl`` as follows:

.. code-block:: bash

   $ curl -O https://www.percona.com/downloads/Percona-XtraDB-Cluster-80/Percona-XtraDB-Cluster/binary/tarball/TARBALL_NAME

Be sure to check your system and make sure that the packages are installed that |pxc| |version| depends on.

.. rubric:: For Debian or Ubuntu:

.. code-block:: bash

   $ sudo apt-get install -y \
   socat libdbd-mysql-perl \
   libaio1 libc6 libcurl3 libev4 libgcc1 libgcrypt20 \
   libgpg-error0 libssl-dev libstdc++6 zlib1g libatomic1 libnuma libinfo

.. rubric:: For Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

   $ sudo yum install -y openssl socat  \
   procps-ng chkconfig procps-ng coreutils shadow-utils \
   grep libaio libev libcurl perl-DBD-MySQL perl-Digest-MD5 \
   libgcc libstdc++ libgcrypt libgpg-error zlib glibc openssl-libs
