.. _wsrep_file_index:

===============================
 Index of files created by PXC
===============================

* :file:`GRA_*.log`
   These files contain binlog events in ROW format representing the failed
   transaction. That means that the replica thread was not able to apply one of
   the transactions. For each of those file, a corresponding warning or error
   message is present in the mysql error log file. Those error can also be
   false positives like a bad ``DDL`` statement (dropping  a table that doesn't
   exists for example) and therefore nothing to worry about. However it's
   always recommended to check these log to understand what's is happening.

   To be able to analyze these files binlog header needs to be added to the log
   file. To create the ``GRA_HEADER`` file you need an instance running with
   :variable:`binlog_checksum` set to ``NONE`` and extract first 120 bytes from
   the binlog file:

   .. code-block:: bash

      $ head -c 123 mysqld-bin.000001 > GRA_HEADER
      $ cat GRA_HEADER > /var/lib/mysql/GRA_1_2-bin.log
      $ cat /var/lib/mysql/GRA_1_2.log >> /var/lib/mysql/GRA_1_2-bin.log
      $ mysqlbinlog -vvv /var/lib/mysql/GRA_1_2-bin.log

      /*!50530 SET @@SESSION.PSEUDO_SLAVE_MODE=1*/;
      /*!50003 SET @OLD_COMPLETION_TYPE=@@COMPLETION_TYPE,COMPLETION_TYPE=0*/;
      DELIMITER /*!*/;
      # at 4
      #160809  16:04:05 server id 3  end_log_pos 123     Start: binlog v 4, server v 8.0-log created 160809 16:04:05 at startup
      # Warning: this binlog is either in use or was not closed properly.
      ROLLBACK/*!*/;
      BINLOG '
      nbGpVw8DAAAAdwAAAHsAAAABAAQANS43LjEyLTVyYzEtbG9nAAAAAAAAAAAAAAAAAAAAAAAAAAAA
      AAAAAAAAAAAAAAAAAACdsalXEzgNAAgAEgAEBAQEEgAAXwAEGggAAAAICAgCAAAACgoKKioAEjQA
      ALfQ8hw=
      '/*!*/;
      # at 123
      #160809  16:05:49 server id 2  end_log_pos 75     Query    thread_id=11    exec_time=0    error_code=0
      use `test`/*!*/;
      SET TIMESTAMP=1470738949/*!*/;
      SET @@session.pseudo_thread_id=11/*!*/;
      SET @@session.foreign_key_checks=1, @@session.sql_auto_is_null=0, @@session.unique_checks=1, @@session.autocommit=1/*!*/;
      SET @@session.sql_mode=1436549152/*!*/;
      SET @@session.auto_increment_increment=1, @@session.auto_increment_offset=1/*!*/;
      /*!\C utf8 *//*!*/;
      SET @@session.character_set_client=33,@@session.collation_connection=33,@@session.collation_server=8/*!*/;
      SET @@session.lc_time_names=0/*!*/;
      SET @@session.collation_database=DEFAULT/*!*/;
      drop table t
      /*!*/;
      SET @@SESSION.GTID_NEXT= 'AUTOMATIC' /* added by mysqlbinlog */ /*!*/;
      DELIMITER ;
      # End of log file
      /*!50003 SET COMPLETION_TYPE=@OLD_COMPLETION_TYPE*/;
      /*!50530 SET @@SESSION.PSEUDO_SLAVE_MODE=0*/;

   This information can be used for checking the MySQL error log for the corresponding error message. ::

     160805  9:33:37 8:52:21 [ERROR] Slave SQL: Error 'Unknown table 'test'' on query. Default database: 'test'. Query: 'drop table test', Error_code: 1051
     160805  9:33:37 8:52:21 [Warning] WSREP: RBR event 1 Query apply warning: 1, 3

   In this example ``DROP TABLE`` statement was executed on a table that doesn't exist.


