<   if (first_table->table->part_info->set_partition_bitmaps(first_table)) {
<     WSREP_NBO_1ST_PHASE_END;
>   if (first_table->table->part_info->set_partition_bitmaps(first_table))
<   }
<     if (thd->mdl_context.upgrade_shared_lock(ticket, MDL_EXCLUSIVE, timeout)) {
<       WSREP_NBO_1ST_PHASE_END;
>     if (thd->mdl_context.upgrade_shared_lock(ticket, MDL_EXCLUSIVE, timeout))
<     }
<           first_table->db, first_table->table_name, &table_def)) {
<     WSREP_NBO_1ST_PHASE_END;
>           first_table->db, first_table->table_name, &table_def))
<   }
< 
<   WSREP_NBO_1ST_PHASE_END;
