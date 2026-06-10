#!/bin/bash

# Some notes about this script:
# 1. When started as suites-groups.sh check <path to mysql-test-run.pl> it checks if there are any incosistencies
#    between suites specified in mysql-test-run.pl and in this script
# 2. Jenkins pipeline checks for inconsistencies (1) and then sources this script to learn about suites split
# 3. The default split is defined in ./jenkins/suites-groups.sh
# 4. The default can be overrode by mysql-test/suites-groups.sh if the file is present. This allows one to define custom suites
#    split on development branch
# 5. Jenkins pipeline fails if inconsistencies are detected while using the default split (3)
# 6. Jenkins pipeline continues with warning if inconsistencies are detected while using the custom split (4)
# 7. Jenkins scripts support following suite formats:
#
#    main       - all tests will be allowed to be executed (big and no-big). Note that the final decision belongs to --big-tests MTR parameter
#    main|nobig - only no-big tests are allowed
#    main|big   - only big tests are allowed
#
#    Such approach makes it possible to split the suite execution among two workers, where one woker executes no-big test
#    and another executes only bit tests.


# usage: set_suites <BUILD_TYPE>
function set_suites() {
<<<<<<< HEAD
  if [[ "$1" == "RelWithDebInfo" ]]; then
    echo "Setting WORKER_x_MTR_SUITES for BUILD_TYPE=RelWithDebInfo"
    # Unit tests will be executed by worker 1
    WORKER_1_MTR_SUITES="innodb_undo,test_services,audit_null,service_sys_var_registration,connection_control,service_udf_registration,service_status_var_registration,procfs,interactive_utilities,percona-pam-for-mysql,galera_3nodes"
    WORKER_2_MTR_SUITES="galera_nbo,galera_3nodes_nbo,galera_3nodes_sr,galera_encryption,wsrep,galera-x"
    WORKER_3_MTR_SUITES="engines/funcs,innodb,galera_sr"
    WORKER_4_MTR_SUITES="main,rpl"
    WORKER_5_MTR_SUITES="rpl_nogtid,rpl_gtid,galera|big"
    WORKER_6_MTR_SUITES="parts,group_replication,clone,innodb_gis"
    WORKER_7_MTR_SUITES="stress,perfschema,component_keyring_file,binlog,innodb_fts,sys_vars,innodb_zip,x,gcol,engines/iuds,encryption,federated,funcs_1,auth_sec,binlog_nogtid,binlog_gtid,funcs_2,jp,information_schema,rpl_encryption,sysschema,json,opt_trace,collations,gis,query_rewrite_plugins,test_service_sql_api,secondary_engine,component_audit_log_filter,component_encryption_udf,percona,percona_innodb,component_masking_functions"
    WORKER_8_MTR_SUITES="galera|nobig,audit_log,component_js_lang"
  else # Debug (and everything different from "RelWithDebInfo")
    echo "Setting WORKER_x_MTR_SUITES for BUILD_TYPE=Debug"
    # Unit tests will be executed by worker 1
    WORKER_1_MTR_SUITES="innodb_undo,test_services,audit_null,service_sys_var_registration,connection_control,service_udf_registration,service_status_var_registration,procfs,interactive_utilities,percona-pam-for-mysql,binlog,innodb_zip,x,sys_vars,innodb_fts,stress"
    WORKER_2_MTR_SUITES="galera_nbo,galera_3nodes,galera_sr,galera_3nodes_nbo,galera_3nodes_sr,galera_encryption,wsrep,galera-x,rpl|nobig"
    WORKER_3_MTR_SUITES="engines/funcs,innodb,perfschema,percona_innodb,component_keyring_file"
    WORKER_4_MTR_SUITES="main|big,rpl|big"
    WORKER_5_MTR_SUITES="rpl_nogtid,rpl_gtid,galera|big,group_replication|nobig"
    WORKER_6_MTR_SUITES="parts,group_replication|big,innodb_gis"
    WORKER_7_MTR_SUITES="clone,gcol,engines/iuds,encryption,federated,funcs_1,auth_sec,binlog_nogtid,binlog_gtid,funcs_2,jp,information_schema,rpl_encryption,sysschema,json,opt_trace,collations,gis,query_rewrite_plugins,test_service_sql_api,secondary_engine,component_audit_log_filter,component_encryption_udf,percona,component_masking_functions"
    WORKER_8_MTR_SUITES="galera|nobig,main|nobig,audit_log,component_js_lang"
||||||| 42cc0cf2bd6
  # Comparing to 8.0 added in 8.4: component_audit_log_filter, percona_innodb, component_js_lang
  # Comparing to 8.0 removed from 8.4: data_masking, binlog_57_decryption, audit_log_filter
  if [[ "$1" == "Valgrind" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with Valgrind (a custom suite split)"
    WORKER_1_MTR_SUITES="rpl|nobig,percona_innodb|nobig,perfschema|nobig,clone|nobig,innodb_undo|big,sys_vars|nobig,engines/iuds|big,component_keyring_file|nobig,funcs_1|big,sys_vars|big,gcol|big,funcs_2|nobig,service_udf_registration,interactive_utilities"
    WORKER_2_MTR_SUITES="innodb|big"
    WORKER_3_MTR_SUITES="innodb|nobig,stress|big,innodb_gis|nobig,auth_sec|nobig,innodb_gis|big,opt_trace|nobig,json,gis|nobig,encryption|big,audit_null,procfs,connection_control|big"
    WORKER_4_MTR_SUITES="main|big,rocksdb|big,percona|nobig,percona_innodb|big,audit_log,innodb_fts|nobig,binlog_nogtid|nobig,auth_sec|big,component_masking_functions,query_rewrite_plugins,test_service_sql_api,connection_control|nobig,gis|big,component_js_lang|big"
    WORKER_5_MTR_SUITES="clone|big,rpl_gtid|big,binlog|nobig,component_encryption_udf|big,sysschema|big,parts|big,rpl_encryption,collations,innodb_zip|big,sysschema|nobig,test_services,federated|nobig,service_status_var_registration,percona|big"
    WORKER_6_MTR_SUITES="main|nobig,rpl|big,x|big,innodb_undo|nobig,x|nobig,engines/funcs|nobig,binlog_gtid|nobig,funcs_1|nobig,gcol|nobig,perfschema|big,secondary_engine,information_schema,percona-pam-for-mysql"
    WORKER_7_MTR_SUITES="group_replication|big,component_keyring_file|big,rocksdb|nobig,rpl_nogtid|nobig,innodb_zip|nobig,parts|nobig,innodb_fts|big,binlog_nogtid|big,funcs_2|big,encryption|nobig,jp,rocksdb_sys_vars,component_encryption_udf|nobig,opt_trace|big,component_js_lang|nobig"
    WORKER_8_MTR_SUITES="engines/funcs|big,group_replication|nobig,rpl_nogtid|big,component_audit_log_filter,rpl_gtid|nobig,binlog|big,federated|big,rocksdb_rpl|big,rocksdb_rpl|nobig,binlog_gtid|big,engines/iuds|nobig,service_sys_var_registration,stress|nobig,rocksdb_stress"
  elif [[ "$1" == "RelWithDebInfo" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with BUILD_TYPE=RelWithDebInfo (a custom suite split)"
    WORKER_1_MTR_SUITES="main|nobig,rpl_gtid,rpl_nogtid|big,innodb_gis,binlog_nogtid,binlog_gtid,json,information_schema,service_status_var_registration"
    WORKER_2_MTR_SUITES="group_replication|big,funcs_1,component_masking_functions,collations,gis,percona-pam-for-mysql"
    WORKER_3_MTR_SUITES="rocksdb|big,clone,x|nobig,rocksdb_stress,funcs_2,test_services,encryption,component_js_lang"
    WORKER_4_MTR_SUITES="rocksdb|nobig,innodb|nobig,sys_vars,component_keyring_file,innodb_zip,federated,rpl_encryption,secondary_engine"
    WORKER_5_MTR_SUITES="component_encryption_udf,main|big,parts,x|big,binlog,audit_log,component_audit_log_filter,connection_control,rocksdb_sys_vars"
    WORKER_6_MTR_SUITES="innodb|big,percona_innodb,rocksdb_rpl,engines/funcs,innodb_fts,engines/iuds,service_sys_var_registration,audit_null,procfs"
    WORKER_7_MTR_SUITES="rpl|nobig,rpl_nogtid|nobig,auth_sec,perfschema,gcol,test_service_sql_api,interactive_utilities,jp,service_udf_registration"
    WORKER_8_MTR_SUITES="group_replication|nobig,rpl|big,sysschema,percona,innodb_undo,stress,query_rewrite_plugins,opt_trace"
  else # Debug (and everything different from "RelWithDebInfo" and "Valgrind")
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with BUILD_TYPE=Debug (a custom suite split)"
    WORKER_1_MTR_SUITES="innodb|nobig,rpl|big,perfschema|nobig,component_encryption_udf,clone|nobig,federated,rpl_encryption,collations,audit_null,component_js_lang"
    WORKER_2_MTR_SUITES="rocksdb|big,component_keyring_file|big,percona,x|nobig,component_keyring_file|nobig,engines/iuds,funcs_2,component_masking_functions,interactive_utilities,percona-pam-for-mysql"
    WORKER_3_MTR_SUITES="clone|big,percona_innodb|big,innodb_undo|big,audit_log,component_audit_log_filter,rocksdb_rpl|big,innodb_zip|big,opt_trace,test_services,jp,service_udf_registration"
    WORKER_4_MTR_SUITES="group_replication|big,rpl_nogtid|nobig,innodb_undo|nobig,binlog|big,rpl_nogtid|big,perfschema|big,binlog_nogtid|nobig,innodb_zip|nobig,gis,information_schema"
    WORKER_5_MTR_SUITES="main|big,percona_innodb|nobig,rpl_gtid|nobig,gcol,sysschema|nobig,parts|nobig,rocksdb_stress,rocksdb_sys_vars,connection_control,procfs"
    WORKER_6_MTR_SUITES="main|nobig,group_replication|nobig,engines/funcs,rpl_gtid|big,innodb_fts|nobig,auth_sec|big,auth_sec|nobig,binlog_nogtid|big,service_sys_var_registration,service_status_var_registration"
    WORKER_7_MTR_SUITES="innodb|big,innodb_gis,innodb_fts|big,sys_vars,rocksdb_rpl|nobig,sysschema|big,stress,binlog_gtid|big,test_service_sql_api,secondary_engine"
    WORKER_8_MTR_SUITES="rpl|nobig,rocksdb|nobig,parts|big,binlog|nobig,encryption,funcs_1,x|big,binlog_gtid|nobig,json,query_rewrite_plugins"
=======
  # Comparing to 8.0 added in 8.4: component_audit_log_filter, percona_innodb, component_js_lang
  # Comparing to 8.0 removed from 8.4: data_masking, binlog_57_decryption, audit_log_filter
  if [[ "$1" == "Valgrind" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with Valgrind (a custom suite split)"
    WORKER_1_MTR_SUITES="rpl|nobig,percona_rpl|nobig,percona_innodb|nobig,perfschema|nobig,clone|nobig,innodb_undo|big,sys_vars|nobig,engines/iuds|big,component_keyring_file|nobig,funcs_1|big,sys_vars|big,gcol|big,funcs_2|nobig,service_udf_registration,interactive_utilities"
    WORKER_2_MTR_SUITES="innodb|big"
    WORKER_3_MTR_SUITES="innodb|nobig,stress|big,innodb_gis|nobig,auth_sec|nobig,innodb_gis|big,opt_trace|nobig,json,gis|nobig,encryption|big,audit_null,procfs,connection_control|big"
    WORKER_4_MTR_SUITES="main|big,rocksdb|big,percona|nobig,percona_innodb|big,audit_log,innodb_fts|nobig,binlog_nogtid|nobig,auth_sec|big,component_masking_functions,query_rewrite_plugins,test_service_sql_api,connection_control|nobig,gis|big,component_js_lang|big"
    WORKER_5_MTR_SUITES="clone|big,rpl_gtid|big,percona_rpl_gtid|big,binlog|nobig,percona_binlog|nobig,component_encryption_udf|big,sysschema|big,parts|big,rpl_encryption,collations,innodb_zip|big,sysschema|nobig,test_services,federated|nobig,service_status_var_registration,percona|big"
    WORKER_6_MTR_SUITES="main|nobig,rpl|big,percona_rpl|big,x|big,innodb_undo|nobig,x|nobig,engines/funcs|nobig,binlog_gtid|nobig,funcs_1|nobig,gcol|nobig,perfschema|big,secondary_engine,information_schema,percona-pam-for-mysql"
    WORKER_7_MTR_SUITES="group_replication|big,component_keyring_file|big,rocksdb|nobig,rpl_nogtid|nobig,innodb_zip|nobig,parts|nobig,innodb_fts|big,binlog_nogtid|big,funcs_2|big,encryption|nobig,jp,rocksdb_sys_vars,component_encryption_udf|nobig,opt_trace|big,component_js_lang|nobig"
    WORKER_8_MTR_SUITES="engines/funcs|big,group_replication|nobig,rpl_nogtid|big,component_audit_log_filter,rpl_gtid|nobig,percona_rpl_gtid|nobig,binlog|big,percona_binlog|big,federated|big,rocksdb_rpl|big,rocksdb_rpl|nobig,binlog_gtid|big,engines/iuds|nobig,service_sys_var_registration,stress|nobig,rocksdb_stress"
  elif [[ "$1" == "RelWithDebInfo" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with BUILD_TYPE=RelWithDebInfo (a custom suite split)"
    WORKER_1_MTR_SUITES="main|nobig,rpl_gtid,percona_rpl_gtid,rpl_nogtid|big,innodb_gis,binlog_nogtid,binlog_gtid,json,information_schema,service_status_var_registration"
    WORKER_2_MTR_SUITES="group_replication|big,funcs_1,component_masking_functions,collations,gis,percona-pam-for-mysql"
    WORKER_3_MTR_SUITES="rocksdb|big,clone,x|nobig,rocksdb_stress,funcs_2,test_services,encryption,component_js_lang"
    WORKER_4_MTR_SUITES="rocksdb|nobig,innodb|nobig,sys_vars,component_keyring_file,innodb_zip,federated,rpl_encryption,secondary_engine"
    WORKER_5_MTR_SUITES="component_encryption_udf,main|big,parts,x|big,binlog,percona_binlog,audit_log,component_audit_log_filter,connection_control,rocksdb_sys_vars"
    WORKER_6_MTR_SUITES="innodb|big,percona_innodb,rocksdb_rpl,engines/funcs,innodb_fts,engines/iuds,service_sys_var_registration,audit_null,procfs"
    WORKER_7_MTR_SUITES="rpl|nobig,percona_rpl|nobig,rpl_nogtid|nobig,auth_sec,perfschema,gcol,test_service_sql_api,interactive_utilities,jp,service_udf_registration"
    WORKER_8_MTR_SUITES="group_replication|nobig,rpl|big,percona_rpl|big,sysschema,percona,innodb_undo,stress,query_rewrite_plugins,opt_trace"
  else # Debug (and everything different from "RelWithDebInfo" and "Valgrind")
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 8.4 with BUILD_TYPE=Debug (a custom suite split)"
    WORKER_1_MTR_SUITES="innodb|nobig,rpl|big,percona_rpl|big,perfschema|nobig,component_encryption_udf,clone|nobig,federated,rpl_encryption,collations,audit_null,component_js_lang"
    WORKER_2_MTR_SUITES="rocksdb|big,component_keyring_file|big,percona,x|nobig,component_keyring_file|nobig,engines/iuds,funcs_2,component_masking_functions,interactive_utilities,percona-pam-for-mysql"
    WORKER_3_MTR_SUITES="clone|big,percona_innodb|big,innodb_undo|big,audit_log,component_audit_log_filter,rocksdb_rpl|big,innodb_zip|big,opt_trace,test_services,jp,service_udf_registration"
    WORKER_4_MTR_SUITES="group_replication|big,rpl_nogtid|nobig,innodb_undo|nobig,binlog|big,percona_binlog|big,rpl_nogtid|big,perfschema|big,binlog_nogtid|nobig,innodb_zip|nobig,gis,information_schema"
    WORKER_5_MTR_SUITES="main|big,percona_innodb|nobig,rpl_gtid|nobig,percona_rpl_gtid|nobig,gcol,sysschema|nobig,parts|nobig,rocksdb_stress,rocksdb_sys_vars,connection_control,procfs"
    WORKER_6_MTR_SUITES="main|nobig,group_replication|nobig,engines/funcs,rpl_gtid|big,percona_rpl_gtid|big,innodb_fts|nobig,auth_sec|big,auth_sec|nobig,binlog_nogtid|big,service_sys_var_registration,service_status_var_registration"
    WORKER_7_MTR_SUITES="innodb|big,innodb_gis,innodb_fts|big,sys_vars,rocksdb_rpl|nobig,sysschema|big,stress,binlog_gtid|big,test_service_sql_api,secondary_engine"
    WORKER_8_MTR_SUITES="rpl|nobig,percona_rpl|nobig,rocksdb|nobig,parts|big,binlog|nobig,percona_binlog|nobig,encryption,funcs_1,x|big,binlog_gtid|nobig,json,query_rewrite_plugins"
>>>>>>> ps/release-8.4.9-9
  fi
}


# usage: check_suites <path_to_mysql-test-run.pl>
function check_suites() {
  INPUT=${1:-./mysql-test-run.pl}

  if [[ ! -f ${INPUT} ]]
    then
    echo "${INPUT} file does not exist on your filesystem."
    return 1
  fi

  echo "Checking if suites list is consistent with the one specified in mysql-test-run.pl"
  echo

  local all_suites_1=,${WORKER_1_MTR_SUITES},${WORKER_2_MTR_SUITES},${WORKER_3_MTR_SUITES},${WORKER_4_MTR_SUITES},${WORKER_5_MTR_SUITES},${WORKER_6_MTR_SUITES},${WORKER_7_MTR_SUITES},${WORKER_8_MTR_SUITES},

  local all_suites_2=
  local capturing=0
  while read -r line
  do
    if [[ "${capturing}" == "1" ]]; then
      if [[ "${line}" == *");"* ]]; then
        capturing=0
        break
      fi
    fi

    if [[ "$capturing" == "1" ]]; then
      local all_suites_2=${all_suites_2}${line},
    fi

    if [[ "${line}" == *"DEFAULT_SUITES = qw"* ]]; then
      capturing=1
    fi

  done < "${INPUT}"

  # add leading and trailing commas for easier parsing
  all_suites_2=,${all_suites_2},

  echo "Suites for Jenkins: ${all_suites_1}"
  echo
  echo "Suites from mysql-test-run.pl: ${all_suites_2}"
  echo

  local failure=0

  # check if splited suite contains both big/nobig parts
  for suite in ${all_suites_1//,/ }
  do
    if [[ ${suite} == *"|"* ]]; then

        arrSuite=(${suite//|/ })
        suite=${arrSuite[0]}
        nobig_found=0
        for suite_nobig in ${all_suites_1//,/ }
        do
          if [[ ${suite_nobig} == "${suite}|nobig" ]]; then
            nobig_found=1
          fi
        done

        big_found=0
        for suite_big in ${all_suites_1//,/ }
        do
          if [[ ${suite_big} == "${suite}|big" ]]; then
            big_found=1
          fi
        done

        if [[ ${nobig_found} == "0" || ${big_found} == "0" ]]; then
          echo "${suite} big|nobig (${big_found}|${nobig_found} mismatch)"
          failure=1
        fi
    fi
  done
  # get rid of bin/nobig before two-way matching
  all_suites_1=${all_suites_1//"|big"/""}
  all_suites_1=${all_suites_1//"|nobig"/""}

  # check if the suite from pl scipt is assigned to any worker
  for suite in ${all_suites_2//,/ }
  do
    if [[ ${all_suites_1} != *",${suite},"* ]]; then
      echo "${suite} specified in mysql-test-run.pl but missing in Jenkins"
      failure=1
    fi
  done

  # check if the suite from pl scipt is assigned to any worker
  for suite in ${all_suites_1//,/ }
  do
    if [[ ${all_suites_2} != *",${suite},"* ]]; then
      echo "${suite} specified in Jenkins but not present in mysql-test-run.pl"
      failure=1
    fi
  done

  echo "************************"
  if [[ "${failure}" == "1" ]]; then
    echo "Inconsitencies detected"
  else
    echo "Everything is OK"
  fi
  echo "************************"

  return ${failure}
}


# This code will be run when this script is included as "source"
if [[ "$BUILD_TYPE" != "" ]]; then
  echo "Using BUILD_TYPE=$BUILD_TYPE"
  set_suites $BUILD_TYPE
fi

case "$1" in
  'check')
    set_suites ${3:-$BUILD_TYPE}
    check_suites $2
    ;;
esac
