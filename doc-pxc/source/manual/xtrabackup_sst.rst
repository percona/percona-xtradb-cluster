.. _xtrabackup_sst:

===============================
 Xtrabackup SST Configuration
===============================

Following SST specific options are allowed in my.cnf under [sst]                                     
-----------------------------------------------------------------
      
.. note:: 
    In following options:
    
        * Non-integer options which have no default are disabled if not set.
    
        * ":Match: Yes" implies that options should match on donor and joiner (in their cnf). 
    
        * ":Recommended: Yes" implies the value which is recommended. 
          
        * In following options, path always means full path.

        * Following options are only applicable for **PXC 5.5.32** and above. For 5.5.31 PXC refer to documentation in wsrep_sst_xtrabackup.

.. option:: streamfmt

     :Values: xbstream, tar  
     :Default: tar             
     :Match: Yes

Refer to :ref:`Xbstream v/s Tar<tar_ag_xbstream>` for details and caveats of using tar v/s xbstream for SST.
             
.. option:: transferfmt

     :Values: socat, nc
     :Default: socat
     :Match: Yes
     
socat is recommended because it allows for socket options like transfer buffer sizes. Refer to `socat(1) <http://www.dest-unreach.org/socat/doc/socat.html>`_ for details.
                                                                                                             
.. option:: tca 

     :Description: CA file for openssl based encryption with socat.                                                   
     :Type: Full path to CA file.
                          
.. option:: tcert
    
    :Description: PEM for openssl based encryption with socat.                                                     
    :Type:  Full path to PEM.

.. note::
    For tca and tcert, refer to http://www.dest-unreach.org/socat/doc/socat-openssltunnel.html for an example.      
                                                                                                             
.. option:: encrypt

    :Values: 0,1,2  
    :Default: 0
    :Match: Yes
    :Recommended: 2

Decides whether encryption is to be done or not, if this is zero, no    
encryption is done.                                                    

  * Xtrabackup based encryption  with ``encrypt=1``.

  * OpenSSL based encryption with ``encrypt=2``. Socat must be built with openSSL for encryption: ``socat -V | grep OPENSSL``.

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
indexes may be used as an optimization.                                 
                             
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

.. _tar_ag_xbstream:

Tar against xbstream
---------------------

  * Features - encryption, compression, parallel streaming, streaming incremental backups, compaction - won't work with tar. Refer to `xbstream docs <http://www.percona.com/doc/percona-xtrabackup/2.1/xbstream/xbstream.html>`_ for more. 
  * Bug :bug:`1193240` requires you to manually cleanup the directory (empty data directory) prior to SST. After that is fixed (and xtrabackup is released), SST will work without any modifications to wsrep_sst_xtrabackup.

Xtrabackup SST Dependencies
----------------------------

Following are optional dependencies of PXC introduced by wsrep_sst_xtrabackup: (obvious and direct dependencies are not provided here)

    * qpress for decompression. It is a dependency of Xtrabackup 2.1.4 and is available in our repos.
    * my_print_defaults to extract values from my.cnf. Provided by the server package.
    * openbsd-netcat or socat for transfer. socat is a direct dependency of PXC and is the default.
    * xbstream/tar for streaming. tar is default.
    * pv. Required for :option:`progress` and :option:`rlimit`. Provided by pv.
    * mkfifo. Required for :option:`progress`. Provided by coreutils.
    * mktemp. Required for :option:`incremental`. Provided by coreutils.

.. _tip::
    For qpress download `from <http://www.quicklz.com/qpress-11-linux-x64.tar>`_ till it is available. You can also build it from `source <http://www.quicklz.com/qpress-11-source.zip>`_.

.. _xtrabackup_sst_encryption:

Xtrabackup-based encryption
----------------------------

This is enabled when :option:`encrypt` is set to 1 under [sst]. However, due to lp:1190335, it will also be enabled when you specify any of the following options under [xtrabackup] in my.cnf:

.. _xtrabackup_encrypt_options:

    * encrypt
    * encrypt-key
    * encrypt-key-file

There is no way to disallow encryption from innobackupex if the above are in my.cnf under [xtrabackup]. For that reason, do the following:

    #. If you want to use xtrabackup based encryption for SST but not otherwise, use ``encrypt=1`` under [sst] and provide xtrabackup_encrypt_options_ under [sst]. Details of those options can be found `here <http://www.percona.com/doc/percona-xtrabackup/2.1/innobackupex/encrypted_backups_innobackupex.html>`_.

    #. If you want to use xtrabackup based encryption always, use ``encrypt=1`` under [sst] and have those xtrabackup_encrypt_options_ either under [sst] or [xtrabackup].

    #. If you don't want xtrabackup based encryption for SST but want it otherwise, use ``encrypt=0`` or ``encrypt=2`` and do **NOT** provide xtrabackup_encrypt_options_ under [xtrabackup]. You can still have them under [sst] though. You will need to provide those options on innobackupex commandline then.

    #. If you don't want to use xtrabackup based encryption at all (or only the openssl-based for SST with ``encrypt=2``), then you don't need worry about these options! (just don't provide them in my.cnf)

.. _encrypt_algo_note:

.. note:: 
    The :option:`encrypt` under [sst] is different from under [xtrabackup]. The former is for disabling/changing encryption mode, latter is to provide encryption algorithm. To disambiguate, if you need to provide latter under [sst] (which you need to, for points #1 and #2 above) then it should be specified as :option:`encrypt-algo`.

.. warning:: 
    An implication of the above is that if you specify xtrabackup_encrypt_options_ but ``encrypt=0`` under [sst], it will **STILL** be encrypted and SST will fail. Look at point#3 above for resolution.
