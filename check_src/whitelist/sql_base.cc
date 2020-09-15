<   ulong binlog_format= thd->variables.binlog_format;
<   if (log_on == FALSE || 
<       (WSREP_BINLOG_FORMAT(binlog_format) == BINLOG_FORMAT_ROW))
>   if (log_on == false ||
>       thd->variables.binlog_format == BINLOG_FORMAT_ROW)
