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

sleep 5

echo 4

exit 0
