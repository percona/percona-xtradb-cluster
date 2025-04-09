#!/bin/bash

# Create a temporary directory to fetch the PS source tarball.
PS_PATH=`mktemp -d`
SCRIPT_PATH=${BASH_SOURCE[0]}
BASE_DIR=$(dirname $SCRIPT_PATH)
RET=1

# Get the MySQL Version to download to compare the sources.
source MYSQL_VERSION
PS_VERSION=$MYSQL_VERSION_MAJOR.$MYSQL_VERSION_MINOR.$MYSQL_VERSION_PATCH$MYSQL_VERSION_EXTRA

echo "Using PS Version: $PS_VERSION"
echo "PS_PATH: $PS_PATH"

# Get the source tarball from Percona downloads
pushd $PS_PATH

PS_REPO_URL="https://github.com/percona/percona-server"

echo "Trying tag Percona-Server-$PS_VERSION"

if curl --head --silent --fail "${PS_REPO_URL}/releases/tag/Percona-Server-$PS_VERSION" > /dev/null; then
    echo "Tag Percona-Server-$PS_VERSION exists"
    curl -L ${PS_REPO_URL}/archive/refs/tags/Percona-Server-$PS_VERSION.tar.gz -o percona-server-$PS_VERSION.tar.gz    
else
    echo "Tag Percona-Server-$PS_VERSION does not exist. Trying branch release-$PS_VERSION"
    if curl --silent --fail "https://api.github.com/repos/percona/percona-server/branches/release-$PS_VERSION" > /dev/null; then
        echo "Branch release-$PS_VERSION exists"
        curl -L ${PS_REPO_URL}/archive/refs/heads/release-$PS_VERSION.tar.gz -o percona-server-$PS_VERSION.tar.gz
    else
        echo "Branch percona-server-$PS_VERSION does not exist. Exiting."
        exit 1
    fi
fi

if [[ $? -ne 0 ]]; then
  echo "Downloading source reference failed"
  exit 1 
fi

tar -xf percona-server-$PS_VERSION.tar.gz
if [[ $? -ne 0 ]]; then
  echo "Unpacking source reference failed"
  exit 1 
fi

shopt -s nullglob
PS_DIR=(percona-server-*/)
echo "PS_DIR: ${PS_DIR}"

popd

# Run the diff checker tool
echo "Running Diff Checker..."
$BASE_DIR/check_src/run.sh $PS_PATH/$PS_DIR 2>&1 > $PS_PATH/diff-check.log
if [[ $? -ne 0 ]]; then
  echo "Running check_src tool failed"
  exit 1 
fi

echo "Running Diff Checker...DONE"

if [[ -s $PS_PATH/diff-check.log ]]; then
  echo "Diff checker failed with below error. Please add them to whitelist if they are expected"
  echo ""
  cat $PS_PATH/diff-check.log
  RET=1
else
  RET=0
fi


# Cleanup
rm -rf $PS_PATH
echo "Diff Checker exits with status code ${RET}"
exit $RET
