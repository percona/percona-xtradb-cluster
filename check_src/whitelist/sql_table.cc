<           if (thd->variables.binlog_ddl_skip_rewrite) {
>           if (thd->variables.binlog_ddl_skip_rewrite ||
>               thd->system_thread == SYSTEM_THREAD_SLAVE_SQL ||
>               thd->system_thread == SYSTEM_THREAD_SLAVE_WORKER ||
>               thd->is_binlog_applier()) {
<       if (thd->variables.binlog_ddl_skip_rewrite) {
>       if (thd->variables.binlog_ddl_skip_rewrite ||
>           thd->system_thread == SYSTEM_THREAD_SLAVE_SQL ||
>           thd->system_thread == SYSTEM_THREAD_SLAVE_WORKER ||
>           thd->is_binlog_applier()) {
<       4. WSREP tables
