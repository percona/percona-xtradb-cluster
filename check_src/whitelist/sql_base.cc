<   ulong binlog_format= thd->variables.binlog_format;
<   if ((log_on == FALSE) || (WSREP_BINLOG_FORMAT(binlog_format) == BINLOG_FORMAT_ROW) ||
<       (table_list->table->s->table_category == TABLE_CATEGORY_LOG) ||
>   if (log_on == false ||
>       thd->variables.binlog_format == BINLOG_FORMAT_ROW)
>     return TL_READ;
> 
>   if ((table_list->table->s->table_category == TABLE_CATEGORY_LOG) ||
