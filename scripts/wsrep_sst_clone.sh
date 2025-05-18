#!/usr/bin/env bash
# Copyright (C) 2024 Percona Inc
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

#############################################################################
# While this script is based on clone script from Codership, we have
# done many changes to adapt it to PXC
#===========================================================================
#              ┌──────────────────┐
#              │  Joiner starts   │
#              └────────┬─────────┘
#              ┌────────▼─────────┐
#              │Open NetCat listnr│
#              └────────┬─────────┘
#              ┌────────▼─────────┐
#              │Send message to Dn│
#              └────────┬─────────┘
#              ┌────────▼─────────┐
#              │Donor decide IST  │
#              │or SST. Send msg  │
#              │through NC and    │
#              │waits for Joiner  │
#              │Clone instance    │
#              └────────┬─────────┘
#              ┌────────▼─────────┐
#              │Joiner get IST    │
#              │or SST.If IST     │
#              │Bypass all and    │
#              │Wait for IST.     │
#              │SST Start clone   │
#              │Instance and wait │
#              │for donor to clone│
#              └─────────┬────────┘
#              ┌─────────▼─────────────┐
#              │Clone process is       │
#              │reported.              │
#              │When done Dn waits     │
#              │Joiner close instance  │
#              │And performs 2 restarts│
#              └──────────┬────────────┘
#              ┌──────────▼──────────────┐
#              │1) To fix dictionary  &  │
#              │   Final cleanup         │
#              │2) To recover position   │
#              └──────────┬──────────────┘
#              ┌──────────▼──────────────┐
#              │Send final ready signal  │
#              │Waits for IST            │
#              └─────────────────────────┘
#----------------------------------------------------------------------------
# Config variables:
# [sst]
# wsrep-debug=true
# joiner-timeout-wait-donor-message=60
# joiner-timeout-clone-instance=90
# donor-timeout-wait-joiner=200
#
# NOTE for SSL usage
# When using clone SSL certificates MUST be manually (or by automation) placed in a location on both servers DONOR/RECEIVER
# this location better not be the data directory given it is manipulated by the clone process.
# It is recommended to explicitly set the ssl certificates in the my.cnf as:
# [client]
#   ssl-ca = /<path>/ca.pem
#   ssl-cert = /<path>/client-cert.pem
#   ssl-key = /<path>/client-key.pem
# [mysqld]
#   ssl-ca = /<path>/ca.pem
#   ssl-cert = /<path>/server-cert.pem
#   ssl-key = /<path>/server-key.pem
# OR
#  [mysqld]
#   clone_ssl_ca=/path/to/ca.pem
#   clone_ssl_cert=/path/to/client-cert.pem
#   clone_ssl_key=/path/to/client-key.pem
# Where <path> is not the data directory
#############################################################################

CMDLINE="$0 $*"

set -o nounset -o errexit

readonly EINVAL=22
readonly EPIPE=32
readonly ETIMEDOUT=110
readonly ENODATA=61
CLEANUP_CLONE_PLUGIN=""
CLONE_USER=""
NC_PID=""
CLONE_INSTANCE_PID=""

# The following variable will store the position (GTID:POS) coming from the donor in the message exchange.
# It will be used ONLY in emergency as last measure if for any reason the position recovery (at the end) will fail
RP_PURGED_EMERGENCY=""
WSREP_SST_OPT_LPORT=""
WSREP_SST_OPT_PARENT=""
#Default to 4444 as we do for XB
CLONE_INSTANCE_PORT=4444


# wsrep_gen_secret() generates enough randomness, yet some MySQL password
# policy may insist on having upper and lower case letters, numbers and
# special symbols. Make sure that whatever we generate, it has it.
readonly PSWD_POLICY="Aa"

OS=$(uname)
[ "$OS" = "Darwin" ] && export -n LD_LIBRARY_PATH

. $(dirname "$0")/wsrep_sst_common #_clone

JOINER_TIMEOUT_WAIT_DONOR_MESSAGE=$(parse_cnf sst joiner-timeout-wait-donor-message "60")
JOINER_TIMEOUT_WAIT_CLONE_INSTANCE=$(parse_cnf sst joiner-timeout-clone-instance "90")
DONOR_TIMEOUT_WAIT_JOINER_CLONE_INSTANCE=$(parse_cnf sst donor-timeout-wait-joiner "200")


wsrep_log_info "Running: $CMDLINE"

# READ user/pw from stdin
read_variables_from_stdin
readonly WSREP_SST_OPT_ADDR_LOCAL=$WSREP_SST_OPT_ADDR
wsrep_log_debug "-> WSREP_SST_OPT_HOST: $WSREP_SST_OPT_HOST"
wsrep_log_debug "-> WSREP_SST_OPT_USER: $WSREP_SST_OPT_USER"
#wsrep_log_debug "-> WSREP_SST_OPT_PSWD: $WSREP_SST_OPT_PSWD"
wsrep_log_debug "-> WSREP_SST_OPT_ADDR: $WSREP_SST_OPT_ADDR"
wsrep_log_debug "-> WSREP_SST_OPT_ADDR_LOCAL: $WSREP_SST_OPT_ADDR_LOCAL"
wsrep_log_debug "-> WSREP_SST_OPT_LPORT: $WSREP_SST_OPT_LPORT"

# To not use possible [client] section in my.cnf
MYSQL_CLIENT="$MYSQL_CLIENT --no-defaults"

# Linux and FreeBSD have different mktemp syntax with respect to a parent
# directory. This wrapper takes parent dir as a first argument and passes
# the rest to mktemp directly.
wsrep_mktemp_in_dir()
{
    local OS=$(uname)
    local tmpdir="$1"
    shift
    if [ "$OS" = "Linux" ]
    then
        # Linux mktemp does not respect TMPDIR if template is given
        mktemp --tmpdir="$tmpdir" $*
    else
        echo $(export TMPDIR="$tmpdir"; mktemp $*)
    fi
}

# Generate a string equivalent to 16 random bytes
wsrep_gen_secret()
{
    if [ -x /usr/bin/openssl ]
    then
        echo `/usr/bin/openssl rand -hex 16`
    else
        printf "%04x%04x%04x%04x%04x%04x%04x%04x" \
                $RANDOM $RANDOM $RANDOM $RANDOM   \
                $RANDOM $RANDOM $RANDOM $RANDOM
    fi
}

cleanup_donor()
{
    wsrep_log_info "Cleanup DONOR."
#    wsrep_log_debug "Cleanup MySQL ADMIN_PSWD: $ADMIN_PSWD"
    wsrep_log_debug "Cleanup MySQL MYSQL_ACLIENT: $MYSQL_ACLIENT"
    export MYSQL_PWD=$ADMIN_PSWD
    if [ "$CLEANUP_CLONE_PLUGIN" == "yes" ]
    then
        CLEANUP_CLONE_PLUGIN="UNINSTALL PLUGIN CLONE;"
    fi
    if [ ! "$WSREP_SST_OPT_REMOTE_JOINER_USER" == "" ]; then
        $MYSQL_ACLIENT -e "SET wsrep_on=OFF; DROP USER IF EXISTS '$WSREP_SST_OPT_REMOTE_JOINER_USER'@'%'; $CLEANUP_CLONE_PLUGIN" || :
    fi

    rm -rf "$CLONE_EXECUTE_SQL" ||:
    rm -rf "$CLONE_PREPARE_SQL" ||:
    wsrep_log_info "Cleanup DONOR DONE."
}

