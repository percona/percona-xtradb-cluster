.. _encrypt-traffic:

======================
Encrypting PXC Traffic
======================

|PXC| supports encryption for all traffic involved in cluster operation.

.. contents::
   :local:
   :depth: 1

.. _client-server-encryption:

Client-Server Communication
===========================

This refers to communication between client applications and cluster nodes.
To secure client connections,
you need to generate and use SSL keys and certificates.

The following example shows :file:`my.cnf` configuration
for server nodes and client instances to use SSL:

.. code-block:: text

   [mysqld]
   ssl-ca=ca.pem
   ssl-cert=server-cert.pem
   ssl-key=server-key.pem

   [client]
   ssl-ca=ca.pem
   ssl-cert=client-cert.pem
   ssl-key=client-key.pem

For more information, see the relevant sections about SSL certificates
in `Galera Cluster Documentation
<http://galeracluster.com/documentation-webpages/sslcert.html>`_.
and `MySQL Server Documentation
<http://dev.mysql.com/doc/refman/5.7/en/creating-ssl-files-using-openssl.html>`_.

SST Traffic
===========

This refers to full data transfer
that usually occurs when a new node (JOINER) joins the cluster
and receives data from an existing node (DONOR).

For more information, see :ref:`state_snapshot_transfer`.

When copying encrypted data via SST,
the keyring must be sent over with the files for decryption.
Make sure that the following options are set in :file:`my.cnf` on all nodes:

.. code-block:: text

   early-plugin-load=keyring_file.so
   keyring-file-data=/path/to/keyring/file

.. warning:: The cluster will not work if keyring configuration across nodes
   is different.

The following SST methods are available:
``rsync``, ``mysqldump``, and ``xtrabackup``.

rsync
-----

This SST method does not support encryption.
Avoid using this method if you need to secure traffic
between DONOR and JOINER nodes.

mysqldump
---------

This SST method dumps data from DONOR and imports it to JOINER.
Encryption in this case is performed using the SSL certificates configured
for :ref:`secure MySQL client-server communication <client-server-encryption>`.

Here is how to perform secure SST using ``mysqldump``:

1. Ensure that the DONOR node is configured for SSL encryption
   (both ``[mysqld]`` and ``[client]`` sections).
   For more information, see :ref:`client-server-encryption`.

#. Create an SSL user on the DONOR node.

   .. code-block:: sql

      mysql> GRANT USAGE ON *.* TO 'sslsst' REQUIRE SSL;

#. Specify the DONOR superuser credentials
   in the :variable:`wsrep_sst_auth` variable.

#. Start the JOINER node without Galera library (``--wsrep_provider=none``)
   and create an SSL user with the same name and grants as on the DONOR node.

#. Configure SSL encryption on JOINER node with the same parameters as DONOR
   (both ``[mysqld]`` and ``[client]`` sections).

#. Restart JOINER node with Galera library.

If you do everything correctly,
``mysqldump`` will connect to DONOR using SSL user,
generate a dump file, and import it to JOINER node.

For more information, see `the relevant section in Galera Cluster documentation <http://galeracluster.com/documentation-webpages/mysqldump.html>`_.

xtrabackup
----------

This is the default SST method,
which uses Percona XtraBackup to perform non-blocking transfer of files.
For more information,
see :ref:`xtrabackup_sst`.

Encryption mode for this method is selected using the :option:`encrypt` option.
Depending on the mode you select, other options will be required.

* To enable built-in XtraBackup encryption, use the following configuration:

  .. code-block:: text

     [sst]
     encrypt=1
     encrypt-algo=AES256
     encrypt-key=A1EDC73815467C083B0869508406637E

  In this example, you can set ``encrypt-key-file`` instead of ``encrypt-key``.

  For more information,
  see `Encrypted Backups <https://www.percona.com/doc/percona-xtrabackup/2.4/innobackupex/encrypted_backups_innobackupex.html>`_.

