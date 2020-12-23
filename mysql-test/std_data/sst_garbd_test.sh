#!/bin/bash

set -x

# Stability improvement for the parallel backup test:
# makes the SST process longer
sleep 3

FIRST_RECEIVED=0
SST_FAILED=0
function handle_sigint() {
  if (( $FIRST_RECEIVED == 0 )); then
    echo "SST request failed"
    SST_FAILED=1
    kill $(ps -s $$ -o pid=)
  fi
}

trap 'handle_sigint' 2

echo 1

(socat TCP-LISTEN:$1 - > /dev/null) &
wait $!

FIRST_RECEIVED=1
if (( $SST_FAILED == 0 )); then
  echo 2
  (socat TCP-LISTEN:$1 - > /dev/null) &
  wait $!
  echo 3
fi
exit $SST_FAILED
