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
    WORKER_8_MTR_SUITES="galera|nobig,component_connection_control,jdv,component_js_lang"
  else # Debug (and everything different from "RelWithDebInfo")
    echo "Setting WORKER_x_MTR_SUITES for BUILD_TYPE=Debug"
    # Unit tests will be executed by worker 1
    WORKER_1_MTR_SUITES="innodb_undo,test_services,audit_null,service_sys_var_registration,connection_control,service_udf_registration,service_status_var_registration,procfs,interactive_utilities,percona-pam-for-mysql,binlog,innodb_zip,x,sys_vars,innodb_fts,stress"
    WORKER_2_MTR_SUITES="galera_nbo,galera_3nodes,galera_sr,galera_3nodes_nbo,galera_3nodes_sr,galera_encryption,wsrep,galera-x,rpl|nobig"
    WORKER_3_MTR_SUITES="engines/funcs,innodb,perfschema,percona_innodb,component_keyring_file"
    WORKER_4_MTR_SUITES="main|big,rpl|big,rpl_nogtid"
    WORKER_5_MTR_SUITES="rpl_gtid,galera|big,group_replication|nobig"
    WORKER_6_MTR_SUITES="parts,group_replication|big,clone|nobig,innodb_gis"
    WORKER_7_MTR_SUITES="clone|big,gcol,engines/iuds,encryption,federated,funcs_1,auth_sec,binlog_nogtid,binlog_gtid,funcs_2,jp,information_schema,rpl_encryption,sysschema,json,opt_trace,collations,gis,query_rewrite_plugins,test_service_sql_api,secondary_engine,component_audit_log_filter,component_encryption_udf,percona,component_masking_functions"
    WORKER_8_MTR_SUITES="galera|nobig,main|nobig,component_connection_control,jdv,component_js_lang"
