.. _data_at_rest_encryption:

=======================
Data at Rest Encryption
=======================

This feature is considered **BETA** quality.

.. contents::
   :local:

.. _innodb_general_tablespace_encryption:

Introduction
============

"Data-at-rest" term refers to all the data stored on disk by some server within
system tablespace, general tablespace, redo-logs, undo-logs, etc. As an
opposite, "data-in-transit" means data transmitted to other node or client.
Data-in-transit can be encrypted using SSL connection (details are available in
the `encrypt traffic documentation <https://www.percona.com/doc/percona-xtradb-cluster/LATEST/security/encrypt-traffic.html>`_) and
therefore supposed to be safe. Below sections are about securing the
data-at-rest only. 

Currently data-at-rest encryption is supported in |PXC| for general tablespace,
file-per-tablespace, and temporary tables.

InnoDB tablespace encryption
============================

|MySQL| supports tablespace encryption, but only for file-per-table tablespace.
User should create a table that has its own dedicated tablespace, making this
tablespace encrypted by specifying the appropriate option.

Percona Server starting from :rn:`5.7.21-20` is extending support for
encrypting `other tablespaces <https://www.percona.com/doc/percona-server/LATEST/management/data_at_rest_encryption.html>`_ too.

|PXC| already supported data-at-rest encryption starting from ``5.7``.
File-per-tablespace and General tablespace encryption are table/tablespace
specific features and are enabled on object level through DDL:

.. code-block:: guess

   CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION=’Y’;
   CREATE TABLESPACE foo ADD DATAFILE 'foo.ibd' ENCRYPTION='Y';

DDL statements are replicated in PXC cluster, thus creating encrypted table or
tablespace on all the nodes of the cluster.

This feature requires a keyring plugin to be loaded before it can be used.
Currently Percona Server (and in turn |PXC|) supports 2 types of keyring
plugin: ``keyring_file`` and ``keyring_vault``.

Configuring PXC to use keyring_file plugin
==========================================

keyring_file
------------

Support for the keyring file was added back when |PXC| 5.7 got General
Availability (GA) status. Following subsection covers some of the important
semantics of ``keyring_file`` plugin needed to use it in scope of data-at-rest
encryption.

``keyring_file`` stores encryption key to a physical file. Location of this
file is specified by ``keyring_file_data`` parameter configured during startup.

Configuration
*************

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

If user has bootstrapped node with keyring enabled, then upcoming nodes of the
cluster will inherit the keyring (encrypted key) from the DONOR node
(in |PXC| prior to 5.7.22) or generate it (starting from |PXC| 5.7.22).

Usage
*****

Prior to |PXC| :rn:`5.7.22-29.26` DONOR node had to send keyring to JOINER,
because |xtrabackup| backs up encrypted tablespaces in encrypted fashion and in
order for JOINER to read these encrypted tablespaces it needs the same
encryption key that was used for its encryption on DONOR. This restriction has
been relaxed in |PXC| 5.7.22 and now |xtrabackup| re-encrypts the data using
transition-key and JOINER re-encrypts it using a new generated master-key.

Keyring is sent from DONOR to JOINER as part of SST process (prior to |PXC|
5.7.22) or generated on JOINER. SST can be done
using xtrabackup (the recommended way), rsync, or mysqldump. In *xtrabackup*
case, keyring is sent over explicitly before the real data backup/streaming
starts. Other two SST variants behave differently: mysqldump uses logical
backup so it doesn’t need to send keyring, while rsync will sync the keys when
it syncs data directories. 

.. warning:: rsync doesn’t provide a secure channel. This means keyring sent
   using rsync SST could be vulnerable to attack. As an opposite, following the
   recommended SST way with xtrabackup user can configure secure channel and so
   keyring is fully secured (in fact, xtrabackup will not allow user to send
   the keyring if the SST channel is not secured). 

.. warning Percona doesn't recommend rsync-based SST for data-at-rest
   encryption using keyring.

|PXC| doesn't allow to combine nodes with encryption and nodes without 
encryption. This is not allowed in order to maintain data consistency. For
example, user creates node-1 with encryption (keyring) enabled and node-2
with encryption (keyring) disabled. Now if user tries to create a table with
encryption on node-1, it will fail on node-2 causing data inconsistency.
With |PXC| :rn:`5.7.22-29.26`, node will fail to start if it fails to load
keyring plugin. 

.. note:: If user hasn’t specifiy keyring parameters there is no way for node
   to know that it needs to load keyring. JOINER node may start but eventually
   shutdown when DML level inconsistency with encrypted tablespace will be
   detected.

If a node doesn’t have encrypted tablespace, keyring is not generated and
the keyring file is empty. Actual keyring is generated only when node starts
using encrypted tablespace.

User can rotate the key as and when needed. 
``ALTER INSTANCE ROTATE INNODB MASTER KEY`` statement is not replicated on
cluster, so it is local operation for the said node.

Starting from |PXC| 5.7.22 JOINER generates its own keyring. In |PXC| prior to
5.7.22 when JOINER joined the cluster its keyring was the same as DONOR’s one.
User could rotate the key if different keys for each of the node where demanded
by the user’s requirements (internal rules). But using different keys for each
node is not necessary from the technical side, as all nodes of the cluster can
continue operating with same MASTER-key.

Compatibility
*************

Keyring (or, more generally, the |PXC| SST process) is backward compatible, as
in higher version JOINER can join from lower version DONOR, but not vice-versa.
More details are covered below, in `Upgrade and compatibility issues`_ section.

