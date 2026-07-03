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
  if [[ "$1" == "RelWithDebInfo" ]]; then
    echo "Setting WORKER_x_MTR_SUITES for BUILD_TYPE=RelWithDebInfo"
    # Unit tests will be executed by worker 1
    # Split derived from measured KH timings (whole-suite and |big/|nobig runs).
    # Only group_replication and galera are split: their whole-suite times exceed
    # what a single worker can absorb. Everything else runs as a whole suite per
    # rule 4 (prefer non-split). Makespan ~4241s (bound by group_replication|big).
    WORKER_1_MTR_SUITES="main,component_encryption_udf,innodb,sysschema,galera_nbo,galera_encryption,engines/funcs,audit_log,rpl_encryption,test_services,service_sys_var_registration,json,percona,interactive_utilities"
    WORKER_2_MTR_SUITES="group_replication|big"
    WORKER_3_MTR_SUITES="galera|nobig"
    WORKER_4_MTR_SUITES="rpl"
    WORKER_5_MTR_SUITES="galera|big,galera_3nodes_nbo,x,binlog,clone,test_service_sql_api,binlog_gtid,information_schema,federated,secondary_engine,data_masking,procfs"
    WORKER_6_MTR_SUITES="group_replication|nobig,galera_sr,perfschema,component_percona_telemetry,parts,innodb_fts,innodb_undo,innodb_gis,jp,collations,service_status_var_registration,service_udf_registration"
    WORKER_7_MTR_SUITES="rpl_nogtid,galera-x,galera_3nodes_sr,sys_vars,binlog_nogtid,wsrep,component_masking_functions,query_rewrite_plugins,gcol,engines/iuds,audit_null,funcs_2,encryption"
    WORKER_8_MTR_SUITES="galera_3nodes,rpl_gtid,component_keyring_file,auth_sec,stress,audit_log_filter,funcs_1,innodb_zip,connection_control,opt_trace,gis,binlog_57_decryption,percona-pam-for-mysql"
  else # Debug (and everything different from "RelWithDebInfo")
    echo "Setting WORKER_x_MTR_SUITES for BUILD_TYPE=Debug"
    # Unit tests will be executed by worker 1
    # Split derived from measured KH timings (whole-suite and |big/|nobig runs).
    # group_replication, galera and rpl are split because their whole-suite times
    # would otherwise dominate the makespan. Everything else runs as a whole suite
    # per rule 4 (prefer non-split). Makespan ~6623s, workers balanced within 20s.
    WORKER_1_MTR_SUITES="rpl_gtid,rpl|big,galera_3nodes_nbo,innodb_undo,encryption,binlog_nogtid,innodb_zip,json,jp,secondary_engine"
    WORKER_2_MTR_SUITES="group_replication|big,percona-pam-for-mysql"
    WORKER_3_MTR_SUITES="galera|nobig,innodb_fts,galera_3nodes_sr,galera_nbo,information_schema,component_masking_functions,test_services,audit_null,interactive_utilities"
    WORKER_4_MTR_SUITES="main,innodb_gis,engines/funcs,binlog,gcol,funcs_1,engines/iuds,opt_trace,service_status_var_registration,procfs"
    WORKER_5_MTR_SUITES="galera|big,galera_3nodes,stress,sys_vars,audit_log_filter,federated,rpl_encryption,gis,service_udf_registration"
    WORKER_6_MTR_SUITES="rpl|nobig,component_encryption_udf,perfschema,x,wsrep,binlog_gtid,funcs_2,service_sys_var_registration,percona"
    WORKER_7_MTR_SUITES="group_replication|nobig,component_keyring_file,galera_sr,parts,auth_sec,component_percona_telemetry,audit_log,collations,connection_control,binlog_57_decryption"
    WORKER_8_MTR_SUITES="innodb,rpl_nogtid,galera-x,clone,sysschema,galera_encryption,test_service_sql_api,query_rewrite_plugins,data_masking"
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
