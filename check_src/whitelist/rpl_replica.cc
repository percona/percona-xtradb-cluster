<             then need the cache to be at position 0 (see comments at beginning
<             of init_info()). b) init_relay_log_pos(), because the BEGIN may be
<             an older relay log.
>           then need the cache to be at position 0 (see comments at beginning of
>           init_info()).
>           b) init_relay_log_pos(), because the BEGIN may be an older relay log.
<           DBUG_PRINT("info",
<                      ("Resetting retry counter, rli->trans_retries: %lu",
>         DBUG_PRINT("info", ("Resetting retry counter, rli->trans_retries: %lu",
<   // Replicas shall not create GIPKs if source tables have no PKs
<     rli->report(WARNING_LEVEL, 0,
>       rli->report(
>           WARNING_LEVEL, 0,
<            llstr(rli->get_group_master_log_pos(), llbuff),
>              llstr(rli->get_group_master_log_pos_info(), llbuff),
<            llstr(rli->get_group_master_log_pos(), llbuff),
>              llstr(rli->get_group_master_log_pos_info(), llbuff),
<            llstr(rli->get_group_master_log_pos(), llbuff));
>              llstr(rli->get_group_master_log_pos_info(), llbuff));
<            llstr(rli->get_group_master_log_pos(), llbuff));
< 
>              llstr(rli->get_group_master_log_pos_info(), llbuff));
<   DBUG_EXECUTE_IF("simulate_replica_delay_at_terminate_bug38694",
<                   sleep(5););
>     DBUG_EXECUTE_IF("simulate_replica_delay_at_terminate_bug38694", sleep(5););
