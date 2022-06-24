.. _selinux:

===========================================
Enabling SELinux 
===========================================

SELinux helps protects the user's home directory data. SELinux provides the following:

* Prevents unauthorized users from exploiting the system

* Allows authorized users to access files 

* Used as a role-based access control system

For more information, see `Percona Server and SELinux <https://www.percona.com/doc/percona-server/LATEST/security/selinux.html>`_

Red Hat and CentOS distributes a policy module to extend the SELinux policy module for mysqld. We provide the following:

* Extended module for pxc - an extension of the default module for mysqld distributed by the operating system. 

* wsrep-sst-xtrabackup-v2 - allows execution of the xtrabackup-v2 SST script

Modifying Policies
-------------------

Modifications described in `Percona Server and SELinux <https://www.percona.com/doc/percona-server/LATEST/security/selinux.html>`_ can also be applied for |Percona XtraDB Cluster|.

To adjust PXC-specific configurations, especially SST/IST ports, use the following procedures as ``root``:

To enable port ``14567`` instead of the default port ``4567``:

Find the tag associated with the ``4567`` port:

.. sourcecode:: bash

    $ semanage port -l | grep 4567 
    tram_port_t tcp 4567

Run a command to find which rules grant mysqld access to the port:

.. sourcecode:: bash

    $ sesearch -A -s mysqld_t -t tram_port_t -c tcp_socket
    Found 5 semantic av rules:
        allow mysqld_t port_type : tcp_socket { recv_msg send_msg } ;
        allow mysqld_t tram_port_t : tcp_socket { name_bind name_connect } ;
        allow mysqld_t port_type : tcp_socket { recv_msg send_msg } ;
        allow mysqld_t port_type : tcp_socket name_connect ;
        allow nsswitch_domain port_type : tcp_socket { recv_msg send_msg } ;

You could tag port 14567 with the ``tramp_port_t`` tag, but this tag may cause issues because port 14567 is not a TRAM port. Use the general ``mysqld_port_t`` tag to add ports. For example, the following command adds port 14567 to the policy module with the ``mysqld_port_t`` tag.

.. sourcecode:: bash

    $ semanage port -a -t mysqld_port_t -p tcp 14567

You can verify the addition with the following command:

.. sourcecode:: bash

    $ semanage port -l | grep 14567
    mysqld_port_t                  tcp      4568, 14567, 1186, 3306, 63132-63164

To see the tag associated with the 4444 port, run the following command:

.. sourcecode:: bash

    $ semanage port -l | grep 4444
    kerberos_port_t                tcp      88, 750, 4444
    kerberos_port_t                udp      88, 750, 4444

To find the rules associated with ``kerberos_port_t``, run the following:

.. sourcecode:: bash

    $ sesearch -A -s mysqld_t -t kerberos_port_t -c tcp_socket
    Found 9 semantic av rules:
    allow mysqld_t port_type : tcp_socket { recv_msg send_msg } ;
    allow mysqld_t rpc_port_type : tcp_socket name_bind ;
    allow mysqld_t port_type : tcp_socket { recv_msg send_msg } ;
    allow mysqld_t port_type : tcp_socket name_connect ;
    allow nsswitch_domain kerberos_port_t : tcp_socket name_connect ;
    allow nsswitch_domain kerberos_port_t : tcp_socket { recv_msg send_msg } ;
    allow nsswitch_domain reserved_port_type : tcp_socket name_connect ;
    allow mysqld_t reserved_port_type : tcp_socket name_connect ;
    allow nsswitch_domain port_type : tcp_socket { recv_msg send_msg } ;

If you require port 14444 added, use the same method used to add port 14567.

If you must use a port that is already tagged, you can use either of the following ways:

* Change the port tag to ``mysqld_port_t``

* Adjust the mysqld/sst script policy module to allow access to the given port. This method is better since all PXC-related adjustments are within the PXC-related policy modules.

Working with ``pxc_encrypt_cluster_traffic``
--------------------------------------------

By default, the ``pxc_encrypt_cluster_traffic`` is ``ON``, which means that all cluster traffic is protected with certificates. However, these certificates cannot be located in the data directory since that location is overwritten during the SST process.

Review `How to set up the certificates <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html#encrypt-replication>`_. When SELinux is enabled, mysqld must have access to these certificates. The following items must be checked or considered:

* Certificates inside ``/etc/mysql/certs/`` directory must use the ``mysqld_etc_t`` tag. This tag is applied automatically when the files are copied into the directory. When they are moved, the files retain their original context. 

* Certificates are accessible to the mysql user. The server certificates should be readable only by this user.

* Certificates without the proper SELinux context can be restored with the following command:

    .. sourcecode:: bash

        $ restorecon -v /etc/mysql/certs/*

Enabling enforcing mode for PXC
--------------------------------

The process, mysqld, runs in permissive mode, by default, even if SELinux runs in enforcing mode:

.. sourcecode:: bash

    $ semodule -l | grep permissive
    permissive_mysqld_t
    permissivedomains

After ensuring that the system journal does not list any issues, the administrator can remove the permissive mode for mysqld_t:

.. sourcecode:: bash

    $ semanage permissive -d mysqld_t


.. seealso::

    `MariaDB 10.2 Galera Cluster with SELinux-enabled on CentOS 7 <https://ospi.fi/blog/mariadb-10-2-galera-cluster-with-selinux-enabled-on-centos-7.html>`_



