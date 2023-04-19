< #if 0
<     /* If the increment step of the auto increment column
<     decreases then it is not affecting the immediate
<     next value in the series. */
<     if (m_prebuilt->autoinc_increment > increment) {
<       current = autoinc - m_prebuilt->autoinc_increment;
< 
<       current = innobase_next_autoinc(current, 1, increment, 1, col_max_value);
< 
<       dict_table_autoinc_initialize(m_prebuilt->table, current);
< 
<       *first_value = current;
<     }
< #endif
< 

