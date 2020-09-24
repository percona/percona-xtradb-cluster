if (`SELECT COUNT(*)=0 FROM performance_schema.global_variables WHERE
    VARIABLE_NAME LIKE 'wsrep_provider' AND VARIABLE_VALUE NOT LIKE 'none'`)
{
  --skip Test requires wsrep provider library (libgalera_smm.so). Did you set $WSREP_PROVIDER?
}

