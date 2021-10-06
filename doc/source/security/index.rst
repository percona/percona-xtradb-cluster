.. _security:

================================================================================
Security Basics
================================================================================

By default, Percona XtraDB Cluster does not provide any protection for stored data. There are
several considerations to take into account for securing Percona XtraDB Cluster:

* :ref:`secure-network`

  Anyone with access to your network can connect to any Percona XtraDB Cluster node
  either as a client or as another node joining the cluster.
  You should consider restricting access using VPN
  and filter traffic on ports used by Percona XtraDB Cluster.

* :ref:`encrypt-traffic`

  Unencrypted traffic can potentially be viewed by anyone monitoring your
  network. In Percona XtraDB Cluster |version| traffic encryption is enabled by default.

* Data-at-rest encryption

  Percona XtraDB Cluster supports `tablespace encryption
  <https://dev.mysql.com/doc/refman/8.0/en/innodb-tablespace-encryption.html>`_
  to provide at-rest encryption for physical tablespace data files.

  For more information, see the following blog post:

  * `MySQL Data at Rest Encryption <https://www.percona.com/blog/2016/04/08/mysql-data-at-rest-encryption/>`_

.. _security-modules:

Security Modules
================================================================================

Most modern distributions include special security modules
that control access to resources for users and applications.
By default, these modules will most likely constrain communication
between Percona XtraDB Cluster nodes.

The easiest solution is to disable or remove such programs,
however, this is not recommended for production environments.
You should instead create necessary security policies for Percona XtraDB Cluster.

SELinux
--------------------------------------------------------------------------------

`SELinux <https://selinuxproject.org>`_ is usually enabled by default
in Red Hat Enterprise Linux and derivatives (including CentOS). SELinux helps protects the user's home directory data and provides the following:

* Prevents unauthorized users from exploiting the system

* Allows authorized users to access files 

* Used as a role-based access control system

To help with troubleshooting, during installation and configuration,
you can set the mode to ``permissive``::

 setenforce 0

.. note::  This action changes the mode only at runtime.

.. seealso::

    For more information, see :ref:`selinux`

.. apparmor

AppArmor
-----------

`AppArmor <http://wiki.apparmor.net/index.php/Main_Page>`_ is included
in Debian and Ubuntu. |Percona XtraDB Cluster| contains several AppArmor profiles which allows for easier maintenance.
To help with troubleshooting, during the installation and configuration,
you can set the mode to ``complain`` for ``mysqld``.


.. seealso::

    For more information, see :ref:`apparmor`