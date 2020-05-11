.. _data_at_rest_encryption:

=======================
Data at Rest Encryption
=======================

.. contents::
   :local:

.. _innodb_general_tablespace_encryption:

Introduction
============

The *data at rest* term refers to all the data stored on disk on some server within
system tablespace, general tablespace, redo-logs, undo-logs, etc. As an
opposite, *data in transit* means data transmitted to other node or client.
Data-in-transit can be encrypted using SSL connection (details are available in
the `encrypt traffic documentation <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html>`_) and
therefore supposed to be safe. Below sections are about securing the
data-at-rest only. 

|PXC| |version| supports all |data-at-rest| encryption features available from |percona-server| 8.0.

.. seealso::

   |percona-server| Documentation: Transparent Data Encryption
      https://www.percona.com/doc/percona-server/8.0/security/data-at-rest-encryption.html#data-at-rest-encryption

InnoDB tablespace encryption
============================

|MySQL| supports tablespace encryption, but only for file-per-table tablespace.
User should create a table that has its own dedicated tablespace, making this
tablespace encrypted by specifying the appropriate option.

File-per-tablespace and General tablespace encryption are table/tablespace
specific features and are enabled on object level through DDL:

.. code-block:: sql

   CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION=’Y’;
   CREATE TABLESPACE foo ADD DATAFILE 'foo.ibd' ENCRYPTION='Y';

DDL statements are replicated in PXC cluster, thus creating encrypted table or
tablespace on all the nodes of the cluster.

This feature requires a keyring plugin to be loaded before it can be used.
Currently, Percona Server (and in turn |PXC|) supports 2 types of keyring
plugin: ``keyring_file`` and ``keyring_vault``.

Configuring PXC to use keyring_file plugin
==========================================

keyring_file
--------------------------------------------------------------------------------

The following subsection covers some of the important semantics of
``keyring_file`` plugin needed to use it in scope of data-at-rest encryption.

``keyring_file`` stores encryption key to a physical file. Location of this
file is specified by ``keyring_file_data`` parameter configured during startup.

Configuration
--------------------------------------------------------------------------------

|PXC| inherits upstream (Percona Server) behavior to configure ``keyring_file``
plugin. Following options are to be set in the configuration file:

.. code-block:: text

   [mysqld]
   early-plugin-load=keyring_file.so
   keyring_file_data=<PATH>/keyring

A ``SHOW PLUGINS`` statement can be used further to check if plugin has been
successfully loaded.

.. note:: PXC recommends same configuration on all the nodes of the cluster,
   and that also means all the nodes of the cluster should have keyring
   configured. Mismatch in keyring configuration will not allow JOINER node to
   join the cluster.

If the user has bootstrapped node with keyring enabled, then upcoming nodes of the
cluster will inherit the keyring (encrypted key) from the DONOR node
or generate it.

Usage
--------------------------------------------------------------------------------

|xtrabackup| re-encrypts the data using a transition-key and the JOINER node
re-encrypts it using a new generated master-key.

The keyring is generated on the JOINER node. SST is done using the
``xtrabackup-v2`` method. The keyring is sent over explicitly before
the real data backup/streaming starts.

|PXC| doesn't allow to combine nodes with encryption and nodes without 
encryption. This is not allowed in order to maintain data consistency. For
example, the user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. If the user tries to create a table with
encryption on node-1, it will fail on node-2 causing data inconsistency.
A node will fail to start if it fails to load the keyring plugin. 

.. note:: If the user hasn’t specified keyring parameters there is no way for the node
   to know that it needs to load the keyring. The JOINER node may start but it eventually
   shuts down when the DML level inconsistency with encrypted tablespace is
   detected.

If a node doesn’t have encrypted tablespace, the keyring is not generated and
the keyring file is empty. The actual keyring is generated only when the node starts
using encrypted tablespace.

The user can rotate the key as needed. This operation is local to the
node. The ``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is
not replicated on cluster.

The JOINER node generates its own keyring. 

Compatibility
--------------------------------------------------------------------------------

