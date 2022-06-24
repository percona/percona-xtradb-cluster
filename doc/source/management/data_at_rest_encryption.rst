.. _data_at_rest_encryption:

========================
Data at Rest Encryption
========================

Introduction
============

The *data at rest* encryption refers to encrypting data stored on a disk on a
server. If an unauthorized user accesses the data files from the file system,
encryption ensures the user cannot read the file contents. |Percona Server| lets
you enable, disable, and apply encryptions to the following objects:

 The within the following objects:

* File-per-tablespace table

* Schema

* General tablespace

* System tablespace

* Temporary table

* Binary log files

* Redo log files

* Undo tablespaces

* Doublewrite buffer files

The transit data is defined as data that is transmitted to another node or client. Encrypted transit data uses an SSL connection.

Percona XtraDB Cluster |version| supports all |data-at-rest| generally-available encryption
features available from |percona-server| 8.0.

Configuring PXC to use keyring_file plugin
==========================================

Configuration
--------------------------------------------------------------------------------

Percona XtraDB Cluster inherits the Percona Server for MySQL behavior to
configure the ``keyring_file``
plugin. `Install the plugin <https://dev.mysql.com/doc/refman/5.7/en/install-plugin.html>`__ and add the following options in the configuration file:

.. code-block:: text

   [mysqld]
   early-plugin-load=keyring_file.so
   keyring_file_data=<PATH>/keyring

The ``SHOW PLUGINS`` statement checks if the plugin has been
successfully loaded.

.. note:: PXC recommends the same configuration on all cluster nodes,
   and all nodes should have the keyring configured. A mismatch in the keyring configuration does not allow the JOINER node to join the cluster.

If the user has a bootstrapped node with keyring enabled, then upcoming cluster
nodes inherit the keyring (the encrypted key) from the DONOR node.

Usage
*****

XtraBackup re-encrypts the data using a transition-key and the JOINER node
re-encrypts it using a newly generated master-key.

Keyring (or, more generally, the Percona XtraDB Cluster SST process) is backward compatible, as
in higher version JOINER can join from lower version DONOR, but not vice-versa.


Percona XtraDB Cluster does not allow the combination of nodes with encryption and nodes without
encryption to maintain data consistency. For
example, the user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. If the user attempts to create a table with
encryption on node-1, the creation fails on node-2, causing data inconsistency.
A node fails to start if it fails to load the keyring plugin.

.. note:: If the user does not specify the keyring parameters, the node does not
   know that it must load the keyring. The JOINER node may start, but it
   eventually shuts down when the DML level inconsistency with encrypted
   tablespace is detected.

If a node does not have an encrypted tablespace, the keyring is not generated,
and the keyring file is empty. Creating an encrypted table on the node generates
the keyring.

In an operation that is local to the node, you can rotate the key as needed. The ``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is not replicated on cluster.

The JOINER node generates its keyring.

Compatibility
--------------------------------------------------------------------------------

Keyring (or, more generally, the Percona XtraDB Cluster SST process) is backward compatible. A
higher version JOINER can join from lower version DONOR, but not vice-versa.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin allows storing the master-key in vault-server
(vs. local file as in case of ``keyring_file``).

.. warning:: The rsync tool does not support the ``keyring_vault``. Any rysnc-SST on a joiner is
   aborted if the ``keyring_vault`` is configured.

Configuration
--------------------------------------------------------------------------------

Configuration options are the same as
`upstream
<https://www.percona.com/doc/percona-server/8.0/security/using-keyring-plugin.html>`_.
The ``my.cnf`` configuration file should contain
the following options:

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

The detailed description of these options can be found in the `upstream
documentation
<https://www.percona.com/doc/percona-server/8.0/security/using-keyring-plugin.html>`_.

Vault-server is an external server, so make sure the PXC node can reach the
server.

.. note:: Percona XtraDB Cluster recommends using the same keyring_plugin type on all
   cluster nodes. Mixing the keyring plugin types is recommended only while
   transitioning
   from
   ``keyring_file`` -> ``keyring_vault`` or vice-versa.

All nodes do not need to refer to the same vault server. Whatever
vault server is used, it must be accessible from the respective node. All nodes
do not need to use the same mount point.

If the node is not able to reach or connect to the vault server, an error is
notified during the server boot, and the node refuses to start:

.. code-block:: text

   2018-05-29T03:54:33.859613Z 0 [Warning] Plugin keyring_vault reported:
   'There is no vault_ca specified in keyring_vault's configuration file.
   Please make sure that Vault's CA certificate is trusted by the machine
   from which you intend to connect to Vault.'
   2018-05-29T03:54:33.977145Z 0 [ERROR] Plugin keyring_vault reported:
   'CURL returned this error code: 7 with error message : Failed to connect
   to 127.0.0.1 port 8200: Connection refused'

