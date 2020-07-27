.. _tarball:

====================================
Installing |PXC| from Binary Tarball
====================================

Percona provides generic tarballs with all required files and binaries
for manual installation.

You can download the appropriate tarball package from
https://www.percona.com/downloads/Percona-XtraDB-Cluster-80

There are multiple tarballs in the **Linux - Generic** section
depending on the *OpenSSL* library available in your distribution:

* ``ssl100``: for Debian prior to 9, and Ubuntu prior to 14.04 versions
* ``ssl101``: for CentOS 7, and 8
* ``ssl102``: for Debian 9, or 10, and Ubuntu versions starting from 14.04

.. note::

    In CentOS version 7.04 and later, the *OpenSSL* library is ``ssl102``. 

For example, you can use ``curl`` as follows:

.. code-block:: bash

   $ curl -O https://www.percona.com/downloads/Percona-XtraDB-Cluster-80/Percona-XtraDB-Cluster/binary/tarball/TARBALL_NAME

Be sure to check your system and make sure that the packages are installed that |pxc| |version| depends on.

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
   grep libaio libev libcurl perl-DBD-MySQL perl-Digest-MD5 \
   libgcc libstdc++ libgcrypt libgpg-error zlib glibc openssl-libs
