<   /* Leave the original location if wsrep is not involved otherwise
<   we do this before initializing WSREP as wsrep needs access to
<   gtid_mode which and for accessing gtid_mode gtid_sid_locks has to be
<   initialized which is done by this function. */
< 
<       LogErr(ERROR_LEVEL,
<              ER_RPL_GTID_MODE_REQUIRES_ENFORCE_GTID_CONSISTENCY_ON);
>     LogErr(ERROR_LEVEL, ER_RPL_GTID_MODE_REQUIRES_ENFORCE_GTID_CONSISTENCY_ON);
<     if (opt_bin_log) {
<       if (binlog_space_limit) mysql_bin_log.purge_logs_by_size(true);
<     } else {
>   if (!opt_bin_log) {
<    "Keyring plugin or component to which the keys are "
<    "migrated to. This option must be specified along with "
<    "--keyring-migration-source.",
>      "Keyring plugin or component to which the keys are migrated to.",
<   {"keyring-migration-socket", OPT_KEYRING_MIGRATION_SOCKET,
<    "The socket file to use for connection.", &opt_keyring_migration_socket,
<    &opt_keyring_migration_socket, nullptr, GET_STR, REQUIRED_ARG, 0, 0, 0,
<    nullptr, 0, nullptr},
<   {"keyring-migration-port", OPT_KEYRING_MIGRATION_PORT,
<    "Port number to use for connection.", &opt_keyring_migration_port,
<    &opt_keyring_migration_port, nullptr, GET_ULONG, REQUIRED_ARG, 0, 0, 0,
<    nullptr, 0, nullptr},
<    &opt_validate_config, &opt_validate_config, nullptr, GET_BOOL, NO_ARG, 0, 0,
<    0, nullptr, 0, nullptr},
>      &opt_validate_config, &opt_validate_config, nullptr, GET_BOOL, NO_ARG, 0,
>      0, 0, nullptr, 0, nullptr},
<    &opt_libcoredumper_path, &opt_libcoredumper_path, 0, GET_STR, OPT_ARG, 0, 0,
<    0, 0, 0, 0},
>      &opt_libcoredumper_path, &opt_libcoredumper_path, 0, GET_STR, OPT_ARG, 0,
>      0, 0, 0, 0, 0},
<   {nullptr, 0, nullptr, nullptr, nullptr, nullptr, GET_NO_ARG, NO_ARG, 0, 0, 0,
<    nullptr, 0, nullptr}
< };
>     {nullptr, 0, nullptr, nullptr, nullptr, nullptr, GET_NO_ARG, NO_ARG, 0, 0,
>      0, nullptr, 0, nullptr}};
<    &slow_start_timeout, &slow_start_timeout, 0, GET_ULONG, REQUIRED_ARG, 15000,
<    0, 0, 0, 0, 0},
>      &slow_start_timeout, &slow_start_timeout, 0, GET_ULONG, REQUIRED_ARG,
>      15000, 0, 0, 0, 0, 0},
<   {"standalone", 0, "Dummy option to start as a standalone program (NT).", 0, 0,
<    0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0},
>     {"standalone", 0, "Dummy option to start as a standalone program (NT).", 0,
>      0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0},
<    &my_enable_symlinks, &my_enable_symlinks, nullptr, GET_BOOL, NO_ARG, 0, 0, 0,
<    nullptr, 0, nullptr},
>      &my_enable_symlinks, &my_enable_symlinks, nullptr, GET_BOOL, NO_ARG, 0, 0,
>      0, nullptr, 0, nullptr},
<    &global_system_variables.sysdate_is_now, nullptr, nullptr, GET_BOOL, NO_ARG,
<    0, 0, 1, nullptr, 1, nullptr},
>      &global_system_variables.sysdate_is_now, nullptr, nullptr, GET_BOOL,
>      NO_ARG, 0, 0, 1, nullptr, 1, nullptr},
<    &tc_heuristic_recover, &tc_heuristic_recover, &tc_heuristic_recover_typelib,
<    GET_ENUM, REQUIRED_ARG, TC_HEURISTIC_NOT_USED, 0, 0, nullptr, 0, nullptr},
>      &tc_heuristic_recover, &tc_heuristic_recover,
>      &tc_heuristic_recover_typelib, GET_ENUM, REQUIRED_ARG,
>      TC_HEURISTIC_NOT_USED, 0, 0, nullptr, 0, nullptr},
<    &opt_debug_sync_timeout, nullptr, nullptr, GET_UINT, OPT_ARG, 0, 0, UINT_MAX,
<    nullptr, 0, nullptr},
>      &opt_debug_sync_timeout, nullptr, nullptr, GET_UINT, OPT_ARG, 0, 0,
>      UINT_MAX, nullptr, 0, nullptr},
<    nullptr, nullptr, nullptr, GET_BOOL, OPT_ARG, 0, 0, 0, nullptr, 0, nullptr},
>      nullptr, nullptr, nullptr, GET_BOOL, OPT_ARG, 0, 0, 0, nullptr, 0,
>      nullptr},
<    &utility_user_dynamic_privileges, 0, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0,
<    0},
>      &utility_user_dynamic_privileges, 0, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0,
>      0, 0},
<    &utility_user_schema_access, 0, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},
>      &utility_user_schema_access, 0, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0,
>      0},
<   {nullptr, 0, nullptr, nullptr, nullptr, nullptr, GET_NO_ARG, NO_ARG, 0, 0, 0,
<    nullptr, 0, nullptr}
< };
>     {nullptr, 0, nullptr, nullptr, nullptr, nullptr, GET_NO_ARG, NO_ARG, 0, 0,
>      0, nullptr, 0, nullptr}};