If some nodes of the cluster are unable to connect to vault-server, this
relates only to these specific nodes: e.g., if node-1 can connect, and
node-2 cannot connect, only node-2 refuses to start. Also, if the server has
a pre-existing encrypted object and on reboot, the server fails to connect to
the vault-server, the object is not accessible.

In case when vault-server is accessible, but authentication credential is incorrect,
the consequences are the same, and the corresponding error looks like the following:

.. code-block:: text

   2018-05-29T03:58:54.461911Z 0 [Warning] Plugin keyring_vault reported:
   'There is no vault_ca specified in keyring_vault's configuration file.
   Please make sure that Vault's CA certificate is trusted by the machine
   from which you intend to connect to Vault.'
   2018-05-29T03:58:54.577477Z 0 [ERROR] Plugin keyring_vault reported:
   'Could not retrieve list of keys from Vault. Vault has returned the
   following error(s): ["permission denied"]'

In case of an accessible vault-server with the wrong mount point, there is no
error during server boot, but the node still refuses to start:

.. code-block:: text

   mysql> CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring
   plugin is loaded.

   ... [ERROR] Plugin keyring_vault reported: 'Could not write key to Vault. ...
   ... [ERROR] Plugin keyring_vault reported: 'Could not flush keys to keyring'

Mixing keyring plugin types
============================

With XtraBackup introducing transition-key logic, it is now possible to
mix and match keyring plugins. For example, the user has node-1 configured to use the
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note:: Percona recommends the same configuration for all the nodes of the
   cluster. A mix and match in keyring plugin types is recommended only during
   the transition from one keying type to another.

Temporary file encryption
=========================

Migrating Keys Between Keyring Keystores
========================================

Percona XtraDB Cluster supports key migration between keystores. The migration can be performed
offline or online.

Offline Migration
-----------------

In offline migration, the node to migrate is shut down, and the migration server
takes care of migrating keys for the said server to a new keystore.

For example, a cluster has three Percona XtraDB Cluster nodes, n1, n2, and n3. The nodes use the
``keyring_file``. To migrate the n2 node to use ``keyring_vault``, use the following procedure:

1. Shut down the n2 node.
2. Start the Migration Server (``mysqld`` with a special option).
3. The Migration Server copies the keys from the n2 keyring file and adds them
   to the vault server.
4. Start the n2 node with the vault parameter, and the keys are available.

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

On a successful migration, the destination keystore receives additional migrated keys
(pre-existing keys in the destination keystore are not touched or removed). The source
keystore retains the keys as the migration performs a copy operation and
not a move operation.

If the migration fails, the destination keystore is unchanged.

Online Migration
----------------

In online migration, the node to migrate is kept running, and the migration
server takes
care of migrating keys for the said server to a new keystore by connecting to
the node.

For example, a cluster has three Percona XtraDB Cluster nodes, n1, n2, and n3. The nodes use the
``keyring_file``. Migrate the n3 node to use ``keyring_vault`` using the following procedure:

1. Start the Migration Server (``mysqld`` with a special option).
2. The Migration Server copies the keys from the n3 keyring file and adds them
   to the vault server.
3. Restart the n3 node with the vault parameter, and the keys are available.

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

On a successful migration, the destination keystore receives the additional
migrated keys. Any pre-existing keys in the destination keystore are unchanged.
The source keystore retains the keys as the migration performs a copy operation and
not a move operation.

If the migration fails, the destination keystore is not changed.

Migration server options
------------------------

* ``--keyring-migration-source``: The source keyring plugin that manages the
  keys to be migrated.

* ``--keyring-migration-destination``: The destination keyring plugin to which
  the migrated keys are to be copied

  .. note:: For offline migration, no additional key migration options are
     needed.

* ``--keyring-migration-host``: The host where the running server is located.
  This host is always the local host.

* ``--keyring-migration-user``, ``--keyring-migration-password``: The username
  and password for the account used to connect to the running server.

* ``--keyring-migration-port``: Used for TCP/IP connections, the running
  server's port  number used to connect.

* ``--keyring-migration-socket``: Used for Unix socket file or Windows named pipe
  connections, the running server socket or named pipe used to connect.

Prerequisite for migration:

Make sure to pass required keyring options and other configuration parameters
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

.. seealso::

    `encrypt traffic documentation <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html>`_

    |percona-server| Documentation:  Data-at-Rest Encryption
       https://www.percona.com/doc/percona-server/8.0/security/data-at-rest-encryption.html#data-at-rest-encryption