sig_cleanup_joiner()
{
    if [ -n "$CLONE_INSTANCE_PID" ]; then
        kill -SIGKILL $CLONE_INSTANCE_PID /dev/null 2>&1 ||:
    fi

    if [ -n "$NC_PID" ]; then
        kill -SIGKILL $NC_PID /dev/null 2>&1 ||:
    fi

    rm -rf $CLONE_SOCK_DIR || :
    rm -rf $CLONE_PID_FILE || :
    rm -fr $tmp_datadir || :
    rm -f $WSREP_SST_OPT_DATA/XST_FILE.txt > /dev/null 2>&1 || :


   if [ $CLEANUP_FILES ]
    then
        rm -rf $CLONE_ERR || :
        rm -rf $CLONE_SQL || :
    fi
}

cleanup_joiner()
{
    wsrep_log_info "Joiner cleanup. SST daemon PID: $CLONE_INSTANCE_PID"
    sig_cleanup_joiner
    wsrep_log_info "Joiner cleanup done."
}

# Check whether process in PID file is still running.
check_pid_file()
{
    local pid_file=$1
    [ -r "$pid_file" ] && ps -p $(cat $pid_file) >/dev/null 2>&1
}

# Check client version
check_client_version()
{
    local readonly min_version="8.0.19"
    IFS="." read -ra min_vers <<< $min_version # split into major minor and patch

    local readonly client_version=${1%%[-/]*} # take only a.b.c from a.b.c-d.e
    IFS="." read -ra client_vers <<< $client_version

    set +e
    for i in ${!min_vers[@]}
    do
        if compare_versions "${client_vers[$i]}" "<" "${min_vers[$i]}"
        then
            wsrep_log_error "this operation requires MySQL client version $min_version," \
                            " this client is '$client_version'"
            set -e
            return $EINVAL
        fi
    done
    set -e
}

# This function is common for Donor and Joiner.
# Installs clone plugin if necessary.
# If donor does not have clone plugin installed by default, it sets CLEANUP_CLONE_PLUGIN=yes
# This flag is sent to Joiner, to inform the Joiner that it should uninstall clone plugin after SST.
# Flag is also used on Donor side to decide if clone plugin should be uninstalled after SST.
# Sets the following environment variables: CLEANUP_CLONE_PLUGIN_ON_DONOR
install_clone_plugin()
{
    local -r role=$1

    # Note that install_clone_plugin() is called on Joiner side after Joiner receives
    # info that clone plugin should be uninstalled after SST. Joiner stores this info
    # in CLEANUP_CLONE_PLUGIN variable, so we don't want to override it here.
    if [ "$role" == "donor" ]
    then
        CLEANUP_CLONE_PLUGIN="no"
    fi

    CLONE_PLUGIN_LOADED=$($MYSQL_ACLIENT -e "SELECT COUNT(*) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_TYPE = 'CLONE';")

    if [ "$CLONE_PLUGIN_LOADED" -eq 0 ]
    then
        wsrep_log_info "Installing CLONE plugin"

        # INSTALL PLUGIN is replicated by default, so we need to switch off
        # session replication on donor
        if [ "$role" = "donor" ]
        then
            WSREP_OFF="SET SESSION wsrep_on=OFF; "
            CLEANUP_CLONE_PLUGIN="yes"
        else
            WSREP_OFF="" # joiner does not have replication enabled
        fi
        $MYSQL_ACLIENT -e "${WSREP_OFF}INSTALL PLUGIN CLONE SONAME 'mysql_clone.so';"
    fi
}

# Setup certificates on Donor instance
# 1. If Donor has server certificates set, it means it accepts only secure connections.
#    Before SST, we create a dedicated clone user that will be used by Recipient instance
#    for cloning. In such a case this user with 'REQUIRE SSL' to enforce SSL during clone.
#    This affects 'REQUIRE_SSL' global variable.
# 2. Donor needs to connect to Recipient instance to trigger cloning.
#    if [sst] (with fallback to [client]) certificates are present - use them.
#    This affects CLIENT_SSL_OPTIONS global variable.
# Sets the following environment variables: REQUIRE_SSL, CLIENT_SSL_OPTIONS
setup_certificates_for_donor()
{
    local client_ssl_cert
    local client_ssl_key
    local client_ssl_ca
    local client_ssl_mode
    local server_ssl_cert
    local server_ssl_key
    local server_ssl_ca

    # client_ssl_* is used by Donor to connect to Joiner's clone (aka Recipient) instance
    client_ssl_cert=$(parse_cnf client ssl_cert "")
    client_ssl_key=$(parse_cnf client ssl_key "")
    client_ssl_ca=$(parse_cnf client ssl_ca "")
    client_ssl_mode=$(parse_cnf client ssl_mode "")

    wsrep_log_debug "SSL Client settings on $role from client section:  CERT=$client_ssl_cert, KEY=$client_ssl_key, CA=$client_ssl_ca, ssl-mode=$client_ssl_mode "

    # Server info
    # If donor certificates are set, we require all connections to donor
    # to be SSL-protected. That means connection from Recipient instance
    # to Donor instance will have to be over SSL.
    server_ssl_cert=$($MYSQL_ACLIENT -e "SELECT @@ssl_cert")
    server_ssl_key=$($MYSQL_ACLIENT -e "SELECT @@ssl_key")
    server_ssl_ca=$($MYSQL_ACLIENT -e "SELECT @@ssl_ca")

    # REQUIRE_SSL is used during clone user creation on Donor server
    REQUIRE_SSL=""
    if [ -n "$server_ssl_ca" ] && [ -n "$server_ssl_cert" ] && [ -n "$server_ssl_key" ] && [ ! "$server_ssl_ca" == "NULL" ]; then
        REQUIRE_SSL="REQUIRE SSL"
    fi
    wsrep_log_debug "Clone SSL Server settings on $role from runtime:  CERT=$server_ssl_cert, KEY=$server_ssl_key, CA=$server_ssl_ca"

    # CLIENT_SSL_OPTIONS is used when Donor connects to the Recipient instance to trigger clone
    CLIENT_SSL_OPTIONS=""
    if [ -n "$client_ssl_ca" ] && [ -n "$client_ssl_cert" ] && [ -n "$client_ssl_key" ]
    then
        CLIENT_SSL_OPTIONS="--ssl-cert=$client_ssl_cert --ssl-key=$client_ssl_key"
        if [ -n "$client_ssl_ca" ]
        then
            CLIENT_SSL_OPTIONS+=" --ssl-ca=$client_ssl_ca"
            if [ -n "$client_ssl_mode" ]
            then
                CLIENT_SSL_OPTIONS+=" --ssl-mode=$client_ssl_mode"
            fi
        fi
    else
        CLIENT_SSL_OPTIONS+=" --ssl-mode=DISABLED"
    fi
    wsrep_log_debug "SSL Client settings on $role for connection CLIENT_SSL_OPTIONS=${CLIENT_SSL_OPTIONS}"
}

