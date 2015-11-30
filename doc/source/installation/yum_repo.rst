.. _yum-repo:

===========================================
 Percona :program:`yum` Repository
===========================================

The |Percona| :program:`yum` repository supports popular *RPM*-based operating systems, including the *Amazon Linux AMI*.

The easiest way to install the *Percona Yum* repository is to install an *RPM* that configures :program:`yum` and installs the `Percona GPG key <https://www.percona.com/downloads/RPM-GPG-KEY-percona>`_. 

Automatic Installation
=======================

Execute the following command as a ``root`` user: ::

  $ yum install http://www.percona.com/downloads/percona-release/redhat/0.1-3/percona-release-0.1-3.noarch.rpm

You should see some output such as the following: ::

  Retrieving http://www.percona.com/downloads/percona-release/redhat/0.1-3/percona-release-0.1-3.noarch.rpm
  Preparing...                ########################################### [100%]
     1:percona-release        ########################################### [100%]

Resolving package conflicts
===========================

In *CentOS* 5 ``mysql`` package (a dependency of ``perl-DBD-MySQL`` for ``libmysqlclient.so.15``) conflicts with ``Percona-XtraDB-Cluster-client-55.x86_64`` package. To avoid this, install ``Percona-Server-shared-compat-51`` first and then proceed to install |Percona XtraDB Cluster| with ``yum install Percona-XtraDB-Cluster-55``.  Also, in case, ``mysql`` package was installed,  you will need to remove the it before installing |Percona XtraDB Cluster| as mentioned before

Install XtraDB Cluster
=======================

Following command will install Cluster packages: ::

  $ yum install Percona-XtraDB-Cluster-55

Instead of ``Percona-XtraDB-Cluster-55`` you can install ``Percona-XtraDB-Cluster-full-55`` meta-package which will install ``Percona-XtraDB-Cluster-devel-55``, ``Percona-XtraDB-Cluster-test-55``, ``Percona-XtraDB-Cluster-debuginfo-55``, ``Percona-XtraDB-Cluster-galera-2-debuginfo``, and ``Percona-XtraDB-Cluster-shared-55`` packages in addition.

.. warning:: 

   In order to sucessfully install |Percona XtraDB Cluster| ``socat`` package will need to be installed first. ``socat`` package can be installed from the `EPEL <https://fedoraproject.org/wiki/EPEL>`_ repositories.


Percona `yum` Testing repository
================================

Percona offers pre-release builds from the testing repository. To subscribe to the testing repository, you'll need to enable the testing repository in :file:`/etc/yum.repos.d/percona-release.repo` (both ``$basearch`` and ``noarch``). **NOTE:** You'll need to install the Percona repository first if this hasn't been done already.