Keyring (or, more generally, the |PXC| SST process) is backward compatible, as
in higher version JOINER can join from lower version DONOR, but not vice-versa.
More details are covered below, in
:ref:`data-at-rest-encryption-upgrade-compatibility-issues` section.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin allows storing the master-key in vault-server
(vs. local file as in case of ``keyring_file``).

Configuration
--------------------------------------------------------------------------------

Configuration options are same as `upstream
<https://www.percona.com/doc/percona-server/8.0/management/data_at_rest_encryption.html#keyring-vault-plugin>`_. The
``my.cnf`` configuration file should contain following options:

.. code-block:: text

   [mysqld]
   early-plugin-load="keyring_vault=keyring_vault.so"
   keyring_vault_config="<PATH>/keyring_vault_n1.conf"

Also ``keyring_vault_n1.conf`` file contents should be :

.. code-block:: text

   vault_url = http://127.0.0.1:8200
   secret_mount_point = secret1
   token = e0345eb4-35dd-3ddd-3b1e-e42bb9f2525d
   vault_ca = /data/keyring_vault_confs/vault_ca.crt

Detailed description of these options can be found in the `upstream documentation <https://www.percona.com/doc/percona-server/8.0/management/data_at_rest_encryption.html#keyring-vault-plugin>`_.

Vault-server is an external server so make sure PXC node is able to reach to the said
server.

.. note:: |PXC| recommends to use same keyring_plugin on all the nodes of the
   cluster. Mix-match is recommended to use it only while transitioning from
   ``keyring_file`` -> ``keyring_vault`` or vice-versa.

It is not necessary that all the nodes refer to same vault server. Whatever
vault server is used, it should be accessible from the respective node. Also
there is no restriction for all nodes to use the same mount point.

If the node is not able to reach/connect to vault server, an error is notified
during the server boot, and node refuses to start:

