.. _data_at_rest_encryption:

=======================
Data at Rest Encryption
=======================

<<<<<<< HEAD
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

.. code-block:: guess

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
------------

The following subsection covers some of the important semantics of
``keyring_file`` plugin needed to use it in scope of data-at-rest encryption.

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

If the user has bootstrapped node with keyring enabled, then upcoming nodes of the
cluster will inherit the keyring (encrypted key) from the DONOR node
or generate it.

Usage
*****

|xtrabackup| re-encrypts the data using a transition-key and the JOINER node
re-encrypts it using a new generated master-key.

The keyring is generated on the JOINER node. SST is done using the
``xtrabackup-v2` method. The keyring is sent over explicitly before
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
*************

Keyring (or, more generally, the |PXC| SST process) is backward compatible, as
in higher version JOINER can join from lower version DONOR, but not vice-versa.
More details are covered below, in `Upgrade and compatibility issues`_ section.

Configuring PXC to use keyring_vault plugin
===========================================

keyring_vault
-------------

The ``keyring_vault`` plugin allows storing the master-key in vault-server
(vs. local file as in case of ``keyring_file``).

Configuration
*************

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
||||||| merged common ancestors
<<<<<<<<< Temporary merge branch 1
This variable works in combination with the :variable:`innodb_encrypt_tables`
variable set to ``ONLINE_TO_KEYRING``. This variable configures the number of
threads for background encryption. For the online encryption to work, this
variable must contain a value greater than **zero**.

.. variable:: innodb_online_encryption_rotate_key_age

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-online-encryption-rotate-key-age``
   :dyn: Yes
   :scope: Global
   :vartype: Numeric
   :default: 1

By using this variable, you can re-encrypt the table encrypted using
KEYRING. The value of this variable determines how frequently the encrypted
tables should be encrypted again. If it is set to **1**, the encrypted table is
re-encrypted on each key rotation. If it is set to **2**, the table is encrypted
on every other key rotation.
      
.. variable:: innodb_encrypt_online_alter_logs

   :version 5.7.21-21: Implemented
   :cli: ``--innodb-encrypt-online-alter-logs``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: OFF

This variable simultaneously turns on the encryption of files used by InnoDB for
full text search using parallel sorting, building indexes using merge sort, and
online DDL logs created by InnoDB for online DDL.

.. _data-at-rest-encryption.undo-tablespace:

InnoDB Undo Tablespace Encryption
================================================================================

:Availability: This feature is **Experimental** quality

The encryption of InnoDB Undo tablespaces is only available when using
separate undo tablespaces. Otherwise, the InnoDB undo log is part of
the InnoDB system tablespace.

.. seealso::

   More information about how the encryption of the system tablespace
      :ref:`data-at-rest-encryption.innodb-system-tablespace`

System variables
--------------------------------------------------------------------------------

.. variable:: innodb_undo_log_encrypt

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-undo-log-encrypt``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``Off``

Enables the encryption of InnoDB Undo tablespaces. You can enable encryption and
disable encryption while the server is running. 

.. note:: 

    If you enable undo log encryption, the server writes encryption information
    into the header. That information stays in the header during the life of the
    undo log. If you restart the server, the server will try to load the
    encryption key from the keyring during startup. If the keyring is not available, the server
    cannot start.

Binary log encryption
================================================================================

A new option, implemented since |Percona Server| :rn:`5.7.20-19`, is
encryption of binary and relay logs, triggered by the
:variable:`encrypt_binlog` variable.

Besides turning :variable:`encrypt_binlog` ``ON``, this feature requires both
`master_verify_checksum
<https://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_master_verify_checksum>`_
and `binlog_checksum
<https://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_checksum>`_
variables to be turned ``ON``.

While replicating, master sends the stream of decrypted binary log events to a
slave (SSL connections can be set up to encrypt them in transport). That said,
masters and slaves use separate keyring storages and are free to use differing
keyring plugins.

Dumping of encrypted binary logs involves decryption, and can be done using
``mysqlbinlog`` with ``--read-from-remote-server`` option.

.. note::

   Taking into account that ``--read-from-remote-server`` option  is only
   relevant to binary logs, encrypted relay logs can not be dumped/decrypted
   in this way.

