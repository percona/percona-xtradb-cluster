#!/usr/bin/env bash

# Copyright (C) 2010-2014 Codership Oy
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

# This is a reference script for rsync-based state snapshot tansfer

set -o nounset -o errexit

RSYNC_PID=
RSYNC_CONF=
KEYRING_FILE=
OS=$(uname)
[ "$OS" == "Darwin" ] && export -n LD_LIBRARY_PATH

# Setting the path for lsof on CentOS
export PATH="/usr/sbin:/sbin:$PATH"

. $(dirname $0)/wsrep_sst_common

wsrep_check_programs rsync

encrypt=0
keyring_plugin=0
keyring_file_data=""
keyring_vault_config=""

keyring_file_data=$(parse_cnf mysqld keyring-file-data "")
keyring_vault_config=$(parse_cnf mysqld keyring-vault-config "")
if [[ -n $keyring_file_data || -n $keyring_vault_config ]]; then
     keyring_plugin=1
fi

if [[ -n $keyring_vault_config ]]; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "SST using rsync with vault plugin is not supported." \
                    " Percona recommends using SST with xtrabackup for" \
                    " data-at-rest encryption (using keyring)."
    wsrep_log_error "****************************************************** "
    exit 255 # unknown
fi

if [[ $keyring_plugin -eq 1 ]]; then
    wsrep_log_warning "Percona doesn't recommend using data-at-rest encryption" \
                      " (using keyring) with rsync. Please use SST with xtrabackup."
fi

RSYNC_REAL_PID=

cleanup_joiner()
{
    wsrep_log_debug "Joiner cleanup. rsync PID: $RSYNC_REAL_PID"
    [ "0" != "$RSYNC_REAL_PID" ]            && \
    kill $RSYNC_REAL_PID                    && \
    sleep 0.5                               && \
    kill -9 $RSYNC_REAL_PID >/dev/null 2>&1 || \
    :
    rm -f "$RSYNC_CONF"
    rm -f "$MAGIC_FILE"
    rm -f "$RSYNC_LOG_FILE"
    rm -f "$RSYNC_PID"
    rm -f "$KEYRING_FILE"
    rm -f "$STUNNEL_CONF"
    rm -f "$STUNNEL_PID"
    wsrep_log_debug "Joiner cleanup done."

    if [ "${WSREP_SST_OPT_ROLE}" = "joiner" ];then
        wsrep_cleanup_progress_file
    fi
}

check_pid()
{
    local pid_file=$1
    [ -r "$pid_file" ] && ps -p $(cat $pid_file) >/dev/null 2>&1
}

check_pid_and_port()
{
    local pid_file=$1
    local rsync_pid=$2
    local rsync_port=$3

    case $OS in
    FreeBSD)
        local port_info="$(sockstat -46lp ${rsync_port} 2>/dev/null | \
            grep ":${rsync_port}")"
        local is_rsync="$(echo $port_info | \
            grep -E '[[:space:]]+(rsync|stunnel)[[:space:]]+'"$rsync_pid" 2>/dev/null)"
        ;;
    *)
        if ! which lsof > /dev/null; then
            wsrep_log_error "lsof tool not found in PATH! Make sure you have it installed."
            exit 2 # ENOENT
        fi

        local port_info="$(lsof -i :$rsync_port -Pn 2>/dev/null | \
            grep "(LISTEN)")"
        local is_rsync="$(echo $port_info | \
            grep -E '^(rsync|stunnel)[[:space:]]+'"$rsync_pid" 2>/dev/null)"
        ;;
    esac

    if [ -n "$port_info" -a -z "$is_rsync" ]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "rsync daemon port '$rsync_port' has been taken"
        wsrep_log_error "****************************************************** "
        exit 16 # EBUSY
    fi
    check_pid $pid_file && \
        [ -n "$port_info" ] && [ -n "$is_rsync" ] && \
        [ $(cat $pid_file) -eq $rsync_pid ]
}

is_local_ip()
{
  local address="$1"
  local get_addr_bin=`which ifconfig`
  if [ -z "$get_addr_bin" ]
  then
    get_addr_bin=`which ip`
    get_addr_bin="$get_addr_bin address show"
    # Add an slash at the end, so we don't get false positive : 172.18.0.4 match
    # ip output format is "X.X.X.X/mask"
    address="${address}/"
  else
    # Add an space at the end, so we don't get false positive : 172.18.0.4 match
    # ifconfig output format is "X.X.X.X "
    address="$address "
  fi

  $get_addr_bin | grep -F "$address" > /dev/null
}