||||||| merged common ancestors
  # Comparing to 8.4 added in 9.4: component_connection_control, jdv
  if [[ "$1" == "Valgrind" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with Valgrind (a custom suite split)"
    WORKER_1_MTR_SUITES="router,rpl_gtid|big,percona|nobig,component_audit_log_filter,rpl_nogtid|nobig,sysschema|big,innodb_zip|big,rpl_encryption,rocksdb_rpl|nobig,jp,service_sys_var_registration,audit_null,rocksdb_stress,component_js_lang|big"
    WORKER_2_MTR_SUITES="innodb|nobig,innodb_fts|big,funcs_1|big,encryption|nobig,component_masking_functions,component_encryption_udf|nobig,component_connection_control|nobig,procfs,encryption|big"
    WORKER_3_MTR_SUITES="main|big,binlog|nobig,innodb_zip|nobig,engines/funcs|nobig,innodb_gis|big,parts|big,binlog_nogtid|nobig,collations,sysschema|nobig,rocksdb_sys_vars,connection_control|nobig,jdv"
    WORKER_4_MTR_SUITES="innodb|big,group_replication|nobig,rpl_gtid|nobig,binlog|big,innodb_fts|nobig,binlog_nogtid|big,funcs_1|nobig,innodb_gis|nobig,perfschema|big,federated|nobig,gis|big"
    WORKER_5_MTR_SUITES="main|nobig,rpl|big,rpl_nogtid|big,component_encryption_udf|big,x|nobig,binlog_gtid|nobig,component_keyring_file|nobig,funcs_2|big,test_services,binlog_gtid|big,stress|nobig,information_schema,component_js_lang|nobig,component_connection_control|big"
    WORKER_6_MTR_SUITES="group_replication|big,percona_innodb|nobig,rocksdb|big,innodb_undo|nobig,stress|big,x|big,auth_sec|nobig,rocksdb_rpl|big,gcol|nobig,query_rewrite_plugins,secondary_engine,interactive_utilities,connection_control|big,percona|big"
    WORKER_7_MTR_SUITES="rpl|nobig,innodb_undo|big,perfschema|nobig,component_keyring_file|big,percona_innodb|big,sys_vars|nobig,auth_sec|big,opt_trace|nobig,json,engines/iuds|nobig,test_service_sql_api,service_status_var_registration,opt_trace|big"
    WORKER_8_MTR_SUITES="engines/funcs|big,clone|big,rocksdb|nobig,clone|nobig,parts|nobig,federated|big,engines/iuds|big,gis|nobig,sys_vars|big,gcol|big,funcs_2|nobig,service_udf_registration,percona-pam-for-mysql"
  elif [[ "$1" == "RelWithDebInfo" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with BUILD_TYPE=RelWithDebInfo (a custom suite split)"
    WORKER_1_MTR_SUITES="main|nobig,main|big,parts,percona,component_audit_log_filter,engines/iuds,component_connection_control,opt_trace,information_schema"
    WORKER_2_MTR_SUITES="component_encryption_udf,innodb_zip,gis,json,component_js_lang,router"
    WORKER_3_MTR_SUITES="group_replication|big,binlog,test_service_sql_api,component_masking_functions,connection_control,service_sys_var_registration,jdv"
    WORKER_4_MTR_SUITES="rocksdb|big,rpl_gtid,auth_sec,engines/funcs,innodb_undo,interactive_utilities,query_rewrite_plugins,audit_null"
    WORKER_5_MTR_SUITES="rocksdb|nobig,percona_innodb,sys_vars,perfschema,innodb_fts,binlog_gtid,rpl_encryption,service_udf_registration,procfs"
    WORKER_6_MTR_SUITES="rpl|nobig,rpl_nogtid|big,rocksdb_rpl,clone,innodb_gis,funcs_1,funcs_2,collations,jp,percona-pam-for-mysql"
    WORKER_7_MTR_SUITES="group_replication|nobig,rpl_nogtid|nobig,innodb|nobig,component_keyring_file,rocksdb_stress,gcol,stress,test_services,secondary_engine,service_status_var_registration"
    WORKER_8_MTR_SUITES="innodb|big,rpl|big,sysschema,x|big,x|nobig,binlog_nogtid,federated,rocksdb_sys_vars,encryption"
  else # Debug (and everything different from "RelWithDebInfo" and "Valgrind")
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with BUILD_TYPE=Debug (a custom suite split)"
    WORKER_1_MTR_SUITES="rocksdb|nobig,rpl|big,innodb_undo|nobig,percona,sysschema|nobig,x|big,binlog_nogtid|nobig,encryption,service_sys_var_registration,audit_null,router"
    WORKER_2_MTR_SUITES="clone|big,component_keyring_file|big,perfschema|nobig,rocksdb_rpl|nobig,clone|nobig,innodb_zip|big,rocksdb_stress,binlog_nogtid|big,query_rewrite_plugins,jdv"
    WORKER_3_MTR_SUITES="rocksdb|big,rpl_nogtid|nobig,percona_innodb|big,component_audit_log_filter,sysschema|big,parts|nobig,rpl_encryption,component_masking_functions,jp,information_schema"
    WORKER_4_MTR_SUITES="group_replication|big,percona_innodb|nobig,binlog|nobig,rpl_nogtid|big,rocksdb_rpl|big,federated,innodb_gis|nobig,collations,test_services,component_js_lang"
    WORKER_5_MTR_SUITES="main|big,innodb_gis|big,rpl_gtid|big,binlog|big,funcs_1,auth_sec|big,binlog_gtid|nobig,opt_trace,secondary_engine,service_status_var_registration"
    WORKER_6_MTR_SUITES="main|nobig,group_replication|nobig,parts|big,innodb_undo|big,innodb_fts|nobig,perfschema|big,engines/iuds,innodb_zip|nobig,rocksdb_sys_vars,component_connection_control,service_udf_registration"
    WORKER_7_MTR_SUITES="innodb|big,rpl|nobig,engines/funcs,sys_vars,x|nobig,component_keyring_file|nobig,stress,funcs_2,json,interactive_utilities,procfs"
    WORKER_8_MTR_SUITES="component_encryption_udf,innodb|nobig,innodb_fts|big,rpl_gtid|nobig,gcol,gis,auth_sec|nobig,binlog_gtid|big,test_service_sql_api,connection_control,percona-pam-for-mysql"
=======
  # Comparing to 8.4 added in 9.4: component_connection_control, jdv
  if [[ "$1" == "Valgrind" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with Valgrind (a custom suite split)"
    WORKER_1_MTR_SUITES="router,rpl_gtid|big,percona_rpl_gtid|big,percona|nobig,component_audit_log_filter,rpl_nogtid|nobig,sysschema|big,innodb_zip|big,rpl_encryption,rocksdb_rpl|nobig,jp,service_sys_var_registration,audit_null,rocksdb_stress,component_js_lang|big"
    WORKER_2_MTR_SUITES="innodb|nobig,innodb_fts|big,funcs_1|big,encryption|nobig,component_masking_functions,component_encryption_udf|nobig,component_connection_control|nobig,procfs,encryption|big"
    WORKER_3_MTR_SUITES="main|big,binlog|nobig,percona_binlog|nobig,innodb_zip|nobig,engines/funcs|nobig,innodb_gis|big,parts|big,binlog_nogtid|nobig,collations,sysschema|nobig,rocksdb_sys_vars,connection_control|nobig,jdv"
    WORKER_4_MTR_SUITES="innodb|big,group_replication|nobig,rpl_gtid|nobig,percona_rpl_gtid|nobig,binlog|big,percona_binlog|big,innodb_fts|nobig,binlog_nogtid|big,funcs_1|nobig,innodb_gis|nobig,perfschema|big,federated|nobig,gis|big"
    WORKER_5_MTR_SUITES="main|nobig,rpl|big,percona_rpl|big,rpl_nogtid|big,component_encryption_udf|big,x|nobig,binlog_gtid|nobig,component_keyring_file|nobig,funcs_2|big,test_services,binlog_gtid|big,stress|nobig,information_schema,component_js_lang|nobig,component_connection_control|big"
    WORKER_6_MTR_SUITES="group_replication|big,percona_innodb|nobig,rocksdb|big,innodb_undo|nobig,stress|big,x|big,auth_sec|nobig,rocksdb_rpl|big,gcol|nobig,query_rewrite_plugins,secondary_engine,interactive_utilities,connection_control|big,percona|big"
    WORKER_7_MTR_SUITES="rpl|nobig,percona_rpl|nobig,innodb_undo|big,perfschema|nobig,component_keyring_file|big,percona_innodb|big,sys_vars|nobig,auth_sec|big,opt_trace|nobig,json,engines/iuds|nobig,test_service_sql_api,service_status_var_registration,opt_trace|big"
    WORKER_8_MTR_SUITES="engines/funcs|big,clone|big,rocksdb|nobig,clone|nobig,parts|nobig,federated|big,engines/iuds|big,gis|nobig,sys_vars|big,gcol|big,funcs_2|nobig,service_udf_registration,percona-pam-for-mysql"
  elif [[ "$1" == "RelWithDebInfo" ]]; then
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with BUILD_TYPE=RelWithDebInfo (a custom suite split)"
    WORKER_1_MTR_SUITES="main|nobig,main|big,parts,percona,component_audit_log_filter,engines/iuds,component_connection_control,opt_trace,information_schema"
    WORKER_2_MTR_SUITES="component_encryption_udf,innodb_zip,gis,json,component_js_lang,router"
    WORKER_3_MTR_SUITES="group_replication|big,binlog,percona_binlog,test_service_sql_api,component_masking_functions,connection_control,service_sys_var_registration,jdv"
    WORKER_4_MTR_SUITES="rocksdb|big,rpl_gtid,percona_rpl_gtid,auth_sec,engines/funcs,innodb_undo,interactive_utilities,query_rewrite_plugins,audit_null"
    WORKER_5_MTR_SUITES="rocksdb|nobig,percona_innodb,sys_vars,perfschema,innodb_fts,binlog_gtid,rpl_encryption,service_udf_registration,procfs"
    WORKER_6_MTR_SUITES="rpl|nobig,percona_rpl|nobig,rpl_nogtid|big,rocksdb_rpl,clone,innodb_gis,funcs_1,funcs_2,collations,jp,percona-pam-for-mysql"
    WORKER_7_MTR_SUITES="group_replication|nobig,rpl_nogtid|nobig,innodb|nobig,component_keyring_file,rocksdb_stress,gcol,stress,test_services,secondary_engine,service_status_var_registration"
    WORKER_8_MTR_SUITES="innodb|big,rpl|big,percona_rpl|big,sysschema,x|big,x|nobig,binlog_nogtid,federated,rocksdb_sys_vars,encryption"
  else # Debug (and everything different from "RelWithDebInfo" and "Valgrind")
    # Unit tests, KEYRING_VAULT tests, ps_protocol, ci_fs will be executed by worker 1
    echo "Setting WORKER_x_MTR_SUITES for PS 9.x with BUILD_TYPE=Debug (a custom suite split)"
    WORKER_1_MTR_SUITES="rocksdb|nobig,rpl|big,percona_rpl|big,innodb_undo|nobig,percona,sysschema|nobig,x|big,binlog_nogtid|nobig,encryption,service_sys_var_registration,audit_null,router"
    WORKER_2_MTR_SUITES="clone|big,component_keyring_file|big,perfschema|nobig,rocksdb_rpl|nobig,clone|nobig,innodb_zip|big,rocksdb_stress,binlog_nogtid|big,query_rewrite_plugins,jdv"
    WORKER_3_MTR_SUITES="rocksdb|big,rpl_nogtid|nobig,percona_innodb|big,component_audit_log_filter,sysschema|big,parts|nobig,rpl_encryption,component_masking_functions,jp,information_schema"
    WORKER_4_MTR_SUITES="group_replication|big,percona_innodb|nobig,binlog|nobig,percona_binlog|nobig,rpl_nogtid|big,rocksdb_rpl|big,federated,innodb_gis|nobig,collations,test_services,component_js_lang"
    WORKER_5_MTR_SUITES="main|big,innodb_gis|big,rpl_gtid|big,percona_rpl_gtid|big,binlog|big,percona_binlog|big,funcs_1,auth_sec|big,binlog_gtid|nobig,opt_trace,secondary_engine,service_status_var_registration"
    WORKER_6_MTR_SUITES="main|nobig,group_replication|nobig,parts|big,innodb_undo|big,innodb_fts|nobig,perfschema|big,engines/iuds,innodb_zip|nobig,rocksdb_sys_vars,component_connection_control,service_udf_registration"
    WORKER_7_MTR_SUITES="innodb|big,rpl|nobig,percona_rpl|nobig,engines/funcs,sys_vars,x|nobig,component_keyring_file|nobig,stress,funcs_2,json,interactive_utilities,procfs"
    WORKER_8_MTR_SUITES="component_encryption_udf,innodb|nobig,innodb_fts|big,rpl_gtid|nobig,percona_rpl_gtid|nobig,gcol,gis,auth_sec|nobig,binlog_gtid|big,test_service_sql_api,connection_control,percona-pam-for-mysql"
>>>>>>> ps/release-9.7.0-1
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