# Setup certificates for Joiner's clone instance (aka "Recipient")
# Recipient instance potentially needs clone_ssl_* certificates to be set.
# If they are specified in Joiner's my.cnf, they should be set automatically
# after we install clone plugin in Recipient instance. If not, let's try
# to set them manually. If still not present, Recipient instance will use
# not encrypted connection. Note that if Donor requires encrypted connection, clone
# will fail, but it is a configuration problem.
setup_certificates_for_recipient()
{
    local clone_ssl_cert
    local clone_ssl_key
    local clone_ssl_ca

    clone_ssl_cert=$($MYSQL_ACLIENT -e "SELECT @@clone_ssl_cert")
    clone_ssl_key=$($MYSQL_ACLIENT -e "SELECT @@clone_ssl_key")
    clone_ssl_ca=$($MYSQL_ACLIENT -e "SELECT @@clone_ssl_ca")
    wsrep_log_debug "Clone SSL settings on $role from runtime:  CERT=$clone_ssl_cert, KEY=$clone_ssl_key, CA=$clone_ssl_ca"

    # We check if ssl certificates for clone are available.
    # If not we try from cnf 
    if [ -z "$clone_ssl_cert" ] || [ "$clone_ssl_cert" == "NULL" ]
    then
        clone_ssl_cert=$(parse_cnf mysqld clone_ssl_cert "")
        clone_ssl_key=$(parse_cnf mysqld clone_ssl_key "")
        clone_ssl_ca=$(parse_cnf mysqld clone_ssl_ca "")
        wsrep_log_debug "Clone SSL settings on $role from mysqld section:  CERT=$clone_ssl_cert, KEY=$clone_ssl_key, CA=$clone_ssl_ca"
    fi
    # If not we try from client section
    if [ -z "$clone_ssl_cert" ] || [ "$clone_ssl_cert" == "NULL" ]
    then
        clone_ssl_cert=$(parse_cnf client ssl_cert "")
        clone_ssl_key=$(parse_cnf client ssl_key "")
        clone_ssl_ca=$(parse_cnf client ssl_ca "")
        wsrep_log_debug "Clone SSL settings on $role from client section:  CERT=$clone_ssl_cert, KEY=$clone_ssl_key, CA=$clone_ssl_ca"
    fi

    # Even if certificates were set at the beginning, setting them again will not hurt
    if [ -n "$clone_ssl_cert" ] && [ -n "$clone_ssl_key" ] && [ -n "$clone_ssl_ca" ]
    then
        wsrep_log_info "Clone SSL settings on $role we will use:  CERT=$clone_ssl_cert, KEY=$clone_ssl_key, CA=$clone_ssl_ca"

        $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_cert='$clone_ssl_cert'"
        $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_key='$clone_ssl_key'"
        $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_ca='$clone_ssl_ca'"
    fi
}

# Install clone plugin and setup needed certificates
setup_clone_plugin()
{
    # Either donor or recipient
    local -r role=$1

    install_clone_plugin "$role"

    wsrep_log_debug "-> ############## SSL SECTION [START] ($role) ############"
    if [ "$role" == "donor" ]
    then
        setup_certificates_for_donor
    else
        setup_certificates_for_recipient
    fi
    wsrep_log_debug "-> ############## SSL SECTION [END] ($role) ############"
}

# Let's use the simple approach to detect if the transfer is running or not.
# Monitor p_s.clone_progress table. If anything changes there, it means something
# is happening.
# One of the columns is DATA: The amount of data transferred in current state, in bytes,
# so we should be good.
# Allow transfer stuck for sst-idle-timeout
monitor_sst_progress() {
  local progress_monitor_command=$1
  local timeout=$2
  local clone_instance_pid=$3
  local parent_process_pid=$4

  local current_timeout=0
  local previous_command_output=
  local sleep=5
  local previous_progress=

  wsrep_log_debug "progress_monitor_command: ${progress_monitor_command}"
  wsrep_log_debug "timeout: ${timeout}"
  wsrep_log_debug "clone_instance_pid: ${clone_instance_pid}"
  wsrep_log_debug "parent_process_pid: ${parent_process_pid}"

  if [ ${timeout} -eq 0 ]; then
    return 2;
  fi

  while true; do
    # check if parent process is still alive
    # Is it needed? If parent process dies, SST script dies as well
    # and will cleanup clone instance in cleanup_joiner() trap.
    kill -0 $parent_process_pid 2> /dev/null
    if [ $? -ne 0 ]; then
      return 1;
    fi

    # check if clone instance process is still alive
    kill -0 $clone_instance_pid 2> /dev/null
    if [ $? -ne 0 ]; then
      # Clone instance is done with its job
      return 0;
    fi

    # check if there is any SST progress
    command_output=$(eval "$progress_monitor_command" 2> /dev/null ||:)
    if [ "${command_output}" != "${previous_command_output}" ]; then
        current_timeout=0
        previous_command_output=${command_output}
        # if the transfer is ongoing, keep checking once per 5 seconds
        sleep=5
    else
        current_timeout=$((current_timeout + 1))
        wsrep_log_debug "sst idle for ${current_timeout} sec"
        # if we detected transfer stall, start checking more often (1 sec)
        sleep=1
    fi

    if [ ${current_timeout} -eq  ${timeout} ]; then
      return 2
    fi

    # Report the current progress
    current_progress=`$MYSQL_ACLIENT -NB -e "select format(((data/estimate)*100),2) 'completed%' from performance_schema.clone_progress where stage like 'FILE_COPY';" 2> /dev/null` || :
    if [ "$previous_progress" != "$current_progress" ]; then
        wsrep_log_info "SST progress: ${current_progress}%"
        previous_progress=$current_progress
    fi

    sleep ${sleep};
  done

  wsrep_log_debug "We should never get here"
  return 0
}


# This is the script execution start point

if test -z "$WSREP_SST_OPT_HOST"; then wsrep_log_error "HOST cannot be empty"; exit $EINVAL; fi

# MySQL client does not seem to agree to [] around IPv6 addresses
wsrep_check_programs sed
SST_HOST_STRIPPED=$(echo $WSREP_SST_OPT_HOST | sed 's/^\[//' | sed 's/\]$//')

# Option to CREATE USER
WITH_OPTION="WITH caching_sha2_password"