STUNNEL_CONF="$WSREP_SST_OPT_DATA/stunnel.conf"
rm -f "$STUNNEL_CONF"

STUNNEL_PID="$WSREP_SST_OPT_DATA/stunnel.pid"
rm -f "$STUNNEL_PID"

MAGIC_FILE="$WSREP_SST_OPT_DATA/rsync_sst_complete"
rm -rf "$MAGIC_FILE"

RSYNC_LOG_FILE="$WSREP_SST_OPT_DATA/rsync_sst_log"
rm -f "$RSYNC_LOG_FILE"

BINLOG_TAR_FILE="$WSREP_SST_OPT_DATA/wsrep_sst_binlog.tar"
BINLOG_N_FILES=1
rm -f "$BINLOG_TAR_FILE" || :

if ! [ -z $WSREP_SST_OPT_BINLOG ]
then
    BINLOG_DIRNAME=$(dirname $WSREP_SST_OPT_BINLOG)
    BINLOG_FILENAME=$(basename $WSREP_SST_OPT_BINLOG)
fi

KEYRING_FILE="$WSREP_SST_OPT_DATA/keyring-sst"
rm -rf "$KEYRING_FILE"


WSREP_LOG_DIR=${WSREP_LOG_DIR:-""}
# if WSREP_LOG_DIR env. variable is not set, try to get it from my.cnf
if [ -z "$WSREP_LOG_DIR" ]; then
    WSREP_LOG_DIR=$(parse_cnf mysqld-5.6 innodb-log-group-home-dir "")
fi
if [ -z "$WSREP_LOG_DIR" ]; then
    WSREP_LOG_DIR=$(parse_cnf mysqld innodb-log-group-home-dir "")
fi
if [ -z "$WSREP_LOG_DIR" ]; then
    WSREP_LOG_DIR=$(parse_cnf server innodb-log-group-home-dir "")
fi

if [ -n "$WSREP_LOG_DIR" ]; then
    # handle both relative and absolute paths
    WSREP_LOG_DIR=$(cd $WSREP_SST_OPT_DATA; mkdir -p "$WSREP_LOG_DIR"; cd $WSREP_LOG_DIR; pwd -P)
else
    # default to datadir
    WSREP_LOG_DIR=$(cd $WSREP_SST_OPT_DATA; pwd -P)
fi

# Old filter - include everything except selected
# FILTER=(--exclude '*.err' --exclude '*.pid' --exclude '*.sock' \
#         --exclude '*.conf' --exclude core --exclude 'galera.*' \
#         --exclude grastate.txt --exclude '*.pem' \
#         --exclude '*.[0-9][0-9][0-9][0-9][0-9][0-9]' --exclude '*.index')

# New filter - exclude everything except dirs (schemas) and innodb files
FILTER=(-f '- /lost+found'
        -f '- /.fseventsd'
        -f '- /.Trashes'
        -f '+ /wsrep_sst_binlog.tar'
        -f '+ /ib_lru_dump'
        -f '+ /ibdata*'
        -f '+ /undo*'
        -f '+ /*.ibd'
        -f '+ /*/'
        -f '- /*')

# Fetch the encrypt option from cnf file.
encrypt=$(parse_cnf sst encrypt 0)

if [[ $encrypt -eq 1 || $encrypt -eq 2 || $encrypt -eq 3 || $encrypt -ge 5 ]]
then
    wsrep_log_error "********************* FATAL ERROR ************************ "
    wsrep_log_error "* encrypt=$encrypt is not supported with rsync based SST.  "
    wsrep_log_error "********************************************************** "
    return 2
fi

