.. _encrypt-traffic:

======================
Encrypting PXC Traffic
======================

There are two kinds of traffic in |PXC|: client-server traffic (the one between
client applications and cluster nodes), and replication traffic, which includes
:term:`SST`, :term:`IST`, write-set replication, and various service messages.
|PXC| supports encryption for all types of traffic. Replication traffic
encryption can be configured either in automatic or in manual mode. Both modes
are considered in the following sections, and additional section covers manual
configuration for client-server traffic encryption - the one which is now
enabled in MySQL by default and can be used by a client.

.. contents::
   :local:
   :depth: 1

.. _ssl-auto-conf:

SSL Automatic Configuration
===========================

Automatic configuration needs two steps to be done:

1. :ref:`creating-certs`
2. :ref:`enabling_encrypt-cluster-traffic`

.. _creating-certs:

Creating Certificates
---------------------

Automatic configuration of the SSL encryption needs key and certificate files.
Starting from the version 5.7, |MySQL| generates default key and certificate
files and places them in data directory. These auto-generated files are
suitable for automatic SSL configuration, but you should use the same key and
certificate files on all nodes. Also you can override auto-generated files with
manually created ones, as covered by the :ref:`generate-keys-certs` section.

Necessary key and certificate files are first searched at the ``ssl-ca``,
``ssl-cert``, and ``ssl-key`` options under ``[mysqld]``. If these options are
not set, it then looks in the data directory for :file:`ca.pem`,
:file:`server-cert.pem`, and :file:`server-key.pem` files.

.. note:: The ``[sst]`` section is not searched.

If all three files are found, they are used to configure encryption with
``encrypt=4`` mode (as explained in :ref:`xtrabackup` subsection), which
enables encryption of the SST, IST, and replication traffic). If any of the
files are missing, a fatal error is generated.

.. _enabling_encrypt-cluster-traffic:

Enabling :variable:`pxc-encrypt-cluster-traffic`
------------------------------------------------

|PXC| includes the :variable:`pxc-encrypt-cluster-traffic` variable that
enables automatic configuration of SSL encryption there-by encrypting
:term:`SST`, :term:`IST`, and replication traffic.

This variable cannot be changed on the command line during runtime. To
enable automatic configuration of SSL encryption, set
``pxc-encrypt-cluster-traffic=ON`` in the the ``[mysqld]`` section of the
:file:`my.cnf` file, and restart the cluster. Otherwise it is disabled,
meaning that you need to configure SSL manually to have encrypted SST
and IST traffic, and internal communication.

.. note:: Setting ``pxc-encrypt-cluster-traffic=ON`` has effect of applying
          the following settings in :file:`my.cnf` configuration file::

           [mysqld]
           wsrep_provider_options=”socket.ssl_key=server-key.pem;socket.ssl_cert=server-cert.pem;socket.ssl_ca=ca.pem”

           [sst]
           encrypt=4
           ssl-key=server-key.pem
           ssl-ca=ca.pem
           ssl-cert=server-cert.pem

          For :variable:`wsrep_provider_options`, only the mentioned options are affected
          (``socket.ssl_key``, ``socket,ssl_cert``, and ``socket.ssl_ca``),
          the rest are not modified.

.. _generate-keys-certs:

Generating Keys and Certificates Manually
=========================================

As mentioned above, |MySQL| generates default key and certificate
files and places them in data directory. If user wants to override these
certificates, the following new sets of files can be generated:

* *Certificate Authority (CA) key and certificate*
  to sign the server and client certificates.
* *Server key and certificate*
  to secure database server activity and write-set replication traffic.
* *Client key and certificate*
  to secure client communication traffic.

These files should be generated using `OpenSSL <https://www.openssl.org/>`_.

.. note:: The ``Common Name`` value
   used for the server and client keys and certificates
   must differ from that value used for the CA certificate.

.. _generate-ca-key-cert:

Generating CA Key and Certificate
---------------------------------

The Certificate Authority is used to verify the signature on certificates.