if [ "$WSREP_SST_OPT_ROLE" = "donor" ]
then
    WSREP_SST_OPT_REMOTE_JOINER_USER=$WSREP_SST_OPT_REMOTE_USER
    WSREP_SST_OPT_REMOTE_JOINER_PSWD=$WSREP_SST_OPT_REMOTE_PSWD

    echo "continue" # donor can resume updating data

    WSREP_SST_OPT_REMOTE_HOST_WITH_PORT=$WSREP_SST_OPT_ADDR_LOCAL
    WSREP_SST_OPT_REMOTE_HOST=$(echo $WSREP_SST_OPT_REMOTE_HOST_WITH_PORT | cut -d ':' -f 1)
    WSREP_SST_OPT_REMOTE_HOSTPORT=$(echo $WSREP_SST_OPT_REMOTE_HOST_WITH_PORT | cut -d ':' -f 2)

    SST_HOST_STRIPPED=$(echo $WSREP_SST_OPT_REMOTE_HOST | sed 's/^\[//' | sed 's/\]$//')

    wsrep_log_debug "-> WSREP_SST_OPT_REMOTE_JOINER_USER = $WSREP_SST_OPT_REMOTE_JOINER_USER "
    wsrep_log_debug "-> WSREP_SST_OPT_REMOTE_HOST_WITH_PORT = $WSREP_SST_OPT_REMOTE_HOST_WITH_PORT "
    wsrep_log_debug "-> WSREP_SST_OPT_REMOTE_HOST = $WSREP_SST_OPT_REMOTE_HOST "
    wsrep_log_debug "-> WSREP_SST_OPT_REMOTE_HOSTPORT = $WSREP_SST_OPT_REMOTE_HOSTPORT "
    wsrep_log_debug "-> SST_HOST_STRIPPED = $SST_HOST_STRIPPED "

    # Split auth string at the last ':'
    if test -z "$WSREP_SST_OPT_USER";   then wsrep_log_error "USER cannot be empty";   exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_REMOTE_JOINER_USER"; then wsrep_log_error "REMOTE_USER cannot be empty"; exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_PORT";   then wsrep_log_error "PORT cannot be empty";   exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_SOCKET"; then wsrep_log_error "SOCKET cannot be empty"; exit $EINVAL; fi

    CLIENT_VERSION=$($MYSQL_CLIENT --version | grep -vi MariaDB | cut -d ' ' -f 4)
    check_client_version $CLIENT_VERSION

    readonly ADMIN_USER=$WSREP_SST_OPT_USER
    readonly ADMIN_PSWD=$WSREP_SST_OPT_PSWD

    MYSQL_RCLIENT="$MYSQL_CLIENT -u$WSREP_SST_OPT_REMOTE_JOINER_USER -h$SST_HOST_STRIPPED \
                   -P$WSREP_SST_OPT_REMOTE_HOSTPORT --batch --skip_column_names --silent"
    MYSQL_ACLIENT="$MYSQL_CLIENT -u$ADMIN_USER -S$WSREP_SST_OPT_SOCKET \
                   --batch --skip_column_names --silent"

    export MYSQL_PWD=$ADMIN_PSWD
    readonly WSREP_SST_OPT_LPORT=`$MYSQL_ACLIENT -e "select @@port"`

    wsrep_log_debug "-> WSREP_SST_OPT_LPORT: $WSREP_SST_OPT_LPORT "
    wsrep_log_debug "-> MYSQL_ACLIENT: $MYSQL_ACLIENT "
    #wsrep_log_debug "-> MYSQL_ACLIENT ADMIN_PSWD: $ADMIN_PSWD"
    wsrep_log_debug "-> MYSQL_RCLIENT: $MYSQL_RCLIENT "
    #wsrep_log_debug "-> MYSQL_RUSER: $WSREP_SST_OPT_REMOTE_JOINER_USER | PWD  $WSREP_SST_OPT_REMOTE_JOINER_PSWD"

    if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
    then
        #
        #  Prepare DONOR for cloning
        #

        # Use script file to avoid sensitive information on the command line
        CLONE_EXECUTE_SQL=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" --suffix=.sql clone_execute_XXXX)
        CLONE_PREPARE_SQL=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" --suffix=.sql clone_prepare_XXXX)

        cleanup_donor # Remove potentially existing clone user

        export MYSQL_PWD=$ADMIN_PSWD
        wsrep_log_debug "-> PREPARE DONOR"
        setup_clone_plugin "donor"

        wsrep_log_info "REQUIRE_SSL=$REQUIRE_SSL, CLIENT_SSL_OPTIONS=$CLIENT_SSL_OPTIONS"

        # Server configuration might have changed, make sure it is restored
        # on exit
        trap cleanup_donor EXIT

cat << EOF > "$CLONE_PREPARE_SQL"
SET wsrep_on=OFF;
CREATE USER "$WSREP_SST_OPT_REMOTE_JOINER_USER"@"%" IDENTIFIED BY '$WSREP_SST_OPT_REMOTE_JOINER_PSWD' $REQUIRE_SSL;
GRANT BACKUP_ADMIN ON *.* TO "$WSREP_SST_OPT_REMOTE_JOINER_USER"@"%";
GRANT SELECT ON performance_schema.* TO "$WSREP_SST_OPT_REMOTE_JOINER_USER"@"%";
GRANT EXECUTE ON *.* TO "$WSREP_SST_OPT_REMOTE_JOINER_USER"@"%";
EOF
        RC=0
        export MYSQL_PWD=$ADMIN_PSWD
#        wsrep_log_debug "-> connecting to donor: $CLONE_PREPARE_SQL $MYSQL_PWD $MYSQL_ACLIENT"
        $MYSQL_ACLIENT --connect-timeout=60 < $CLONE_PREPARE_SQL || RC=$?
        wsrep_log_debug "-> $RC connect to donor done"

        if [ "$RC" -ne 0 ]
        then
            wsrep_log_error "Donor prepare returned code $RC"
            cat $CLONE_PREPARE_SQL >> /dev/stderr
            exit $RC
        fi

        # Before waiting for the Joiner Clone mysql we send out the message this is a SST
        wsrep_log_debug "-> NETCAT signal to nc -w 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT"
        wsrep_log_debug "-> Signal for CLONE Plugin CLEANUP_CLONE_PLUGIN=$CLEANUP_CLONE_PLUGIN"
        echo "$CLEANUP_CLONE_PLUGIN|SST@$WSREP_SST_OPT_GTID<EOF>" | nc -w 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT || :

        # We stay on hold now, waiting for the Joiner to expose the service
        wsrep_log_info "-> WAIT for Joiner MySQL to be available nc -w 1 -i 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT"
        while [ true ]; do
        	NCPING=`nc -w 1 -i 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT 2> /dev/null | tr -d '\0'` || :
            if [ "$NCPING" == "" ]; then
                wsrep_log_info "[second(s) to timeout: $DONOR_TIMEOUT_WAIT_JOINER_CLONE_INSTANCE]"
            else
                wsrep_log_info "Joiner instance available at $SST_HOST_STRIPPED:$WSREP_SST_OPT_REMOTE_HOSTPORT"
                break
            fi
            if [ "$DONOR_TIMEOUT_WAIT_JOINER_CLONE_INSTANCE" == "0" ]; then
                    wsrep_log_error "TIMEOUT Waiting for the Joiner to setup MySQL clone instance $LINENO"
                    break ;
            fi

            sleep 1
            ((DONOR_TIMEOUT_WAIT_JOINER_CLONE_INSTANCE-=1))
        done
        # Wait is over
        wsrep_log_debug "-> Wait is over"

        export MYSQL_PWD="$WSREP_SST_OPT_REMOTE_JOINER_PSWD"
        export MYSQL_USER="$WSREP_SST_OPT_REMOTE_JOINER_USER"
        # Find own address (from which we connected)
        wsrep_log_debug "-> Connecting to JOINER to get exact IP of donor: $MYSQL_RCLIENT" # $MYSQL_PWD"
        USER=`$MYSQL_RCLIENT --skip-column-names $CLIENT_SSL_OPTIONS -e 'SELECT USER()'`
        LHOST=${USER##*@}
        DONOR=$LHOST:$WSREP_SST_OPT_LPORT

        wsrep_log_debug "-> DONOR string: $DONOR"

        # Use script file to avoid sensitive information on the command line
cat << EOF > "$CLONE_EXECUTE_SQL"
SET GLOBAL clone_valid_donor_list = '$DONOR';
CLONE INSTANCE FROM '$WSREP_SST_OPT_REMOTE_JOINER_USER'@'$LHOST':$WSREP_SST_OPT_LPORT IDENTIFIED BY '$WSREP_SST_OPT_REMOTE_JOINER_PSWD' $REQUIRE_SSL;
EOF

        wsrep_log_debug "JOINER CLONE ACTION SQL: $CLONE_EXECUTE_SQL $MYSQL_PWD $MYSQL_RCLIENT"

        # Actual cloning process
        wsrep_log_info "JOINER CLONE ACTION: cloning"
        LOCALOUTPUT=`$MYSQL_RCLIENT --connect-timeout=60 $CLIENT_SSL_OPTIONS < $CLONE_EXECUTE_SQL  2>&1 || RC=$?`
        wsrep_log_info "JOINER CLONE ACTION: cloning done $RC"
        wsrep_log_debug "-> LOCALOUTPUT: $LOCALOUTPUT"

        # We force the signal to be 0 because we KNOW that with clone when using the mysqld directly (not mysqld_safe) the daemon is shutdown at the end of the clone process.
        # However an error is returned because server cannot be restarted
        # This error does not indicate a cloning failure. It means that the recipient MySQL server instance must be started again manually after the data is cloned.
        if [ "$RC" -ne 0 ]; then
            RC=0
            wsrep_log_debug " JOINER CLONE ACTION abruptly terminated, but we can continue"
        fi

        # If still there we will manually shutdown
        LOCALOUTPUT=`$MYSQL_RCLIENT $CLIENT_SSL_OPTIONS -e "SHUTDOWN"  2>&1 || :`
        wsrep_log_debug "-> LOCALOUTPUT: $LOCALOUTPUT"

        if [ "$RC" -ne 0 ]
        then
            wsrep_log_error "Clone command returned code $RC"
            wsrep_log_debug "JOINER RETURN ERROR $MYSQL_RCLIENT"
            cat $CLONE_EXECUTE_SQL >> /dev/stderr
            # Attempt to shutdown recipient daemon
            eval $MYSQL_RCLIENT -e "SHUTDOWN"
            wsrep_log_info "Recipient shutdown: $?"
            case $RC in
            *)  RC=255 # unknown error
                ;;
            esac
            exit $RC
        fi
    else # BYPASS
        wsrep_log_info "Bypassing state dump."

        # Instruct recipient to shutdown
        # export MYSQL_PWD="$WSREP_SST_OPT_REMOTE_JOINER_PSWD"
        wsrep_log_info "BYPASS SENDING IST_FILE TO JOINER NetCat: nc -w 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT"
        echo "$WSREP_SST_OPT_GTID<EOF>" | nc -w 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_REMOTE_HOSTPORT || :
        wsrep_log_debug "-> Exiting with gtid: $WSREP_SST_OPT_GTID"
    fi

    wsrep_log_debug "-> SENDING DONE_MESSAGE $WSREP_SST_OPT_GTID"
    # DONOR must be clean BEFORE exit given user is removed on DONE

    echo "done $WSREP_SST_OPT_GTID"