#
# Pull the parameters needed for encrypt=4.
#
if [ $encrypt -eq 4 ]
then

    # First look for the files specified in [sst] section, if not present then
    # search in [mysqld] section.
    ssl_ca=$(parse_cnf sst ssl-ca "")
    if [[ -z "$ssl_ca" ]]; then
        ssl_ca=$(parse_cnf mysqld ssl-ca "")
    fi
    ssl_cert=$(parse_cnf sst ssl-cert "")
    if [[ -z "$ssl_cert" ]]; then
        ssl_cert=$(parse_cnf mysqld ssl-cert "")
    fi
    ssl_key=$(parse_cnf sst ssl-key "")
    if [[ -z "$ssl_key" ]]; then
        ssl_key=$(parse_cnf mysqld ssl-key "")
    fi

    # Check that we have all the files
    verify_and_match_all_ssl_files "$WSREP_LOG_DIR" "$ssl_ca" "$ssl_cert" "$ssl_key"

    wsrep_log_debug "Using encrypt=4 ssl_ca=$ssl_ca  ssl_cert=$ssl_cert  ssl_key=$ssl_key"
else
    wsrep_log_debug "Using encrypt=0"
fi


#
# Setup stunnel parameters for encryption
#
if [ $encrypt -eq 4 ]
then

    STUNNEL=""
    STUNNEL_VERSION=""
    if wsrep_check_programs stunnel > /dev/null
    then
        wsrep_log_info "Using stunnel for SSL encryption: ssl_ca=$ssl_ca  ssl_cert=$ssl_cert  ssl_key=$ssl_key"
        STUNNEL="stunnel ${STUNNEL_CONF}"
        STUNNEL_VERSION=`stunnel -version 2>&1 | grep -oe '[0-9]\.[0-9][\.0-9]*' | head -n1`
    else
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "stunnel program required for SSL encryption was not found in PATH. Can't continue"
        wsrep_log_error "****************************************************** "
        exit 2 # ENOENT
    fi

    # Step-1: Setup VERIFY_OPT
    # verifyPeer and verifyChain were introduced in stunnel 5.34.
    # For older versions, use verify = level instead
    if ! check_for_version "$STUNNEL_VERSION" "5.34"; then
      VERIFY_OPT="verify = 2"
    else
      VERIFY_OPT="verifyChain = yes"
    fi

    # Step-2: Setup CAFILE_OPT
    CAFILE_OPT="CAfile = $ssl_ca"

    # Step-3: Create stunnel.conf
cat << EOF > "$STUNNEL_CONF"
key = $ssl_key
cert = $ssl_cert
${CAFILE_OPT}
foreground = yes
pid = $STUNNEL_PID
debug = warning
client = yes
connect = ${WSREP_SST_OPT_HOST_UNESCAPED}:${WSREP_SST_OPT_PORT}
TIMEOUTclose = 0
${VERIFY_OPT}
EOF
#connect = ${WSREP_SST_OPT_ADDR%/*}

else
    # Handle encrypt = 0 cases here

    # Step-1: Setup VERIFY_OPT
    VERIFY_OPT=""

    # Step-2: Setup CAFILE_OPT
    CAFILE_OPT=""
fi

readonly SECRET_TAG="secret"

