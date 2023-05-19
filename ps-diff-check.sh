#!/bin/bash

# Create a temporary directory to fetch the PS source tarball.
PS_80_PATH=`mktemp -d`
SCRIPT_PATH=${BASH_SOURCE[0]}
BASE_DIR=$(dirname $SCRIPT_PATH)
RET=1

# Get the MySQL Version to download to compare the sources.
source MYSQL_VERSION
PS_VERSION=$MYSQL_VERSION_MAJOR.$MYSQL_VERSION_MINOR.$MYSQL_VERSION_PATCH$MYSQL_VERSION_EXTRA

echo "Using PS Version: $PS_VERSION"

# Get the source tarball from Percona downloads
pushd $PS_80_PATH
curl https://downloads.percona.com/downloads/Percona-Server-8.0/Percona-Server-$PS_VERSION/source/tarball/percona-server-$PS_VERSION.tar.gz -o percona-server-$PS_VERSION.tar.gz
tar -xf percona-server-$PS_VERSION.tar.gz
popd

# Run the diff checker tool
echo "Running Diff Checker..."
$BASE_DIR/check_src/run.sh $PS_80_PATH/percona-server-$PS_VERSION/ 2>&1 > $PS_80_PATH/diff-check.log

if [[ -s $PS_80_PATH/diff-check.log ]]; then
  echo "Diff checker failed with below error. Please add them to whitelist if they are expected"
  echo ""
  cat $PS_80_PATH/diff-check.log
  RET=1
else
  RET=0
fi


# Cleanup
rm -rf $PS_80_PATH
exit $RET
