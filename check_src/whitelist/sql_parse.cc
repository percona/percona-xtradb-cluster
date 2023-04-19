<     case SQLCOM_UNLOCK_TABLES: {
< 
>     case SQLCOM_UNLOCK_TABLES:
<     }
<             tmp->awake(only_kill_query ? THD::KILL_QUERY
<                                        : THD::KILL_CONNECTION);
>           tmp->awake(only_kill_query ? THD::KILL_QUERY : THD::KILL_CONNECTION);