* :file:`gcache.page`

  See :variable:`gcache.page_size`  

  .. seealso::

     Percona Database Performance Blog: All You Need to Know About GCache (Galera-Cache)
        https://www.percona.com/blog/2016/11/16/all-you-need-to-know-about-gcache-galera-cache/

.. _galera.cache: galera_cache

* :file:`galera.cache`
   This file is used as a main writeset store. It's implemented as a permanent
   ring-buffer file that is preallocated on disk when the node is initialized.
   File size can be controlled with the variable :variable:`gcache.size`. If
   this value is bigger, more writesets are cached and chances are better that
   the re-joining node will get IST instead of SST. Filename can be changed
   with the :variable:`gcache.name` variable.

.. _galera-grastate-dat:
   
* :file:`grastate.dat`
   This file contains the Galera state information.

  * ``version`` - grastate version
  * ``uuid`` - a unique identifier for the state and the sequence of changes it
    undergoes.For more information on how UUID is generated see :term:`UUID`.
  * ``seqno`` - Ordinal Sequence Number, a 64-bit signed integer used to denote
    the position of the change in the sequence. ``seqno`` is ``0`` when no
    writesets have been generated or applied on that node, i.e., not
    applied/generated across the lifetime of a :file:`grastate` file. ``-1`` is
    a special value for the ``seqno`` that is kept in the :file:`grastate.dat`
    while the server is running to allow Galera to distinguish between a clean
    and an unclean shutdown. Upon a clean shutdown, the correct ``seqno`` value
    is written to the file. So, when the server is brought back up, if the
    value is still ``-1`` , this means that the server did not shut down
    cleanly. If the value is greater than ``0``, this means that the shutdown
    was clean. ``-1`` is then written again to the file in order to allow the
    server to correctly detect if the next shutdown was clean in the same
    manner.
  * ``cert_index`` - cert index restore through grastate is not implemented yet

  Examples of this file look like this:

  In case server node has this state when not running it means that that node
  crashed during the transaction processing. ::

    # GALERA saved state
    version: 2.1
    uuid:    1917033b-7081-11e2-0800-707f5d3b106b
    seqno:   -1
    cert_index:

  In case server node has this state when not running it means that the node
  was gracefully shut down. ::

    # GALERA saved state
    version: 2.1
    uuid:    1917033b-7081-11e2-0800-707f5d3b106b
    seqno:   5192193423942
    cert_index:

  In case server node has this state when not running it means that the node
  crashed during the DDL. ::

    # GALERA saved state
    version: 2.1
    uuid:    00000000-0000-0000-0000-000000000000
    seqno:   -1
    cert_index:

* :file:`gvwstate.dat`
  This file is used for Primary Component recovery feature. This file is
  created once primary component is formed or changed, so you can get the
  latest primary component this node was in. And this file is deleted when the
  node is shutdown gracefully.

  First part contains the node :term:`UUID` information. Second part contains
  the view information. View information is written between ``#vwbeg`` and
  ``#vwend``. View information consists of:

 - view_id: [view_type] [view_uuid] [view_seq]. - ``view_type`` is always ``3``
   which means primary view. ``view_uuid`` and ``view_seq`` identifies a unique
   view, which could be perceived as identifier of this primary component.

 - bootstrap: [bootstarp_or_not]. - It could be ``0`` or ``1``, but it does not
   affect primary component recovery process now.

 - member: [node's uuid] [node's segment]. - it represents all nodes in this
   primary component.

   Example of this file looks like this: ::

    my_uuid: c5d5d990-30ee-11e4-aab1-46d0ed84b408
    #vwbeg
    view_id: 3 bc85bd53-31ac-11e4-9895-1f2ce13f2542 2
    bootstrap: 0
    member: bc85bd53-31ac-11e4-9895-1f2ce13f2542 0
    member: c5d5d990-30ee-11e4-aab1-46d0ed84b408 0
    #vwend
