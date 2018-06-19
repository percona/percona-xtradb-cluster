.. _tarball:

====================================
Installing |PXC| from Binary Tarball
====================================

Percona provides generic tarballs with all required files and binaries
for manual installation.

You can download the appropriate tarball package from
https://www.percona.com/downloads/Percona-XtraDB-Cluster-57

There are multiple tarballs in the **Linux - Generic** section
depending on the *OpenSSL* library available in your distribution:

* ``ssl100``: for Debian prior to 9 and Ubuntu prior to 14.04 versions

* ``ssl101``: for CentOS 6 and CentOS 7

* ``ssl102``: for Debian 9 and Ubuntu versions starting from 14.04

For example, you can use ``curl`` as follows::

  curl -O https://www.percona.com/downloads/Percona-XtraDB-Cluster-57/Percona-XtraDB-Cluster-5.7.14-26.17/binary/tarball/Percona-XtraDB-Cluster-5.7.14-rel8-26.17.1.Linux.x86_64.ssl101.tar.gz


