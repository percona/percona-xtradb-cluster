.. _data_at_rest_encryption:

=======================
Data at Rest Encryption
=======================

.. _innodb_general_tablespace_encryption:

Introduction
============

The "Data-at-rest" enables data at rest encryption of the InnoDB
(file-per-table) tablespace by encrypting the physical database files.
The data is automatically encrypted prior to writing to storage and
automatically decrypted when read. If unauthorized users access the data files,
they cannot read the contents.
Data-in-transit can be encrypted using an SSL connection (details are available in
the `encrypt traffic documentation
<https://www.percona.com/doc/percona-xtradb-cluster/5.7/security/encrypt-traffic.html>`_).
Data-at-rest encryption is supported in Percona XtraDB Cluster for
file-per-table tablespace and temporary files.

.. note::

   The Percona Server for MySQL 5.7 data at rest encryption is similar to
   the `MySQL 5.7 data-at-rest encryption
   <https://dev.mysql.com/doc/refman/5.7/en/innodb-data-encryption.html>`_.
   First, review the available encryption features for `Percona Server for
   MySQL 5.7
   <https://www.percona.com/doc/percona-server/5.7/security/data-at-rest-encryption.html>`__.
   Percona Server for MySQL 8.0 provides encryption features and options
   not available in the 5.7 version.

   .. list-table::
       :widths: 20 20 20
       :header-rows: 1

       * - Feature
         - Status
         - GA Version
       * - keyring_plugin
         - Generally Available, supported
         - :rn:`5.7.21-21`
       * - File-Per_Table Tablespace
         - Generally Available, supported
         - :rn:`5.7.21-21`
       * - Temporary Files
         - Generally Available, supported
         - :rn:`5.7.22-22`

Using the keyring_file plugin
==========================================

About the keyring_file
-----------------------

The ``keyring_file`` stores an encryption key in a physical file. Specify
the location of the file with the ``keyring_file_data`` parameter during startup. The following subsections cover
some of the essential procedures for the ``keyring_file`` plugin.


Configuration
*************

Percona XtraDB Cluster inherits the Percona Server for MySQL behavior to
configure the ``keyring_file``
plugin. `Install the plugin <https://dev.mysql.com/doc/refman/5.7/en/install-plugin.html>`__ and add the following options in the configuration file:

.. code-block:: text

   [mysqld]
   early-plugin-load=keyring_file.so
   keyring_file_data=<PATH>/keyring

The ``keyring_file`` must be loaded using the ``--early-plugin-load`` option.

A ``SHOW PLUGINS`` statement can be used to check if the plugin has been
successfully loaded.

.. note:: PXC recommends the same configuration on all cluster nodes,
   and all nodes should have the keyring configured. A JOINER node
   cannot join the cluster if there is a mismatch in the keyring configuration.

If the user has bootstrapped node with keyring enabled, then upcoming cluster nodes
inherit the keyring (the encrypted key) from the DONOR node, in Percona XtraDB
Cluster prior to 5.7.22, or generate the keyring, implemented in Percona XtraDB
Cluster 5.7.22.

Usage
*****

The operations for a keyring are transactional. During write operations, the ``keyring_file`` plugin creates a backup file to ensure the operation can be rolled back if needed.

Prior to Percona XtraDB Cluster :rn:`5.7.22-29.26` the DONOR node had to send the
keyring to the JOINER,
because Percona XtraBackup backs up encrypted tablespaces. The JOINER must have
the encryption key used by the DONOR to encrypt the tables to read these encrypted
tablespaces. This restriction has
been relaxed in Percona XtraDB Cluster 5.7.22 and now Percona XtraBackup
re-encrypts the data using a
transition-key and the JOINER re-encrypts the table using a generated master-key.

A keyring is sent from the DONOR to the JOINER as part of SST process (prior to Percona XtraDB Cluster
5.7.22) or is generated on the JOINER.

The SST process can be done
using xtrabackup (the recommended way), mysqldump or the rsync tool. In *xtrabackup*
case, the keyring is sent before the data backup/streaming
begins. The mysqldump utility uses a logical
backup so it does not need to send keyring. The rsync tool syncs the keys when the data directories are synced.

