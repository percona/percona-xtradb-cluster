.. _pxc.docker-container.running:

===================================
Running |PXC| in a Docker Container
===================================

Docker images of |PXC| are hosted publicly on Docker Hub at
https://hub.docker.com/r/percona/percona-xtradb-cluster/.

For more information about using Docker, see the `Docker Docs`_.

.. _`Docker Docs`: https://docs.docker.com/

.. note:: Make sure that you are using the latest version of Docker.
   The ones provided via ``apt`` and ``yum``
   may be outdated and cause errors.

.. note:: By default, Docker will pull the image from Docker Hub
   if it is not available locally.

The following procedure describes how to set up a simple 3-node cluster
for evaluation and testing purposes,
with all nodes running |PXC| 5.7 in separate containers on one host:

1. Create a Docker network::

    docker network create pxc-network

#. Bootstrap the cluster (create the first node)::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=root \
      -e CLUSTER_NAME=cluster1 \
      --name=node1 \
      --net=pxc-network \
      percona/percona-xtradb-cluster:5.7

#. Join the second node::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=root \
      -e CLUSTER_NAME=cluster1 \
      -e CLUSTER_JOIN=node1 \
      --name=node2 \
      --net=pxc-network \
      percona/percona-xtradb-cluster:5.7

#. Join the third node::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=root \
      -e CLUSTER_NAME=cluster1 \
      -e CLUSTER_JOIN=node1 \
      --name=node3 \
      --net=pxc-network \
      percona/percona-xtradb-cluster:5.7

To ensure that the cluster is running:

1. Access the MySQL client. For example, on the first node::

    $ sudo docker exec -it node1 /usr/bin/mysql -uroot -proot
    mysql: [Warning] Using a password on the command line interface can be insecure.
    Welcome to the MySQL monitor.  Commands end with ; or \g.
    Your MySQL connection id is 12
    Server version: 5.7.19-17-57-log Percona XtraDB Cluster (GPL), Release rel17, Revision c10027a, WSREP version 29.22, wsrep_29.22
    
    Copyright (c) 2009-2017 Percona LLC and/or its affiliates
    Copyright (c) 2000, 2017, Oracle and/or its affiliates. All rights reserved.
    
    Oracle is a registered trademark of Oracle Corporation and/or its
    affiliates. Other names may be trademarks of their respective
    owners.
    
    Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.
    
    mysql@node1>

#. View the wsrep status variables::

    mysql@node1> show status like 'wsrep%';
    +------------------------------+-------------------------------------------------+
    | Variable_name                | Value                                           |
    +------------------------------+-------------------------------------------------+
    | wsrep_local_state_uuid       | 625318e2-9e1c-11e7-9d07-aee70d98d8ac            |
    ...
    | wsrep_local_state_comment    | Synced                                          |
    ...
    | wsrep_incoming_addresses     | 172.18.0.2:3306,172.18.0.3:3306,172.18.0.4:3306 |
    ...
    | wsrep_cluster_conf_id        | 3                                               |
    | wsrep_cluster_size           | 3                                               |
    | wsrep_cluster_state_uuid     | 625318e2-9e1c-11e7-9d07-aee70d98d8ac            |
    | wsrep_cluster_status         | Primary                                         |
    | wsrep_connected              | ON                                              |
    ...
    | wsrep_ready                  | ON                                              |
    +------------------------------+-------------------------------------------------+
    59 rows in set (0.02 sec)