1. Generate the CA key file::

    $ openssl genrsa 2048 > ca-key.pem

#. Generate the CA certificate file::

    $ openssl req -new -x509 -nodes -days 3600
        -key ca-key.pem -out ca.pem

.. _generate-server-key-cert:

Generating Server Key and Certificate
-------------------------------------

1. Generate the server key file::

    $ openssl req -newkey rsa:2048 -days 3600 \
        -nodes -keyout server-key.pem -out server-req.pem

#. Remove the passphrase::

    $ openssl rsa -in server-key.pem -out server-key.pem

#. Generate the server certificate file::

    $ openssl x509 -req -in server-req.pem -days 3600 \
        -CA ca.pem -CAkey ca-key.pem -set_serial 01 \
        -out server-cert.pem

.. _generate-client-key-cert:

Generating Client Key and Certificate
-------------------------------------

1. Generate the client key file::

    $ openssl req -newkey rsa:2048 -days 3600 \
        -nodes -keyout client-key.pem -out client-req.pem

#. Remove the passphrase::

    $ openssl rsa -in client-key.pem -out client-key.pem

#. Generate the client certificate file::

    $ openssl x509 -req -in client-req.pem -days 3600 \
        -CA ca.pem -CAkey ca-key.pem -set_serial 01 \
        -out client-cert.pem

.. _verify-certs:

Verifying Certificates
----------------------

To verify that the server and client certificates
are correctly signed by the CA certificate,
run the following command::

 $ openssl verify -CAfile ca.pem server-cert.pem client-cert.pem

If the verification is successful, you should see the following output::

 server-cert.pem: OK
 client-cert.pem: OK

Deploying Keys and Certificates
-------------------------------

Use a secure method (for example, ``scp`` or ``sftp``)
to send the key and certificate files to each node.
Place them under the :file:`/etc/mysql/certs/` directory
or similar location where you can find them later.

.. note:: Make sure that this directory is protected with proper permissions.
   Most likely, you only want to give read permissions
   to the user running ``mysqld``.

The following files are required:

* Certificate Authority certificate file (``ca.pem``)

  This file is used to verify signatures.

* Server key and certificate files (``server-key.pem`` and ``server-cert.pem``)

  These files are used to secure database server activity
  and write-set replication traffic.

* Client key and certificate files (``client-key.pem`` and ``client-cert.pem``)

  These files are required only if the node should act as a MySQL client.
  For example, if you are planning to perform SST using ``mysqldump``.

.. note:: :ref:`upgrade-certs` subsection covers the details on upgrading
   certificates, if necessary.

.. _enable-encryption:

Enabling Encryption Manually
============================

To enable encryption manually, you need to specify the location
of the required key and certificate files in the |PXC| configuration.
If you do not have the necessary files, see :ref:`generate-keys-certs`.

.. note:: Encryption settings are not dynamic.
   To enable it on a running cluster, you need to restart the entire cluster.

There are three aspects of |PXC| operation, where you can enable encryption:

* :ref:`encrypt-client-server`

  This refers to communication between client applications and cluster nodes.

* :ref:`encrypt-replication`

  This refers to all internal |PXC| communication,
  such as, write-set replication, :term:`IST`, and various service messages.

* :ref:`encrypt-sst`

  This refers to :term:`SST` traffic during full data copy
  from one cluster node (donor) to the joining node (joiner).

.. _encrypt-client-server:

Encrypting Client-Server Communication
--------------------------------------

|PXC| uses the underlying MySQL encryption mechanism
to secure communication between client applications and cluster nodes.
Specify the following settings in the :file:`my.cnf` configuration file
for each node::

 [mysqld]
 ssl-ca=/etc/mysql/certs/ca.pem
 ssl-cert=/etc/mysql/certs/server-cert.pem
 ssl-key=/etc/mysql/certs/server-key.pem

 [client]
 ssl-ca=/etc/mysql/certs/ca.pem
 ssl-cert=/etc/mysql/certs/client-cert.pem
 ssl-key=/etc/mysql/certs/client-key.pem

