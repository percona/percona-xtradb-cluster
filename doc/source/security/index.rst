.. _security:

===============
Security Basics
===============

By default, |PXC| does not provide any protection for stored data.
There are several considerations to take into account for securing |PXC|:

* :ref:`secure-network`

  Anyone with access to your network can connect to any |PXC| node
  either as a client or as another node joining the cluster.
  You should consider restricting access using VPN
  and filter traffic on ports used by |PXC|.

* :ref:`encrypt-traffic`

  Unencrypted traffic can potentially be viewed
  by anyone monitoring your network.
  You should enable encryption for all nodes in the cluster to prevent this.

* Data-at-rest encryption

  |PXC| supports `tablespace encryption
  <https://dev.mysql.com/doc/refman/5.7/en/innodb-data-encryption.html#innodb-data-encryption-enabling-disabling>`_
  to provide at-rest encryption for physical tablespace data files.

  For more information, see the following blog post:

  * `MySQL Data at Rest Encryption <https://www.percona.com/blog/2016/04/08/mysql-data-at-rest-encryption/>`_

.. _security-modules:

Security Modules
================

Most modern distributions include special security modules
that control access to resources for users and applications.
By default, these modules will most likely constrain communication
between |PXC| nodes.

The easiest solution is to disable or remove such programs,
however, this is not recommended for production environments.
You should instead create necessary security policies for |PXC|.

SELinux
-------

`SELinux <https://selinuxproject.org>`_ is usually enabled by default
in Red Hat Enterprise Linux and derivatives (including CentOS).
During installation and configuration,
you can set the mode to ``permissive`` by running the following command::

 setenforce 0

.. note::  This only changes the mode at runtime.
   To run SELinux in permissive mode after a reboot,
   set ``SELINUX=permissive`` in the :file:`/etc/selinux/config`
   configuration file.

To use SELinux with |PXC|, you need to create an access policy.
For more information, see `SELinux and MySQL
<https://blogs.oracle.com/jsmyth/selinux-and-mysql>`_.

AppArmor
--------

`AppArmor <http://wiki.apparmor.net/index.php/Main_Page>`_ is included
in Debian and Ubuntu.
During installation and configuration,
you can disable AppArmor for ``mysqld``:

1. Create the following symbolic link::

    $ sudo ln -s /etc/apparmor.d/usr /etc/apparmor.d/disable/.sbin.mysqld

#. Restart AppArmor::

    $ sudo service apparmor restart

   .. note:: If your system uses ``systemd``,
      run the following command instead::

       $ sudo systemctl restart apparmor

To use AppArmor with |PXC|, you need to create or extend the MySQL profile.
For more information, see `AppArmor and MySQL
<https://blogs.oracle.com/mysql/apparmor-and-mysql-v2>`__.

