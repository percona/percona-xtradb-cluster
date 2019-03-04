.. _configure:

===========================================
Configuring Nodes for Write-Set Replication
===========================================

After installing |PXC| on a node,
configure it with information about the cluster.

.. note:: Make sure that the |PXC| server is not running.

   .. code-block:: bash

      $ sudo service mysql stop

Configuration examples assume there are three |PXC| nodes:

+--------+-----------+---------------+
| Node   | Host      | IP            |
+========+===========+===============+
| Node 1 | pxc1      | 192.168.70.61 |
+--------+-----------+---------------+
| Node 2 | pxc2      | 192.168.70.62 |
+--------+-----------+---------------+
| Node 3 | pxc3      | 192.168.70.63 |
+--------+-----------+---------------+

If you are running Debian or Ubuntu,
add the following configuration variables to :file:`/etc/percona-xtradb-cluster.conf.d/wsrep.cnf`
on the first node::

 wsrep_provider=/usr/lib/libgalera_smm.so

 wsrep_cluster_name=pxc-cluster
 wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

 wsrep_node_name=pxc1
 wsrep_node_address=192.168.70.61

 wsrep_sst_method=xtrabackup-v2
 wsrep_sst_auth=sstuser:passw0rd

 pxc_strict_mode=ENFORCING

 binlog_format=ROW
 default_storage_engine=InnoDB
 innodb_autoinc_lock_mode=2

If you are running Red Hat or CentOS,
add the following configuration variables to :file:`/etc/percona-xtradb-cluster.conf.d/wsrep.cnf`
on the first node::

 wsrep_provider=/usr/lib64/galera3/libgalera_smm.so

 wsrep_cluster_name=pxc-cluster
 wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

 wsrep_node_name=pxc1
 wsrep_node_address=192.168.70.61

 wsrep_sst_method=xtrabackup-v2
 wsrep_sst_auth=sstuser:passw0rd

 pxc_strict_mode=ENFORCING

 binlog_format=ROW
 default_storage_engine=InnoDB
 innodb_autoinc_lock_mode=2

Use the same configuration for the second and third nodes,
except the ``wsrep_node_name`` and ``wsrep_node_address`` variables:

* For the second node::

   wsrep_node_name=pxc2
   wsrep_node_address=192.168.70.62

* For the third node::

   wsrep_node_name=pxc3
   wsrep_node_address=192.168.70.63

Configuration Reference
=======================

:variable:`wsrep_provider`

  Specify the path to the Galera library.

  .. note:: The location depends on the distribution:

     * Debian or Ubuntu: :file:`/usr/lib/libgalera_smm.so`

     * Red Hat or CentOS: :file:`/usr/lib64/galera3/libgalera_smm.so`

:variable:`wsrep_cluster_name`

  Specify the logical name for your cluster.
  It must be the same for all nodes in your cluster.

:variable:`wsrep_cluster_address`

  Specify the IP addresses of nodes in your cluster.
  At least one is required for a node to join the cluster,
  but it is recommended to list addresses of all nodes.
  This way if the first node in the list is not available,
  the joining node can use other addresses.

  .. note:: No addresses are required for the initial node in the cluster.
     However, it is recommended to specify them
     and :ref:`properly bootstrap the first node <bootstrap>`.
     This will ensure that the node is able to rejoin the cluster
     if it goes down in the future.

:variable:`wsrep_node_name`

  Specify the logical name for each individual node.
  If this variable is not specified, the host name will be used.

:variable:`wsrep_node_address`

  Specify the IP address of this particular node.

:variable:`wsrep_sst_method`

  By default, |PXC| uses |PXB|_ for *State Snapshot Transfer* (:term:`SST`).
  Setting ``wsrep_sst_method=xtrabackup-v2`` is highly recommended.
  This method requires a user for SST to be set up on the initial node.
  Provide SST user credentials with the :variable:`wsrep_sst_auth` variable.

:variable:`wsrep_sst_auth`

  Specify authentication credentials for :term:`SST`
  as ``<sst_user>:<sst_pass>``.
  You must create this user when :ref:`bootstrap`
  and provide necessary privileges for it:

  .. code-block:: sql

     mysql> CREATE USER 'sstuser'@'localhost' IDENTIFIED BY 'passw0rd';
     mysql> GRANT RELOAD, LOCK TABLES, PROCESS, REPLICATION CLIENT ON *.* TO
       'sstuser'@'localhost';
     mysql> FLUSH PRIVILEGES;

  For more information, see `Privileges for Percona XtraBackup
  <https://www.percona.com/doc/percona-xtrabackup/2.4/using_xtrabackup/privileges.html>`_.

:variable:`pxc_strict_mode`

  :ref:`pxc-strict-mode` is enabled by default and set to ``ENFORCING``,
  which blocks the use of experimental and unsupported features in |PXC|.

|binlog_format|_

  Galera supports only row-level replication, so set ``binlog_format=ROW``.

.. |binlog_format| replace:: ``binlog_format``
.. _binlog_format: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_format

|default_storage_engine|_

  Galera fully supports only the InnoDB storage engine.
  It will not work correctly with MyISAM
  or any other non-transactional storage engines.
  Set this variable to ``default_storage_engine=InnoDB``.

.. |default_storage_engine| replace:: ``default_storage_engine``
.. _default_storage_engine: http://dev.mysql.com/doc/refman/5.7/en/server-system-variables.html#sysvar_default_storage_engine

|innodb_autoinc_lock_mode|_

  Galera supports only interleaved (``2``) lock mode for InnoDB.
  Setting the traditional (``0``) or consecutive (``1``) lock mode
  can cause replication to fail due to unresolved deadlocks.
  Set this variable to ``innodb_autoinc_lock_mode=2``.

.. |innodb_autoinc_lock_mode| replace:: ``innodb_autoinc_lock_mode``
.. _innodb_autoinc_lock_mode: http://dev.mysql.com/doc/refman/5.7/en/innodb-parameters.html#sysvar_innodb_autoinc_lock_mode

Next Steps
==========

After you configure all your nodes,
initialize |PXC| by bootstrapping the first node
according to the procedure described in :ref:`bootstrap`.