After you restart the node,
it will use these files for encrypting communication with clients.
MySQL clients require only the second part of the configuration
to communicate with cluster nodes.

.. _encrypt-replication:

Encrypting Replication Traffic
------------------------------

Replication traffic refers to the following:

* Write-set replication is the main workload of |PXC|
  (replicating transactions that execute on one node to all other nodes).
* Incremental State Transfer (:term:`IST`)
  is copying only missing transactions from DONOR to JOINER node.
* Service messages ensure that all nodes are synchronized.

All this traffic is transferred via the same underlying communication channel
(``gcomm``). Securing this channel will ensure that :term:`IST` traffic,
write-set replication, and service messages are encrypted.

To enable encryption for all these processes,
define the paths to the key, certificate and certificate authority files
using the following :ref:`wsrep provider options <wsrep_provider_index>`:

* :variable:`socket.ssl_ca`
* :variable:`socket.ssl_cert`
* :variable:`socket.ssl_key`

To set these options, use the :variable:`wsrep_provider_options` variable
in the configuration file::

 wsrep_provider_options="socket.ssl=yes;socket.ssl_ca=/etc/mysql/certs/ca.pem;socket.ssl_cert=/etc/mysql/certs/server-cert.pem;socket.ssl_key=/etc/mysql/certs/server-key.pem"

.. note:: You must use the same key and certificate files on all nodes,
   preferably those used for :ref:`encrypt-client-server`.

.. _upgrade-certs:

Upgrading Certificates
**********************

The following procedure shows how to upgrade certificates
used for securing replication traffic when there are two nodes in the cluster:

1. Restart the first node with the :variable:`socket.ssl_ca` option
   set to a combination of the the old and new certificates in a single file.

   For example, you can merge contents of ``old-ca.pem``
   and ``new-ca.pem`` into ``upgrade-ca.pem`` as follows:

   .. code-block:: bash

      cat old-ca.pem > upgrade-ca.pem && \
      cat new-ca.pem >> upgrade-ca.pem

   Set the :variable:`wsrep_provider_options` variable as follows:

   .. code-block:: text

      wsrep_provider_options="socket.ssl=yes;socket.ssl_ca=/etc/mysql/certs/upgrade-ca.pem;socket.ssl_cert=/etc/mysql/certs/old-cert.pem;socket.ssl_key=/etc/mysql/certs/old-key.pem"

#. Restart the second node with the :variable:`socket.ssl_ca`,
   :variable:`socket.ssl_cert`, and :variable:`socket.ssl_key` options
   set to the corresponding new certificate files.

   .. code-block:: text

      wsrep_provider_options="socket.ssl=yes;socket.ssl_ca=/etc/mysql/certs/new-ca.pem;socket.ssl_cert=/etc/mysql/certs/new-cert.pem;socket.ssl_key=/etc/mysql/certs/new-key.pem"

#. Restart the first node with the new certificate files,
   as in the previous step.

#. You can remove the old certificate files.

.. _encrypt-sst:

Encrypting SST Traffic
----------------------

This refers to full data transfer
that usually occurs when a new node (JOINER) joins the cluster
and receives data from an existing node (DONOR).

For more information, see :ref:`state_snapshot_transfer`.

.. note:: If ``keyring_file`` plugin is used, then SST encryption is mandatory:
          when copying encrypted data via SST, the keyring must be sent over
          with the files for decryption. In this case following options are to
          be set in :file:`my.cnf` on all nodes:

          .. code-block:: text

             early-plugin-load=keyring_file.so
             keyring-file-data=/path/to/keyring/file

          The cluster will not work if keyring configuration across nodes is
          different.

The following SST methods are available:
``xtrabackup``, ``rsync``, and ``mysqldump``.

.. _xtrabackup:

xtrabackup
**********

This is the default SST method (the :variable:`wsrep_sst_method` is set
to ``xtrabackup-v2``), which uses |PXB|_ to perform non-blocking transfer
of files. For more information, see :ref:`xtrabackup_sst`.

