==========
 Glossary
==========

.. glossary::

   LSN
     Each InnoDB page (usually 16kb in size) contains a log sequence number, or
     LSN. The LSN is the system version number for the entire database. Each
     page's LSN shows how recently it was changed.

   InnoDB
      Storage engine which provides ACID-compliant transactions and foreign key
      support, among others improvements over :term:`MyISAM`. It is the default
      engine for |MySQL| as of the 5.5 series.

   MyISAM
     Previous default storage engine for |MySQL| for versions prior to 5.5. It
     doesn't fully support transactions but in some scenarios may be faster
     than :term:`InnoDB`. Each table is stored on disk in 3 files: :term:`.frm`,i
     :file:`.MYD`, :file:`.MYI`.

   GTID
     Global Transaction ID, in *Percona XtraDB Cluster* it consists of
     :term:`UUID` and an ordinal sequence number which denotes the position of
     the change in the sequence.

   HAProxy
     `HAProxy <http://haproxy.1wt.eu/>`_ is a free, very fast and reliable
     solution offering high availability, load balancing, and proxying for TCP
     and HTTP-based applications. It is particularly suited for web sites
     crawling under very high loads while needing persistence or Layer7
     processing. Supporting tens of thousands of connections is clearly
     realistic with todays hardware. Its mode of operation makes its
     integration into existing architectures very easy and riskless, while
     still offering the possibility not to expose fragile web servers to the
     net.

   IST
     Incremental State Transfer. Functionality which instead of whole state
     snapshot can catch up with te group by receiving the missing writesets,
     but only if the writeset is still in the donor's writeset cache.

   SST
     State Snapshot Transfer is the full copy of data from one node to another.
     It's used when a new node joins the cluster, it has to transfer data from
     existing node. There are three methods of SST available in |Percona XtraDB
     Cluster|: :program:`mysqldump`, :program:`rsync` and
     :program:`xtrabackup`. The downside of `mysqldump` and `rsync` is that the
     node becomes *READ-ONLY* while data is being copied from one node to
     another (SST applies :command:`FLUSH TABLES WITH READ LOCK` command).
     Xtrabackup SST does not require :command:`READ LOCK` for the entire
     syncing process, only for syncing the |MySQL| system tables and writing
     the information about the binlog, galera and slave information (same as
     the regular |Percona XtraBackup| backup). State snapshot transfer method
     can be configured with the :variable:`wsrep_sst_method` variable.

   UUID
     Universally Unique IDentifier which uniquely identifies the state and the
     sequence of changes node undergoes. 128-bit UUID is a classic DCE UUID
     Version 1 (based on current time and MAC address). Although in theory this
     UUID could be generated based on the real MAC-address, in the Galera it is
     always (without exception) based on the generated pseudo-random addresses
     ("locally administered" bit in the node address (in the UUID structure) is
     always equal to unity).

     Complete structure of the 128-bit UUID field and explanation for its
     generation are as follows:

     ===== ====  ======= =====================================================
     From  To    Length  Content
     ===== ====  ======= =====================================================
      0     31    32     Bits 0-31 of Coordinated Universal Time (UTC) as a
                         count of 100-nanosecond intervals since 00:00:00.00,
                         15 October 1582, encoded as big-endian 32-bit number.
     32     47    16     Bits 32-47 of UTC as a count of 100-nanosecond
                         intervals since 00:00:00.00, 15 October 1582, encoded
                         as big-endian 16-bit number.
     48     59    12     Bits 48-59 of UTC as a count of 100-nanosecond
                         intervals since 00:00:00.00, 15 October 1582, encoded
                         as big-endian 16-bit number.
     60     63     4     UUID version number: always equal to 1 (DCE UUID).
     64     69     6     most-significants bits of random number, which
                         generated from the server process PID and Coordinated
                         Universal Time (UTC) as a count of 100-nanosecond
                         intervals since 00:00:00.00, 15 October 1582.
     70     71     2     UID variant: always equal to binary 10 (DCE variant).
     72     79     8     8 least-significant bits of  random number, which
                         generated from the server process PID and Coordinated
                         Universal Time (UTC) as a count of 100-nanosecond
                         intervals since 00:00:00.00, 15 October 1582.
     80     80     1     Random bit ("unique node identifier").
     81     81     1     Always equal to the one ("locally administered MAC
                         address").
     82    127    46     Random bits ("unique node identifier"): readed from
                         the :file:`/dev/urandom` or (if :file:`/dev/urandom`
                         is unavailable) generated based on the server process
                         PID, current time and bits of the default "zero node
                         identifier" (entropy data).
     ===== ====  ======= =====================================================

   XtraBackup
     *Percona XtraBackup* is an open-source hot backup utility for |MySQL| -
     based servers that doesn't lock your database during the backup.

   XtraDB
     *Percona XtraDB* is an enhanced version of the InnoDB storage engine,
     designed to better scale on modern hardware, and including a variety of
     other features useful in high performance environments. It is fully
     backwards compatible, and so can be used as a drop-in replacement for
     standard InnoDB. More information `here
     <http://www.percona.com/doc/percona-server/5.7/percona_xtradb.html>`_ .

   XtraDB Cluster
     *Percona XtraDB Cluster* is a high availability solution for MySQL.

   Percona XtraDB Cluster
     *Percona XtraDB Cluster* (PXC) is a high availability solution for MySQL.

   my.cnf
     This file refers to the database server's main configuration file. Most
     Linux distributions place it as :file:`/etc/mysql/my.cnf` or
     :file:`/etc/my.cnf`, but the location and name depends on the particular
     installation. Note that this is not the only way of configuring the
     server, some systems does not have one even and rely on the command
     options to start the server and its defaults values.

   cluster replication
     Normal replication path for cluster members. Can be encrypted (not by
     default) and unicast or multicast (unicast by default). Runs on tcp port
     4567 by default.

   datadir
    The directory in which the database server stores its databases. Most Linux
    distribution use :file:`/var/lib/mysql` by default.

   donor node
    The node elected to provide a state transfer (SST or IST).

   ibdata
     Default prefix for tablespace files, e.g. :file:`ibdata1` is a 10MB
     autoextendable file that |MySQL| creates for the shared tablespace by
     default.

   joiner node
     The node joining the cluster, usually a state transfer target.

   node
     A cluster node -- a single mysql instance that is in the cluster.

   primary cluster
     A cluster with :term:`quorum`. A non-primary cluster will not allow any
     operations and will give ``Unknown command`` errors on any clients
     attempting to read or write from the database.

   quorum
     A majority (> 50%) of nodes. In the event of a network partition, only the
     cluster partition that retains a quorum (if any) will remain Primary by
     default.

   split brain
     Split brain occurs when two parts of a computer cluster are disconnected,
     each part believing that the other is no longer running. This problem can
     lead to data inconsistency.

   .frm
     For each table, the server will create a file with the :file:`.frm`
     extension containing the table definition (for all storage engines).