if [ "$WSREP_SST_OPT_ROLE" = "donor" ]
then

    if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
    then

        FLUSHED="$WSREP_SST_OPT_DATA/tables_flushed"
        rm -rf "$FLUSHED"

        # Use deltaxfer only for WAN
        inv=$(basename $0)
        [ "$inv" = "wsrep_sst_rsync_wan" ] && WHOLE_FILE_OPT="" \
                                           || WHOLE_FILE_OPT="--whole-file"

        echo "flush tables"

        # wait for tables flushed and state ID written to the file
        while [ ! -r "$FLUSHED" ] && ! grep -q ':' "$FLUSHED" >/dev/null 2>&1
        do
            sleep 0.2
        done

        STATE="$(cat $FLUSHED)"
        rm -rf "$FLUSHED"

        sync

        if ! [ -z $WSREP_SST_OPT_BINLOG ]
        then
            # Prepare binlog files
            pushd $BINLOG_DIRNAME &> /dev/null
            binlog_files_full=$(tail -n $BINLOG_N_FILES ${BINLOG_FILENAME}.index)
            binlog_files=""
            for ii in $binlog_files_full
            do
                binlog_files="$binlog_files $(basename $ii)"
            done
            if ! [ -z "$binlog_files" ]
            then
                wsrep_log_debug "Preparing binlog files for transfer:"
                tar -cf $BINLOG_TAR_FILE $binlog_files >&2
            fi
            popd &> /dev/null
        fi

        wsrep_log_info "Starting rsync of data-dir............"
        # first, the normal directories, so that we can detect incompatible protocol
        RC=0
        rsync ${STUNNEL:+--rsh="$STUNNEL"} \
              --owner --group --perms --links --specials \
              --ignore-times --inplace --dirs --delete --quiet \
              $WHOLE_FILE_OPT "${FILTER[@]}" "$WSREP_SST_OPT_DATA/" \
              rsync://$WSREP_SST_OPT_ADDR >&2 || RC=$?

        if [ "$RC" -ne 0 ]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "rsync returned code $RC:"
            wsrep_log_error "****************************************************** "

            case $RC in
            12) RC=71  # EPROTO
                wsrep_log_error \
                "rsync server on the other end has incompatible protocol. " \
                "Make sure you have the same version of rsync on all nodes."
                ;;
            22) RC=12  # ENOMEM
                ;;
            *)  RC=255 # unknown error
                ;;
            esac
            exit $RC
        fi

        # second, we transfer InnoDB log files
        rsync ${STUNNEL:+--rsh="$STUNNEL"} \
              --owner --group --perms --links --specials \
              --ignore-times --inplace --dirs --delete --quiet \
              $WHOLE_FILE_OPT -f '+ /ib_logfile[0-9]*' -f '- **' "$WSREP_LOG_DIR/" \
              rsync://$WSREP_SST_OPT_ADDR-log_dir >&2 || RC=$?
        if [ $RC -ne 0 ]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "rsync innodb_log_group_home_dir returned code $RC:"
            wsrep_log_error "****************************************************** "
            exit 255 # unknown error
        fi

        # third, transfer the keyring file (this requires SSL, check for encryption)
        if [[ -r $keyring_file_data ]]; then
            rsync ${STUNNEL:+--rsh="$STUNNEL"} \
                  --owner --group --perms --links --specials \
                  --ignore-times --inplace --quiet \
                  $WHOLE_FILE_OPT "$keyring_file_data" \
                  rsync://$WSREP_SST_OPT_ADDR/keyring-sst >&2 || RC=$?

            if [ $RC -ne 0 ]; then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "rsync keyring returned code $RC:"
                wsrep_log_error "****************************************************** "
                exit 255 # unknown error
            fi
        fi

        # then, we parallelize the transfer of database directories, use . so that pathconcatenation works
        pushd "$WSREP_SST_OPT_DATA" >/dev/null

        count=1
        [ "$OS" == "Linux" ] && count=$(grep -c processor /proc/cpuinfo)
        [ "$OS" == "Darwin" -o "$OS" == "FreeBSD" ] && count=$(sysctl -n hw.ncpu)

        find . -maxdepth 1 -mindepth 1 -type d -not -name "lost+found" \
             -print0 | xargs -I{} -0 -P $count \
             rsync ${STUNNEL:+--rsh="$STUNNEL"} \
             --owner --group --perms --links --specials \
             --ignore-times --inplace --recursive --delete --quiet \
             $WHOLE_FILE_OPT --exclude '*/ib_logfile*' "$WSREP_SST_OPT_DATA"/{}/ \
             rsync://$WSREP_SST_OPT_ADDR/{} >&2 || RC=$?

        popd >/dev/null

        if [ $RC -ne 0 ]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "find/rsync returned code $RC:"
            wsrep_log_error "****************************************************** "
            exit 255 # unknown error
        fi
        wsrep_log_info "...............rsync completed"
    else # BYPASS
        wsrep_log_info "Bypassing state dump (SST)."
        STATE="$WSREP_SST_OPT_GTID"
    fi

    # This is the very last piece of data to send
    # After receiving the MAGIC_FILE, the joiner knows that it has
    # received all the data.
    printf "$STATE\n" > "$MAGIC_FILE"

    if [ -n "$WSREP_SST_OPT_REMOTE_PSWD" ]; then
        # Let joiner know that we know its secret
        echo "$SECRET_TAG ${WSREP_SST_OPT_REMOTE_PSWD}" >> ${MAGIC_FILE}
    fi

    rsync ${STUNNEL:+--rsh="$STUNNEL"} \
        --archive --quiet --checksum "$MAGIC_FILE" rsync://$WSREP_SST_OPT_ADDR

    echo "continue" # now server can resume updating data
    echo "done $STATE"