.. code-block:: text

   ... [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. ...
   ... [ERROR] Plugin keyring_vault reported: 'CURL returned this error code: 7 with error message : ...

If some nodes of the cluster are unable to connect to vault-server, this
relates only to these specific nodes: e.g. if node-1 is able to connect, and
node-2 is not, only node-2 will refuse to start. Also, if server has
pre-existing encrypted object and on reboot server fails to connect to
vault-server, the object is not accessible.

In case when vault-server is accessible but authentication credential are wrong, 
consequences are the same, and the corresponding error looks like following:

.. code-block:: text

   ... [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. ...
   ... [ERROR] Plugin keyring_vault reported: 'Could not retrieve list of keys from Vault. ...

In case of accessible vault-server with the wrong mount point, there is no
error during server boot, but sitll node refuses to start:

.. code-block:: text

   mysql> CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring plugin is loaded.

   ... [ERROR] Plugin keyring_vault reported: 'Could not write key to Vault. ...
   ... [ERROR] Plugin keyring_vault reported: 'Could not flush keys to keyring'

Mix-match keyring plugins
=========================

With |xtrabackup| introducing transition-key logic it is now possible to
mix-match keyring plugins. For example, user has node-1 configured to use
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note:: Percona recommends same configuration for all the nodes of the
   cluster. Mix-match (in keyring plugins) is recommended only during
   transition from one keying to other.

.. _data-at-rest-encryption-upgrade-compatibility-issues:

Upgrade and compatibility issues
--------------------------------

If all the nodes are using |PXC| 8.0, then the user can freely
configure some nodes to use ``keyring_file`` and other to use
``keyring_vault``. This setup, however, is not recommended and should be used
during transitioning to vault only.

Temporary file encryption
=========================

Percona Server supports the encryption of temporary file storage
enabled using ``encrypt-tmp-files``. This storage or files are local to the
node and has no direct effect on |PXC| replication. |PXC| recommends enabling
it on all the nodes of the cluster, though that is not mandatory. Parameter to
enable this option is same as in Percona Server:

.. code-block:: text

   [mysqld]
   encrypt-tmp-files=ON

Migrating Keys Between Keyring Keystores
========================================

|PXC| supports key migration between keystores. The migration can be performed
offline or online.

Offline Migration
-----------------

In offline migration, the node to migrate is shutdown, and the migration server
takes care of migrating keys for the said server to a new keystore.

Following example illustrates this scenario:

1. Let's say there are 3 |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
   and n2 should be migrated to use ``keyring_vault``
2. User shuts down n2 node.
3. User start's Migration Server (``mysqld`` with a special option).
4. Migration Server copies keys from n2 keyring file and adds them to the vault
   server.
5. User starts n2 node with the vault parameter, and keys should be available.

Here is how the migration server output should look like:

.. code-block:: text

   /dev/shm/pxc80/bin/mysqld --defaults-file=/dev/shm/pxc80/copy_mig.cnf \
   --keyring-migration-source=keyring_file.so \
   --keyring_file_data=/dev/shm/pxc80/node2/keyring \
   --keyring-migration-destination=keyring_vault.so \
   --keyring_vault_config=/dev/shm/pxc80/vault/keyring_vault.cnf &

   ... [Warning] TIMESTAMP with implicit DEFAULT value is deprecated. Please use
       --explicit_defaults_for_timestamp server option (see documentation for more details).
   ... [Note] --secure-file-priv is set to NULL. Operations related to importing and
       exporting data are disabled
   ... [Warning] WSREP: Node is not a cluster node. Disabling pxc_strict_mode
   ... [Note] /dev/shm/pxc80/bin/mysqld (mysqld 8.0-debug) starting as process 5710 ...
   ... [Note] Keyring migration successful.

On successful migration, destination keystore will get additional migrated keys
(pre-existing keys in destination keystore are not touched or removed). Source
keystore continues to retain the keys as migration performs copy operation and
not move operation.

If the migration fails, then destination keystore will be left untouched.

Online Migration
----------------

In online migration, node to migrate is kept running and migration server takes
care of migrating keys for the said server to a new keystore by connecting to
the node.

The following example illustrates this scenario:

1. Let's say there are 3 |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
   and n3 should be migrated to use ``keyring_vault``
2. User start's Migration Server (``mysqld`` with a special option).
3. Migration Server copies keys from n3 keyring file and adds them to the vault
   server.
4. User restarts n3 node with the vault parameter, and keys should be available.

.. code-block:: text

   /dev/shm/pxc80/bin/mysqld --defaults-file=/dev/shm/pxc80/copy_mig.cnf \
   --keyring-migration-source=keyring_vault.so \
   --keyring_vault_config=/dev/shm/pxc80/keyring_vault3.cnf \
   --keyring-migration-destination=keyring_file.so \
   --keyring_file_data=/dev/shm/pxc80/node3/keyring \
   --keyring-migration-host=localhost \
   --keyring-migration-user=root \
   --keyring-migration-port=16300 \
   --keyring-migration-password='' &

On successful migration, the destination keystore will get additional migrated keys
(pre-existing keys in destination keystore are not touched or removed). Source
keystore continues to retain the keys as migration performs copy operation and
not move operation. 

If the migration fails, then destination keystore will be left untouched.

Migration server options
------------------------

* ``--keyring-migration-source``: The source keyring plugin that manages the
  keys to be migrated.

* ``--keyring-migration-destination``: The destination keyring plugin to which
  the migrated keys are to be copied
  
  .. note:: For an offline migration, no additional key migration options are
     needed. 

* ``--keyring-migration-host``: The host where the running server is located.
  This is always the local host.

* ``--keyring-migration-user``, ``--keyring-migration-password``: The username
  and password for the account to use to connect to the running server.

* ``--keyring-migration-port``: For TCP/IP connections, the port number to
  connect to on the running server.

* ``--keyring-migration-socket``: For Unix socket file or Windows named pipe
  connections, the socket file or named pipe to connect to on the running
  server. 

Prerequisite for migration:

Make sure to pass required kerying options and other configuration parameters
for the two keyring plugins. For example, if ``keyring_file`` is one of the
plugins, you must set the :variable:`keyring_file_data` system variable if the
keyring data file location is not the default location.

Other non-keyring options may be required as well. One way to specify these
options is by using ``--defaults-file`` to name an option file that contains
the required options.

.. code-block:: text

   [mysqld]
   basedir=/dev/shm/pxc80
   datadir=/dev/shm/pxc80/copy_mig
   log-error=/dev/shm/pxc80/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400

.. include:: ../.res/replace.txt