.. rubric:: Upgrading from |Percona Server| |changed-version| to any higher version

The key format in the :ref:`keyring vault plugin
<keyring_vault_plugin>` was changed for binlog encryption in |Percona
Server| |changed-version| release. When you are upgrading from
|Percona Server| 5.7.20-19 to a higher version in the |Percona Server|
5.7 series or to a version prior to 8.0.15-5 in the |Percona Server|
8.0 series, the binary log encryption will work after you complete the
following steps:

1. Upgrade to a version higher than |Percona Server| |changed-version|
#. Start the server without enabling the binary log encryption: :bash:`--encrypt_binlog=OFF`
#. Enforce the key rotation: :mysql:`SELECT rotate_system_key("percona_binlog")`
#. Restart the server enabling the binary log encryption: :bash:`--encrypt_binlog=ON`

.. seealso::

   |Percona Server| Documentation: Important changes in |Percona Server| 8.0.15-5
      - `Binary log encryption to use the upstream implementation
	<https://www.percona.com/doc/percona-server/LATEST/management/data_at_rest_encryption.html#binary-log-encryption>`_

.. |changed-version| replace:: 5.7.20-19

System Variables
----------------

.. variable:: encrypt_binlog

   :version 5.7.20-19: Implemented
   :cli: ``--encrypt-binlog``
   :dyn: No
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

The variable turns on binary and relay logs encryption.

.. _ps.data-at-rest-encryption.redo-log:

Redo Log Encryption
================================================================================

:Availability: This feature is **Experimental** quality

InnoDB redo log encryption is enabled by setting the variable
:variable:`innodb_redo_log_encrypt`. This variable has three values:
``MASTER_KEY``, ``KEYRING_KEY`` and ``OFF`` (set by default).

``MASTER_KEY`` uses the InnoDB master key to encrypt with unique keys for each
log file in the redo log header.

``KEYRING_KEY`` uses the ``percona_redo`` versioned key from the keyring. When
:variable:`innodb_redo_log_encrypt` is set to ``KEYRING_KEY``, each new redo log
file is encrypted with the latest ``percona_redo`` key from the keyring.

System variables
--------------------------------------------------------------------------------

.. variable:: innodb_redo_log_encrypt

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-redo-log-encrypt``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default: ``OFF``

Enables the encryption of the redo log.

.. .. variable:: innodb_key_rotation_interval
.. 	      
..    :version 5.7.23-24: Implemented
..    :cli: ``--innodb-key-rotation_interval``
..    :dyn: Yes
..    :scope: Global
..    :vartype: Text
..    :default: ``0``
.. 
.. This variable stores the time (in seconds) that should pass between key
.. rotations. It is only used if :variable:`innodb_redo_log_encrypt` is set to
.. ``KEYRING_KEY``.
.. 	     

.. _data-at-rest-encryption.variable.innodb-scrub-log:

.. variable:: innodb_scrub_log

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-scrub-log``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

Specifies if data scrubbing should be automatically applied to the redo log.


.. variable:: innodb_scrub_log_speed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-scrub-log-speed``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default: 
 
Specifies the velocity of data scrubbing (writing dummy redo log records) in bytes per second.

