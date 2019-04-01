#!/bin/bash -ue

# Copyright (C) 2019 Percona Inc
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston
# MA  02110-1301  USA.

# Documentation: http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html
# Make sure to read that before proceeding!


#-------------------------------------------------------------------------------
#
# Step-1: Load common code
#
. $(dirname $0)/wsrep_sst_common


#-------------------------------------------------------------------------------
#
# Step-2: Setup default global variable that we plan to use during processing.
#

declare errcode=0
declare DATA="${WSREP_SST_OPT_DATA}"


#-------------------------------------------------------------------------------
#
# Step-3: Helper function to carry-out the main workload.
#
function cleanup_upgrade()
{
    # Since this is invoked just after exit NNN
    local estatus=$?
    if [[ $estatus -ne 0 ]]; then
        wsrep_log_error "Cleanup after exit with status:$estatus"
    fi

    if [[ -n ${MYSQL_UPGRADE_TMPDIR} ]]; then
        rm -rf "${MYSQL_UPGRADE_TMPDIR}"
    fi

    exit $estatus
}


#-------------------------------------------------------------------------------
#
# Step-4: Main workload logic starts here.
#

# Get our MySQL version
MYSQL_VERSION=$WSREP_SST_OPT_VERSION
if [[ -z $MYSQL_VERSION ]]; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "FATAL: Cannot determine the mysqld server version"
    wsrep_log_error "****************************************************** "
    exit 2
fi

auto_upgrade=$(parse_cnf sst auto-upgrade "")
auto_upgrade=$(normalize_boolean "$auto_upgrade" "on")

# Check the WSREP_SST_OPT_FORCE_UPGRADE environment variable
force_upgrade=${WSREP_SST_OPT_FORCE_UPGRADE:-""}
if [[ -z $force_upgrade ]]; then
    force_upgrade=$(parse_cnf sst force-upgrade "")
fi
force_upgrade=$(normalize_boolean "$force_upgrade" "off")


read_variables_from_stdin
[[ $? -ne 0 ]] && exit 2

trap cleanup_upgrade EXIT

# Retrieve the version of the datadir (if possible)
DATADIR_WSREP_SCHEMA_VERSION="0.0.0"
if [[ -r "${DATA}/wsrep_state.dat" ]]; then
    read_variables_from_wsrep_state "${DATA}/wsrep_state.dat"
    [[ $? -ne 0 ]] && exit 2
fi

wsrep_log_info "Running upgrade..........."
set +e

# Pretend that we've done an IST, so that the code:
# (1) will compare the datadir schema versions (not the server versions)
# (2) will NOT run the async slave reset
run_post_processing_steps "$DATA" "${WSREP_SST_OPT_PORT:-4444}" \
        "$MYSQL_VERSION" "$MYSQL_VERSION" "xtrabackup" "ist" "$force_upgrade" "$auto_upgrade"
errcode=$?

set -e

if [[ $errcode -ne 0 ]]; then
    wsrep_log_info "...........upgrade failed.  Exiting"
    exit $errcode
else
    wsrep_log_info "...........upgrade done"
fi

# Let the server now that we are done with the upgrade
echo "ready"
exit 0
