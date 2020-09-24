.. _tarball:

====================================
Installing |PXC| from Binary Tarball
====================================

Percona provides generic tarballs with all required files and binaries
for manual installation. Download the appropriate tarball package from
https://www.percona.com/downloads/Percona-XtraDB-Cluster-57

In |Percona XtraDB Cluster| 5.7.31-31.45 and later, the multiple binary tarballs available in the **Linux - Generic** section are replaced with the following:

.. tabularcolumns:: |p{5cm}|p{5cm}|p{5cm}|

.. list-table::
   :header-rows: 1

   * - Name
     - Type
     - Description
   * - Percona-XtraDB-Cluster-5.7.xx-relxx-xx-Linux.x86_64.glibc2.12.tar.gz
     - Full
     - Contains binaries, libraries, test files, and debug symbols
   * - Percona-XtraDB-Cluster-5.7.xx-relxx-xx-Linux.x86_64.glibc2.12-minimal.tar.gz
     - Minimal
     - Contains binaries, and libraries but does not include test files, or debug symbols

Both binary tarballs support all distributions.

For installations before |Percona XtraDB Cluster| 5.7.31-31.45, the **Linux - Generic** section contains multiple tarballs which are based on the *OpenSSL* library available in your distribution:

* ``ssl100``: for Debian prior to 9 and Ubuntu prior to 14.04 versions

* ``ssl101``: for CentOS 6 and CentOS 7

* ``ssl102``: for Debian 9 and Ubuntu versions starting from 14.04

.. note::

    In CentOS version 7.04 and later, the *OpenSSL* library is ``ssl102``. 

For example, you can use ``curl`` as follows::

  curl -O https://www.percona.com/downloads/Percona-XtraDB-Cluster-57/Percona-XtraDB-Cluster-5.7.31-31.45/binary/tarball/Percona-XtraDB-Cluster-5.7.31-rel34-31.45.1.Linux.x86_64.glibc2.tar.gz


