.. _xtrabackup_sst:

===============================
 Xtrabackup SST Configuration
===============================

Following SST specific options are allowed in my.cnf under [sst]:                                     
==================================================================
      
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
     
socat is recommended because it allows for socket options like transfer buffer sizes.
                                                                                                             
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

  * Xtrabackup based encryption                                                                          

  * OpenSSL based encryption - socat must be built with openSSL for encryption: ``socat -V | grep OPENSSL``. 

.. note::
   You can also enable SSL based compression with :option:`sockopt` mentioned below.
            
.. option:: sockopt

Comma separated key/value pairs of socket options. Must begin with a comma. You can use tcpwrap option here to blacklist/whitelist the clients. Refer to socat manual for further details.                     

.. option:: progress

    :Values: 1,path/to/file,path/to/fifo

If equal to:

    * 1 it writes to mysql stderr 
    * path/to/file writes to that file. 
    * path/to/fifo, it will be created and cleaned up at exit.This is the preferred way. You need to cat the fifo file to monitor the progress, not tail it.

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
    
Ratelimit to x kilobytes, megabytes etc. Refer to pv manual for details.

.. option:: incremental

    :Values: 0,1
    :Default: 0

To be set on joiner only, supersedes IST if set. Currently requires
manual setup. Hence, not supported currently.

.. _tar_ag_xbstream:

===============================
 Tar against xbstream
===============================

  * Features - encryption, compression, parallel streaming, streaming incremental backups, compaction - won't work with tar. Refer to `xbstream docs <http://www.percona.com/doc/percona-xtrabackup/2.1/xbstream/xbstream.html>`_ for more. 
  * Bug :bug:`1193240` requires you to manually cleanup the directory (empty data directory) prior to SST. After that is fixed (and xtrabackup is released), SST will work without any modifications to wsrep_sst_xtrabackup.

===============================
 Xtrabackup SST Dependencies
===============================

Following are optional dependencies of PXC introduced by wsrep_sst_xtrabackup: (obvious and direct dependencies are not provided here)

    * qpress for decompression. Download  `here <http://www.quicklz.com/qpress-11-linux-x64.tar>`_ till `this <https://blueprints.launchpad.net/percona-xtrabackup/+spec/package-qpress>`_ is fixed.
    * my_print_defaults to extract values from my.cnf. Provided by the server package.
    * openbsd-netcat or socat for transfer. socat is a direct dependency of PXC and is the default.
    * xbstream/tar for streaming. tar is default.
    * pv. Required for :option:`progress` and :option:`rlimit`. Provided by pv.
    * mkfifo. Required for :option:`progress`. Provided by coreutils.
    * mktemp. Required for :option:`incremental`. Provided by coreutils.
