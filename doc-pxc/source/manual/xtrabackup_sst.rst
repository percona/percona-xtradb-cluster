.. _xtrabackup_sst:

===============================
 Xtrabackup SST Configuration
===============================

XtraBackup SST works in two stages:

 * Stage I on joiner checks if it is |SST| or |IST| based on presence of :file:`xtrabackup_ist` file. 
 * In Stage II it starts the data transfer, if it's |SST|, it empties the data directory sans few files (galera.cache, sst_in_progress, grastate.dat) and then proceed with the SST or if it's IST, proceeds as before.

.. warning::

   :ref:`xtrabackup_sst` implementation added in |Percona XtraDB Cluster| :rn:`5.5.33-23.7.6` has been renamed to ``xtrabackup-v2``, so :variable:`wsrep_sst_method` =xtrabackup will use xtrabackup implementation before :rn:`5.5.33-23.7.6` and will be compatible with older |Percona XtraDB Cluster| versions. In order to use the new version :variable:`wsrep_sst_method` should be set to ``xtrabackup-v2``.

Latest |Percona Xtrabackup| 2.1.x is strongly recommended for Xtrabackup SST. Refer to `Incompatibilities <http://www.percona.com/doc/percona-xtradb-cluster/errata.html#incompatibilities>`_ for possible caveats.

Following SST specific options are allowed in my.cnf under [sst]                                     
-----------------------------------------------------------------
      
.. note:: 
    In following options:
    
        * Non-integer options which have no default are disabled if not set.
    
        * ":Match: Yes" implies that options should match on donor and joiner (in their cnf). 
    
        * ":Recommended: Yes" implies the value which is recommended. 
          
        * In following options, path always means full path.

        * Following options are only applicable for :rn:`5.5.33-23.7.6` and above. For :rn:`5.5.31-23.7.5` refer to documentation in wsrep_sst_xtrabackup.

.. option:: streamfmt

     :Values: xbstream, tar  
     :Default: xbstream
     :Match: Yes

Xbstream is highly recommended. Refer to :ref:`Xbstream v/s Tar <tar_ag_xbstream>` for details and caveats of using tar v/s xbstream for SST.
             
.. option:: transferfmt

     :Values: socat, nc
     :Default: socat
     :Match: Yes
     
socat is recommended because it allows for socket options like transfer buffer sizes. Refer to `socat(1) <http://www.dest-unreach.org/socat/doc/socat.html>`_ for details.
                                                                                                             
.. option:: tca 

     :Description: CA file for openssl based encryption with socat.                                                   
     :Type: Full path to CRT file (.crt).
                          
.. option:: tcert
    
    :Description: PEM for openssl based encryption with socat.                                                     
    :Type:  Full path to PEM (.pem).

.. note::
    For tca and tcert, refer to http://www.dest-unreach.org/socat/doc/socat-openssltunnel.html for an example. The ``tca`` is essentially the self-signed certificate in that example, and ``tcert`` is the PEM file generated after concatenation of the key and the certificate generated earlier. The names of options were chosen so as to be compatible with socat's parameter' names as well as with MySQL's SSL authentication. For testing you can also download certificates from `launchpad <https://bazaar.launchpad.net/~percona-core/percona-xtradb-cluster/5.5/files/head:/tests/certs/>`_. **Note** that irrespective of what is shown in the example, you can use same crt and pem files on all nodes and it will work, since there is no server-client paradigm here but a cluster with homogeneous nodes.
                                                                                                             
.. option:: encrypt

    :Values: 0,1,2,3
    :Default: 0
    :Match: Yes

Decides whether encryption is to be done or not, if this is zero, no    
encryption is done. ``encrypt=2`` is recommended if your nodes are      
over WAN and security constraints are higher, while ``encrypt=1``       
(Xtrabackup-based symmetric encryption) is easier to setup.             

  * Xtrabackup based encryption  with ``encrypt=1``.

  * OpenSSL based encryption with ``encrypt=2``. Socat must be built with openSSL for encryption: ``socat -V | grep OPENSSL``.

  * Support for SSL encryption for just the key and crt files as implemented in `Galera <http://www.codership.com/wiki/doku.php?id=ssl_support>`_ can be enabled with ``encrypt=3`` option. Information on this option can be found :ref:`here <galera_sst_encryption>`.