.. warning:: The rsync tool does not provide a secure channel. A keyring sent
   using the rsync SST could be vulnerable to attack.

The recommended SST process uses xtrabackup. The user can configure a secure channel and the keyring is fully secured. In fact, xtrabackup does not allow a user to send the keyring if the SST channel is not secured.

.. warning Percona does not recommend rsync-based SST for data-at-rest
   encryption using keyring.

To maintain data consistency, Percona XtraDB Cluster does not allow a combination of nodes
with encryption and nodes without encryption. For
example, a user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. A table created with
encryption on node-1 fails on node-2, causing data inconsistency.

With Percona XtraDB Cluster :rn:`5.7.22-29.26`, a node will fail to start if it fails to load keyring plugin.

.. note:: If you do not specify the keyring parameters, the node does not know
   that it must load keyring. A JOINER node may start but eventually
   shutdown when a DML-level inconsistency with encrypted tablespace is
   detected.

If a node does not have an encrypted tablespace, the keyring is not generated and
the keyring file is empty. The keyring is generated only when node uses an
encrypted tablespace.

A user can rotate the key when needed. The ``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is local to the node and is not replicated on
cluster.

Starting from Percona XtraDB Cluster 5.7.22, the JOINER generates its keyring. In Percona XtraDB Cluster before
5.7.22 when JOINER joined the cluster, its keyring was the same as DONOR's keyring.
The user could rotate the key if different keys for each node is part
of the user's requirements (internal rules). Using different keys for each
node is not necessary from the technical side, since all cluster nodes can
continue operating with the same MASTER-key.

Compatibility
*************

The keyring and the Percona XtraDB Cluster SST process is backward compatible.  A higher-version JOINER can join from lower-version DONOR, but not vice-versa.
More details are covered in the `Upgrade and compatibility issues`_ section.

.. note::

    Percona XtraDB Cluster 5.6 does not have encrypted tablespaces. No major
    upgrade scenario for data-at-rest encryption is possible.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin is supported starting from PXC 5.7.22. This plugin
allows storing the master-key in vault-server.

.. warning:: The rsync tool does not support the ``keyring_vault``. Any rysnc-SST on a joiner is
   aborted if the ``keyring_vault`` is configured.

Configuration
*************

Configuration options are the same as
`upstream
<https://www.percona.com/doc/percona-server/5.7/security/data-at-rest-encryption.html>`__.
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

The detailed description of these options can be found in the `upstream documentation <https://www.percona.com/doc/percona-server/5.7/security/data-at-rest-encryption.html>`_.

Vault-server is an external server, so make sure a PXC node can reach the
server.

.. note:: Percona XtraDB Cluster recommends using the same keyring_plugin on all
   cluster nodes. Mixing keyring plugins is recommended only while transitioning from
   ``keyring_file`` to ``keyring_vault`` or vice-versa.

All nodes do not need to refer to same vault server. Whatever
vault server is used, it should be accessible from the respective node. Also
there is no restriction for all nodes to use the same mount point.

If the node is not able to reach/connect to the vault server, an error is notified
during the server boot, and node refuses to start:

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
pre-existing encrypted object and on reboot, the server fails to connect to
vault-server, the object is not accessible.

