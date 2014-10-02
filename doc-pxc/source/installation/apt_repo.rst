.. _apt-repo:

===============================================
 Percona :program:`apt` Repository
===============================================

*Debian* and *Ubuntu* packages from *Percona* are signed with a key. Before using the repository, you should add the key to :program:`apt`. To do that, run the following commands: ::

  $ apt-key adv --keyserver keys.gnupg.net --recv-keys 1C4CBDCDCD2EFD2A

Add this to :file:`/etc/apt/sources.list`, replacing ``VERSION`` with the name of your distribution: ::

  deb http://repo.percona.com/apt VERSION main 
  deb-src http://repo.percona.com/apt VERSION main

Remember to update the local cache: ::

  $ apt-get update

Supported Architectures
=======================

 * x86_64 (also known as amd64)
 * x86

Supported Releases
==================

Debian
------

 * 6.0 squeeze
 * 7.0 wheezy

Ubuntu
------

 * 10.04LTS lucid
 * 12.04LTS precise
 * 13.10 saucy
 * 14.04LTS trusty


Install Percona XtraDB Cluster
==============================

Make sure to remove existing |Percona XtraDB Cluster| 5.5 and |Percona Server| 5.5/5.6 packages before proceeding.

Following command will install |Percona XtraDB Cluster| packages: :: 

  $ sudo apt-get install percona-xtradb-cluster-56

Instead of ``percona-xtradb-cluster-56`` you can install ``percona-xtradb-cluster-full-56`` meta-package which will install ``percona-xtradb-cluster-test-5.6``, ``percona-xtradb-cluster-5.6-dbg``, ``percona-xtradb-cluster-garbd-3.x``, ``percona-xtradb-cluster-galera-3.x-dbg``, ``percona-xtradb-cluster-garbd-3.x-dbg`` and ``libmysqlclient18`` packages in addition.

.. note:: 
    
   Garbd is packaged separately as part of debian split packaging. The garbd debian package is ``percona-xtradb-cluster-garbd-3.x``. The package contains, garbd, daemon init script and related config files. This package will be installed if you install the ``percona-xtradb-cluster-56-full`` meta package.

Percona `apt` Testing repository
=================================

Percona offers pre-release builds from the testing repository. To enable it add the following lines to your  :file:`/etc/apt/sources.list` , replacing ``VERSION`` with the name of your distribution: ::

  deb http://repo.percona.com/apt VERSION main testing
  deb-src http://repo.percona.com/apt VERSION main testing

Apt-Pinning the packages
========================

In some cases you might need to "pin" the selected packages to avoid the upgrades from the distribution repositories. You'll need to make a new file :file:`/etc/apt/preferences.d/00percona.pref` and add the following lines in it: :: 

  Package: *
  Pin: release o=Percona Development Team
  Pin-Priority: 1001

For more information about the pinning you can check the official `debian wiki <http://wiki.debian.org/AptPreferences>`_.

