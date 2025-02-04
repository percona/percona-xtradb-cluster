<             then need the cache to be at position 0 (see comments at beginning
<             of init_info()). b) init_relay_log_pos(), because the BEGIN may be
<             an older relay log.
>           then need the cache to be at position 0 (see comments at beginning of
>           init_info()).
>           b) init_relay_log_pos(), because the BEGIN may be an older relay log.
<           DBUG_PRINT("info",
<                      ("Resetting retry counter, rli->trans_retries: %lu",
>         DBUG_PRINT("info", ("Resetting retry counter, rli->trans_retries: %lu",
