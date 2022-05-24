#!/bin/bash

set -x
socat_pid=

function handle_sigint() {
  if [[ -n "${socat_pid}" ]]; then
      echo "SST request failed"
      echo "killing ${socat_pid}"    
      kill -9 ${socat_pid} ||:
      exit 1
  fi
}

trap 'handle_sigint' 2

echo 1

(socat TCP-LISTEN:$1,reuseaddr - </dev/null > /dev/null) &
socat_pid=$!
echo "socat_pid: ${socat_pid}"
wait ${socat_pid}
socat_pid=


echo 2
socat TCP-LISTEN:$1,reuseaddr - > /dev/null
echo 3

exit 0