elif [ "$WSREP_SST_OPT_ROLE" = "joiner" ]
then

    touch $SST_PROGRESS_FILE
    MYSQLD_PID=$WSREP_SST_OPT_PARENT

    MODULE="rsync_sst"

    RSYNC_PID="$WSREP_SST_OPT_DATA/$MODULE.pid"
    # give some time for lingering rsync from previous SST to complete
    check_round=0
    while check_pid $RSYNC_PID && [ $check_round -lt 10 ]
    do
        wsrep_log_info "lingering rsync daemon found at startup, waiting for it to exit"
        check_round=$(( check_round + 1 ))
        sleep 1
    done

    if check_pid $RSYNC_PID
    then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "rsync daemon already running."
        wsrep_log_error "****************************************************** "
        exit 114 # EALREADY
    fi
    rm -rf "$RSYNC_PID"

    trap "exit 32" HUP PIPE
    trap "exit 3"  INT TERM ABRT
    trap cleanup_joiner EXIT

    RSYNC_CONF="$WSREP_SST_OPT_DATA/$MODULE.conf"

cat << EOF > "$RSYNC_CONF"
pid file = $RSYNC_PID
use chroot = no
read only = no
timeout = 300
transfer logging = true
log file = $RSYNC_LOG_FILE
log format = %o %a file:%f %l
[$MODULE]
    path = $WSREP_SST_OPT_DATA
[$MODULE-log_dir]
    path = $WSREP_LOG_DIR
EOF

    # listen at all interfaces (for firewalled setups)
    wsrep_log_info "Waiting for data-dir through rsync................"
    readonly RSYNC_PORT=${WSREP_SST_OPT_PORT:-4444}
    # If the IP is local listen only in it
    if is_local_ip "$WSREP_SST_OPT_HOST"
    then
      RSYNC_EXTRA_ARGS="--address $WSREP_SST_OPT_HOST"
      STUNNEL_ACCEPT="$WSREP_SST_OPT_HOST_UNESCAPED:$RSYNC_PORT"
    else
      # Not local, possibly a NAT, listen on all interfaces
      RSYNC_EXTRA_ARGS=""
      if [ $WSREP_SST_OPT_HOST = $WSREP_SST_OPT_HOST_UNESCAPED ]
      then
          # IPv4 address
          STUNNEL_ACCEPT="$RSYNC_PORT"
      else
          # IPv6 address
          # At least on Linux this will also listen on IPv4 port so this
          # IPv4/IPv6 distinction is mostly moot, but just in case someone
          # has system that does not support IPv6...
          STUNNEL_ACCEPT=":::$RSYNC_PORT"
      fi
    fi

    if [ $encrypt -eq 0 ]
    then
      rsync --daemon --no-detach --port "$RSYNC_PORT" --config "$RSYNC_CONF" \
            ${RSYNC_EXTRA_ARGS} &
      RSYNC_REAL_PID=$!
    else
      cat << EOF > "$STUNNEL_CONF"