In case when vault-server is accessible but authentication credential is incorrect,
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
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring plugin is loaded.

   2018-05-29T04:01:33.774684Z 5 [ERROR] Plugin keyring_vault reported:
   'Could not write key to Vault. Vault has returned the following error(s):
   ["no handler for route
   'secret1/NDhfSU5OT0RCS2V5LTkzNzVmZWQ0LTVjZTQtMTFlOC05YTc3LTM0MDI4NmI4ODhiZS0xMF8='"]'
   2018-05-29T04:01:33.774786Z 5 [ERROR] Plugin keyring_vault reported:
   'Could not flush keys to keyring'

Mixing keyring plugins
=========================

With Percona XtraBackup introducing transition-key logic, it is now possible to
mix and match keyring plugins. For example, the user has node-1 configured to use
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note::

    Percona recommends the same configuration for all the nodes of the
    cluster. A mix and match (in keyring plugins) is recommended only during
    transition from one type of keying to another.

Upgrade and compatibility issues
--------------------------------

Percona XtraDB Cluster server before ``5.7.22`` only supported the ``keyring_file`` and the
dependent Percona XtraBackup did not have the concept of transition-key. This makes the
mix and match of old Percona XtraDB Cluster server (pre-5.7.21) using ``keyring_file`` with new
Percona XtraDB Cluster server (post-5.7.22) using ``keyring_vault`` not possible. A user should
first upgrade Percona XtraDB Cluster server to version 5.7.22 or newer using ``keyring_file``
plugin and then let it act as a DONOR to a new booting ``keyring_vault`` running the
JOINER.

If all the nodes use Percona XtraDB Cluster 5.7.22, then the user can configure
some nodes to use the ``keyring_file`` and other to use the
``keyring_vault``, but this setup is not recommended and should be used
during only during the transition to vault.

If all the nodes are using Percona XtraDB Cluster 5.7.21 and the user would like to use
``keyring_vault`` plugin, all the nodes should be upgraded to use Percona XtraDB Cluster 5.7.22
(that is where vault plugin support was introduced in PXC) or newer. Once all nodes are
configured to use Percona XtraDB Cluster 5.7.22, users can switch one node to use
``vault-plugin``.

.. note::

    MySQL 5.7.21 supports `migration between keystores <https://dev.mysql.com/doc/mysql-security-excerpt/5.7/en/keyring-key-migration.html>`_. Migration requires a restart.



InnoDB tablespace encryption
============================

Percona XtraDB Cluster supports tablespace encryption for the file-per-table tablespace.
File-per-tablespace encryption is a table or tablespace-specific feature and
is enabled through DDL:

.. code-block:: mysql

   CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   CREATE TABLESPACE foo ADD DATAFILE 'foo.ibd' ENCRYPTION='Y';

The PXC cluster replicates the DDL statements and creates the encrypted table or
tablespace on all cluster nodes.

This feature requires a keyring plugin to be loaded before it can be used.
Percona XtraDB Cluster supports two types of keyring
plugin: ``keyring_file`` and ``keyring_vault``.

Temporary file encryption
=========================

Percona Server for MySQL 5.7.22 added support for encrypting temporary file storage
enabled using ``encrypt-tmp-files``. This storage or files are local to the
node and has no direct effect on Percona XtraDB Cluster replication. Percona XtraDB Cluster recommends enabling
it on all the cluster nodes, though the action is not mandatory. The parameter
is the same as in Percona Server:

.. code-block:: text

   [mysqld]
   encrypt-tmp-files=ON



Migrating Keys Between Keyring Keystores
========================================

Percona XtraDB Cluster supports key migration between keystores. The migration can be performed
offline or online.

Offline Migration
-----------------

In offline migration, the node to migrate is shutdown and the migration server
takes care of migrating keys for the said server to a new keystore.

Following example illustrates this scenario:

1. Three Percona XtraDB Cluster nodes n1, n2, n3 - all using ``keyring_file``,
   and n2 should be migrated to use ``keyring_vault``
2. The user shuts down n2 node.
3. The user starts the Migration Server (``mysqld`` with a special option).
4. The Migration Server copies keys from n2 keyring file and adds them to the vault
   server.
5. The user starts n2 node with the vault parameter, and keys should be available.

Here is how the migration server output should look like:

.. code-block:: text

   /dev/shm/pxc57/bin/mysqld --defaults-file=/dev/shm/pxc57/copy_mig.cnf \
   --keyring-migration-source=keyring_file.so \
   --keyring_file_data=/dev/shm/pxc57/node2/keyring \
   --keyring-migration-destination=keyring_vault.so \
   --keyring_vault_config=/dev/shm/pxc57/vault/keyring_vault.cnf &

   2018-05-30T03:44:11.803459Z 0 [Warning] TIMESTAMP with implicit DEFAULT
   value is deprecated. Please use
   --explicit_defaults_for_timestamp server option (see documentation for
   more details).
   2018-05-30T03:44:11.803534Z 0 [Note] --secure-file-priv is set to NULL.
   Operations related to importing and
   exporting data are disabled
   2018-05-30T03:44:11.803550Z 0 [Warning] WSREP: Node is not a cluster node.
   Disabling pxc_strict_mode
   2018-05-30T03:44:11.803564Z 0 [Note] /dev/shm/pxc57/bin/mysqld
   (mysqld 5.7.21-21-29.26-debug) starting as process
   5710 ...
   2018-05-30T03:44:11.805917Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-30T03:44:11.805932Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-30T03:44:11.945989Z 0 [Note] Keyring migration successful.
   2018-05-30T03:44:11.946015Z 0 [Note] Binlog end
   2018-05-30T03:44:11.946047Z 0 [Note] Shutting down plugin 'keyring_vault'
   2018-05-30T03:44:11.946166Z 0 [Note] Shutting down plugin 'keyring_file'
   2018-05-30T03:44:11.947334Z 0 [Note] /dev/shm/pxc57/bin/mysqld: Shutdown complete

The destination keystore receives additional migrated keys
(pre-existing keys in destination keystore are not touched or removed) on successful
migration. The source
keystore continues to retain the keys as migration performs copy operation and
not move operation.

If the migration fails, then the destination keystore is left untouched.

Online Migration
----------------

In online migration, node to migrate is kept running, and the migration server takes
care of migrating keys for the said server to a new keystore by connecting to
the node.

The following example illustrates this scenario:

1. Three Percona XtraDB Cluster nodes n1, n2, n3 - all using ``keyring_file``,
   and n3 should be migrated to use ``keyring_vault``
2. User starts the Migration Server (``mysqld`` with a special option).
3. Migration Server copies keys from the n3 keyring file and adds them to the vault
   server.
4. The user restarts n3 node with the vault parameter, and keys should be available.

Here is how the migration server output should look like:

.. code-block:: text

   /dev/shm/pxc57/bin/mysqld --defaults-file=/dev/shm/pxc57/copy_mig.cnf \
   --keyring-migration-source=keyring_vault.so \
   --keyring_vault_config=/dev/shm/pxc57/keyring_vault3.cnf \
   --keyring-migration-destination=keyring_file.so \
   --keyring_file_data=/dev/shm/pxc57/node3/keyring \
   --keyring-migration-host=localhost \
   --keyring-migration-user=root \
   --keyring-migration-port=16300 \
   --keyring-migration-password='' &

   2018-05-29T14:07:32.789673Z 0 [Warning] TIMESTAMP with implicit DEFAULT value is deprecated. Please use
   --explicit_defaults_for_timestamp server option (see documentation for more details).
   2018-05-29T14:07:32.789748Z 0 [Note] --secure-file-priv is set to NULL. Operations related to importing and
   exporting data are disabled
   2018-05-29T14:07:32.789766Z 0 [Warning] WSREP: Node is not a cluster node. Disabling pxc_strict_mode
   2018-05-29T14:07:32.789780Z 0 [Note] /dev/shm/pxc57/bin/mysqld (mysqld 5.7.21-21-29.26-debug) starting as process
   4936 ...
   2018-05-29T14:07:32.792036Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-29T14:07:32.792052Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-29T14:07:32.927612Z 0 [Note] Keyring migration successful.
   2018-05-29T14:07:32.927636Z 0 [Note] Binlog end
   2018-05-29T14:07:32.927671Z 0 [Note] Shutting down plugin 'keyring_vault'
   2018-05-29T14:07:32.927793Z 0 [Note] Shutting down plugin 'keyring_file'
   2018-05-29T14:07:32.928864Z 0 [Note] /dev/shm/pxc57/bin/mysqld: Shutdown complete

On a successful migration, the destination keystore has the additional migrated keys
(the pre-existing keys in the destination keystore are not touched or removed).
The source
keystore continues to retain the keys as the migration performs copy operation and
not move operation.

If the migration fails, then the destination keystore is left untouched.

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

* ``--keyring-migration-port``: Used for TCP/IP connections, the running server's port  number used to connect.

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
   basedir=/dev/shm/pxc57
   datadir=/dev/shm/pxc57/copy_mig
   log-error=/dev/shm/pxc57/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400
