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

* ``ssl100``: for Debian prior to 9 and Ubuntu prior to 14.04 versions
* ``ssl101``: for CentOS 7 and 8
* ``ssl102``: for Debian 9 or 10 and Ubuntu versions starting from 14.04

For example, you can use ``curl`` as follows::

  curl -O https://www.percona.com/downloads/Percona-XtraDB-Cluster-80/Percona-XtraDB-Cluster/binary/tarball/TARBALL_NAME