.. note:: Since |PXC| 5.6 didn't have encrypted tablespace, no major
   upgrade scenario for data-at-rest encryption is possible from it.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin is supported starting from PXC 5.7.22. This plugin
allows storing the master-key in vault-server (vs. local file as in case of
``keyring_file``). 

.. warning:: rsync doesn't support ``keyring_vault``, and SST on JOINER is
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

   2018-05-29T03:54:33.859613Z 0 [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. Please make sure that Vault's CA certificate is trusted by the machine from which you intend to connect to Vault.'
   2018-05-29T03:54:33.977145Z 0 [ERROR] Plugin keyring_vault reported: 'CURL returned this error code: 7 with error message : Failed to connect to 127.0.0.1 port 8200: Connection refused'

If some nodes of the cluster are unable to connect to vault-server, this
relates only to these specific nodes: e.g. if node-1 is able to connect, and
node-2 is not, only node-2 will refuse to start. Also, if server has
pre-existing encrypted object and on reboot server fails to connect to
vault-server, the object is not accessible.

In case when vault-server is accessible but authentication credential are wrong, 
consequences are the same, and the corresponding error looks like following:

.. code-block:: text

   2018-05-29T03:58:54.461911Z 0 [Warning] Plugin keyring_vault reported: 'There is no vault_ca specified in keyring_vault's configuration file. Please make sure that Vault's CA certificate is trusted by the machine from which you intend to connect to Vault.'
   2018-05-29T03:58:54.577477Z 0 [ERROR] Plugin keyring_vault reported: 'Could not retrieve list of keys from Vault. Vault has returned the following error(s): ["permission denied"]'

In case of accessible vault-server with the wrong mount point, there is no
error during server boot, but sitll node refuses to start:

.. code-block:: text

   mysql> CREATE TABLE t1 (c1 INT, PRIMARY KEY pk(c1)) ENCRYPTION='Y';
   ERROR 3185 (HY000): Can't find master key from keyring, please check keyring plugin is loaded.

   2018-05-29T04:01:33.774684Z 5 [ERROR] Plugin keyring_vault reported: 'Could not write key to Vault. Vault has returned the following error(s): ["no handler for route 'secret1/NDhfSU5OT0RCS2V5LTkzNzVmZWQ0LTVjZTQtMTFlOC05YTc3LTM0MDI4NmI4ODhiZS0xMF8='"]'
   2018-05-29T04:01:33.774786Z 5 [ERROR] Plugin keyring_vault reported: 'Could not flush keys to keyring'

Mix-match keyring plugins
=========================

With |xtrabackup| introducing transition-key logic it is now possible to
mix-match keyring plugins. For example, user has node-1 configured to use
``keyring_file`` plugin and node-2 configured to use ``keyring_vault``.

.. note:: Percona recommends same configuration for all the nodes of the
   cluster. Mix-match (in keyring plugins) is recommended only during
   transition from one keying to other.

Upgrade and compatibility issues
--------------------------------

|PXC| server before ``5.7.22`` only supported ``keyring_file`` and the
dependent |xtrabackup| didn’t had concept of transition-key then. This makes
mix-match of old |PXC| server (pre-5.7.21) using ``keyring_file`` with new
|PXC| server (post-5.7.22) using ``keyring_vault`` not possible. User should
first upgrade |PXC| server to version 5.7.22 or newer using ``keyring_file``
plugin and then let it act as DONOR to new booting ``keyring_vault`` running
JOINER.

If all the nodes are using |PXC| 5.7.22, then user can freely
configure some nodes to use ``keyring_file`` and other to use
``keyring_vault``, but still this setup is not recommended and should be used
during transitioning to vault only.

If all the nodes are using |PXC| 5.7.21 and user would like to move to use
``keyring_vault`` plugin, all the nodes should be upgraded to use |PXC| 5.7.22
(that’s where vault plugin support was introduced in PXC). Once all nodes are
configured to use |PXC| 5.7.22, user can switch one node at a time to use
``vault-plugin``.

.. note:: |MySQL| 5.7.21 has support for `migration between keystores <https://dev.mysql.com/doc/mysql-security-excerpt/5.7/en/keyring-key-migration.html>`_. Although a restart is required.

Temporary file encryption
=========================

Percona Server 5.7.22 added support for encrypting temporary file storage
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

On successful migration, destination keystore will get additional migrated keys
(pre-existing keys in destination keystore are not touched or removed). Source
keystore continues to retain the keys as migration performs copy operation and
not move operation.

If migration fails, then destination keystore will be left untouched.

Online Migration
----------------

In online migration, node to migrate is kept running and migration server takes
care of migrating keys for the said server to a new keystore by connecting to
the node.

Following example illustrates this scenario:

1. Let's say there are 3 |PXC| nodes n1, n2, n3 - all using ``keyring_file``, 
   and n3 should be migrated to use ``keyring_vault``
2. User start's Migration Server (``mysqld`` with a special option).
3. Migration Server copies keys from n3 keyring file and adds them to the vault
   server.
4. User restarts n3 node with the vault parameter, and keys should be available.

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

On successful migration, destination keystore will get additional migrated keys
(pre-existing keys in destination keystore are not touched or removed). Source
keystore continues to retain the keys as migration performs copy operation and
not move operation. 

If migration fails, then destination keystore will be left untouched.

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
   basedir=/dev/shm/pxc57
   datadir=/dev/shm/pxc57/copy_mig
   log-error=/dev/shm/pxc57/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400
