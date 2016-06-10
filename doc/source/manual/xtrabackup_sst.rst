.. _xtrabackup_sst:

====================================
Percona XtraBackup SST Configuration
====================================

Percona XtraBackup SST works in two stages:

* First it identifies the type of data transfer based on the presence of :file:`xtrabackup_ist` file on the joiner node.

* Then it starts data transfer:

  * In case of |SST|, it empties the data directory except for some files (:file:`galera.cache`, :file:`sst_in_progress`, :file:`grastate.dat`) and then proceeds with the SST

  * In case of |IST|, it proceeds as before.

.. note:: To maintain compatibility with |Percona XtraDB Cluster| older than 5.5.33-23.7.6, use ``xtrabackup`` as SST method. For newer versions, ``xtrabackup-v2`` is recommended and also the default SST method.

.. note::  |Percona Xtrabackup| 2.1.x and later is strongly recommended for XtraBackup SST.

SST Options
-----------

The following options specific to |SST| can be used in :file:`my.cnf` under ``[sst]``.

.. note::

   * Non-integer options which have no default value are disabled if not set.

   * ``:Match: Yes`` implies that option should match on donor and joiner nodes.

   * SST script reads :file:`my.cnf` when it runs on either donor or joiner node, not during ``mysqld`` startup.

   * SST options must be specified in the main :file:`my.cnf` file.

.. option:: streamfmt

     :Values: xbstream, tar  
     :Default: xbstream
     :Match: Yes

Used to specify the Percona XtraBackup streaming format. The recommended value is ``streamfmt=xbstream``. Certain features are not available with ``tar``: encryption, compression, parallel streaming, streaming incremental backups. For more information about the ``xbstream`` format, see `The xbstream Binary <https://www.percona.com/doc/percona-xtrabackup/2.3/xbstream/xbstream.html>`_.
             
.. option:: transferfmt

     :Values: socat, nc
     :Default: socat
     :Match: Yes
     
Used to specify the data transfer format. The recommended value is ``transferfmt=socat`` because it allows for socket options, such as transfer buffer sizes. For more information, see `socat(1) <http://www.dest-unreach.org/socat/doc/socat.html>`_.

.. option:: tca

     :Example: tca=~/etc/ssl/certs/mycert.crt

Used to specify the full path to the certificate authority (CA) file for ``socat`` encryption based on OpenSSL.
                          
.. option:: tcert

     :Example: tcert=~/etc/ssl/certs/mycert.pem

Used to specify the full path to the certificate file in PEM format for ``socat`` encryption based on OpenSSL.
    
.. note:: For more information about ``tca`` and ``tcert``, refer to http://www.dest-unreach.org/socat/doc/socat-openssltunnel.html. The ``tca`` is essentially the self-signed certificate in that example, and ``tcert`` is the PEM file generated after concatenation of the key and the certificate generated earlier. The names of options were chosen to be compatible with ``socat`` parameter names as well as with MySQL's SSL authentication. For testing you can also download certificates from `launchpad <https://bazaar.launchpad.net/~percona-core/percona-xtradb-cluster/5.5/files/head:/tests/certs/>`_.

.. note:: Irrespective of what is shown in the example, you can use the same .crt and .pem files on all nodes and it will work, since there is no server-client paradigm here but a cluster with homogeneous nodes.

.. option:: encrypt

    :Values: 0, 1, 2, 3
    :Default: 0
    :Match: Yes

Used to enable and specify SST encryption mode:

* Set ``encrypt=0`` to disable SST encryption. This is the default value.

* Set ``encrypt=1`` to perform symmetric SST encryption based on XtraBackup.

* Set ``encrypt=2`` to perform SST encryption based on OpenSSL with ``socat``. Ensure that ``socat`` is built with OpenSSL: ``socat -V | grep OPENSSL``. This is recommended if your nodes are over WAN and security constraints are higher.

* Set ``encrypt=3`` to perform SST encryption based on SSL for just the key and certificate files as implemented in `Galera <http://galeracluster.com/documentation-webpages/ssl.html>`_.

  The latter has been implemented in :rn:`5.5.34-23.7.6` for compatibility with Galera. It does not provide certificate validation. In order to work correctly, paths to the key and certificate files need to be specified as well, for example: ::

    [sst]
    encrypt=3
    tkey=/etc/mysql/key.pem
    tcert=/etc/mysql/cert.pem

  .. note:: The ``encrypt=3``  option can only be used when :variable:`wsrep_sst_method` is set to ``xtrabackup-v2`` (which is the default now).

