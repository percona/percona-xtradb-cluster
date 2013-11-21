.. _bootstrap:

===========================
 Bootstrapping the cluster
===========================

Bootstrapping refers to getting the initial cluster up and running. By bootstrapping you are defining which node is has the correct information, that all the other nodes should synchronize to (via |SST|). In the event of a cluster-wide crash, bootstrapping functions the same way: by picking the initial node, you are essentially deciding which cluster node contains the database you want to go forward with.

The |MySQL| configuration file should contain necessary configuration options to start the |Percona XtraDB Cluster|: :: 

  [mysqld]
  # Path to Galera library
  wsrep_provider=/usr/lib64/libgalera_smm.so
  # Cluster connection URL
  wsrep_cluster_address=gcomm://
  # In order for Galera to work correctly binlog format should be ROW
  binlog_format=ROW
  # MyISAM storage engine has only experimental support
  default_storage_engine=InnoDB
  # This changes how |InnoDB| autoincrement locks are managed and is a requirement for Galera
  innodb_autoinc_lock_mode=2

Bootstrapping the cluster is a bit of a manual process. On the initial node, variable :variable:`wsrep_cluster_address` should be set to the value: ``gcomm://``. The ``gcomm://`` tells the node it can bootstrap without any cluster to connect to. Setting that and starting up the first node should result in a cluster with a :variable:`wsrep_cluster_conf_id` of 1. After this single-node cluster is started, variable :variable:`wsrep_cluster_address` should be updated to the list of all nodes in the cluster. For example: :: 

  wsrep_cluster_address=gcomm://192.168.70.2,192.168.70.3,192.168.70.4
 
Although note that cluster membership is not defined by this setting, it is defined by the nodes that join the cluster with the proper cluster name configured Variable :variable:`wsrep_cluster_name` is used for that, if not explicitly set it will default to ``my_wsrep_cluster``. Hence, variable :variable:`wsrep_cluster_address` does not need to be identical on all nodes, it's just a best practice because on restart the node will try all other nodes in that list and look for any that are currently up and running the cluster.

Once the first node is configured, then each other node should be started, one at a time. In a bootstrap situation, SST is most likely, so generally multiple nodes joining at once should be avoided. 

In case cluster that's being bootstrapped has already been set up before, and to avoid editing the :file:`my.cnf` twice to change the :variable:`wsrep_cluster_address` to ``gcomm://`` and then to change it back to other node addresses, first node can be started with: :: 
 
  /etc/init.d/mysql bootstrap-pxc

This way values in :file:`my.cnf` would remain unchanged. Next time node is restarted it won't require updating the configuration file. This can be useful in case cluster has been previously set up and for some reason all nodes went down and the cluster needs to be bootstrapped again. 

Other Reading
=============

* `How to start a Percona XtraDB Cluster <http://www.mysqlperformanceblog.com/2013/01/29/how-to-start-a-percona-xtradb-cluster/>`_
