<     if ((WSREP_BINLOG_FORMAT(variables.binlog_format) == BINLOG_FORMAT_MIXED)&&
>     if ((variables.binlog_format == BINLOG_FORMAT_MIXED) &&
<       if (WSREP_BINLOG_FORMAT(variables.binlog_format) == BINLOG_FORMAT_ROW)
>       if (variables.binlog_format == BINLOG_FORMAT_ROW)
< #define CF_SKIP_WSREP_CHECK     0