.. option:: encrypt-algo

   :Values: AES128, AES192, AES256

Used to specify the SST encryption algorithm. It uses the same values as the ``--encryption`` option for XtraBackup (see `this document <http://www.percona.com/doc/percona-xtrabackup/2.3/innobackupex/encrypted_backups_innobackupex.html>`_). The ``encrypt-algo`` option is considered only if :option:`encrypt` is set to ``1``.

.. option:: sockopt

Used to specify key/value pairs of socket options, separated by commas. Must begin with a comma. You can use the ``tcpwrap`` option to blacklist or whitelist clients. For more information about socket options, see `socat (1) <http://www.dest-unreach.org/socat/doc/socat.html>`_.

.. note:: You can also enable SSL based compression with :option:`sockopt`. This can be used in place of the XtraBackup ``compress`` option.

.. option:: progress

    :Values: 1, path/to/file

Used to specify where to write SST progress. If set to ``1``, it writes to MySQL ``stderr``. Alternatively, you can specify the full path to a file. If this is a FIFO, it needs to exist and be open on reader end before itself, otherwise ``wsrep_sst_xtrabackup`` will block indefinitely.

.. note:: Value of 0 is not valid.
           
.. option:: rebuild

    :Values: 0, 1 
    :Default: 0
    
Used to enable rebuilding of index on joiner node. Set to ``1`` to enable. This is independent of compaction, though compaction enables it. Rebuild of     
indexes may be used as an optimization.

.. note:: :bug:`1192834` affects this option.

.. option:: time

    :Values: 0, 1 
    :Default: 0   

Enabling this option instruments key stages of backup and restore in SST.
               
.. option:: rlimit

    :Example: rlimit=128k

Used to set a a ratelimit in bytes. Add a suffix (k, m, g, t) to specify other units. For example, ``128k`` is 128 kilobytes. Refer to `pv(1) <http://linux.die.net/man/1/pv>`_ for details.

.. note:: Rate is limited on donor node. The rationale behind this is to not allow SST to saturate the donor's regular cluster operations or to limit the rate for other purposes.

.. option:: incremental

    :Values: 0, 1
    :Default: 0

Used to sepersede IST on joiner node. Requires manual setup and is not supported currently.

.. option:: use_extra

    :Values: 0, 1
    :Default: 0


Used to force SST to use the thread pool's `extra_port <http://www.percona.com/doc/percona-server/5.6/performance/threadpool.html#extra_port>`_. Make sure that thread pool is enabled and the ``extra_port`` option is set in :file:`my.cnf` before you enable this option.

.. option:: cpat

Used to define the files that need to be deleted in the :term:`datadir` before running SST, so that the state of the other node can be restored cleanly. For example: :: 

  [sst]
  cpat='.*galera\.cache$\|.*sst_in_progress$\|.*grastate\.dat$\|.*\.err$\|.*\.log$\|.*RPM_UPGRADE_MARKER$\|.*RPM_UPGRADE_HISTORY$\|.*\.xyz$'

.. note:: This option can only be used when :variable:`wsrep_sst_method` is set to ``xtrabackup-v2``.

.. option:: sst_special_dirs
   
     :Values: 0, 1
     :Default: 1

This option was introduced in |Percona XtraDB Cluster| :rn:`5.6.15-25.2` to enable XtraBackup SST to support :variable:`innodb_data_home_dir` and :variable:`innodb_log_home_dir` variables in the configuration file. |Percona Xtrabackup| 2.1.6 or later is required for this option to work.
 
.. note:: This option can only be used when :variable:`wsrep_sst_method` is set to ``xtrabackup-v2``.
 
.. option:: compressor
 
    :Default: not set (disabled)
    :Example: compressor='gzip'

.. option:: decompressor

    :Default: not set (disabled)
    :Example: decompressor='gzip -dc'

Two previous options enable stream-based compression/decompression. When these options are set, compression/decompression is performed on stream, in contrast to earlier PXB-based one where decompression was done after streaming to disk, involving additional I/O. This saves a lot of I/O (up to twice less I/O on joiner node).