Refer to this :ref:`document <xtrabackup_sst_encryption>` when enabling with ``encrypt=1``.

.. option:: encrypt-algo

This option is only considered when :option:`encrypt` is equal to 1. Refer to :ref:`this <encrypt_algo_note>` before setting this. This option takes the same value as encrypt option `here <http://www.percona.com/doc/percona-xtrabackup/2.1/innobackupex/encrypted_backups_innobackupex.html>`_. 

.. option:: sockopt

Comma separated key/value pairs of socket options. Must begin with a comma. You can use tcpwrap option here to blacklist/whitelist the clients. Refer to socat `manual <http://www.dest-unreach.org/socat/doc/socat.html>`_ for further details.                     

.. note::
   You can also enable SSL based compression with :option:`sockopt`. This can be used in place of compress option of Xtrabackup.

.. option:: progress

    :Values: 1,path/to/file

If equal to:

    * 1 it writes to mysql stderr 
    * path/to/file writes to that file. If this is a fifo, it needs to exist and be open on reader end before itself, otherwise wsrep_sst_xtrabackup will block indefinitely.

.. note::
    Value of 0 is not valid.
           
.. option:: rebuild

    :Values: 0,1 
    :Default: 0
    
Used only on joiner. 1 implies rebuild indexes. Note that this is       
independent of compaction, though compaction enables it. Rebuild of     
indexes may be used as an optimization. Note that :bug:`1192834`        
affects this, hence use of ``compact`` and ``rebuild`` are recommended  
after that is fixed in Percona Xtrabackup and released.                 
                             
.. option:: time

    :Values: 0,1  
    :Default: 0   

Enabling it instruments key stages of backup/restore in SST.
               
.. option:: rlimit 

    :Values: x(k|m|g|t) 
    
Ratelimit to ``x`` kilobytes, megabytes etc. Refer to `pv(1) <http://linux.die.net/man/1/pv>`_ for details. Note this rate-limiting happens on donor. The rationale behind this is to not allow SST to saturate the donor's regular cluster operations and/or to ratelimit for other purposes.

.. option:: incremental

    :Values: 0,1
    :Default: 0

To be set on joiner only, supersedes IST if set. Currently requires
manual setup. Hence, not supported currently.

.. option:: use_extra

    :Values: 0,1
    :Default: 0


If set to 1, SST will use the thread pool's `extra_port <http://www.percona.com/doc/percona-server/5.6/performance/threadpool.html#extra_port>`_. Make sure that thread pool is enabled and extra_port option is set in my.cnf before you turn on this option.

.. option:: cpat

During the SST, the :term:`datadir` is cleaned up so that state of other node can be restored cleanly. This option provides the ability to define the files that need to be deleted before the SST. It can be set like: :: 

  [sst]
  cpat='.*galera\.cache$\|.*sst_in_progress$\|.*grastate\.dat$\|.*\.err$\|.*\.log$\|.*RPM_UPGRADE_MARKER$\|.*RPM_UPGRADE_HISTORY$\|.*\.xyz$'

**NOTE:** This option can only be used when :variable:`wsrep_sst_method` is set to xtrabackup-v2.

.. option:: sst_special_dirs
   
     :Values: 0,1
     :Default: 0

In order for XtraBackup SST to support :variable:`innodb_data_home_dir` and :variable:`innodb_log_home_dir` variables in the configuration file this option was introduced in |Percona XtraDB Cluster| :rn:`5.5.34-25.9`. This requires sst-special-dirs to be set under [sst] in the configuration file to either 0 or 1. Also, :variable:`innodb-data-home-dir` and/or :variable:`innodb-log-group-home-dir` need to be defined in :file:`my.cnf` under [mysqld]. |Percona Xtrabackup| 2.1.6 or higher is required in order for this to work.

