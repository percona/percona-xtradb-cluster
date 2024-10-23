.. _garbd_howto:

=========================================
Setting up Galera Arbitrator
=========================================

The size of a cluster increases when a node joins the cluster and decreases when a node leaves. A cluster reacts to replication problems with inconsistency voting. The size of the cluster determines the required votes to achieve a quorum. If a node no longer responds and is disconnected from the cluster the remaining nodes vote. The majority of the nodes that vote are considered to be in the cluster.

The arbitrator is important if you have an even number of nodes remaining in the cluster. The arbitrator keeps the number of nodes as an odd number, which avoids the split-brain situation. 

A `Galera Arbitrator
<https://galeracluster.com/library/documentation/arbitrator.html>`__ 
is a lightweight member of a **Percona XtraDB Cluster**. This member can vote but does not do any replication and is not included in flow control calculations. The Galera Arbitrator is a separate daemon called ``garbd``. You can start this daemon separately from the cluster and run this daemon either as a service or from the shell. You cannot configure this daemon using the ``my.cnf`` file. 

.. note::

  For more information on how to set up a cluster, read the
  :ref:`ubuntu_howto` or :ref:`centos_howto` manuals.

Installation
============

Galera Arbitrator does not need a dedicated server and can be installed on a machine running other applications. The server must have good network connectivity.

*Galera Arbitrator* can be installed from Percona's repository on Debian/Ubuntu distributions with the following command:

.. code-block:: bash

   root@ubuntu:~# apt install percona-xtradb-cluster-garbd

*Galera Arbitrator* can be installed from Percona's repository on RedHat or derivative distributions with the following command:

.. code-block:: bash

   [root@centos ~]# yum install percona-xtradb-cluster-garbd

Starting ``garbd`` and configuration
=========================================

.. note::

  On **Percona XtraDB Cluster** 8.0, SSL is enabled by default. To run the Galera Arbitrator, you must copy the SSL certificates and configure ``garbd`` to use the certificates. 
  
  It is necessary to specify the cipher. In this example, it is ``AES128-SHA256``. If you do not specify the cipher, an error occurs with a "Terminate called after throwing an instance of 'gnu::NotSet'" message.

  For more information, see `socket.ssl_cipher <https://galeracluster.com/library/documentation/galera-parameters.html#socket-ssl-cipher>`__

When starting from the shell, you can set the parameters from the command line or edit the configuration file. This is an example of starting from the command line:

.. code-block:: bash

  $ garbd --group=my_ubuntu_cluster \
  --address="gcomm://192.168.70.61:4567, 192.168.70.62:4567, 192.168.70.63:4567" \
  --option="socket.ssl=YES; socket.ssl_key=/etc/ssl/mysql/server-key.pem; \
  socket.ssl_cert=/etc/ssl/mysql/server-cert.pem; \
  socket.ssl_ca=/etc/ssl/mysql/ca.pem; \
  socket.ssl_cipher=AES128-SHA256; \
  socket.ssl=yes"

To avoid entering the options each time you start ``garbd``, edit the options in the configuration file. To configure *Galera Arbitrator* on *Ubuntu/Debian*, edit the :file:`/etc/default/garb` file. On RedHat or derivative distributions, the configuration can be found in :file:`/etc/sysconfig/garb` file.

The configuration file should look like this after the installation and before you have added your parameters:

.. code-block:: text

  # Copyright (C) 2013-2015 Codership Oy
  # This config file is to be sourced by garb service script.

  # REMOVE THIS AFTER CONFIGURATION

  # A comma-separated list of node addresses (address[:port]) in the cluster
  # GALERA_NODES=""

  # Galera cluster name, should be the same as on the rest of the nodes.
  # GALERA_GROUP=""

  # Optional Galera internal options string (e.g. SSL settings)
  # see http://galeracluster.com/documentation-webpages/galeraparameters.html
  # GALERA_OPTIONS=""

  # Log file for garbd. Optional, by default logs to syslog
  # Deprecated for CentOS7, use journalctl to query the log for garbd
  # LOG_FILE=""

Add the parameter information about the cluster. For this document, we use the cluster information from :ref:`ubuntu_howto`.

.. note::

  Please note that you must remove the ``# REMOVE THIS AFTER
  CONFIGURATION`` line before you can start the service.

.. code-block:: text

  # This config file is to be sourced by garb service script.

  # A comma-separated list of node addresses (address[:port]) in the cluster
  GALERA_NODES="192.168.70.61:4567, 192.168.70.62:4567, 192.168.70.63:4567"

  # Galera cluster name, should be the same as on the rest of the nodes.
  GALERA_GROUP="my_ubuntu_cluster"

  # Optional Galera internal options string (e.g. SSL settings)
  # see http://galeracluster.com/documentation-webpages/galeraparameters.html
  # GALERA_OPTIONS="socket.ssl_cert=/etc/ssl/mysql/server-key.pem;socket./etc/ssl/mysql/server-key.pem"

  # Log file for garbd. Optional, by default logs to syslog
  # Deprecated for CentOS7, use journalctl to query the log for garbd
  # LOG_FILE="/var/log/garbd.log"

You can now start the *Galera Arbitrator* daemon (``garbd``) by running:

* On Debian or Ubuntu:

.. code-block:: bash

  root@server:~# service garbd start
  [ ok ] Starting /usr/bin/garbd: :.

.. note::

  On systems that run ``systemd`` as the default system and service manager, use ``systemctl`` instead of ``service`` to invoke the command. Currently, both are supported.

  .. code-block:: bash

    root@server:~# systemctl start garb

* On Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

  root@server:~# service garb start
  [ ok ] Starting /usr/bin/garbd: :.

Additionally, you can check the ``arbitrator`` status by running:

* On Debian or Ubuntu:

.. code-block:: bash

  root@server:~# service garbd status
  [ ok ] garb is running.

* On RedHat or derivative distributions:

.. code-block:: bash

  root@server:~# service garb status
  [ ok ] garb is running.
