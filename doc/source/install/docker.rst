.. _pxc.docker-container.running:

===================================
Running Percona XtraDB Cluster in a Docker Container
===================================

Docker images of Percona XtraDB Cluster are hosted publicly on Docker Hub at
https://hub.docker.com/r/percona/percona-xtradb-cluster/.

For more information about using Docker, see the `Docker Docs`_. Make
sure that you are using the latest version of Docker. The ones
provided via ``apt`` and ``yum`` may be outdated and cause errors.

.. _`Docker Docs`: https://docs.docker.com/

.. note::

   By default, Docker pulls the image from Docker Hub if the image is not
   available locally.

   The image contains only the most essential binaries for Percona XtraDB Cluster to
   run. Some utilities included in a Percona Server for MySQL or MySQL
   installation might be missing from the Percona XtraDB Cluster Docker image.

The following procedure describes how to set up a simple 3-node cluster
for evaluation and testing purposes. Do not use these instructions in a
production environment because the MySQL certificates generated in this
procedure are self-signed. For a
production environment, you should generate and store the certificates to be used by Docker.

In this procedure, all of the nodes run Percona XtraDB Cluster 8.0 in separate containers on
one host:

1. Create a ~/pxc-docker-test/config directory.

2. Create a custom.cnf file with the following contents, and place the
   file in the new directory:

   .. code-block:: text

       [mysqld]
       ssl-ca = /cert/ca.pem
       ssl-cert = /cert/server-cert.pem
       ssl-key = /cert/server-key.pem

       [client]
       ssl-ca = /cert/ca.pem
       ssl-cert = /cert/client-cert.pem
       ssl-key = /cert/client-key.pem

       [sst]
       encrypt = 4
       ssl-ca = /cert/ca.pem
       ssl-cert = /cert/server-cert.pem
       ssl-key = /cert/server-key.pem

3. Create a cert directory and generate self-signed SSL certificates on the host node:

   .. code-block:: bash

       $ mkdir -m 777 -p ~/pxc-docker-test/cert
       docker run --name pxc-cert --rm -v ~/pxc-docker-test/cert:/cert
       percona/percona-xtradb-cluster:8.0 mysql_ssl_rsa_setup -d /cert

4. Create a Docker network::

    docker network create pxc-network

#. Bootstrap the cluster (create the first node)::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=test1234# \
      -e CLUSTER_NAME=pxc-cluster1 \
      --name=pxc-node1 \
      --net=pxc-network \
      -v ~/pxc-docker-test/config:/etc/percona-xtradb-cluster.conf.d \
      percona/percona-xtradb-cluster:8.0

#. Join the second node::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=test1234# \
      -e CLUSTER_NAME=pxc-cluster1 \
      -e CLUSTER_JOIN=pxc-node1 \
      --name=pxc-node2 \
      --net=pxc-network \
      -v ~/pxc-docker-test/cert:/cert \
      -v ~/pxc-docker-test/config:/etc/percona-xtradb-cluster.conf.d \
      percona/percona-xtradb-cluster:8.0

#. Join the third node::

    docker run -d \
      -e MYSQL_ROOT_PASSWORD=test1234# \
      -e CLUSTER_NAME=pxc-cluster1 \
      -e CLUSTER_JOIN=pxc-node1 \
      --name=pxc-node3 \
      --net=pxc-network \
      -v ~/pxc-docker-test/cert:/cert \
      -v ~/pxc-docker-test/config:/etc/percona-xtradb-cluster.conf.d \
      percona/percona-xtradb-cluster:8.0

To verify the cluster is available, do the following:

1. Access the MySQL client. For example, on the first node::

    $ sudo docker exec -it pxc-node1 /usr/bin/mysql -uroot -ptest1234#
    mysql: [Warning] Using a password on the command line interface can be insecure.
    Welcome to the MySQL monitor.  Commands end with ; or \g.
    Your MySQL connection id is 12
    ...
    You are enforcing ssl connection via unix socket. Please consider
    switching ssl off as it does not make connection via unix socket
    any more secure

    mysql>

#. View the wsrep status variables::

    mysql> show status like 'wsrep%';
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



.. seealso::

    `Creating SSL and RSA Certificates and Keys
    <https://dev.mysql.com/doc/refman/8.0/en/creating-ssl-rsa-files.html>`_
    How
    to create the files required for SSL and RSA support in MySQL.