key = $ssl_key
cert = $ssl_cert
${CAFILE_OPT}
foreground = yes
pid = $STUNNEL_PID
debug = warning
client = no
[rsync]
accept = $STUNNEL_ACCEPT
exec = $(which rsync)
execargs = rsync --server --daemon --config=$RSYNC_CONF .
EOF
      stunnel "$STUNNEL_CONF" &
      RSYNC_REAL_PID=$!
      RSYNC_PID=$STUNNEL_PID
    fi

    until check_pid_and_port $RSYNC_PID $RSYNC_REAL_PID $RSYNC_PORT
    do
        sleep 0.2
    done

    if [ $encrypt -eq 4 ]
    then
        # find out my Common Name
        wsrep_check_programs openssl
        CN=$(openssl x509 -noout -subject -in "$ssl_cert" | \
             tr "," "\n" | grep "CN =" | cut -d= -f2 | sed s/^\ // | \
             sed s/\ %//)
        MY_SECRET=$(wsrep_gen_secret)
        # Add authentication data to address
        ADDR="$CN:$MY_SECRET@$WSREP_SST_OPT_HOST"
    else
        MY_SECRET="" # for check down in recv_joiner()
        ADDR=$WSREP_SST_OPT_HOST
    fi

    echo "ready $ADDR:$RSYNC_PORT/$MODULE"

    # wait for SST to complete by monitoring magic file
    while [ ! -r "$MAGIC_FILE" ] && check_pid "$RSYNC_PID" && \
          ps -p $MYSQLD_PID >/dev/null
    do
        sleep 1
    done

    if ! ps -p $MYSQLD_PID >/dev/null
    then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Parent mysqld process (PID:$MYSQLD_PID) terminated unexpectedly."
        wsrep_log_error "****************************************************** "
        exit 32
    fi

    if [ -r "$MAGIC_FILE" ]
    then
        # check donor supplied secret
        SECRET=$(grep "$SECRET_TAG " ${MAGIC_FILE} 2>/dev/null | cut -d ' ' -f 2)
        if [[ $SECRET != $MY_SECRET ]]; then
            wsrep_log_error "Donor does not know my secret!"
            wsrep_log_info "Donor:'$SECRET', my:'$MY_SECRET'"
            exit 32
        fi

        # remove secret from magic file
        grep -v "$SECRET_TAG " ${MAGIC_FILE} > ${MAGIC_FILE}.new
        mv ${MAGIC_FILE}.new ${MAGIC_FILE}

        cat "$MAGIC_FILE" # output UUID:seqno
    else
        # this message should cause joiner to abort
        echo "rsync process ended without creating '$MAGIC_FILE'"
    fi

    if ! [ -z $WSREP_SST_OPT_BINLOG ]
    then

        pushd $BINLOG_DIRNAME &> /dev/null
        if [ -f $BINLOG_TAR_FILE ]
        then
            # Clean up old binlog files first
            rm -f ${BINLOG_FILENAME}.* 2> /dev/null
            wsrep_log_debug "Extracting binlog files:"
            tar -xf $BINLOG_TAR_FILE >&2
            for ii in $(ls -1 ${BINLOG_FILENAME}.*)
            do
                echo ${BINLOG_DIRNAME}/${ii} >> ${BINLOG_FILENAME}.index
            done
        fi
        popd &> /dev/null
    fi

    # We need to determine the transfer_type
    # Do this by looking at the rsync transfer logs
    transfer_type=""
    if [[ ! -r $RSYNC_LOG_FILE ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "FATAL: Could not find the rsync log file : $RSYNC_LOG_FILE"
        wsrep_log_error "****************************************************** "
        exit 32
    fi
    # If we're received a file from the mysql/ directory
    # Then this is an SST, for an IST we will only receive the $MAGIC_FILE
    if grep -q file:mysql/ "$RSYNC_LOG_FILE"; then
        transfer_type="sst"
    else
        transfer_type="ist"
    fi
    wsrep_log_debug "Transfer-type determined to be: $transfer_type"

    if [[ $transfer_type == "sst" ]]; then
        if [[ -n $keyring_file_data ]]; then
            if [[ -r $KEYRING_FILE ]]; then
                wsrep_log_info "Moving sst keyring into place: moving $KEYRING_FILE to $keyring_file_data"
                mv $KEYRING_FILE $keyring_file_data
            else
                # error, missing file
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "FATAL: rsync could not find '${KEYRING_FILE}'"
                wsrep_log_error "The joiner is using a keyring file but the donor has not sent"
                wsrep_log_error "a keyring file.  Please check your configuration to ensure that"
                wsrep_log_error "both sides are using a keyring file"
                wsrep_log_error "****************************************************** "
                exit 32
            fi
        else
            if [[ -r $KEYRING_FILE ]]; then
                # error, file should not be here
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "FATAL: rsync found '${KEYRING_FILE}'"
                wsrep_log_error "The joiner is not using a keyring file but the donor has sent"
                wsrep_log_error "a keyring file.  Please check your configuration to ensure that"
                wsrep_log_error "both sides are using a keyring file"
                wsrep_log_error "****************************************************** "
                rm -rf $KEYRING_FILE
                exit 32
            fi
        fi
    fi


    wsrep_cleanup_progress_file
    wsrep_log_info "..............rsync completed"
else
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "Unrecognized role: '$WSREP_SST_OPT_ROLE'"
    wsrep_log_error "****************************************************** "
    exit 22 # EINVAL
fi

rm -f $BINLOG_TAR_FILE || :

exit 0
