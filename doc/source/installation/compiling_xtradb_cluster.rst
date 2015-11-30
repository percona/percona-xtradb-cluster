===========================================
 Compiling and Installing from Source Code
===========================================

The source code is available from the *Launchpad* project `here <https://launchpad.net/percona-xtradb-cluster>`_. The easiest way to get the code is with :command:`bzr branch` of the desired release, such as the following: ::

  bzr branch lp:percona-xtradb-cluster

You should then have a directory named after the release you branched, such as ``percona-xtradb-cluster``.

.. note::

  In place of a full bzr branch (which can take hours), if the intention is just to build, a `source ball <http://www.percona.com/downloads/Percona-XtraDB-Cluster/LATEST/source/>`_ or a `lightweight checkout <http://doc.bazaar.canonical.com/beta/en/user-guide/using_checkouts.html#>`_, might be better options. 


Compiling on Linux
==================

Prerequisites
-------------

The following packages and tools must be installed to compile *Percona XtraDB Cluster* from source. These might vary from system to system.

In Debian-based distributions, you need to: ::

  $ apt-get install build-essential flex bison automake autoconf bzr \
    libtool cmake libaio-dev mysql-client libncurses-dev zlib1g-dev libboost-dev

In ``RPM``-based distributions, you need to: ::

  $ yum install cmake gcc gcc-c++ libaio libaio-devel automake autoconf bzr \
    bison libtool ncurses5-devel boost

Compiling 
------------

The most esiest way to build binaries is to run script: ::

  BUILD/compile-pentium64-wsrep

If you feel confident to use cmake, you make compile with cmake adding
-DWITH_WSREP=1 to parameters.

.. note::

  Galera tree needs to be checked out in the source directory of ``percona-xtradb-cluster``. Otherwise the build - ``build-binary.sh`` would fail.


Examples how to build ``RPM`` and ``DEB`` packages can be found in packaging/percona directory in the source code.
