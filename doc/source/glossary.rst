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
	
   IST
     Incremental State Transfer. Functionallity which instead of whole state snapshot can catch up with te group by receiving the missing writesets, but only if the writeset is still in the donor's writeset cache.

   XtraBackup
     *Percona XtraBackup* is an open-source hot backup utility for |MySQL| - based servers that doesn’t lock your database during the backup.

   XtraDB
     *Percona XtraDB* is an enhanced version of the InnoDB storage engine, designed to better scale on modern hardware, and including a variety of other features useful in high performance environments. It is fully backwards compatible, and so can be used as a drop-in replacement for standard InnoDB. More information `here <http://www.percona.com/docs/wiki/Percona-XtraDB:start>`_ .

   XtraDB Cluster
     *Percona XtraDB Cluster* is a high availability solution for MySQL.

   Percona XtraDB Cluster
     *Percona XtraDB Cluster* is a high availability solution for MySQL.

   my.cnf
     This file refers to the database server's main configuration file. Most Linux distributions place it as :file:`/etc/mysql/my.cnf`, but the location and name depends on the particular installation. Note that this is not the only way of configuring the server, some systems does not have one even and rely on the command options to start the server and its defaults values.

   datadir
    The directory in which the database server stores its databases. Most Linux distribution use :file:`/var/lib/mysql` by default.

   ibdata
     Default prefix for tablespace files, e.g. :file:`ibdata1` is a 10MB  autoextendable file that |MySQL| creates for the shared tablespace by default. 

   innodb_file_per_table
     InnoDB option to use separate .ibd files for each table.

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
     File containing the triggers associated to a table, e.g. `:file:`mytable.TRG`. With the :term:`.TRN` file, they represent all the trigger definitions.

   .TRN
     File containing the triggers' Names associated to a table, e.g. `:file:`mytable.TRN`. With the :term:`.TRG` file, they represent all the trigger definitions.

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

