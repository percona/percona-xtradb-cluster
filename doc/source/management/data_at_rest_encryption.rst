.. _data_at_rest_encryption:

=======================
Data at Rest Encryption
=======================

.. contents::
   :local:

.. _innodb_general_tablespace_encryption:

Introduction
============

The "data-at-rest" term refers to all the data stored on disk by some server within system tablespace, general tablespace, redo-logs, undo-logs, etc. As the opposite, "data-in-transit" means data transmitted to another node or client.
Data-in-transit can be encrypted using SSL connections (details are available in the `encrypt traffic documentation <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html>`_).

This feature requires a keyring plugin to be loaded before it can be used.
Currently, Percona Server (and in turn |PXC|) supports two types of keyring
plugin: ``keyring_file`` and ``keyring_vault``.

Configuring PXC to use keyring_file plugin
==========================================

keyring_file
------------

Support for the keyring file was added back when |PXC| 5.7 got General
Availability (GA) status. The following subsection covers some of the important
semantics of ``keyring_file`` plugin needed to use it in the scope of data-at-rest
encryption.

``keyring_file`` stores encryption key to a physical file. The location of this
file is specified by the ``keyring_file_data`` parameter configured during startup.

Configuration
*************

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

If the user has bootstrapped node with the keyring enabled, then the upcoming nodes of the
cluster either inherits the keyring (the encrypted key) from the donor node
(in |PXC| prior to 5.7.22) or generates it (starting from |PXC| 5.7.22).

Compatibility
*************

Keyring (or, more generally, the |PXC| SST process) is backward compatible, as
in higher version joiner can join from lower version donor, but not vice-versa.
More details are covered below in `Upgrade and compatibility issues`_ section.

.. note:: Since |PXC| 5.6 does not have encrypted tablespaces, no major
   upgrade scenario for data-at-rest encryption is possible.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin is supported starting from PXC 5.7.22. This plugin
allows storing the master-key in vault-server (vs. local file as in the
``keyring_file``).

.. warning:: rsync does not support ``keyring_vault``, and SST on a joiner is
   aborted if rsync is used on the node with ``keyring_vault`` configured.

Configuration
*************

Configuration options are same as `upstream <https://www.percona.com/doc/percona-server/5.7/management/data_at_rest_encryption.html#keyring-vault-plugin>`_. The ``my.cnf`` configuration file should contain following options:

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

Detailed description of these options can be found in the `upstream documentation <https://www.percona.com/doc/percona-server/5.7/management/data_at_rest_encryption.html#keyring-vault-plugin>`_.

Vault-server is an external server, make sure the PXC node can reach the said
server.

.. note:: |PXC| recommends using the same keyring_plugin on all the nodes of the
   cluster. Mix-match is recommended to use it only while transitioning from
   ``keyring_file`` -> ``keyring_vault`` or vice-versa.

It is not necessary that all the nodes refer to the same vault server, but ensure the vault server is accessible from the respective node. If a node cannot reach/connect to a vault server, an error is raised during the server boot, and node refuses to start:

