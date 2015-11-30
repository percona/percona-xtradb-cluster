==========
 Glossary
==========

.. glossary::

   LSN
     Each InnoDB page (usually 16kb in size) contains a log sequence number, or LSN. The LSN is the system version number for the entire database. Each page's LSN shows how recently it was changed.

   InnoDB
      Storage engine which provides ACID-compliant transactions and foreign key support, among others improvements over :term:`MyISAM`. It is the default engine for |MySQL| as of the 5.5 series.

   MyISAM
     Previous default storage engine for |MySQL| for versions prior to 5.5. It doesn't fully support transactions but in some scenarios may be faster than :term:`InnoDB`. Each table is stored on disk in 3 files: :term:`.frm`, :term:`.MYD`, :term:`.MYI`.

   GTID
     Global Transaction ID, in *Percona XtraDB Cluster* it consists of :term:`UUID` and an ordinal sequence number which denotes the position of the change in the sequence.
   
   HAProxy
     `HAProxy <http://haproxy.1wt.eu/>`_ is a free, very fast and reliable solution offering high availability, load balancing, and proxying for TCP and HTTP-based applications. It is particularly suited for web sites crawling under very high loads while needing persistence or Layer7 processing. Supporting tens of thousands of connections is clearly realistic with todays hardware. Its mode of operation makes its integration into existing architectures very easy and riskless, while still offering the possibility not to expose fragile web servers to the net.
	
   IST
     Incremental State Transfer. Functionality which instead of whole state snapshot can catch up with te group by receiving the missing writesets, but only if the writeset is still in the donor's writeset cache.

   SST
     State Snapshot Transfer is the full copy of data from one node to another. It's used when a new node joins the cluster, it has to transfer data from existing node. There are three methods of SST available in Percona XtraDB Cluster: :program:`mysqldump`, :program:`rsync` and :program:`xtrabackup`. The downside of `mysqldump` and `rsync` is that the node becomes *READ-ONLY* while data is being copied from one node to another (SST applies :command:`FLUSH TABLES WITH READ LOCK` command). Xtrabackup SST does not require :command:`READ LOCK` for the entire syncing process, only for syncing the |MySQL| system tables and writing the information about the binlog, galera and slave information (same as the regular XtraBackup backup). State snapshot transfer method can be configured with the :variable:`wsrep_sst_method` variable.

   UUID 
      Universally Unique IDentifier which uniquely identifies the state and the sequence of changes node undergoes.

   XtraBackup
     *Percona XtraBackup* is an open-source hot backup utility for |MySQL| - based servers that doesn’t lock your database during the backup.

   XtraDB
     *Percona XtraDB* is an enhanced version of the InnoDB storage engine, designed to better scale on modern hardware, and including a variety of other features useful in high performance environments. It is fully backwards compatible, and so can be used as a drop-in replacement for standard InnoDB. More information `here <http://www.percona.com/doc/percona-server/5.5/percona_xtradb.html>`_ .

   XtraDB Cluster
     *Percona XtraDB Cluster* is a high availability solution for MySQL.

   Percona XtraDB Cluster
     *Percona XtraDB Cluster* (PXC) is a high availability solution for MySQL.

   my.cnf
     This file refers to the database server's main configuration file. Most Linux distributions place it as :file:`/etc/mysql/my.cnf`, but the location and name depends on the particular installation. Note that this is not the only way of configuring the server, some systems does not have one even and rely on the command options to start the server and its defaults values.

   cluster replication
     Normal replication path for cluster members. Can be encrypted (not by default) and unicast or multicast (unicast by default). Runs on tcp port 4567 by default.

   datadir
    The directory in which the database server stores its databases. Most Linux distribution use :file:`/var/lib/mysql` by default.

   donor node
    The node elected to provide a state transfer (SST or IST).

   ibdata
     Default prefix for tablespace files, e.g. :file:`ibdata1` is a 10MB  autoextendable file that |MySQL| creates for the shared tablespace by default. 

   innodb_file_per_table
     InnoDB option to use separate .ibd files for each table.

   joiner node
     The node joining the cluster, usually a state transfer target.
    
   node
     A cluster node -- a single mysql instance that is in the cluster.
     
   primary cluster
     A cluster with :term:`quorum`. A non-primary cluster will not allow any operations and will give ``Unknown command`` errors on any clients attempting to read or write from the database.

   quorum 
     A majority (> 50%) of nodes. In the event of a network partition, only the cluster partition that retains a quorum (if any) will remain Primary by default.

   split brain
     Split brain occurs when two parts of a computer cluster are disconnected, each part believing that the other is no longer running. This problem can lead to data inconsistency.

   .frm
     For each table, the server will create a file with the ``.frm`` extension containing the table definition (for all storage engines).

   .ibd
     On a multiple tablespace setup (:term:`innodb_file_per_table` enabled), |MySQL| will store each newly created table on a file with a ``.ibd`` extension.

   .MYD
     Each |MyISAM| table has ``.MYD`` (MYData) file which contains the data on it.

   .MYI
     Each |MyISAM| table has ``.MYI`` (MYIndex) file which contains the table's indexes.

   .MRG
     Each table using the :program:`MERGE` storage engine, besides of a :term:`.frm` file, will have :term:`.MRG` file containing the names of the |MyISAM| tables associated with it.

   .TRG
     File containing the triggers associated to a table, e.g. :file:`mytable.TRG`. With the :term:`.TRN` file, they represent all the trigger definitions.

   .TRN
     File containing the triggers' Names associated to a table, e.g. :file:`mytable.TRN`. With the :term:`.TRG` file, they represent all the trigger definitions.

   .ARM
     Each table with the :program:`Archive Storage Engine` has ``.ARM`` file which contains the metadata of it.

   .ARZ
     Each table with the :program:`Archive Storage Engine` has ``.ARZ`` file which contains the data of it.

   .CSM
     Each table with the :program:`CSV Storage Engine` has ``.CSM`` file which contains the metadata of it.

   .CSV
     Each table with the :program:`CSV Storage` engine has ``.CSV`` file which contains the data of it (which is a standard Comma Separated Value file).

   .opt
     |MySQL| stores options of a database (like charset) in a file with a :option:`.opt` extension in the database directory.

