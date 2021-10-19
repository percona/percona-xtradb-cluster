.. _configure:

===========================================
Configuring Nodes for Write-Set Replication
===========================================

After installing Percona XtraDB Cluster on each node, you need to configure the cluster.
In this section, we will demonstrate how to configure a three node cluster:

+--------+-----------+---------------+
| Node   | Host      | IP            |
+========+===========+===============+
| Node 1 | pxc1      | 192.168.70.61 |
+--------+-----------+---------------+
| Node 2 | pxc2      | 192.168.70.62 |
+--------+-----------+---------------+
| Node 3 | pxc3      | 192.168.70.63 |
+--------+-----------+---------------+

1. Stop the Percona XtraDB Cluster server. After the installation completes the server is not
   started. You need this step if you have started the server manually.

   .. code-block:: bash

      $ sudo service mysql stop

#. Edit the configuration file of the first node to provide the cluster settings.

   *If you use Debian or Ubuntu*, edit |file.mysqld-cnf|:

   .. code-block:: text

      wsrep_provider=/usr/lib/galera4/libgalera_smm.so
      wsrep_cluster_name=pxc-cluster
      wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

   *If you use Red Hat or CentOS*, edit |file.my-cnf|. Note that on these systems you set
   the wsrep_provider option to a different value:

    .. code-block:: text

       wsrep_provider=/usr/lib64/galera4/libgalera_smm.so
       wsrep_cluster_name=pxc-cluster
       wsrep_cluster_address=gcomm://192.168.70.61,192.168.70.62,192.168.70.63

#. Configure *node 1*. 

   .. code-block:: text

      wsrep_node_name=pxc1
      wsrep_node_address=192.168.70.61
      pxc_strict_mode=ENFORCING

#. Set up *node 2* and *node 3* in the same way: Stop the server and update the configuration file
   applicable to your system. All settings are the same except for |wsrep-node-name| and |wsrep-node-address|.

   For node 2
     wsrep_node_name=pxc2
     wsrep_node_address=192.168.70.62

   For node 3
      wsrep_node_name=pxc3
      wsrep_node_address=192.168.70.63

#. Set up the traffic encryption settings. Each node of the cluster must use the same SSL certificates.

   .. code-block:: text

      [mysqld]
      wsrep_provider_options=”socket.ssl_key=server-key.pem;socket.ssl_cert=server-cert.pem;socket.ssl_ca=ca.pem”

      [sst]
      encrypt=4
      ssl-key=server-key.pem
      ssl-ca=ca.pem
      ssl-cert=server-cert.pem

.. important::

   In Percona XtraDB Cluster |version|, the :ref:`encrypt-replication-traffic` is
   enabled by default (via the |pxc-encrypt-cluster-traffic|
   variable).

   The replication traffic encryption cannot be enabled on a running cluster. If
   it was disabled before the cluster was bootstrapped, the cluster must to
   stopped. Then set up the encryption, and bootstrap (see :ref:`bootstrap`)
   again.

   .. seealso::

      More information about the security settings in Percona XtraDB Cluster
         - :ref:`security`
	 - :ref:`encrypt-traffic`
	 - :ref:`ssl-auto-conf`

Template of the configuration file
================================================================================

Here is an example of a full configuration file installed on CentOS to
|file.my-cnf|.

.. include:: .res/code-block/sql/sample-my-conf.txt

Next Steps: Bootstrap the first node 
================================================================================

After you configure all your nodes, initialize Percona XtraDB Cluster by bootstrapping the first
node according to the procedure described in :ref:`bootstrap`.


Essential configuration variables
================================================================================

:variable:`wsrep_provider`

  Specify the path to the Galera library. The location depends on the distribution:

  * Debian and Ubuntu: :file:`/usr/lib/galera4/libgalera_smm.so`
  * Red Hat and CentOS: :file:`/usr/lib64/galera4/libgalera_smm.so`

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

  By default, Percona XtraDB Cluster uses Percona XtraBackup_ for :term:`State Snapshot Transfer <SST>`.
  ``xtrabackup-v2`` is the only supported option for this variable.
  This method requires a user for SST to be set up on the initial node.

:variable:`pxc_strict_mode`

  :ref:`pxc-strict-mode` is enabled by default and set to ``ENFORCING``,
  which blocks the use of tech preview features and unsupported features in Percona XtraDB Cluster.

binlog_format_

  Galera supports only row-level replication, so set ``binlog_format=ROW``.

.. binlog_format replace:: ``binlog_format``
.. _binlog_format: http://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_format

default_storage_engine_

  Galera fully supports only the InnoDB storage engine.
  It will not work correctly with MyISAM
  or any other non-transactional storage engines.
  Set this variable to ``default_storage_engine=InnoDB``.

.. default_storage_engine replace:: ``default_storage_engine``
.. _default_storage_engine: http://dev.mysql.com/doc/refman/8.0/en/server-system-variables.html#sysvar_default_storage_engine

innodb_autoinc_lock_mode_

  Galera supports only interleaved (``2``) lock mode for InnoDB.
  Setting the traditional (``0``) or consecutive (``1``) lock mode
  can cause replication to fail due to unresolved deadlocks.
  Set this variable to ``innodb_autoinc_lock_mode=2``.

.. innodb_autoinc_lock_mode replace:: ``innodb_autoinc_lock_mode``
.. _innodb_autoinc_lock_mode: http://dev.mysql.com/doc/refman/8.0/en/innodb-parameters.html#sysvar_innodb_autoinc_lock_mode

.. include:: .res/replace.file.txt
.. include:: .res/replace.opt.txt