elif [ "$WSREP_SST_OPT_ROLE" = "joiner" ]
then
    JOINER_CLONE_HOST=""
    JOINER_CLONE_PORT=""

    # These variables are used in sig_clean_joiner. Create here to avoid
    # potential unbound variable.
    CLEANUP_FILES=""
    CLONE_SOCK_DIR=""
    CLONE_PID_FILE=""
    tmp_datadir=""
    CLONE_ERR=""
    CLONE_SQL=""

    trap sig_cleanup_joiner HUP PIPE INT TERM
    trap cleanup_joiner EXIT

    wsrep_check_programs grep
    wsrep_check_programs ps
    wsrep_check_programs find

    if test -z "$WSREP_SST_OPT_DATA";   then wsrep_log_error "DATA cannot be empty";   exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_PARENT"; then wsrep_log_error "PARENT cannot be empty"; exit $EINVAL; fi

    #
    #  Find binary to run
    #  get_mysqld_path stores the path in MYSQLD_PATH variable. Later in
    #  the script we use CLONE_BINARY variable.
    MYSQLD_PATH=""
    get_mysqld_path
    CLONE_BINARY="${MYSQLD_PATH}"

    if [ ! -x "$CLONE_BINARY" ]
    then
        wsrep_log_error "Could not determine binary to run: $CLONE_BINARY"
        exit $EINVAL
    fi

    #
    # Find where libs needed for the binary are
    #
    CLONE_LIBS="$WSREP_SST_OPT_PLUGINDIR"

    # 1. Try plugins dir
    if [ -z "$CLONE_LIBS" ] && $MY_PRINT_DEFAULTS "mysqld" | grep -q plugin[_-]dir
    then
        CLONE_LIBS=$($MY_PRINT_DEFAULTS mysqld | grep -- "--plugin[_-]dir" | cut -d= -f2)
        # scale up to the first "mysql" occurence
        while [ ${#CLONE_LIBS} -gt "1" ]
        do
            [ `basename $CLONE_LIBS` = "mysql" ] && break
            CLONE_LIBS=$(dirname $CLONE_LIBS)
        done
        if ls $CLONE_LIBS/private/lib* > /dev/null 2>&1
        then
            CLONE_LIBS="$CLONE_LIBS/private"
        else
            wsrep_log_info "Could not find private libs in '$CLONE_LIBS' from plugin_dir: $($MY_PRINT_DEFAULTS mysqld | grep -- '--plugin_dir' | cut -d= -f2)"
            CLONE_LIBS=
        fi
    fi

    # 2. Try binary path
    if [ -z "$CLONE_LIBS" ]
    then
        CLONE_LIBS=$(dirname $(dirname $CLONE_BINARY))
        if ls $CLONE_LIBS/lib64/mysql/private/lib* > /dev/null 2>&1
        then
            CLONE_LIBS="$CLONE_LIBS/lib64/mysql/private/"
        elif ls $CLONE_LIBS/lib/mysql/private/lib* > /dev/null 2>&1
        then
            CLONE_LIBS="$CLONE_LIBS/lib/mysql/private/"
        else
            wsrep_log_info "Could not find private libs by binary name: $(dirname $(dirname $CLONE_BINARY))"
            CLONE_LIBS=
        fi
    fi

    # 3. Try this script path
    if [ -z "$CLONE_LIBS" ]
    then
        CLONE_LIBS=$(dirname $(dirname $0))
        if ls $CLONE_LIBS/lib64/mysql/private/lib* > /dev/null 2>&1
        then
            CLONE_LIBS="$CLONE_LIBS/lib64/mysql/private"
        elif ls $CLONE_LIBS/lib/mysql/private/lib* > /dev/null 2>&1
        then
            CLONE_LIBS="$CLONE_LIBS/lib/mysql/private"
        else
            wsrep_log_info "Could not find private libs by script path: $(dirname $(dirname $0))"
            CLONE_LIBS=
        fi
    fi

    if [ -d "$CLONE_LIBS" ]
    then
        CLONE_ENV="LD_LIBRARY_PATH=$CLONE_LIBS:${LD_LIBRARY_PATH:-}"
    else
        wsrep_log_info "Could not determine private library path for mysqld: $CLONE_LIBS. Leaving LD_LIBRARY_PATH unmodified."
        CLONE_ENV=""
    fi

    MODULE="clone_sst"
    CLONE_SOCK_DIR=`mktemp --tmpdir -d ${MODULE}_XXXXXX`

    CLONE_PID_FILE="$WSREP_SST_OPT_DATA/$MODULE.pid"

    if check_pid_file $CLONE_PID_FILE
    then
        CLONE_PID=`cat $CLONE_PID_FILE`
        wsrep_log_error "cloning daemon already running (PID: $CLONE_PID)"
        exit 114 # EALREADY
    fi
    rm -rf "$CLONE_PID_FILE"

    # If port was not set up explicitly in wsrep_sst_address,
    # then we need to set the port to a default that is different to the default port existing in the running instance.
    # This because clone port <> mysql port, due to the need to spin a second MySQL instance able to accomodate the clone action
    wsrep_log_debug "-> WSREP_SST_OPT_PORT $WSREP_SST_OPT_PORT"
    wsrep_log_debug "-> WSREP_SST_OPT_ADDR_LOCAL $WSREP_SST_OPT_ADDR_LOCAL"

    IFS=':' read -ra my_array <<< "$WSREP_SST_OPT_ADDR_LOCAL"

    lenAr=${#my_array[@]}
    wsrep_log_debug "-> lenarray $lenAr"

    if [ $lenAr -gt 1 ]; then
        JOINER_CLONE_HOST=$(echo $WSREP_SST_OPT_ADDR_LOCAL | cut -d ':' -f 1)
        JOINER_CLONE_PORT=$(echo $WSREP_SST_OPT_ADDR_LOCAL | cut -d ':' -f 2)
     else
        JOINER_CLONE_HOST="$WSREP_SST_OPT_ADDR_LOCAL"
        JOINER_CLONE_PORT=$CLONE_INSTANCE_PORT
    fi

    wsrep_log_debug "-> JOINER_CLONE_HOST $JOINER_CLONE_HOST"
    wsrep_log_debug "-> JOINER_CLONE_PORT $JOINER_CLONE_PORT"

    # Define the tmp directory
    tmp_datadir=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" -d)

    CLONE_ERR="$WSREP_SST_OPT_DATA/$MODULE.err"
    CLONE_SQL="$WSREP_SST_OPT_DATA/$MODULE.sql"
    GRASTATE_FILE="$WSREP_SST_OPT_DATA/grastate.dat"

    CLONE_SOCK="$WSREP_SST_OPT_DATA/recover_clone.sock"
    CLONE_X_SOCK="$WSREP_SST_OPT_DATA/cmysqlx.sock"

    if [ ${#CLONE_X_SOCK} -ge 104 ]
    then
        wsrep_log_error "Unix socket path name length for CLONE SST receiver"\
                        "'$CLONE_X_SOCK' is greater than commonly acceptable"\
                        "limit of 104 bytes."
                        # Linux: 108, FreeBSD: 104
        exit $EINVAL
    fi

    [ -z "$WSREP_SST_OPT_CONF" ] \
    && DEFAULTS_FILE_OPTION="" \
    || DEFAULTS_FILE_OPTION="--defaults-file='$WSREP_SST_OPT_CONF'"

    [ -z "$WSREP_SST_OPT_CONF_SUFFIX" ] \
    && DEFAULTS_GROUP_OPTION="" \
    || DEFAULTS_GROUP_OPTION="--defaults-group-suffix='$WSREP_SST_OPT_CONF_SUFFIX'"

    # Parent process already did the master key rotation
    INVARIANT_OPTIONS="--binlog-rotate-encryption-master-key-at-startup=OFF"

    wsrep_log_debug "-> CLONE_SOCK $CLONE_SOCK "
    wsrep_log_debug "-> CLONE_LIBS $CLONE_LIBS "

    DEFAULT_OPTIONS=" \
     $DEFAULTS_FILE_OPTION \
     $DEFAULTS_GROUP_OPTION \
     $INVARIANT_OPTIONS \
     --datadir='$WSREP_SST_OPT_DATA' \
     --wsrep_on=0 \
     --secure_file_priv='' \
     --socket='$CLONE_SOCK' \
     --log_error='$CLONE_ERR' \
     --pid_file='$CLONE_PID_FILE' \
     --plugin-dir='$CLONE_LIBS' \
     --port='$JOINER_CLONE_PORT' \
    "
    SKIP_NETWORKING_OPTIONS=" \
     --skip-mysqlx \
     --skip-networking \
    "

    # Define USER and PW
    CLONE_PSWD=`wsrep_gen_secret`"$PSWD_POLICY"
    CLONE_USER="clone_sst"

    ##################################################################################################
    # LET US Move the send of the READY here.
    # 1) we send the message ready
    # 2) we wait for donor to send message
    # 3) IF message is SST@DONORIP:PORT (inside the IST file) we exit loop and continue SST
    #    IF IST we exit SST script sending the UUID:position
    # 4) IF SST Process continue

    # Check if there is another netcat process on the IP port we need and try to kill it.
    NETCAT_KILL=$(ps -xo pid,command | grep -e "nc -l -k $JOINER_CLONE_HOST $JOINER_CLONE_PORT" | grep -v "grep" | cut -d ' ' -f 1)
    if [ ! "$NETCAT_KILL" == "" ];then
        wsrep_log_info "-> Existing NetCat PID: $NETCAT_KILL. Will try to kill it"
        kill -9 $NETCAT_KILL
        if [ "$?" != "0" ]; then
            wsrep_log_debug "-> Cannot kill the existing NetCat process PID $NETCAT_KILL"
        fi
    fi

    # OPEN NETCAT to receive messages from DONOR (like IST)
    wsrep_log_info "-> Opening NETCAT: nc -l $JOINER_CLONE_HOST $JOINER_CLONE_PORT"
    nc -l $JOINER_CLONE_HOST $JOINER_CLONE_PORT > $WSREP_SST_OPT_DATA/XST_FILE.txt &
    NC_PID=$!
    wsrep_log_info "-> NETCAT PID $NC_PID"
    if [ $NC_PID == "" ];then
        wsrep_log_error "-> Cannot open Netcat at given port $JOINER_CLONE_HOST $JOINER_CLONE_PORT check if the port is already taken"
        exit 1
    fi

    # Report clone credentials/address to the caller
    wsrep_log_debug "-> ready passing string |$CLONE_USER:CLONE_PSWD@$JOINER_CLONE_HOST:$JOINER_CLONE_PORT|"
    echo "ready $CLONE_USER:$CLONE_PSWD@$JOINER_CLONE_HOST:$JOINER_CLONE_PORT"

    # WAIT for Donor message
    wsrep_log_debug "-> wait $JOINER_TIMEOUT_WAIT_DONOR_MESSAGE"

    until grep -q ".*<EOF>$" "$WSREP_SST_OPT_DATA/XST_FILE.txt" &> /dev/null
    do
         if [ "$JOINER_TIMEOUT_WAIT_DONOR_MESSAGE" == "0" ]; then
            wsrep_log_error "************ FATAL ERROR ************"
            wsrep_log_error "TIMEOUT Waiting for the DONOR MESSAGE"
            wsrep_log_error "Donor message was either incomplete or not sent"
            donor_message=`cat $WSREP_SST_OPT_DATA/XST_FILE.txt`
            wsrep_log_error "donor message received: $donor_message"
            wsrep_log_error "*************************************"
            exit 1 ;
         fi
         sleep 1

        ((JOINER_TIMEOUT_WAIT_DONOR_MESSAGE-=1))
    done

    wsrep_log_debug "-> WAIT DIR DONOR MESSAGE DONE"

    if ! grep -q "SST@" "$WSREP_SST_OPT_DATA/XST_FILE.txt"; then
        wsrep_log_info "DONOR SAY IST"
        wsrep_log_debug "-> RECOVER POSITION TO SEND OUT DONOR IST"
        RP_PURGED=`cat $WSREP_SST_OPT_DATA/XST_FILE.txt`
        RP_PURGED=${RP_PURGED%"<EOF>"}
        wsrep_log_debug "-> POSITION: $RP_PURGED"
        rm -f  $WSREP_SST_OPT_DATA/XST_FILE.txt || :

        echo $RP_PURGED
        exit 0
    else
        RP_PURGED=`cat $WSREP_SST_OPT_DATA/XST_FILE.txt`
        wsrep_log_info "DONOR SAY SST ($RP_PURGED)"
        readonly CLEANUP_CLONE_PLUGIN=${RP_PURGED%|SST*}
        RP_PURGED_EMERGENCY=${RP_PURGED#*SST@}
        RP_PURGED_EMERGENCY=${RP_PURGED_EMERGENCY%"<EOF>"}
        wsrep_log_debug "-> CLONE Plugin to remove? (yes/no) = $CLEANUP_CLONE_PLUGIN"
        wsrep_log_debug "-> recovered position from DONOR: $RP_PURGED_EMERGENCY"
    fi

    ##################################################################################################
    # innodb_sys_tablespace_encrypt=1 requires keyring to be loaded at bootstrap
    # and it will work only with global manifest. So if it is enabled, and we use
    # local manifest/keyring config, we need to exit with error
    sys_tbl_enc_enabled=$(parse_cnf mysqld innodb_sys_tablespace_encrypt "off")
    sys_tbl_enc_enabled=$(normalize_boolean "$sys_tbl_enc_enabled" "off")
    if [ "$sys_tbl_enc_enabled" = "on" ]; then
        # sys_tablespace_encrypt will work only if we have global manifest and global config
        mainfest_config_type=$(get_keyring_manifest_and_config_types $WSREP_SST_OPT_DATA $WSREP_SST_OPT_PLUGINDIR $CLONE_BINARY)
        wsrep_log_debug "mainfest_config_type: $mainfest_config_type"
        if [ "$mainfest_config_type" != "manifest=global;config=global" ]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Joiner is configured to use system tablespace encryption."
            wsrep_log_error "Joiner is also configured to use local manifest"
            wsrep_log_error "or local keyring config file or to not use keyring at all."
            wsrep_log_error "These options are not compatible. Either disable system"
            wsrep_log_error "tablespace encryption or configure joiner to use global"
            wsrep_log_error "manifest and keyring component config."
            wsrep_log_error "Current config:"
            wsrep_log_error "innodb_sys_tablespace_encrypt=$sys_tbl_enc_enabled"
            wsrep_log_error "$mainfest_config_type"
            wsrep_log_error "Line $LINENO"
            wsrep_log_error "****************************************************** "
            exit 1
        fi
    fi

    # If we need to SST in any case we must remove the data, so let us do it here and be sure we work on a clean directory
    wsrep_log_info "Cleaning Data directory $WSREP_SST_OPT_DATA"
    ib_home_dir=$(parse_cnf mysqld innodb-data-home-dir "")
    ib_log_dir=$(parse_cnf mysqld innodb-log-group-home-dir "")
    ib_undo_dir=$(parse_cnf mysqld innodb-undo-directory "")

    cpat=$(parse_cnf sst cpat '.*\.pem$\|.*init\.ok$\|.*galera\.cache$\|.*gvwstate\.dat$\|.*\.err$\|.*\.log$\|.*RPM_UPGRADE_MARKER$\|.*RPM_UPGRADE_HISTORY$\|.*component_keyring_.*\.cnf$\|.*mysqld.my$')
    find $ib_home_dir $ib_log_dir $ib_undo_dir $WSREP_SST_OPT_DATA -mindepth 1  -regex $cpat  -prune  -o -exec rm -rfv {} 1>/dev/null \+

    # Before starting let us be sure we remove Netcat given it is using same MySQL port
    # check if there is another netcat process on the IP port we need and try to kill it.
    NETCAT_KILL=`ps -xo pid,command|grep -e "nc -l -k $JOINER_CLONE_HOST $JOINER_CLONE_PORT"|grep -v "grep"|cut -d ' ' -f 1`
    if [ ! "$NETCAT_KILL" == "" ];then
        wsrep_log_info "-> NetCat still up PID: $NETCAT_KILL. Will try to kill it"
        kill -9 $NETCAT_KILL
        if [ "$?" != "0" ]; then
            wsrep_log_debug "-> Cannot kill the existing NetCat process PID $NETCAT_KILL"
        fi
    fi

    NC_PID=""

    # No data dir, need to initialize one first, to make connections to
    # this node possible.
    # We need to use a temporary empty data directory, because the
    # actual datadir may already contain some wsrep-related files
    wsrep_log_info "Initializing data directory at $tmp_datadir"

    # Disable any option that would reqiure keyring component.
    # All of them can be safely disabled during the initialization and enabled
    # later. We disable them, to handle the case when joiner has local manifest
    # or keyring component config. In such a case it won't be possible to initialize
    # the datadir, because mysqld expects an empty dir (no manifest, no config)
    # The case when my.cnf has innodb_sys_tablespace_encrypt=1 is checked above,
    # so at this point there is innodb_sys_tablespace_encrypt=0 or manifest and
    # config are both global.
    CLONE_BOOTSTRAP_EXTRA=" --innodb-undo-log-encrypt=0 --innodb-redo-log-encrypt=0 --innodb-temp-tablespace-encrypt=0 --default-table-encryption=0 --innodb-encrypt-online-alter-logs=0 --binlog-encryption=0"

    echo "" > $CLONE_ERR
    eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS \
         $SKIP_NETWORKING_OPTIONS $CLONE_BOOTSTRAP_EXTRA \
         --initialize-insecure --wsrep_provider=none --datadir="$tmp_datadir" >> $CLONE_ERR 2>&1 || \
    ( wsrep_log_error "Failed to initialize data directory. Some hints below:"
      grep '[ERROR]' $CLONE_ERR | cut -d ']' -f 4- | while read msg
      do
          wsrep_log_error "> $msg"
      done
      wsrep_log_error "Full log at $CLONE_ERR"
      exit 1 )

    # Move initialized data directory structure to real datadir and cleanup
    mv -n "$tmp_datadir"/* "$WSREP_SST_OPT_DATA/"
    wsrep_log_debug "-> REMOVE $tmp_datadir"
    rm -rf "$tmp_datadir"

    wsrep_log_info "INITIALIZE DB DONE"

    # Need to create an extra user for 'localhost' because in some installations
    # by default exists user ''@'localhost' and shadows every user with '%'.
 cat << EOF > "$CLONE_SQL"
 SET SESSION sql_log_bin=OFF;
 CREATE USER '$CLONE_USER'@'%' IDENTIFIED $WITH_OPTION BY '$CLONE_PSWD';
 GRANT CLONE_ADMIN, SYSTEM_VARIABLES_ADMIN, SUPER, SHUTDOWN, EXECUTE ON *.* to '$CLONE_USER'@'%';
 GRANT INSERT ON mysql.plugin TO '$CLONE_USER'@'%';
 GRANT SELECT,UPDATE,INSERT ON performance_schema.* TO '$CLONE_USER'@'%';
 CREATE USER '$CLONE_USER'@'localhost' IDENTIFIED $WITH_OPTION BY '$CLONE_PSWD';
 GRANT CLONE_ADMIN, SYSTEM_VARIABLES_ADMIN, SUPER, SHUTDOWN, EXECUTE ON *.* TO '$CLONE_USER'@'localhost';
 GRANT EXECUTE ON *.* TO '$CLONE_USER'@'localhost';
 GRANT INSERT ON mysql.plugin TO '$CLONE_USER'@'localhost';
 GRANT SELECT,UPDATE,INSERT ON performance_schema.* TO '$CLONE_USER'@'localhost';
EOF

    wsrep_log_info "Launching clone recipient daemon"
    wsrep_log_debug "-> using: $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS "
    wsrep_log_debug "-> Test connection as: -u$CLONE_USER -pxxxxxx -h $JOINER_CLONE_HOST -P $JOINER_CLONE_PORT"

    # Define client to be used on the Joiner side
    MYSQL_ACLIENT="$MYSQL_CLIENT -u$CLONE_USER -S$CLONE_SOCK --batch --skip_column_names --silent"
    wsrep_log_debug "-> MYSQL_ACLIENT: $MYSQL_ACLIENT"

    # HERE We start the instance that will get the clone and pass the CLONE_SQL file to create user
    eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS \
        --wsrep_provider=none --init_file="$CLONE_SQL" >> $CLONE_ERR &

    if [ $? -ne 0 ]; then
        wsrep_log_error "-> User Creation on Joiner node failed, possible permission denied. Check permissions for "
        #we will try to silently shutdown the instance
        `$MYSQL_ACLIENT -e "SHUTDOWN" 2> /dev/null` || :
        exit 1
    fi

    # Wait for the receiver process to start
    until [ -n "$(cat $CLONE_PID_FILE 2>/dev/null)" ]
    do
        sleep 0.2
    done
    CLONE_INSTANCE_PID=$(cat "$CLONE_PID_FILE")

    if [ -n "$WSREP_LOG_DEBUG" ]; then
        GRANTS=$($MYSQL_ACLIENT -uroot -NB -e "show grants for '$CLONE_USER'@'localhost'")
        wsrep_log_debug "-> Clone user grants: $GRANTS"
    fi

    export MYSQL_PWD=$CLONE_PSWD
#    wsrep_log_debug "-> Exported MySQL password $MYSQL_PWD"

    wsrep_log_info "Waiting for clone recipient daemon to be ready for connections at port $JOINER_CLONE_PORT"
    wsrep_log_debug "-> connecting as: $MYSQL_CLIENT -u$CLONE_USER -h$JOINER_CLONE_HOST -P$JOINER_CLONE_PORT"
    to_wait=$JOINER_TIMEOUT_WAIT_CLONE_INSTANCE # 90 sec by defaults but it can be changed in the config
    until $MYSQL_CLIENT -u$CLONE_USER -h$JOINER_CLONE_HOST -P$JOINER_CLONE_PORT \
          -e 'SELECT USER()' > /dev/null
    do
        if [ $to_wait -eq 0 ]
        then
            wsrep_log_error "Timeout waiting for clone recipient daemon"
            exit $ETIMEDOUT
        fi
        to_wait=$((to_wait - 1))
        sleep 1
    done

    wsrep_log_info "Joiner setup."
    setup_clone_plugin "recipient"

    wsrep_log_info "Waiting for clone recipient daemon to finish"

    set +e
    monitor_sst_progress "${MYSQL_ACLIENT} -NB -e 'SELECT * FROM performance_schema.clone_progress;'" ${WSREP_SST_IDLE_TIMEOUT} ${CLONE_INSTANCE_PID} ${WSREP_SST_OPT_PARENT}
    status=$?
    set -e

    if [ $status -eq 1 ]; then
        # parent process died, we need to exit as well
        wsrep_log_error "Parent mysqld process (PID:$WSREP_SST_OPT_PARENT) terminated unexpectedly."
        exit $EPIPE
    fi
    if [ $status -eq 2 ]; then
        # Stale SST, no progress
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Killing SST ($CLONE_INSTANCE_PID) with SIGKILL after stalling for ${WSREP_SST_IDLE_TIMEOUT} seconds."
        wsrep_log_error "Within the last ${WSREP_SST_IDLE_TIMEOUT} seconds (defined by the sst-idle-timeout variable),"
        wsrep_log_error "the SST process on the joiner (this node) has not received any data from the donor."
        wsrep_log_error "This error could be caused by broken network connectivity between"
        wsrep_log_error "the donor and the joiner (this node)."
        wsrep_log_error "Check the network connection and restart the joiner node."
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "****************************************************** "
        exit $ETIMEDOUT
    fi

    wsrep_log_info "clone recepient daemon finished"

    CLONE_INSTANCE_PID=""

    # Execute first restart for
    wsrep_log_info "FIRST RESTART to fix the dictionary"
    wsrep_log_info "Performing data recovery"
    wsrep_log_debug "-> RECOVERY COMMAND LINE: $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS --wsrep_provider=none"

    # Remove created clone user and SST pxc user
    wsrep_log_debug "-> CLEAN OR NOT? $CLEANUP_CLONE_PLUGIN"
    CLEANUP_CLONE_PLUGIN_SQL=""
    if [ "${CLEANUP_CLONE_PLUGIN}" == "yes" ]; then
            CLEANUP_CLONE_PLUGIN_SQL="UNINSTALL PLUGIN CLONE;"
    fi
cat << EOF > "$CLONE_SQL"
SET SESSION sql_log_bin=OFF;
DROP USER IF EXISTS $CLONE_USER@'%';
DROP USER IF EXISTS $CLONE_USER@'localhost';
DROP USER IF EXISTS 'mysql.pxc.sst.user'@'localhost';
$CLEANUP_CLONE_PLUGIN_SQL
SHUTDOWN;
EOF

    eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS --wsrep_provider=none --init_file="$CLONE_SQL"
    cleanup_exit_code=$?
    if [ $cleanup_exit_code -ne 0 ]; then
        wsrep_log_error "Clone instance cleanup failed with exit code: $cleanup_exit_code"
    fi

    wsrep_log_info "CLEANUP and SHUTDOWN done"
    wsrep_log_info "Second restart for recovery position"
    wsrep_log_debug "-> Second restart: $CLONE_BINARY $DEFAULT_OPTIONS $SKIP_NETWORKING_OPTIONS --wsrep_provider=none --wsrep_recover"
    eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS $SKIP_NETWORKING_OPTIONS --wsrep_provider=none --wsrep_recover >> $CLONE_ERR 2>&1

    RP="$(grep -a '\[WSREP\] Recovered position:' $CLONE_ERR || :)"
    RP_PURGED=$(echo "$RP" | sed 's/.*WSREP\]\ Recovered\ position://' | sed 's/^[ \t]*//')
    wsrep_log_info "Recovered POSITION: [$RP_PURGED]"

    # If an invalid recovery position is returned we report the one we get from Donor
    if [ "$RP_PURGED" == "00000000-0000-0000-0000-000000000000:-1" ]; then
        RP_PURGED=$RP_PURGED_EMERGENCY
    fi

    if [ -n "$RP_PURGED" ]; then
        ORIG_IFS=$IFS
        IFS=':'
        read -ra gtid_arr <<< "$RP_PURGED"
        uuid="${gtid_arr[0]}"
        position="${gtid_arr[1]}"
        IFS=$ORIG_IFS
        wsrep_log_debug "-> Creating grastate.dat file in $GRASTATE_FILE UUID: $uuid POSITION: $position"
cat << EOF > "$GRASTATE_FILE"
# GALERA saved state
version: 2.1
uuid:    $uuid
seqno:   $position
safe_to_bootstrap: 0
EOF

    else
        # $RP_PURGED empty
        wsrep_cleanup_progress_file
        wsrep_log_debug "Failed to recover position from $CLONE_ERR";
        wsrep_log_debug "Invalid Recovery position. Exiting with error $ENODATA (No data available)"
        # We terminate but save the log for inspections given the failure
        CLEANUP_FILES=0
        exit $ENODATA
    fi

    echo $RP_PURGED
    CLEANUP_FILES=1
    # exit 0 is at the end of the script, after printing the message
else
    wsrep_log_error "Unrecognized role: '$WSREP_SST_OPT_ROLE'"
    exit $EINVAL
fi

wsrep_log_debug "-> SST PROCESS FINISHED for $WSREP_SST_OPT_ROLE"
exit 0
