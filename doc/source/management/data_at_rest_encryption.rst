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

|PXC| |version| supports all |data-at-rest| generally-available encryption features available from |percona-server| 8.0.

.. seealso::

   |percona-server| Documentation: Transparent Data Encryption
      https://www.percona.com/doc/percona-server/8.0/security/data-at-rest-encryption.html#data-at-rest-encryption

The "data-at-rest" term refers to all the data stored on disk by some server within system tablespace, general tablespace, redo-logs, undo-logs, etc. As the opposite, "data-in-transit" means data transmitted to another node or client.
Data-in-transit can be encrypted using SSL connections (details are available in the `encrypt traffic documentation <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html>`_).

This feature requires a keyring plugin to be loaded before it can be used.
Currently, Percona Server (and in turn |PXC|) supports two types of keyring
plugin: ``keyring_file`` and ``keyring_vault``.

Configuring PXC to use keyring_file plugin
==========================================
keyring_file
--------------------------------------------------------------------------------

The following subsection covers some of the important semantics of
``keyring_file`` plugin needed to use it in scope of data-at-rest encryption.

``keyring_file`` stores encryption key to a physical file. The location of this
file is specified by the ``keyring_file_data`` parameter configured during startup.

Configuration
--------------------------------------------------------------------------------

|PXC| inherits upstream (Percona Server) behavior to configure ``keyring_file``
plugin. The following options are set in the configuration file:

.. code-block:: text

   [mysqld]
   early-plugin-load=keyring_file.so
   keyring_file_data=<PATH>/keyring

A ``SHOW PLUGINS`` statement can be used further to check if the plugin has been
successfully loaded.

.. note:: PXC recommends the same configuration on all the nodes of the cluster,
   and that also means all the nodes of the cluster should have keyring
   configured. Mismatch in keyring configuration will not allow the joiner node to
   join the cluster.

If the user has bootstrapped node with keyring enabled, then upcoming nodes of the
cluster will inherit the keyring (encrypted key) from the DONOR node
or generate it.

Usage
--------------------------------------------------------------------------------

|xtrabackup| re-encrypts the data using a transition-key and the JOINER node
re-encrypts it using a newly generated master-key.

The keyring is generated on the JOINER node. SST is done using the
``xtrabackup-v2`` method. The keyring is sent over explicitly before
the real data backup/streaming starts.

|PXC| doesn't allow you to combine nodes with encryption and nodes without encryption. This combination is not allowed to maintain data consistency. For
example, the user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. If the user tries to create a table with
encryption on node-1, it will fail on node-2, causing data inconsistency.
A node fails to start if it fails to load the keyring plugin. 

.. note:: If the user does not specify the keyring parameters, the node does not know that it must load the keyring. The JOINER node may start, but it eventually shuts down when the DML level inconsistency with encrypted tablespace is detected.

If a node does not have an encrypted tablespace, the keyring is not generated and the keyring file is empty. The actual keyring is generated only when the node starts using encrypted tablespace.

The user can rotate the key as needed. This operation is local to the
node. The ``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is
not replicated on cluster.

The JOINER node generates its own keyring. 

Compatibility
--------------------------------------------------------------------------------

Keyring (or, more generally, the |PXC| SST process) is backward compatible, a higher version JOINER can join from lower version DONOR, but not vice-versa.
More details are covered below, in
:ref:`data-at-rest-encryption-upgrade-compatibility-issues` section.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin allows storing the master-key in vault-server
(vs. local file as in case of ``keyring_file``).

.. warning:: rsync does not support ``keyring_vault``, and SST on a joiner is
   aborted if rsync is used on the node with ``keyring_vault`` configured.

Configuration
--------------------------------------------------------------------------------

Configuration options are same as `upstream
<https://www.percona.com/doc/percona-server/8.0/security/using-keyring-plugin.html>`_. The
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

Detailed description of these options can be found in the `upstream documentation <https://www.percona.com/doc/percona-server/8.0/security/using-keyring-plugin.html>`_.

Vault-server is an external server, make sure the PXC node can reach the said
server.

