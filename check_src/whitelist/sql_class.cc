<     return (int) WSREP_BINLOG_FORMAT(thd->variables.binlog_format);
>     return (int) thd->variables.binlog_format;