.. code-block:: text

   2018-05-29T03:54:33.859613Z 0 [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. Please make sure that Vault's CA certificate is trusted by the machine from which you intend to connect to Vault.'
   2018-05-29T03:54:33.977145Z 0 [ERROR] Plugin keyring_vault reported: 'CURL returned this error code: 7 with error message : Failed to connect to 127.0.0.1 port 8200: Connection refused'

If any nodes of the cluster cannot connect to the vault-server, users cannot access the encrypted data on that node.

If the vault-server is accessible but the authentication credential is wrong,
the consequences are the same. The corresponding error message looks like the following:

.. code-block:: text

   2018-05-29T03:58:54.461911Z 0 [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. Please make sure that Vault's CA certificate is trusted by the machine from which you intend to connect to Vault.'
   2018-05-29T03:58:54.577477Z 0 [ERROR] Plugin keyring_vault reported: 'Could not retrieve list of keys from Vault. Vault has returned the following error(s): ["permission denied"]'

All nodes are not required to use the same mount point. In the case of an accessible vault-server with the wrong mount point, there is no
error during server boot, but node refuses to start:

.. code-block:: text

   mysql> CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring plugin is loaded.

   2018-05-29T04:01:33.774684Z 5 [ERROR] Plugin keyring_vault reported: 'Could not write key to Vault. Vault has returned the following error(s): ["no handler for route 'secret1/NDhfSU5OT0RCS2V5LTkzNzVmZWQ0LTVjZTQtMTFlOC05YTc3LTM0MDI4NmI4ODhiZS0xMF8='"]'
   2018-05-29T04:01:33.774786Z 5 [ERROR] Plugin keyring_vault reported: 'Could not flush keys to keyring'

Mix-match keyring plugins
=========================

With |xtrabackup| introducing transition-key logic, it is now possible to
mix-match keyring plugins. For example, the user has node-1 configured to use
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note:: Percona recommends using the same configuration for all the nodes of the
   cluster. Mix-match (in keyring plugins) is recommended only during the
   transition from one keying method to another.


Upgrade and compatibility issues
--------------------------------

|PXC| server before ``5.7.22`` only supported the ``keyring_file``, and the
dependent |xtrabackup| had not implemented the transition-key method at that time. This change makes the mix-match of the old |PXC| server (pre-5.7.21) using ``keyring_file`` with the more recent |PXC| server (post-5.7.22) using ``keyring_vault`` not possible. Users should first upgrade |PXC| server to version 5.7.22 or newer using the``keyring_file``
plugin and then let it act as a donor to new booting ``keyring_vault`` running
joiner.

If all the nodes are using |PXC| 5.7.22, the user can freely
configure some nodes to use ``keyring_file`` and others to use
``keyring_vault``, but this configuration is not recommended. It should only be used
during transitioning to ``keyring_vault``.

.. note::

    A 5.7.21 DONOR with a keyring_file and a JOINER >= 5.7.22 with
    a keyring_file can be used to perform a rolling upgrade of the cluster.
    XtraBackup SST does not work between 5.7.21 and 5.7.22 with the
    keyring_file plugin. If
    there are issues, change the SST method to rsync.
    
If all the nodes are using |PXC| 5.7.21 and user would like to move to use
``keyring_vault`` plugin, all the nodes should be upgraded to use |PXC| 5.7.22
(that is where vault plugin support was introduced in PXC) or newer. Once all nodes are
configured to use |PXC| 5.7.22, users can switch one node to use
``vault-plugin``.

.. note:: |MySQL| 5.7.21 has support for `migration between keystores <https://dev.mysql.com/doc/mysql-security-excerpt/5.7/en/keyring-key-migration.html>`_. Although a restart is required.

|PXC| currently supports data-at-rest encryption for file-per-tablespace and temporary files.

InnoDB tablespace encryption
============================

|MySQL| supports tablespace encryption, but only for the file-per-table tablespace.
The user should create a table with a dedicated tablespace, making this
tablespace encrypted by specifying the appropriate option.

Percona Server starting from :rn:`5.7.21-20` is extending support for
encrypting `other tablespaces <https://www.percona.com/doc/percona-server/LATEST/management/data_at_rest_encryption.html>`_ too.

|PXC| already supported data-at-rest encryption starting from ``5.7``.
File-per-tablespace and general tablespace encryption are table/tablespace
specific features and are enabled on the object level through DDL:

.. code-block:: guess

   CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION=’Y’;
   CREATE TABLESPACE foo ADD DATAFILE 'foo.ibd' ENCRYPTION='Y';

DDL statements are replicated in PXC cluster, thus creating an encrypted table or
tablespace on all the nodes of the cluster.

Temporary file encryption
=========================

Percona Server 5.7.22 added support for the encryption of temporary file storage. Users enable the encryption with ``encrypt-tmp_files``. The storage and files are local to the node and have no direct effect on |PXC| replication. |PXC| recommends enabling this variable on all nodes of the cluster, though this setting is not mandatory. The parameter to enable this operation is the following:

..  code-block:: text

    [mysqld]
    encrypt-tmp-files=ON

Usage
*****

Prior to |PXC| :rn:`5.7.22-29.26`, the donor node had to send keyring to a joiner,
because |xtrabackup| backs up encrypted tablespaces in an encrypted fashion and for joiner node to read these encrypted tablespaces it needs the same
encryption key used for its encryption on the donor. This restriction has
been relaxed in |PXC| 5.7.22. Now |xtrabackup| re-encrypts the data using a
transition-key and the joiner node re-encrypts it using a newly generated master-key.

The keyring is sent from donor to joiner as part of the SST process (prior to |PXC|
5.7.22) or generated on the joiner. SST can be done
using xtrabackup (the recommended way), rsync, or mysqldump. In the *xtrabackup*
case, the keyring is explicitly sent before the real data backup/streaming
starts. The other two SST variants behave differently: mysqldump uses logical
backup so it does not need to send the keyring, while rsync will sync the keys when
it syncs data directories. 

.. warning:: rsync does not provide a secure channel. The keyring sent by
   using rsync SST could be vulnerable to attack. As opposed to following the
   recommended SST way with xtrabackup the user can configure a secure channel. So,
   the keyring is fully secured (in fact, xtrabackup will not allow a user to send
   the keyring if the SST channel is not secured). 

.. warning Percona does not recommend rsync-based SST for data-at-rest
   encryption using keyring.

|PXC| does not allow the combination of nodes with encryption and nodes without 
encryption. This behavior is not permitted to maintain data consistency. For
example, the user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. If the user attempts to create a table with
encryption on node-1, it will fail on node-2, causing data inconsistency.
With |PXC| :rn:`5.7.22-29.26`, the node will fail to start if it fails to load the
keyring plugin. 

.. note:: If the user has not specified the keyring parameters, there is no way for a node
   to know that it needs to load keyring. A joiner node may start but eventually
   shutdown when DML level inconsistency with encrypted tablespace is
   detected.

The actual keyring is generated only when the node starts using encrypted tablespace. If a node does not have an encrypted tablespace, it is not generated and the keyring file is empty. 

Users can rotate the key as and when needed. 
``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is not replicated on the
cluster, so it is a local operation for the said node.

Starting from |PXC| 5.7.22, the joiner generates its keyring. In |PXC| before
5.7.22, when the joiner joined the cluster, its keyring was the same as the donor.
The users could rotate the key if different keys for each node were demanded
by requirements (internal rules). Using different keys for each
node is not necessary from the technical side, as all nodes of the cluster can
continue operating with the same MASTER-key.



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

   /dev/shm/pxc57/bin/mysqld --defaults-file=/dev/shm/pxc57/copy_mig.cnf \
   --keyring-migration-source=keyring_file.so \
   --keyring_file_data=/dev/shm/pxc57/node2/keyring \
   --keyring-migration-destination=keyring_vault.so \
   --keyring_vault_config=/dev/shm/pxc57/vault/keyring_vault.cnf &

   2018-05-30T03:44:11.803459Z 0 [Warning] TIMESTAMP with implicit DEFAULT value is deprecated. Please use
   --explicit_defaults_for_timestamp server option (see documentation for more details).
   2018-05-30T03:44:11.803534Z 0 [Note] --secure-file-priv is set to NULL. Operations related to importing and
   exporting data are disabled
   2018-05-30T03:44:11.803550Z 0 [Warning] WSREP: Node is not a cluster node. Disabling pxc_strict_mode
   2018-05-30T03:44:11.803564Z 0 [Note] /dev/shm/pxc57/bin/mysqld (mysqld 5.7.21-21-29.26-debug) starting as process
   5710 ...
   2018-05-30T03:44:11.805917Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-30T03:44:11.805932Z 0 [Warning] Can't create test file /dev/shm/pxc57/copy_mig/qaserver-06.lower-test
   2018-05-30T03:44:11.945989Z 0 [Note] Keyring migration successful.
   2018-05-30T03:44:11.946015Z 0 [Note] Binlog end
   2018-05-30T03:44:11.946047Z 0 [Note] Shutting down plugin 'keyring_vault'
   2018-05-30T03:44:11.946166Z 0 [Note] Shutting down plugin 'keyring_file'
   2018-05-30T03:44:11.947334Z 0 [Note] /dev/shm/pxc57/bin/mysqld: Shutdown complete

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

Following example illustrates this scenario:

1. Three |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
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
   basedir=/dev/shm/pxc57
   datadir=/dev/shm/pxc57/copy_mig
   log-error=/dev/shm/pxc57/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400
