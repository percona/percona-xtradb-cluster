<   /* table can be nullptr if we failed with wsrep_toi_replication() */
<   if (table && table->table) table->table->invalidate_dict();
>   if (table->table) table->table->invalidate_dict();
<   WSREP_NBO_1ST_PHASE_END;
< 
