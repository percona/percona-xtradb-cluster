.. _apparmor:

=====================================================================
Enabling AppArmor 
=====================================================================

|Percona XtraDB Cluster| contains several AppArmor profiles. Multiple profiles allow for easier maintenance because the ``mysqld`` profile is decoupled from the SST script profile. This separation allows the introduction of other SST methods or scripts with their own profiles. 

The following profiles are available:

* An extended version of the |Percona Server| profile which allows the execution of SST script.

* An xtrabackup-v2 SST script profile located in ``/etc/apparmor.d/usr.bin.wsrep_sst_xtrabackup-v2

The ``mysqld`` profile allows the execution of the SST script in PUx mode with the /{usr/}bin/wsrep_sst_*PUx command. The profile is applied if the script contains a profile. The SST script runs in unconfined mode if the script does not contain a profile. The system administrator can change the execution mode to Pix. This action causes a fallback to inherited mode in case the SST script profile is absent.

Profile Adjustments
--------------------

The ``mysqld`` profile and the ``SST`` script's profile can be adjusted, such as moving the data directory, in the same way as `modifying the mysqld profile <https://www.percona.com/doc/percona-server/LATEST/security/apparmor.html#modify-mysqld>`_  in |Percona Server|. 

Working with ``pxc_encrypt_cluster_traffic``
---------------------------------------------

By default, the ``pxc_encrypt_cluster_traffic`` is ``ON``, which means that all cluster traffic is protected with certificates. However, these certificates cannot be located in the data directory since that location is overwritten during the SST process.

`Set up the certificates <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html#encrypt-replication>`_ describes the certificate setup. 

The following AppArmor profile rule grants access to certificates located in /etc/mysql/certs. You must be root or have ``sudo`` privileges.

.. sourcecode:: text

    # Allow config access
      /etc/mysql/** r,

This rule is present in both profiles (usr.sbin.mysqld and usr.bin.wsrep_sst_xtrabackup-v2). The rule allows the administrator to store the certificates anywhere inside of the /etc/mysql/ directory. If the certificates are located outside of the specified directory, you must add an additional rule which allows access to the certificates in both profiles. The rule must have the path to the certificates location, like the following:

.. sourcecode:: text

    # Allow config access
      /path/to/certificates/* r,

The server certificates must be accessible to the mysql user and are readable only by this user.





