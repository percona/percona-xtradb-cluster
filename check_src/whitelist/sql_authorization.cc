<     handlerton *db_type = create_info->db_type ? create_info->db_type
<                                                : ha_default_handlerton(thd);
>   handlerton *db_type =
>       create_info->db_type ? create_info->db_type : ha_default_handlerton(thd);