Encryption mode for this method is selected using the :option:`encrypt` option:

* ``encrypt=0`` is the default value, meaning that encryption is disabled.

* ``encrypt=1``, ``encrypt=2``, and ``encrypt=3`` have been deprecated.

* ``encrypt=4`` enables encryption based on key and certificate files
  generated with OpenSSL.
  For more information, see :ref:`generate-keys-certs`.

  To enable encryption for SST using XtraBackup,
  specify the location of the keys and certificate files
  in the each node's configuration under ``[sst]``:

  .. code-block:: text

     [sst]
     encrypt=4
     ssl-ca=/etc/mysql/certs/ca.pem
     ssl-cert=/etc/mysql/certs/server-cert.pem
     ssl-key=/etc/mysql/certs/server-key.pem

.. note:: SSL clients require DH parameters to be at least 1024 bits,
   due to the `logjam vulnerability
   <https://en.wikipedia.org/wiki/Logjam_(computer_security)>`_.
   However, versions of ``socat`` earlier than 1.7.3 use 512-bit parameters.
   If a :file:`dhparams.pem` file of required length
   is not found during SST in the data directory,
   it is generated with 2048 bits, which can take several minutes.
   To avoid this delay, create the :file:`dhparams.pem` file manually
   and place it in the data directory before joining the node to the cluster::

    openssl dhparam -out /path/to/datadir/dhparams.pem 2048

   For more information, see `this blog post <https://www.percona.com/blog/2017/04/23/percona-xtradb-cluster-dh-key-too-small-error-during-an-sst-using-ssl/>`_.

rsync
*****

This SST method does not support encryption.
Avoid using this method if you need to secure traffic
between DONOR and JOINER nodes.

.. _mysqldump_sst:

mysqldump
*********

This SST method dumps data from DONOR and imports it to JOINER.
Encryption in this case is performed using the same certificates
configured for :ref:`encrypt-client-server`,
because ``mysqldump`` connects through the database client.

Here is how to enable encryption for SST using ``mysqldump``
in a running cluster:

1. Create a user for SST on one of the nodes:

   .. code-block:: sql

      mysql> CREATE USER 'sst_user'$'%' IDENTIFIED BY PASSWORD 'sst_password';

   .. note:: This user must have the same name and password on all nodes
      where you want to use ``mysqldump`` for SST.

#. Grant usage privileges to this user and require SSL:

   .. code-block:: sql

      mysql> GRANT USAGE ON *.* TO 'sst_user' REQUIRE SSL;

#. To make sure that the SST user replicated across the cluster,
   run the following query on another node:

   .. code-block:: sql

      mysql> SELECT User, Host, ssl_type FROM mysql.user WHERE User='sst_user';

      +----------+------+----------+
      | User     | Host | ssl_type |
      +----------+------+----------+
      | sst_user | %    | Any      |
      +----------+------+----------+

   .. note:: If the :variable:`wsrep_OSU_method` is set to ROI,
      you need to manually create the SST user on each node in the cluster.

#. Specify corresponding certificate files
   in both ``[mysqld]`` and ``[client]`` sections
   of the configuration file on each node::

    [mysqld]
    ssl-ca=/etc/mysql/certs/ca.pem
    ssl-cert=/etc/mysql/certs/server-cert.pem
    ssl-key=/etc/mysql/certs/server-key.pem

    [client]
    ssl-ca=/etc/mysql/certs/ca.pem
    ssl-cert=/etc/mysql/certs/client-cert.pem
    ssl-key=/etc/mysql/certs/client-key.pem

   For more information, see :ref:`encrypt-client-server`.

#. Also specify the SST user credentials
   in the :variable:`wsrep_sst_auth` variable on each node::

    [mysqld]
    wsrep_sst_auth = sst_user:sst_password

#. Restart the cluster with the new configuration.

If you do everything correctly,
``mysqldump`` will connect to DONOR with the SST user,
generate a dump file, and import it to JOINER node.

