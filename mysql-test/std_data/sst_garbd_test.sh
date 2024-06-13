#!/bin/bash

set -x
socat_pid=
after_sst_sleep_duration=${3:-0}
exit_code=${4:-0}
 
function handle_sigint() {
  if [[ -n "${socat_pid}" ]]; then
      echo "SST request failed"
      echo "killing ${socat_pid}"    
      kill -9 ${socat_pid} ||:
      exit 1
  fi
}

trap 'handle_sigint' SIGINT
trap 'handle_sigint' SIGTERM

echo 1

(socat openssl-listen:$1,reuseaddr,$2 - </dev/null > /dev/null) &
socat_pid=$!
echo "socat_pid: ${socat_pid}"
wait ${socat_pid}
socat_pid=


echo 2
socat openssl-listen:$1,reuseaddr,$2 - > /dev/null
echo 3

echo "Sleeping for ${after_sst_sleep_duration} seconds before finishing sst script"
sleep ${after_sst_sleep_duration}

echo 4

exit ${exit_code}
