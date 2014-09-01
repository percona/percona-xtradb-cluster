.. _wsrep_file_index:

===============================
 Index of files created by PXC
===============================

* :file:`GRA_*.log`
   These files contain binlog events in ROW format representing the failed transaction. That means that the slave thread was not able to apply one of the transactions. For each of those file, a corresponding warning or error message is present in the mysql error log file. Those error can also be false positives like a bad ``DDL`` statement (dropping  a table that doesn't exists for example) and therefore nothing to worry about. However it's always recommended to check these log to understand what's is happening.

   To be able to analyze these files `binlog header <http://www.mysqlperformanceblog.com/wp-content/uploads/2012/12/GRA-header.zip>`_ needs to be added to the log file. :: 
  
      $ cat GRA-header > /var/lib/mysql/GRA_1_2-bin.log
      $ cat /var/lib/mysql/GRA_1_2.log >> /var/lib/mysql/GRA_1_2-bin.log
      $ mysqlbinlog -vvv /var/lib/mysql/GRA_1_2-bin.log 
      /*!40019 SET @@session.max_insert_delayed_threads=0*/;
      /*!50003 SET @OLD_COMPLETION_TYPE=@@COMPLETION_TYPE,COMPLETION_TYPE=0*/;
      DELIMITER /*!*/;
      # at 4
      #120715  9:45:56 server id 1  end_log_pos 107 	Start: binlog v 4, server v 5.5.25-debug-log created 120715  9:45:56 at startup
      # Warning: this binlog is either in use or was not closed properly.
      ROLLBACK/*!*/;
      BINLOG '
      NHUCUA8BAAAAZwAAAGsAAAABAAQANS41LjI1LWRlYnVnLWxvZwAAAAAAAAAAAAAAAAAAAAAAAAAA
      AAAAAAAAAAAAAAAAAAA0dQJQEzgNAAgAEgAEBAQEEgAAVAAEGggAAAAICAgCAA==
      '/*!*/;
      # at 107
      #130226 11:48:50 server id 1  end_log_pos 83 	Query	thread_id=3	exec_time=0	error_code=0
      use `test`/*!*/;
      SET TIMESTAMP=1361875730/*!*/;
      SET @@session.pseudo_thread_id=3/*!*/;
      SET @@session.foreign_key_checks=1, @@session.sql_auto_is_null=0, @@session.unique_checks=1, @@session.autocommit=1/*!*/;
      SET @@session.sql_mode=1437073440/*!*/;
      SET @@session.auto_increment_increment=3, @@session.auto_increment_offset=2/*!*/;
      /*!\C utf8 *//*!*/;
      SET @@session.character_set_client=33,@@session.collation_connection=33,@@session.collation_server=8/*!*/;
      SET @@session.lc_time_names=0/*!*/;
      SET @@session.collation_database=DEFAULT/*!*/;
      drop table test
      /*!*/;
      DELIMITER ;
      # End of log file
      ROLLBACK /* added by mysqlbinlog */;
      /*!50003 SET COMPLETION_TYPE=@OLD_COMPLETION_TYPE*/;

   This information can be used for checking the |MySQL| error log for the corresponding error message. :: 

     130226 11:48:50 [ERROR] Slave SQL: Error 'Unknown table 'test'' on query. Default database: 'test'. Query: 'drop table test', Error_code: 1051
     130226 11:48:50 [Warning] WSREP: RBR event 1 Query apply warning: 1, 3
     130226 11:48:50 [Warning] WSREP: Ignoring error for TO isolated action: source: dd40ad88-7ff9-11e2-0800-e93cbffe93d7 version: 2 local: 0 state: APPLYING flags: 65 conn_id: 3 trx_id: -1 seqnos (l: 5, g: 3, s: 2, d: 2, ts: 1361875730070283555)
  
   In this example ``DROP TABLE`` statement was executed on a table that doesn't exist.

.. _galera.cache: galera_cache

* :file:`galera.cache`
   This file is used as a main writeset store. It's implemented as a permanent ring-buffer file that is preallocated on disk when the node is initialized. File size can be controlled with the variable :variable:`gcache.size`. If this value is bigger, more writesets are cached and chances are better that the re-joining node will get |IST| instead of |SST|. Filename can be changed with the :variable:`gcache.name` variable.
  
* :file:`grastate.dat`
   This file contains the Galera state information.

   Example of this file looks like this: ::
  
    # GALERA saved state
    version: 2.1
    uuid:    1917033b-7081-11e2-0800-707f5d3b106b
    seqno:   -1
    cert_index:

* :file:`gvwstate.dat`
  This file is used for Primary Component recovery feature. This file is created once primary component is formed or changed, so you can get the latest primary component this node was in. And this file is deleted when the node is shutdown gracefully.

  First part contains the node UUID information. Second part contains the view information. View information is written between ``#vwbeg`` and ``#vwend``. View information consists of:

 - view_id: [view_type] [view_uuid] [view_seq]. - ``view_type`` is always ``3`` which means primary view. ``view_uuid`` and ``view_seq`` identifies a unique view, which could be perceived as identifier of this primary component.

 - bootstrap: [bootstarp_or_not]. - It could be ``0`` or ``1``, but it does not affect primary component recovery process now.

 - member: [node's uuid] [node's segment]. - it represents all nodes in this primary component.

   Example of this file looks like this: ::

    my_uuid: c5d5d990-30ee-11e4-aab1-46d0ed84b408
    #vwbeg
    view_id: 3 bc85bd53-31ac-11e4-9895-1f2ce13f2542 2
    bootstrap: 0
    member: bc85bd53-31ac-11e4-9895-1f2ce13f2542 0
    member: c5d5d990-30ee-11e4-aab1-46d0ed84b408 0
    #vwend