You can use any compression utility which works on stream: ``gzip``, ``pigz`` (which is recommended because it is multi-threaded), etc. Compressor has to be set on donor node and decompressor on joiner node (although you can set them vice-versa for configuration homogeneity, it won't affect that particular SST). To use XtraBackup based compression as before, set ``compress`` under ``[xtrabackup]``. Having both enabled won't cause any failure (although you will be wasting CPU cycles).

.. option:: inno-backup-opts

.. option:: inno-apply-opts

.. option:: inno-move-opts

   :Default: Empty
   :Type: Quoted String

This group of options can be used to pass innobackupex options for backup, apply, and move stages.

.. note:: Although these options are related to XtraBackup SST, they cannot be specified in :file:`my.cnf`, because they are for passing innobackupex options.

.. option:: sst-initial-timeout
   
   :Default: 100
   :Unit: seconds

This option is used to configure initial timeout (in seconds) to receive the first packet via SST. This has been implemented, so that if the donor node fails somewhere in the process, the joiner node will not hang up and wait forever.

By default, the joiner node will not wait for more than 100 seconds to get a donor node. The default should be sufficient, however, it is configurable, so you can set it appropriately for your cluster. To disable initial SST timeout, set ``sst-initial-timeout=0``.

.. note:: If you are using :variable:`wsrep_sst_donor`, and you want the joiner node to strictly wait for donors listed in the variable and not fall back (that is, without a terminating comma at the end), **and** there is a possibility of **all** nodes in that variable to be unavailable, disable initial SST timeout or set it to a higher value (maximum threshold that you want the joiner node to wait). You can also disable this option (or set it to a higher value) if you believe all other nodes in the cluster can potentially become unavailable at any point in time (mostly in small clusters) or there is a high network latency / network disturbance (which can cause donor selection to take longer than 100 seconds).

XtraBackup SST Dependencies
---------------------------

The following are optional dependencies of Percona XtraDB Cluster introduced by ``wsrep_sst_xtrabackup`` (except for obvious and direct dependencies):

* ``qpress`` for decompression. It is an optional dependency of |Percona XtraBackup| 2.1.4 and it is available in our software repositories.

* ``my_print_defaults`` to extract values from :file:`my.cnf`. Provided by the server package.

* ``openbsd-netcat`` or ``socat`` for transfer. ``socat`` is a direct dependency of |Percona XtraDB Cluster| and it is the default.

* ``xbstream`` or ``tar`` for streaming. ``tar`` is default.

* ``pv`` is required for :option:`progress` and :option:`rlimit`.

* ``mkfifo`` is required for :option:`progress`. Provided by ``coreutils``.

* ``mktemp`` is required for :option:`incremental`. Provided by ``coreutils``.

XtraBackup-based Encryption
---------------------------

This is enabled when :option:`encrypt` is set to ``1`` under ``[sst]`` in :file:`my.cnf`. However, due to bug :bug:`1190335`, it will also be enabled when you specify any of the following options under ``[xtrabackup]`` in :file:`my.cnf`:

    * ``encrypt``
    * ``encrypt-key``
    * ``encrypt-key-file``

There is no way to disable encryption from innobackupex if any of the above are in :file:`my.cnf` under ``[xtrabackup]``. For that reason, comsider the following scenarios:

  1. If you want to use XtraBackup-based encryption for SST but not otherwise, use ``encrypt=1`` under ``[sst]`` and provide the above XtraBackup encryption options under ``[sst]``. Details of those options can be found `here <http://www.percona.com/doc/percona-xtrabackup/2.1/innobackupex/encrypted_backups_innobackupex.html>`_.

  2. If you want to use XtraBackup-based encryption always, use ``encrypt=1`` under ``[sst]`` and have the above XtraBackup encryption options either under ``[sst]`` or ``[xtrabackup]``.

  3. If you don't want to use XtraBackup-based encryption for SST, but want it otherwise, use ``encrypt=0`` or ``encrypt=2`` and do **NOT** provide any XtraBackup encryption options under ``[xtrabackup]``. You can still have them under ``[sst]`` though. You will need to provide those options on innobackupex command line then.

  4. If you don't want to use XtraBackup-based encryption at all (or only the OpenSSL-based for SST with ``encrypt=2``), then don't provide any XtraBackup encryption options in :file:`my.cnf`.

.. note:: The :option:`encrypt` option under ``[sst]`` is different from the one under ``[xtrabackup]``. The former is for disabling/changing encryption mode, while the latter is to provide an encryption algorithm. To disambiguate, if you need to provide the latter under ``[sst]`` (for example, in cases 1 and 2 above), it should be specified as :option:`encrypt-algo`.

.. warning:: An implication of the above is that if you specify any of the XtraBackup encryption options, and ``encrypt=0`` under ``[sst]``, it will still be encrypted and SST will fail. Look at case 3 above for resolution.
