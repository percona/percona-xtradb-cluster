<     if (WSREP_BINLOG_FORMAT(variables.binlog_format) != BINLOG_FORMAT_ROW && tables)
>     if (variables.binlog_format != BINLOG_FORMAT_ROW && tables)
<       else if (WSREP_BINLOG_FORMAT(variables.binlog_format) == BINLOG_FORMAT_ROW &&
>       else if (variables.binlog_format == BINLOG_FORMAT_ROW &&
<       if (WSREP_BINLOG_FORMAT(variables.binlog_format) == BINLOG_FORMAT_STMT)
>       if (variables.binlog_format == BINLOG_FORMAT_STMT)
<                         WSREP_BINLOG_FORMAT(variables.binlog_format),
>                         variables.binlog_format,