.. note:: |PXC| recommends using the same keyring_plugin on all the nodes of the
   cluster. Mix-match is recommended to use it only while transitioning from
   ``keyring_file`` -> ``keyring_vault`` or vice-versa.

It is not necessary that all the nodes refer to the same vault server, but ensure the vault server is accessible from the respective node. If a node cannot reach/connect to a vault server, an error is raised during the server boot, and node refuses to start:

.. code-block:: text

   ... [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. ...
   ... [ERROR] Plugin keyring_vault reported: 'CURL returned this error code: 7 with error message : ...

If any nodes of the cluster cannot connect to the vault-server, users cannot access the encrypted data on that node.

If the vault-server is accessible but the authentication credential is wrong,
the consequences are the same. The corresponding error message looks like the following:

.. code-block:: text

   ... [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. ...
   ... [ERROR] Plugin keyring_vault reported: 'Could not retrieve list of keys from Vault. ...

All nodes are not required to use the same mount point. In the case of an accessible vault-server with the wrong mount point, there is no
error during server boot, but node refuses to start:

.. code-block:: text

   mysql> CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring plugin is loaded.

   ... [ERROR] Plugin keyring_vault reported: 'Could not write key to Vault. ...
   ... [ERROR] Plugin keyring_vault reported: 'Could not flush keys to keyring'

Mix-match keyring plugins
=========================

With |xtrabackup| introducing transition-key logic, it is now possible to
mix-match keyring plugins. For example, the user has node-1 configured to use
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note:: Percona recommends using the same configuration for all the nodes of the
   cluster. Mix-match (in keyring plugins) is recommended only during the
   transition from one keying method to another.

.. _data-at-rest-encryption-upgrade-compatibility-issues:

Upgrade and compatibility issues
--------------------------------

If all the nodes are using |PXC| 8.0, then the user can freely
configure some nodes to use ``keyring_file`` and other to use
``keyring_vault``. This setup, however, is not recommended and should be used
during transitioning to vault only.

Temporary file encryption
==========================

Percona Server supports the encryption of temporary file storage which is enabled using ``encrypt-tmp-files``. The storage or files are local to the node and have no effect on |PXC| replication. |PXC| recommends enabling the variable on all nodes of the cluster, although this action is not mandatory. The parameter to enable this option is the same as Percona Server:

.. code-block:: text

    [mysqld]
    encrypt-tmp-files=ON

Migrating Keys Between Keyring Keystores
========================================

|PXC| supports key migration between keystores. The migration can be performed
offline or online.

Offline Migration
-----------------

In offline migration, the node to migrate is shut down, and the migration server
takes care of migrating keys for the said server to a new keystore.

Following example illustrates this scenario:

1. Three |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
   and n2 should be migrated to use ``keyring_vault``
2. The user shuts down the n2 node.
3. The user starts the Migration Server (``mysqld`` with a special option).
4. Migration Server copies keys from the n2 keyring file and adds them to the vault
   server.
5. The User starts n2 node with the vault parameter, and keys should be available.

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

On successful migration, destination keystore receives additional migrated keys
(pre-existing keys in the destination keystore are not touched or removed). The source
keystore retains the keys as migration performs copy operation and
not move operation.

If the migration fails, then destination keystore is left untouched.

Online Migration
----------------

In online migration, node to migrate is kept running and migration server takes
care of migrating keys for the said server to a new keystore by connecting to
the node.

The following example illustrates this scenario:

1. Three |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
   and n3 should be migrated to use ``keyring_vault``
2. User starts the Migration Server (``mysqld`` with a special option).
3. Migration Server copies keys from the n3 keyring file and adds them to the vault
   server.
4. The user restarts n3 node with the vault parameter, and keys should be available.

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

   
On a successful migration, the destination keystore receives additional migrated keys
(pre-existing keys in destination keystore are not touched or removed). The source
keystore retains the keys as the migration performs copy operation and
not move operation. 

If the migration fails, then the destination keystore will be left untouched.

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
   basedir=/dev/shm/pxc80
   datadir=/dev/shm/pxc80/copy_mig
   log-error=/dev/shm/pxc80/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400

.. include:: ../.res/replace.txt