* To enable SST encryption based on OpenSSL
  with the certificate authority (``tca``) and certificate (``tcert``) files:

  .. code-block:: text

     [sst]
     encrypt=2
     tcert=/path/to/server.pem
     tca=/path/to/server.crt

  For more information,
  see `Securing Traffic Between two Socat Instances Using SSL <http://www.dest-unreach.org/socat/doc/socat-openssltunnel.html>`.

* To enable SST encryption based on OpenSSL
  with the key (``tkey``) and certificate (``tcert``) files:

  .. code-block:: text

     [sst]
     encrypt=2
     tcert=/path/to/server.pem
     tkey=/path/to/server.key

.. note:: Percona XtraBackup supports keyring transfer in version 2.4.4 and later.

IST Traffic, Write-Set Replication, and Service Messages
========================================================

IST refers to transferring only missing transactions from DONOR to JOINER node.
Write-set replication is the main workload in |PXC|
whenever a transaction is performed on one node,
it is replicated to all other nodes.
Service messages ensure that all nodes are synchronized.

All of this traffic is transferred via the same underlying communication
channel used by Galera (``gcomm``).
Securing this channel will ensure that IST traffic, write-set replication,
and service messages are encypted.

To enable SSL for all internal node processes,
define the paths to the key, certificate and certificate authority files
using the following parameters.

* |socket.ssl_key|_
* |socket.ssl_cert|_
* |socket.ssl_ca|_

.. |socket.ssl_key| replace:: ``socket.ssl_key``
.. _socket.ssl_key: http://galeracluster.com/documentation-webpages/galeraparameters.html#socket-ssl-key

.. |socket.ssl_cert| replace:: ``socket.ssl_cert``
.. _socket.ssl_cert: http://galeracluster.com/documentation-webpages/galeraparameters.html#socket-ssl-cert

.. |socket.ssl_ca| replace:: ``socket.ssl_ca``
.. _socket.ssl_ca: http://galeracluster.com/documentation-webpages/galeraparameters.html#socket-ssl-ca

To set these parameters, use the :variable:`wsrep_provider_options` variable.

.. code-block:: text

   wsrep_provider_options="socket.ssl=yes;socket.ssl_key=/path/to/server-key.pem;socket.ssl_cert=/path/to/server-cert.pem;socket.ssl_ca=/path/to/cacert.pem"

For more information, see `Index of wsrep provider options <https://www.percona.com/doc/percona-xtradb-cluster/5.7/wsrep-provider-index.html>`_.

.. note:: You must use the same key and certificate files on all nodes,
   preferably those used for :ref:`client-server-encryption`.

Upgrading Certificates
----------------------

The following example shows how to upgrade certificates
used for securing IST traffic, write-set replication, and service messages,
assumig there are two nodes in the cluster:

1. Restart Node 1 with a ``socket.ssl_ca``
   that includes both the new and the old certificates in a single file.

   For example, you can merge contents of ``old-ca.pem`` and ``new-ca.pem``
   into ``upgrade-ca.pem`` as follows:

   .. code-block:: bash

      cat old-ca.pem > upgrade-ca.pem && cat new-ca.pem >> upgrade-ca.pem

   Set the :variable:`wsrep_provider_options` variable similar to the following:

   .. code-block:: text

      wsrep_provider_options=socket.ssl=yes;socket.ssl_ca=/path/to/upgrade-ca.pem;socket.ssl_cert=path/to/old-cert.pem;socket.ssl_key=/path/to/old-key.pem

#. Restart Node 2 with the new ``socket.ssl_ca``, ``socket.ssl_cert``,
   and ``socket.ssl_key``.

   .. code-block:: text

      wsrep_provider_options=socket.ssl=yes;socket.ssl_ca=/path/to/upgrade-ca.pem;socket.ssl_cert=/path/to/new-cert.pem;socket.ssl_key=/path/to/new-key.pem

#. Restart Node 1 with the new ``socket.ssl_ca``, ``socket.ssl_cert``,
   and ``socket.ssl_key``.

   .. code-block:: text

      wsrep_provider_options=socket.ssl=yes;socket.ssl_ca=/path/to/upgrade-ca.pem;socket.ssl_cert=/path/to/new-cert.pem;socket.ssl_key=/path/to/new-key.pem


