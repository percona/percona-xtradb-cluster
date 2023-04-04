<   if (ddl_skip_rewrite != print_event_info->ddl_skip_rewrite) {
<     my_b_printf(file, "/*!80026 SET @@session.binlog_ddl_skip_rewrite=%d*/%s\n",
<                 ddl_skip_rewrite, print_event_info->delimiter);
<     print_event_info->ddl_skip_rewrite = ddl_skip_rewrite;
<   }
<       here as the present method does not call dispatch_sql_command().
>       here as the present method does not call mysql_parse().
<       skipped_event_in_transaction(false),
<       ddl_skip_rewrite(false) {
>       skipped_event_in_transaction(false) {