Implemented in version 5.7.27-30, the key rotation is redesigned to allow ``SELECT rotate_system_key("percona_redo)``. The currently used key version is available in the :variable:`innodb_redo_key_version` status. The feature is **Experimental**.


Temporary file encryption
=========================

A new feature, implemented since |Percona Server| :rn:`5.7.22-22`, is the
encryption of temporary files, triggered by the :variable:`encrypt-tmp-files`
option.

Temporary files are currently used in |Percona Server| for the following
purposes:

This feature is considered **Experimental** quality.

* filesort (for example, ``SELECT`` statements with ``SQL_BIG_RESULT`` hints),

* binary log transactional caches,

* Group Replication caches.

For each temporary file, an encryption key is generated locally, only kept
in memory for the lifetime of the temporary file, and discarded afterwards.

System Variables
----------------

.. variable:: encrypt-tmp-files

   :version 5.7.22-22: Implemented
   :cli: ``--encrypt-tmp-files``
   :dyn: No
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

The option turns on encryption of temporary files created by |Percona Server|.

.. _data-at-rest-encryption.key-rotation:

Key Rotation
================================================================================

The keyring management is enabled for each tablespace separately when you set
the encryption in the ``ENCRYPTION`` clause, to `KEYRING` in the supported SQL
statement:

- CREATE TABLE .. ENCRYPTION='KEYRING`
- ALTER TABLE ... ENCRYPTION='KEYRING'
- CREATE TABLESPACE tablespace_name … ENCRYPTION=’KEYRING’

.. note::

   Running ``ALTER TABLE .. ENCRYPTION=’Y’`` on the tablespace created with
   ``ENCRYPTION=’KEYRING’`` converts the table back to the existing MySQL
   scheme.

.. _keyring_vault_plugin:

Keyring Vault plugin
====================

In |Percona Server| :rn:`5.7.20-18` a ``keyring_vault`` plugin has been
implemented that can be used to store the encryption keys inside the
`Hashicorp Vault server <https://www.vaultproject.io>`_.

.. important::

   ``keyring_vault`` plugin only works with kv secrets engine version 1.

   .. seealso::

      HashiCorp Documentation: More information about ``kv`` secrets engine
         https://www.vaultproject.io/docs/secrets/kv/kv-v1.html

Installation
------------

The safest way to load the plugin is to do it on the server startup by
using `--early-plugin-load option
<https://dev.mysql.com/doc/refman/5.7/en/server-options.html#option_mysqld_early-plugin-load>`_
option:

.. code-block:: bash

   $ mysqld --early-plugin-load="keyring_vault=keyring_vault.so" \
   --loose-keyring_vault_config="/home/mysql/keyring_vault.conf"

It should be loaded this way to be able to facilitate recovery for encrypted
tables.

.. warning::

   If server should be started with several plugins loaded early,
   ``--early-plugin-load`` should contain their list separated by
   semicolons. Also it's a good practice to put this list in double quotes so
   that semicolons do not create problems when executed in a script.

Apart from installing the plugin you also need to set the
:variable:`keyring_vault_config` variable. This variable should point to the
keyring_vault configuration file, whose contents are discussed below.

This plugin supports the SQL interface for keyring key management described in
`General-Purpose Keyring Key-Management Functions
<https://dev.mysql.com/doc/refman/5.7/en/keyring-udfs-general-purpose.html>`_
manual.

To enable the functions you'll need to install the ``keyring_udf`` plugin:

.. code-block:: mysql

   mysql> INSTALL PLUGIN keyring_udf SONAME 'keyring_udf.so';

Usage
-----

On plugin initialization ``keyring_vault`` connects to the Vault server using
credentials stored in the credentials file. Location of this file is specified
in by :variable:`keyring_vault_config`. On successful initialization it
retrieves keys signatures and stores them inside an in-memory hash map.

Configuration file should contain the following information:

* ``vault_url`` - the address of the server where Vault is running. It can be a
  named address, like one in the following example, or just an IP address. The
  important part is that it should begin with ``https://``.

* ``secret_mount_point`` - the name of the mount point where ``keyring_vault``
  will store keys.

* ``token`` - a token generated by the Vault server, which ``keyring_vault``
  will further use when connecting to the Vault. At minimum, this token should
  be allowed to store new keys in a secret mount point (when ``keyring_vault``
  is used only for transparent data encryption, and not for ``keyring_udf``
  plugin). If ``keyring_udf`` plugin is combined with ``keyring_vault``, this
  token should be also allowed to remove keys from the Vault (for the
  ``keyring_key_remove`` operation supported by the ``keyring_udf`` plugin).

* ``vault_ca [optional]`` - this variable needs to be specified only when the
  Vault's CA certificate is not trusted by the machine that is going to connect
  to the Vault server. In this case this variable should point to CA
  certificate that was used to sign Vault's certificates.

.. warning::
   
   Each ``secret_mount_point`` should be used by only one server - otherwise
   mixing encryption keys from different servers may lead to undefined
   behavior.
  
An example of the configuration file looks like this: ::

  vault_url = https://vault.public.com:8202
  secret_mount_point = secret
  token = 58a20c08-8001-fd5f-5192-7498a48eaf20
  vault_ca = /data/keyring_vault_confs/vault_ca.crt

When a key is fetched from a ``keyring`` for the first time the
``keyring_vault`` communicates with the Vault server, and retrieves the key
type and data. Next it queries the Vault server for the key type and data and
caches it locally.

Key deletion will permanently delete key from the in-memory hash map and the
Vault server.

.. note::

  |Percona XtraBackup| currently doesn't support backup of tables encrypted
  with :ref:`keyring_vault_plugin`.

System Variables
----------------

.. variable:: keyring_vault_config

   :version 5.7.20-18: Implemented
   :cli: ``--keyring-vault-config``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default:

This variable is used to define the location of the
:ref:`keyring_vault_plugin` configuration file.

.. variable:: keyring_vault_timeout

   :version 5.7.21-20: Implemented
   :cli: ``--keyring-vault-timeout``
   :dyn: Yes
   :scope: Global
   :vartype: Numeric
   :default: ``15``

This variable allows to set the duration in seconds for the Vault server
connection timeout. Default value is ``15``. Allowed range is from ``1``
second to ``86400`` seconds (24 hours). The timeout can be also completely
disabled to wait infinite amount of time by setting this variable to ``0``.

.. _data-at-rest-encryption.data-scrubbing:

Data Scrubbing
================================================================================

While data encryption ensures that the existing data are not stored in plain
form, the data scrubbing literally removes the data once the user decides they
should be deleted. Compare this behavior with how the ``DELETE`` statement works
which only marks the affected data as *deleted* - the space claimed by this data
is overwritten with new data later.

Once enabled, data scrubbing works automatically on each tablespace
separately. To enable data scrubbing, you need to set the following variables:

- :variable:`innodb-background-scrub-data-uncompressed`
- :variable:`innodb-background-scrub-data-compressed`

Uncompressed tables can also be scrubbed immediately, independently of key
rotation or background threads. This can be enabled by setting the variable
:variable:`innodb-immediate-scrub-data-uncompressed`. This option is not supported for
compressed tables.

Note that data scrubbing is made effective by setting the
:variable:`innodb_online_encryption_threads` variable to a value greater than
**zero**.

System Variables
--------------------------------------------------------------------------------

.. variable:: innodb_background_scrub_data_compressed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-background-scrub-data-compressed``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

.. variable:: innodb_background_scrub_data_uncompressed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-background-scrub-data-uncompressed``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``
||||||||| merged common ancestors
This variable works in combination with the :variable:`innodb_encrypt_tables`
variable set to ``ONLINE_TO_KEYRING``. This variable configures the number of
threads for background encryption. For the online encryption to work, this
variable must contain a value greater than **zero**.

.. variable:: innodb_online_encryption_rotate_key_age

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-online-encryption-rotate-key-age``
   :dyn: Yes
   :scope: Global
   :vartype: Numeric
   :default: 1

By using this variable, you can re-encrypt the table encrypted using
KEYRING. The value of this variable determines how frequently the encrypted
tables should be encrypted again. If it is set to **1**, the encrypted table is
re-encrypted on each key rotation. If it is set to **2**, the table is encrypted
on every other key rotation.
      
.. variable:: innodb_encrypt_online_alter_logs

   :version 5.7.21-21: Implemented
   :cli: ``--innodb-encrypt-online-alter-logs``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: OFF

This variable simultaneously turns on the encryption of files used by InnoDB for
full text search using parallel sorting, building indexes using merge sort, and
online DDL logs created by InnoDB for online DDL.

.. _data-at-rest-encryption.undo-tablespace:

InnoDB Undo Tablespace Encryption
================================================================================

:Availability: This feature is **Experimental** quality

The encryption of InnoDB Undo tablespaces is only available when using
separate undo tablespaces. Otherwise, the InnoDB undo log is part of
the InnoDB system tablespace.

.. seealso::

   More information about how the encryption of the system tablespace
      :ref:`data-at-rest-encryption.innodb-system-tablespace`

System variables
--------------------------------------------------------------------------------

.. variable:: innodb_undo_log_encrypt

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-undo-log-encrypt``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``Off``

Enables the encryption of InnoDB Undo tablespaces

Binary log encryption
================================================================================

A new option, implemented since |Percona Server| :rn:`5.7.20-19`, is
encryption of binary and relay logs, triggered by the
:variable:`encrypt_binlog` variable.

Besides turning :variable:`encrypt_binlog` ``ON``, this feature requires both
`master_verify_checksum
<https://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_master_verify_checksum>`_
and `binlog_checksum
<https://dev.mysql.com/doc/refman/5.7/en/replication-options-binary-log.html#sysvar_binlog_checksum>`_
variables to be turned ``ON``.

While replicating, master sends the stream of decrypted binary log events to a
slave (SSL connections can be set up to encrypt them in transport). That said,
masters and slaves use separate keyring storages and are free to use differing
keyring plugins.

Dumping of encrypted binary logs involves decryption, and can be done using
``mysqlbinlog`` with ``--read-from-remote-server`` option.

.. note::

   Taking into account that ``--read-from-remote-server`` option  is only
   relevant to binary logs, encrypted relay logs can not be dumped/decrypted
   in this way.

.. rubric:: Upgrading from |Percona Server| |changed-version| to any higher version

The key format in the :ref:`keyring vault plugin
<keyring_vault_plugin>` was changed for binlog encryption in |Percona
Server| |changed-version| release. When you are upgrading from
|Percona Server| 5.7.20-19 to a higher version in the |Percona Server|
5.7 series or to a version prior to 8.0.15-5 in the |Percona Server|
8.0 series, the binary log encryption will work after you complete the
following steps:

1. Upgrade to a version higher than |Percona Server| |changed-version|
#. Start the server without enabling the binary log encryption: :bash:`--encrypt_binlog=OFF`
#. Enforce the key rotation: :mysql:`SELECT rotate_system_key("percona_binlog")`
#. Restart the server enabling the binary log encryption: :bash:`--encrypt_binlog=ON`

.. seealso::

   |Percona Server| Documentation: Important changes in |Percona Server| 8.0.15-5
      - `Binary log encryption to use the upstream implementation
	<https://www.percona.com/doc/percona-server/LATEST/management/data_at_rest_encryption.html#binary-log-encryption>`_

.. |changed-version| replace:: 5.7.20-19

System Variables
----------------

.. variable:: encrypt_binlog

   :version 5.7.20-19: Implemented
   :cli: ``--encrypt-binlog``
   :dyn: No
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

The variable turns on binary and relay logs encryption.

.. _ps.data-at-rest-encryption.redo-log:

Redo Log Encryption
================================================================================

:Availability: This feature is **Experimental** quality

InnoDB redo log encryption is enabled by setting the variable
:variable:`innodb_redo_log_encrypt`. This variable has three values:
``MASTER_KEY``, ``KEYRING_KEY`` and ``OFF`` (set by default).

``MASTER_KEY`` uses the InnoDB master key to encrypt with unique keys for each
log file in the redo log header.

``KEYRING_KEY`` uses the ``percona_redo`` versioned key from the keyring. When
:variable:`innodb_redo_log_encrypt` is set to ``KEYRING_KEY``, each new redo log
file is encrypted with the latest ``percona_redo`` key from the keyring.

System variables
--------------------------------------------------------------------------------

.. variable:: innodb_redo_log_encrypt

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-redo-log-encrypt``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default: ``OFF``

Enables the encryption of the redo log.

.. .. variable:: innodb_key_rotation_interval
.. 	      
..    :version 5.7.23-24: Implemented
..    :cli: ``--innodb-key-rotation_interval``
..    :dyn: Yes
..    :scope: Global
..    :vartype: Text
..    :default: ``0``
.. 
.. This variable stores the time (in seconds) that should pass between key
.. rotations. It is only used if :variable:`innodb_redo_log_encrypt` is set to
.. ``KEYRING_KEY``.
.. 	     

.. _data-at-rest-encryption.variable.innodb-scrub-log:

.. variable:: innodb_scrub_log

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-scrub-log``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

Specifies if data scrubbing should be automatically applied to the redo log.


.. variable:: innodb_scrub_log_speed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-scrub-log-speed``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default: 
 
Specifies the velocity of data scrubbing (writing dummy redo log records) in bytes per second.

Implemented in version 5.7.27-30, the key rotation is redesigned to allow ``SELECT rotate_system_key("percona_redo)``. The currently used key version is available in the :variable:`innodb_redo_key_version` status. The feature is **Experimental**.


Temporary file encryption
=========================

A new feature, implemented since |Percona Server| :rn:`5.7.22-22`, is the
encryption of temporary files, triggered by the :variable:`encrypt-tmp-files`
option.

Temporary files are currently used in |Percona Server| for the following
purposes:

This feature is considered **Experimental** quality.

* filesort (for example, ``SELECT`` statements with ``SQL_BIG_RESULT`` hints),

* binary log transactional caches,

* Group Replication caches.

For each temporary file, an encryption key is generated locally, only kept
in memory for the lifetime of the temporary file, and discarded afterwards.

System Variables
----------------

.. variable:: encrypt-tmp-files

   :version 5.7.22-22: Implemented
   :cli: ``--encrypt-tmp-files``
   :dyn: No
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

The option turns on encryption of temporary files created by |Percona Server|.

.. _data-at-rest-encryption.key-rotation:

Key Rotation
================================================================================

The keyring management is enabled for each tablespace separately when you set
the encryption in the ``ENCRYPTION`` clause, to `KEYRING` in the supported SQL
statement:

- CREATE TABLE .. ENCRYPTION='KEYRING`
- ALTER TABLE ... ENCRYPTION='KEYRING'
- CREATE TABLESPACE tablespace_name … ENCRYPTION=’KEYRING’

.. note::

   Running ``ALTER TABLE .. ENCRYPTION=’Y’`` on the tablespace created with
   ``ENCRYPTION=’KEYRING’`` converts the table back to the existing MySQL
   scheme.

.. _keyring_vault_plugin:

Keyring Vault plugin
====================

In |Percona Server| :rn:`5.7.20-18` a ``keyring_vault`` plugin has been
implemented that can be used to store the encryption keys inside the
`Hashicorp Vault server <https://www.vaultproject.io>`_.

.. important::

   ``keyring_vault`` plugin only works with kv secrets engine version 1.

   .. seealso::

      HashiCorp Documentation: More information about ``kv`` secrets engine
         https://www.vaultproject.io/docs/secrets/kv/kv-v1.html

Installation
------------

The safest way to load the plugin is to do it on the server startup by
using `--early-plugin-load option
<https://dev.mysql.com/doc/refman/5.7/en/server-options.html#option_mysqld_early-plugin-load>`_
option:

.. code-block:: bash

   $ mysqld --early-plugin-load="keyring_vault=keyring_vault.so" \
   --loose-keyring_vault_config="/home/mysql/keyring_vault.conf"

It should be loaded this way to be able to facilitate recovery for encrypted
tables.

.. warning::

   If server should be started with several plugins loaded early,
   ``--early-plugin-load`` should contain their list separated by
   semicolons. Also it's a good practice to put this list in double quotes so
   that semicolons do not create problems when executed in a script.

Apart from installing the plugin you also need to set the
:variable:`keyring_vault_config` variable. This variable should point to the
keyring_vault configuration file, whose contents are discussed below.

This plugin supports the SQL interface for keyring key management described in
`General-Purpose Keyring Key-Management Functions
<https://dev.mysql.com/doc/refman/5.7/en/keyring-udfs-general-purpose.html>`_
manual.

To enable the functions you'll need to install the ``keyring_udf`` plugin:

.. code-block:: mysql

   mysql> INSTALL PLUGIN keyring_udf SONAME 'keyring_udf.so';

Usage
-----

On plugin initialization ``keyring_vault`` connects to the Vault server using
credentials stored in the credentials file. Location of this file is specified
in by :variable:`keyring_vault_config`. On successful initialization it
retrieves keys signatures and stores them inside an in-memory hash map.

Configuration file should contain the following information:

* ``vault_url`` - the address of the server where Vault is running. It can be a
  named address, like one in the following example, or just an IP address. The
  important part is that it should begin with ``https://``.

* ``secret_mount_point`` - the name of the mount point where ``keyring_vault``
  will store keys.

* ``token`` - a token generated by the Vault server, which ``keyring_vault``
  will further use when connecting to the Vault. At minimum, this token should
  be allowed to store new keys in a secret mount point (when ``keyring_vault``
  is used only for transparent data encryption, and not for ``keyring_udf``
  plugin). If ``keyring_udf`` plugin is combined with ``keyring_vault``, this
  token should be also allowed to remove keys from the Vault (for the
  ``keyring_key_remove`` operation supported by the ``keyring_udf`` plugin).

* ``vault_ca [optional]`` - this variable needs to be specified only when the
  Vault's CA certificate is not trusted by the machine that is going to connect
  to the Vault server. In this case this variable should point to CA
  certificate that was used to sign Vault's certificates.

.. warning::
   
   Each ``secret_mount_point`` should be used by only one server - otherwise
   mixing encryption keys from different servers may lead to undefined
   behavior.
  
An example of the configuration file looks like this: ::

  vault_url = https://vault.public.com:8202
  secret_mount_point = secret
  token = 58a20c08-8001-fd5f-5192-7498a48eaf20
  vault_ca = /data/keyring_vault_confs/vault_ca.crt

When a key is fetched from a ``keyring`` for the first time the
``keyring_vault`` communicates with the Vault server, and retrieves the key
type and data. Next it queries the Vault server for the key type and data and
caches it locally.

Key deletion will permanently delete key from the in-memory hash map and the
Vault server.

.. note::

  |Percona XtraBackup| currently doesn't support backup of tables encrypted
  with :ref:`keyring_vault_plugin`.

System Variables
----------------

.. variable:: keyring_vault_config

   :version 5.7.20-18: Implemented
   :cli: ``--keyring-vault-config``
   :dyn: Yes
   :scope: Global
   :vartype: Text
   :default:

This variable is used to define the location of the
:ref:`keyring_vault_plugin` configuration file.

.. variable:: keyring_vault_timeout

   :version 5.7.21-20: Implemented
   :cli: ``--keyring-vault-timeout``
   :dyn: Yes
   :scope: Global
   :vartype: Numeric
   :default: ``15``

This variable allows to set the duration in seconds for the Vault server
connection timeout. Default value is ``15``. Allowed range is from ``1``
second to ``86400`` seconds (24 hours). The timeout can be also completely
disabled to wait infinite amount of time by setting this variable to ``0``.

.. _data-at-rest-encryption.data-scrubbing:

Data Scrubbing
================================================================================

While data encryption ensures that the existing data are not stored in plain
form, the data scrubbing literally removes the data once the user decides they
should be deleted. Compare this behavior with how the ``DELETE`` statement works
which only marks the affected data as *deleted* - the space claimed by this data
is overwritten with new data later.

Once enabled, data scrubbing works automatically on each tablespace
separately. To enable data scrubbing, you need to set the following variables:

- :variable:`innodb-background-scrub-data-uncompressed`
- :variable:`innodb-background-scrub-data-compressed`

Uncompressed tables can also be scrubbed immediately, independently of key
rotation or background threads. This can be enabled by setting the variable
:variable:`innodb-immediate-scrub-data-uncompressed`. This option is not supported for
compressed tables.

Note that data scrubbing is made effective by setting the
:variable:`innodb_online_encryption_threads` variable to a value greater than
**zero**.

System Variables
--------------------------------------------------------------------------------

.. variable:: innodb_background_scrub_data_compressed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-background-scrub-data-compressed``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``

.. variable:: innodb_background_scrub_data_uncompressed

   :version 5.7.23-24: Implemented
   :cli: ``--innodb-background-scrub-data-uncompressed``
   :dyn: Yes
   :scope: Global
   :vartype: Boolean
   :default: ``OFF``
=========
==============================================================================
Data at Rest Encryption
==============================================================================
>>>>>>>>> Temporary merge branch 2
=======
==============================================================================
Data at Rest Encryption
==============================================================================
>>>>>>> Percona-Server-8.0.19-10

.. code-block:: text

   [mysqld]
   basedir=/dev/shm/pxc80
   datadir=/dev/shm/pxc80/copy_mig
   log-error=/dev/shm/pxc80/logs/copy_mig.err
   socket=/tmp/copy_mig.sock
   port=16400

.. include:: ../.res/replace.txt