**NOTE:** This option can only be used when :variable:`wsrep_sst_method` is set to xtrabackup-v2.

.. _tar_ag_xbstream:

Tar against xbstream
---------------------

  * Features - encryption, compression, parallel streaming, streaming incremental backups, compaction - won't work with tar. Refer to `xbstream docs <http://www.percona.com/doc/percona-xtrabackup/2.1/xbstream/xbstream.html>`_ for more. 

Xtrabackup SST Dependencies
----------------------------

Following are optional dependencies of PXC introduced by wsrep_sst_xtrabackup: (obvious and direct dependencies are not provided here)

    * qpress for decompression. It is an optional dependency of |Percona XtraBackup| 2.1.4 and it is available in our software repositories.
    * my_print_defaults to extract values from my.cnf. Provided by the server package.
    * openbsd-netcat or socat for transfer. socat is a direct dependency of |Percona XtraDB Cluster| and it is the default.
    * xbstream/tar for streaming. tar is default.
    * pv. Required for :option:`progress` and :option:`rlimit`. Provided by pv.
    * mkfifo. Required for :option:`progress`. Provided by coreutils.
    * mktemp. Required for :option:`incremental`. Provided by coreutils.

.. _galera_sst_encryption:

Galera compatible encryption
----------------------------

Support for SSL encryption for just the key and crt files as implemented in `Galera <http://www.codership.com/wiki/doku.php?id=ssl_support>`_ can be enabled with ``encrypt=3`` option. This has been implemented in :rn:`5.5.34-23.7.6` for compatibility with Galera. **NOTE**: This option does not provide certificate validation. In order to work correctly paths to the key and cert files need to be specified as well, like: ::

   [sst] 
   encrypt=3
   tkey=/etc/mysql/key.pem
   tcert=/etc/mysql/cert.pem

**NOTE:** This option can only be used when :variable:`wsrep_sst_method` is set to xtrabackup-v2.

.. _xtrabackup_sst_encryption:

Xtrabackup-based encryption
----------------------------

This is enabled when :option:`encrypt` is set to 1 under [sst]. However, due to bug :bug:`1190335`, it will also be enabled when you specify any of the following options under [xtrabackup] in my.cnf:

.. _xtrabackup_encrypt_options:

    * encrypt
    * encrypt-key
    * encrypt-key-file

There is no way to disallow encryption from innobackupex if the above are in my.cnf under [xtrabackup]. For that reason, do the following:

    #. If you want to use xtrabackup based encryption for SST but not otherwise, use ``encrypt=1`` under [sst] and provide xtrabackup_encrypt_options under [sst]. Details of those options can be found `here <http://www.percona.com/doc/percona-xtrabackup/2.1/innobackupex/encrypted_backups_innobackupex.html>`_.

    #. If you want to use xtrabackup based encryption always, use ``encrypt=1`` under [sst] and have those xtrabackup_encrypt_options either under [sst] or [xtrabackup].

    #. If you don't want xtrabackup based encryption for SST but want it otherwise, use ``encrypt=0`` or ``encrypt=2`` and do **NOT** provide xtrabackup_encrypt_options under [xtrabackup]. You can still have them under [sst] though. You will need to provide those options on innobackupex commandline then.

    #. If you don't want to use xtrabackup based encryption at all (or only the openssl-based for SST with ``encrypt=2``), then you don't need worry about these options! (just don't provide them in my.cnf)

.. _encrypt_algo_note:

.. note:: 
    The :option:`encrypt` under [sst] is different from under [xtrabackup]. The former is for disabling/changing encryption mode, latter is to provide encryption algorithm. To disambiguate, if you need to provide latter under [sst] (which you need to, for points #1 and #2 above) then it should be specified as :option:`encrypt-algo`.

.. warning:: 
    An implication of the above is that if you specify xtrabackup_encrypt_options but ``encrypt=0`` under [sst], it will **STILL** be encrypted and SST will fail. Look at point#3 above for resolution.
