.. _bootstrap:

=========================
Bootstrapping the cluster
=========================

Bootstrapping refers to setting up the initial node in the cluster.
Whether you are creating a new cluster from scratch
or restoring after a cluster-wide crash,
you need to define which node contains the most relevant data.
Other nodes will synchronize to this initial node using |SST|.

The |MySQL| configuration file (:file:`my.cnf`) should contain
at least the following necessary configuration options: ::

  [mysqld]
  # Path to Galera library
  wsrep_provider=/usr/lib64/libgalera_smm.so

  # Cluster connection URL
  wsrep_cluster_address=gcomm://

  # Binlog format (Galera requires it to be ROW)
  binlog_format=ROW

  # Default storage engine (MyISAM is considered experimental)
  default_storage_engine=InnoDB

  # InnoDB autoincrement locks mode (Galera requires it to be 2)
  innodb_autoinc_lock_mode=2

Bootstrapping the cluster involes the following steps:

1. On the initial node, set the :variable:`wsrep_cluster_address` variable
   to ``gcomm://``.

   This tells the node it can bootstrap without any cluster to connect to.
   Setting that and starting up the first node should result in a cluster
   with a :variable:`wsrep_cluster_conf_id` of 1.

2. After starting a single-node cluster,
   change the :variable:`wsrep_cluster_address` variable
   to list all the other nodes in the cluster. For example: ::

    wsrep_cluster_address=gcomm://192.168.70.2,192.168.70.3,192.168.70.4

   Cluster membership is not defined by this setting.
   It is defined by the nodes that join the cluster
   with the proper cluster name configured
   using the :variable:`wsrep_cluster_name` variable
   (defaults to ``my_wsrep_cluster``).
   Hence, the :variable:`wsrep_cluster_address` variable
   does not need to be identical on all nodes.
   However, it is good practice, because on restart
   the node will try all other nodes in that list
   and look for any that are currently running in the cluster.

3. Once the first node is configured, start other nodes, one at a time.

   When bootstrapping, other nodes will most likely be using |SST|,
   so you should avoid joining multiple nodes at once.

If the cluster has been bootstrapped before,
to avoid editing the :file:`my.cnf` twice to change the
:variable:`wsrep_cluster_address` to ``gcomm://`` and then to other node
addresses, the initial node can be started with:

.. code-block:: bash

  /etc/init.d/mysql bootstrap-pxc

.. note::

   On CentOS/RHEL 7, the following bootstrap command should be used:

   .. code-block:: bash

     systemctl start mysql@bootstrap.service

The above commands will ensure that values in :file:`my.cnf` remain unchanged.
The next time the node is restarted,
it won't require updating the configuration file.
This can be useful when the cluster has been previously set up
and for some reason all nodes went down
and the cluster needs to be restored.
