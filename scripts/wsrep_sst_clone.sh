#!/usr/bin/env bash

# Copyright (C) 2020 Codership Oy
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
# This is a reference script for clone-based state snapshot tansfer
#

#############################################################################
# While this script is based on clone script from Codership, we have
# done many changes to adapt it to PXC
#===========================================================================
#              ┌──────────────────┐                      
#              │  Joiner starts   │                      
#              └────────┬─────────┘                      
#              ┌────────▼─────────┐                      
#              │open NetCat listnr│                      
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
#              │And performs 3 restarts│                 
#              └──────────┬────────────┘                 
#              ┌──────────▼──────────────┐               
#              │1) To fix dictionary     │               
#              │2) To recover position   │               
#              │3) Final cleanup         │               
#              └──────────┬──────────────┘               
#              ┌──────────▼──────────────┐               
#              │Send final ready signal  │               
#              │Waits for IST            │               
#              └─────────────────────────┘               
#----------------------------------------------------------------------------
# Config variables:
#[sst]
#netcat_port=4442
#wsrep-debug=true
#clone_instance_port=4444
#
#############################################################################

CMDLINE="$0 $*"

set -o nounset -o errexit

readonly EINVAL=22
readonly EPIPE=32
readonly ETIMEDOUT=110
progress_monitor=""
PARENT_PID=""
CLEANUP_CLONE_PLUGIN=""
CLEANUP_CLONE_SSL=""
CLONE_USER=""
TIMOUT_WAIT_XST=60
TIMOUT_WAIT_JOINER_CLONE_INSTANCE=200
NC_PID=""
RP_PURGED_EMERGENCY=""

# wsrep_gen_secret() generates enough randomness, yet some MySQL password
# policy may insist on having upper and lower case letters, numbers and
# special symbols. Make sure that whatever we generate, it has it.
readonly PSWD_POLICY="Aa"

OS=$(uname)
[ "$OS" = "Darwin" ] && export -n LD_LIBRARY_PATH

. $(dirname "$0")/wsrep_sst_common_clone

wsrep_log_info "Running: $CMDLINE"

MYPID=$$

if [ "$WSREP_SST_OPT_PARENT" == "" ]; then 
        PARENT_PID=`cat /proc/$MYPID/status | grep PPid`
    else
        PARENT_PID=$WSREP_SST_OPT_PARENT
fi 
wsrep_log_debug "-> MYPID: $MYPID PARENT PID $PARENT_PID"


#Reading netcat port if available in config 
TSST_PORT=$(parse_cnf sst netcat_port "4442") 

#reading clone instance port if available in config
CLONE_INSTANCE_PORT=$(parse_cnf sst clone_instance_port "4444")

# To not use possible [client] section in my.cnf
MYSQL_CLIENT="$MYSQL_CLIENT --no-defaults"


cleanup_donor()
{
    wsrep_log_info "Cleanup DONOR."
    wsrep_log_debug "Cleanup MySQL ADMIN_PSWD: $ADMIN_PSWD"
    wsrep_log_debug "Cleanup MySQL MYSQL_ACLIENT: $MYSQL_ACLIENT"
    export MYSQL_PWD=$ADMIN_PSWD
    if [ "$CLEANUP_CLONE_PLUGIN" == "yes" ]
    then
        CLEANUP_CLONE_PLUGIN="UNINSTALL PLUGIN CLONE;"
    else
        if [  "$CLEANUP_CLONE_SSL" == "" ]
        then
            $MYSQL_ACLIENT -e "SET GLOBAL loose_clone_ssl_cert='';
                               SET GLOBAL loose_clone_ssl_key='';
                               SET GLOBAL loose_clone_ssl_ca='';" || :
        fi
    fi
    if [ ! "$CLONE_USER" == "" ]; then  
        $MYSQL_ACLIENT -e "DROP USER IF EXISTS '$CLONE_USER';  SET wsrep_on = OFF; $CLEANUP_CLONE_PLUGIN" || : 
    fi

    rm -rf "$CLONE_EXECUTE_SQL" ||:
    rm -rf "$CLONE_PREPARE_SQL" ||:
    wsrep_log_info "Cleanup DONOR DONE."
}

cleanup_joiner()
{
    wsrep_log_info "Joiner cleanup. SST daemon PID: $CLONE_REAL_PID"
    rm -rf $CLONE_SOCK_DIR || :
    rm -rf $CLONE_PID_FILE || :
    rm -fr $tmp_datadir || :
    rm -f $WSREP_SST_OPT_DATA/XST_FILE.txt || :
    kill -15 $NC_PID || : > /dev/null 2>&1
    
   if [ $CLEANUP_FILES ]
    then
        rm -rf $CLONE_ERR || :
        rm -rf $CLONE_SQL || :
    fi
    wsrep_log_info "Joiner cleanup done."
}

# Check whether process in PID file is still running.
check_pid_file()
{
    local pid_file=$1
    [ -r "$pid_file" ] && ps -p $(cat $pid_file) >/dev/null 2>&1
}

check_parent()
{
    local parent_pid=$1
    if ! ps -p $parent_pid >/dev/null
    then
        wsrep_log_error \
        "Parent mysqld process (PID:$parent_pid) terminated unexpectedly."
        exit $EPIPE
    fi
}

# Check client version
check_client_version()
{
    local readonly min_version="8.0.19"
    IFS="." read -ra min_vers <<< $min_version # split into major minor and patch

    local readonly client_version=${1%%[-/]*} # take only a.b.c from a.b.c-d.e
    IFS="." read -ra client_vers <<< $client_version

    for i in ${!min_vers[@]}
    do
        if [ "${client_vers[$i]}" -lt "${min_vers[$i]}" ]
        then
            wsrep_log_error "this operation requires MySQL client version $min_version," \
                            " this client is '$client_version'"
            return $EINVAL
        fi
    done
}

trim_to_mysql_user_length()
{
    local readonly max_length=32
    local input=$1
    local readonly input_length=${#input}

    if [ $input_length -gt $max_length ]
    then
        local readonly tail_length=6 # to fit port completely
        local readonly head_length=$(($max_length - $tail_length))
        local readonly tail_offset=$(($input_length - $tail_length + 1))
        local readonly head_str=$(echo $input | cut -c -$head_length)
        local readonly tail_str=$(echo $input | cut -c $tail_offset-)
        input=$head_str$tail_str
    fi

    # here we potentially may want to filter out "bad" characters
    echo $input
}

# The following function:
# 1. checks if CLONE plugin is loaded. If not loads it.
# 2. checks if SSL cert and key are configured for any of and in that order:
#    a. CLONE plugin
#    b. MySQL in general
#    c. in [sst] section of my.cnf
# 3. If SSL is configured, but not for the CLONE plugin explicitly, sets up
#    corresponging CLONE plugin variables
#
# Requires environment variables: MYSQL_ACLIENT, MYSQL_PWD
# Sets the following environment variables:
# CLEANUP_PLUGIN, REQUIRE_SSL, CLONE_SSL_CERT,
# CLONE_SSL_KEY, CLONE_SSL_CA, CLIENT_SSL_OPTIONS, CLEANUP_SSL
#
setup_clone_plugin()
{
    # Either donor or recipient
    local -r ROLE=$1

    local CLONE_PLUGIN_LOADED=`$MYSQL_ACLIENT -e "SELECT COUNT(*) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_TYPE = 'CLONE';"`
    if [ "$CLONE_PLUGIN_LOADED" -eq 0 ]
    then
        wsrep_log_info "Installing CLONE plugin"

        # INSTALL PLUGIN is replicated by default, so we need to switch off
        # session replication on donor
        if [ "$ROLE" = "donor" ]
        then
            WSREP_OFF="SET SESSION wsrep_on=OFF; "
        else
            WSREP_OFF="" # joiner does not have replication enabled
        fi
        $MYSQL_ACLIENT -e "${WSREP_OFF}INSTALL PLUGIN CLONE SONAME 'mysql_clone.so';"
        CLEANUP_CLONE_PLUGIN="yes"
        CLONE_SSL_CERT="NULL"
        CLONE_SSL_KEY="NULL"
        CLONE_SSL_CA="NULL"
    else
        CLEANUP_CLONE_PLUGIN=""
        CLONE_SSL_CERT=`$MYSQL_ACLIENT -e "SELECT @@clone_ssl_cert"`
        CLONE_SSL_KEY=`$MYSQL_ACLIENT -e "SELECT @@clone_ssl_key"`
        CLONE_SSL_CA=`$MYSQL_ACLIENT -e "SELECT @@clone_ssl_ca"`
    fi

    local CLIENT_SSL_CERT=$(parse_cnf sst ssl_cert "")
    local CLIENT_SSL_KEY=$(parse_cnf sst ssl_key "")
    local CLIENT_SSL_CA=$(parse_cnf sst ssl_ca "")
    local CLIENT_SSL_MODE=$(parse_cnf sst ssl_mode "")

    if [ -z "$CLIENT_SSL_CERT" -o -z "$CLIENT_SSL_KEY" ]
    then
        CLIENT_SSL_CERT=$(parse_cnf client ssl_cert "")
        CLIENT_SSL_KEY=$(parse_cnf client ssl_key "")
        CLIENT_SSL_CA=$(parse_cnf client ssl_ca "")
        CLIENT_SSL_MODE=$(parse_cnf client ssl_mode "")
    fi

    local SERVER_SSL_CERT=`$MYSQL_ACLIENT -e "SELECT @@ssl_cert"`
    local SERVER_SSL_KEY=`$MYSQL_ACLIENT -e "SELECT @@ssl_key"`
    local SERVER_SSL_CA=`$MYSQL_ACLIENT -e "SELECT @@ssl_ca"`

    [ "$SERVER_SSL_CERT" = "NULL" ] && SERVER_SSL_CERT=
    [ "$SERVER_SSL_KEY" = "NULL" ] && SERVER_SSL_KEY=
    [ "$SERVER_SSL_CA" = "NULL" ] && SERVER_SSL_CA=

    if [ "$CLONE_SSL_CERT" = "NULL" -o "$CLONE_SSL_KEY" = "NULL" ]
    then
        wsrep_log_info "CLONE SSL not configured. Checking general MySQL SSL settings."

        if [ "$ROLE" = "donor" ]
        then
            CLONE_SSL_CERT=$SERVER_SSL_CERT
            CLONE_SSL_KEY=$SERVER_SSL_KEY
            CLONE_SSL_CA=$SERVER_SSL_CA
        else
            CLONE_SSL_CERT=$CLIENT_SSL_CERT
            CLONE_SSL_KEY=$CLIENT_SSL_KEY
            CLONE_SSL_CA=$CLIENT_SSL_CA
        fi

        if [ -n "$CLONE_SSL_CERT" -a -n "$CLONE_SSL_KEY" ]
        then
            wsrep_log_info "Using SSL configuration from MySQL Server."
            $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_cert='$CLONE_SSL_CERT'"
            $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_key='$CLONE_SSL_KEY'"
            $MYSQL_ACLIENT -e "SET GLOBAL clone_ssl_ca='$CLONE_SSL_CA'"
            CLEANUP_CLONE_SSL="yes"
        fi
    else
        if [ -n "$CLONE_SSL_CERT" -a -n "$CLONE_SSL_KEY" ]
        then
            wsrep_log_info "CLONE SSL already configured. Using it."
        else
            wsrep_log_info "CLONE SSL is explicitly empty: @@clone_ssl_cert='$CLONE_SSL_CERT', @@clone_ssl_key='$CLONE_SSL_KEY'"
        fi
        CLEANUP_CLONE_SSL=
    fi

    CLIENT_SSL_OPTIONS=""
    if [ -n "$CLONE_SSL_CERT" -a -n "$CLONE_SSL_KEY" ]
    then
        wsrep_log_info "Server SSL settings on $ROLE: CERT=$SERVER_SSL_CERT, KEY=$SERVER_SSL_KEY, CA=$SERVER_SSL_CA"
        wsrep_log_info "Client SSL settings on $ROLE: CERT=$CLIENT_SSL_CERT, KEY=$CLIENT_SSL_KEY, CA=$CLIENT_SSL_CA"
        wsrep_log_info "CLONE SSL settings on $ROLE: CERT=$CLONE_SSL_CERT, KEY=$CLONE_SSL_KEY, CA=$CLONE_SSL_CA"
        REQUIRE_SSL="REQUIRE SSL"

        if [ -n "$CLIENT_SSL_CERT" -a -n "$CLIENT_SSL_KEY" ]
        then
            CLIENT_SSL_OPTIONS="--ssl-cert=$CLIENT_SSL_CERT --ssl-key=$CLIENT_SSL_KEY"
            if [ -n "$CLIENT_SSL_CA"   ]
            then
                CLIENT_SSL_OPTIONS+=" --ssl-ca=$CLIENT_SSL_CA"
                [ -n "$CLIENT_SSL_MODE" ] && CLIENT_SSL_OPTIONS+=" --ssl-mode=$CLIENT_SSL_MODE"
            else
                CLIENT_SSL_OPTIONS+=" --ssl-mode=REQUIRED"
            fi
        fi
    else
        wsrep_log_info "No suitable SSL configuration found. Not using SSL for SST."
        CLEANUP_CLONE_SSL=
        REQUIRE_SSL=
    fi
    # for now we force no SSL in the clone operations
    CLEANUP_CLONE_SSL=
    REQUIRE_SSL=
    wsrep_log_info "SSL Not supported yet."
}

wsrep_log_debug "-> In the wsrep_sst_clone "

if test -z "$WSREP_SST_OPT_HOST"; then wsrep_log_error "HOST cannot be nil"; exit $EINVAL; fi
# MySQL client does not seem to agree to [] around IPv6 addresses
wsrep_check_programs sed
SST_HOST_STRIPPED=$(echo $WSREP_SST_OPT_HOST | sed 's/^\[//' | sed 's/\]$//')

PROGRESS=$(parse_cnf sst progress "")
if [ -n "$PROGRESS" -a "$PROGRESS" != "none" ]
then
    wsrep_log_warning "Unrecognized SST progress parameter value: '$PROGRESS'" \
                      "Recognized values are 'none' - disable progress reporting " \
                      "and empty string - enable progress reporting (default)" \
                      "SST progress reporting will be disabled"
    PROGRESS="none"
fi

# Option to CREATE USER
WITH_OPTION="WITH caching_sha2_password"

if [ "$WSREP_SST_OPT_ROLE" = "donor" ]
then
    echo "continue" # donor can resume updating data

    if [ ${MTR_SST_DONOR_DELAY:=0} -gt 0 ]
    then
        wsrep_log_info "Sleeping $MTR_SST_DONOR_DELAY seconds for MTR test"
        sleep $MTR_SST_DONOR_DELAY
    fi

    if test -z "$WSREP_SST_OPT_USER";   then wsrep_log_error "USER cannot be nil";   exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_REMOTE_USER"; then wsrep_log_error "REMOTE_USER cannot be nil"; exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_PORT";   then wsrep_log_error "PORT cannot be nil";   exit $EINVAL; fi
#    if test -z "$WSREP_SST_OPT_LPORT";  then wsrep_log_error "LPORT cannot be nil";  exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_SOCKET"; then wsrep_log_error "SOCKET cannot be nil"; exit $EINVAL; fi

    CLIENT_VERSION=$($MYSQL_CLIENT --version | grep -vi MariaDB | cut -d ' ' -f 4)
    check_client_version $CLIENT_VERSION

    readonly RECIPIENT_USER=$WSREP_SST_OPT_REMOTE_USER 
    readonly RECIPIENT_PSWD=$WSREP_SST_OPT_REMOTE_PSWD
    readonly ADMIN_USER=$WSREP_SST_OPT_USER
    readonly ADMIN_PSWD=$WSREP_SST_OPT_PSWD

    MYSQL_RCLIENT="$MYSQL_CLIENT -u$RECIPIENT_USER -h$SST_HOST_STRIPPED \
                   -P$WSREP_SST_OPT_PORT --batch --skip_column_names --silent"
    MYSQL_ACLIENT="$MYSQL_CLIENT -u$ADMIN_USER -S$WSREP_SST_OPT_SOCKET \
                   --batch --skip_column_names --silent"
 
    export MYSQL_PWD=$ADMIN_PSWD 
    if [ -z "$WSREP_SST_OPT_LPORT" ]; then
       readonly WSREP_SST_OPT_LPORT=`$MYSQL_ACLIENT -e "select @@port"` 
    fi
 
    wsrep_log_debug "-> WSREP_SST_OPT_LPORT: $WSREP_SST_OPT_LPORT "
    wsrep_log_debug "-> MYSQL_ACLIENT: $MYSQL_ACLIENT "
    wsrep_log_debug "-> MYSQL_ACLIENT ADMIN_PSWD: $ADMIN_PSWD"
    wsrep_log_debug "-> MYSQL_RCLIENT: $MYSQL_RCLIENT "
    wsrep_log_debug "-> MYSQL_RUSER: $RECIPIENT_USER | $WSREP_SST_OPT_REMOTE_USER | PWD  $WSREP_SST_OPT_REMOTE_PSWD"
    
    
    if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
    then
        #
        #  Prepare DONOR for cloning
        #
        CLEANUP_CLONE_PLUGIN=
        CLEANUP_CLONE_SSL=
        CLONE_PREPARE_SQL=
        CLONE_EXECUTE_SQL=
        CLONE_USER=`trim_to_mysql_user_length clone_to_${WSREP_SST_OPT_HOST}:${WSREP_SST_OPT_PORT}`
        CLONE_PSWD=`wsrep_gen_secret`$PSWD_POLICY

        cleanup_donor # Remove potentially existing clone user

        export MYSQL_PWD=$ADMIN_PSWD
        wsrep_log_debug "-> PREPARE DONOR"
        setup_clone_plugin "donor"
        
#        wsrep_log_info "REQUIRE_SSL=$REQUIRE_SSL, CLIENT_SSL_OPTIONS=$CLIENT_SSL_OPTIONS"
        wsrep_log_debug "-> PREPARED DONE"

        # It would be nice to limit clone user connections to only from
        # the recipient, however, even though we know the recipient address
        # there is no guarantee that when connecting from it source address
        # will be resolved to the same value. Hence we need to allow
        # connection from anywhere
        #  CLONE_USER_AT_HOST=\'$CLONE_USER\'@\'$WSREP_SST_OPT_HOST\'
        CLONE_USER_AT_HOST=\'$CLONE_USER\'

        # Use script files to avoid sensitive information on the command line
        CLONE_PREPARE_SQL=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" --suffix=.sql clone_prepare_XXXX)
        # TODO: last two grants may be unnecessary

cat << EOF > "$CLONE_PREPARE_SQL"
CREATE USER $CLONE_USER_AT_HOST IDENTIFIED BY '$CLONE_PSWD' $REQUIRE_SSL;
GRANT BACKUP_ADMIN ON *.* TO $CLONE_USER_AT_HOST;
GRANT SELECT ON performance_schema.* TO $CLONE_USER_AT_HOST;
GRANT EXECUTE ON *.* TO $CLONE_USER_AT_HOST;
EOF
        RC=0
        export MYSQL_PWD=$ADMIN_PSWD
        wsrep_log_debug "-> connecting to donor: $CLONE_PREPARE_SQL $MYSQL_PWD $MYSQL_ACLIENT"
        $MYSQL_ACLIENT --connect-timeout=60 < $CLONE_PREPARE_SQL || RC=$?
        wsrep_log_debug "-> $RC connect to donor done"
        
        if [ "$RC" -ne 0 ]
        then
            wsrep_log_error "Donor prepare returned code $RC"
            cat $CLONE_PREPARE_SQL >> /dev/stderr
            exit $RC
        fi
        #Before waiting for the Joiner Clone mysql we send out the message this is a SST
        wsrep_log_debug "-> NETCAT signal to nc -w 1 $WSREP_SST_OPT_HOST $TSST_PORT"
        echo "SST@$WSREP_SST_OPT_GTID" | nc -w 1 $WSREP_SST_OPT_HOST $TSST_PORT || :
        
        # we stay on hold now, waiting for the Joiner to expose the service
        wsrep_log_info "-> WAIT for Joiner MySQL to be available nc -w 1 -i 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_PORT"
        while [ 1 == 1 ]; do
        	NCPING=`nc -w 1 -i 1 $SST_HOST_STRIPPED $WSREP_SST_OPT_PORT 2> /dev/null` || : 
            if [ "$NCPING" == "" ]; then
                wsrep_log_info "[second(s) to timeout: $TIMOUT_WAIT_JOINER_CLONE_INSTANCE]"
            else 
                wsrep_log_info "Joiner instance available at $SST_HOST_STRIPPED:$WSREP_SST_OPT_PORT"
                break
            fi
            if [ "$TIMOUT_WAIT_JOINER_CLONE_INSTANCE" == "0" ]; then
                    wsrep_log_info "TIMEOUT Waiting for the Joiner to setup MySQL clone instance"
                    wsrep_log_error "TIMEOUT Waiting for the Joiner to setup MySQL clone instance $LINENO"
                    break ;
            fi            
            sleep 1
            ((TIMOUT_WAIT_JOINER_CLONE_INSTANCE-=1))  
        done
        #Wait is over
        wsrep_log_debug "-> Wait is over"
        
        export MYSQL_PWD="$RECIPIENT_PSWD"
        export MYSQL_USER="$RECIPIENT_USER"
        # Find own address (from which we connected)
        wsrep_log_debug "-> Connecting to JOINER to get exact IP of donor: $MYSQL_RCLIENT $MYSQL_PWD"
        USER=`$MYSQL_RCLIENT --skip-column-names -e 'SELECT USER()'`
        LHOST=${USER##*@}
        DONOR=$LHOST:$WSREP_SST_OPT_LPORT
        
        wsrep_log_debug "-> DONOR string: $DONOR"
        
        # Use script file to avoid sensitive information on the command line
        CLONE_EXECUTE_SQL=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" --suffix=.sql clone_execute_XXXX)
cat << EOF > "$CLONE_EXECUTE_SQL"
SET GLOBAL clone_valid_donor_list = '$DONOR';
CLONE INSTANCE FROM '$CLONE_USER'@'$LHOST':$WSREP_SST_OPT_LPORT IDENTIFIED BY '$CLONE_PSWD' $REQUIRE_SSL;
EOF

        wsrep_log_debug "JOINER CLONE ACTION SQL: $CLONE_EXECUTE_SQL $MYSQL_PWD $MYSQL_RCLIENT"
        if [[ -n "$WSREP_LOG_DEBUG" ]]; then
           CLONE_EXECUTE=`cat $CLONE_EXECUTE_SQL` || :
           wsrep_log_debug $CLONE_EXECUTE
        fi   
        
        # Actual cloning process
        wsrep_log_info "JOINER CLONE ACTION CLONING: connecting"
        $MYSQL_RCLIENT --connect-timeout=60 $CLIENT_SSL_OPTIONS < $CLONE_EXECUTE_SQL || RC=$?
        wsrep_log_info "JOINER CLONE ACTION  CLONING: done $RC"
       
        
        #we force the signal to be 0 because we KNOW that with clone when using the mysqld directly (not mysqld_safe) the daemon is shutdown at the end of the clone process, but sometime pxc return error. 
        if [ "$RC" -ne 0 ]; then
            RC=0
            wsrep_log_debug " JOINER CLONE ACTION abruptly terminated, but we can continue"
        fi
        
        #id still there we will manually shutdown
        eval $MYSQL_RCLIENT -e "SHUTDOWN" || :

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
        cleanup_donor
    else # BYPASS
        wsrep_log_info "Bypassing state dump."

        # Instruct recipient to shutdown
        #export MYSQL_PWD="$RECIPIENT_PSWD"
        wsrep_log_info "BYPASS SENDING IST_FILE TO JOINER NetCat: nc $WSREP_SST_OPT_HOST $TSST_PORT"
        echo $WSREP_SST_OPT_GTID | nc $WSREP_SST_OPT_HOST $TSST_PORT || : 
        sleep 4
        wsrep_log_debug "-> Exiting with gtid: $WSREP_SST_OPT_GTID"
    fi

    wsrep_log_debug "-> SENDING DONE $ADMIN_PSWD | $MYSQL_PWD | $WSREP_SST_OPT_GTID"
    #DONOR must be clean BEFORE exit given user is removed on DONE
    
    echo "done $WSREP_SST_OPT_GTID"

elif [ "$WSREP_SST_OPT_ROLE" = "joiner" ]
then
    CLEANUP_FILES=""
    wsrep_check_programs grep
    wsrep_check_programs ps
    wsrep_check_programs find

    if test -z "$WSREP_SST_OPT_DATA";   then wsrep_log_error "DATA cannot be nil";   exit $EINVAL; fi
    if test -z "$WSREP_SST_OPT_PARENT"; then wsrep_log_error "PARENT cannot be nil"; exit $EINVAL; fi

    #
    #  Find binary to run
    #
    PARENT_PROC=(`ps -hup $WSREP_SST_OPT_PARENT`)
    CLONE_BINARY=${PARENT_PROC[10]}
    CLONE_BINARY_SAFE="${CLONE_BINARY}_safe"
    
    if [ ! -x "$CLONE_BINARY" ]
    then
        # Not always ps shows full path to the executable
        # Look for it in the tree with a common ancestor
        basedir=$0
        while [ "/" != "$basedir" ]
        do
            basedir=$(dirname "$basedir")
            res=$(find "$basedir" -mount -type f -executable -name "$CLONE_BINARY" 2>/dev/null);
            if [ -x "$res" ]
            then
                CLONE_BINARY="$res"
                break
            fi
        done
        if [ ! -x "$CLONE_BINARY" ]
        then
            wsrep_log_error "Could not determine binary to run: $CLONE_BINARY"
            exit $EINVAL
        fi
    fi

    #
    # Find where libs needed for the binary are
    #
    CLONE_LIBS="$WSREP_SST_OPT_PLUGIN_DIR"

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

    # If port was not set up explicitly in wsrep_sst_address, check defaults
    wsrep_log_debug "-> WSREP_SST_OPT_PORT $WSREP_SST_OPT_PORT"
    
    if [ -n "${WSREP_SST_OPT_PORT:-}" ]
    then
        CLONE_PORT="$WSREP_SST_OPT_PORT"
    else
        CLONE_PORT=$(parse_cnf mysqld port "3306")
    fi
    CLONE_PORT=$CLONE_INSTANCE_PORT
    
    wsrep_log_debug "-> CLONE_PORT $CLONE_PORT"
    
    CLONE_ADDR="$WSREP_SST_OPT_HOST:$CLONE_PORT"
    CLONE_HOST="$WSREP_SST_OPT_HOST"
    wsrep_log_debug "-> CLONE_ADDR $CLONE_ADDR"
    wsrep_log_debug "-> CLONE_HOST $CLONE_HOST"

    #define the tmp directory
    tmp_datadir=$(wsrep_mktemp_in_dir "$WSREP_SST_OPT_DATA" -d)


    CLONE_ERR="$WSREP_SST_OPT_DATA/$MODULE.err"
    CLONE_SQL="$WSREP_SST_OPT_DATA/$MODULE.sql"
    IST_FILE="$WSREP_SST_OPT_DATA/IST_FILE.txt"
    GRASTATE_FILE="$WSREP_SST_OPT_DATA/grastate.dat"

    CLONE_SOCK="$WSREP_SST_OPT_DATA/recover_clone.sock"
    CLONE_X_SOCK="$WSREP_SST_OPT_DATA/cmysqlx.sock"

    DONOR_IP=""

    
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

    # parent process already did the master key rotation
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
     --socket="$CLONE_SOCK" \
     --skip-mysqlx \
     --log_error='$CLONE_ERR' \
     --pid_file='$CLONE_PID_FILE' \
     --plugin-dir='$CLONE_LIBS' \
     --port='$CLONE_PORT' \
    "
    SKIP_NETWORKING_OPTIONS=" \
     --skip-networking \
     --skip-mysqlx \
    "
    
    #define USER and PW 
    CLONE_PSWD=`wsrep_gen_secret`"$PSWD_POLICY"
    CLONE_USER="clone_sst"

##################################################################################################
    #LET US Move the send of the READY here.
    # 1) we send the message ready
    # 2) we wait for donor to send message
    # 3) IF message is SST@DONORIP:PORT (inside the IST file) we exit loop and continue SST
    #    IF IST we exit SST script sending the UUID:position 
    # 4) IF SST Process continue 

    #check if there is another netcat process on the IP port we need and try to kill it.
    NETCAT_KILL=`ps -x|grep -e "nc -l -k $WSREP_SST_OPT_HOST $TSST_PORT"|grep -v "grep"|cut -d ' ' -f 2`
    if [ ! "$NETCAT_KILL" == "" ];then
        wsrep_log_info "-> Existing NetCat PID: $NETCAT_KILL. Will try to kill it"
        kill -9 $NETCAT_KILL
        if [ "$?" != "0" ]; then 
            wsrep_log_debug "-> Cannot kill the existing NetCat process PID $NETCAT_KILL"  
        fi
    fi

    #OPEN NETCAT to receive messages from DONOR (like IST)
    wsrep_log_info "-> Opening NETCAT: nc -l $WSREP_SST_OPT_HOST $TSST_PORT"
    nc -l -k $WSREP_SST_OPT_HOST $TSST_PORT > $WSREP_SST_OPT_DATA/XST_FILE.txt & 
    NC_PID=$!
    wsrep_log_info "-> NETCAT PID $NC_PID"
    if [ "$NC_PID" == "" ];then
        wsrep_log_error "-> Cannot open Netcat at given port $WSREP_SST_OPT_HOST $TSST_PORT check if the port is already taken"
        exit 1
    fi
    
    # Report clone credentials/address to the caller
    wsrep_log_debug "-> ready passing string |$CLONE_USER:$CLONE_PSWD]$WSREP_SST_OPT_HOST:$CLONE_PORT|"
    echo "ready $CLONE_USER:$CLONE_PSWD]$WSREP_SST_OPT_HOST:$CLONE_PORT"

    #WAIT for Donor message
    wsrep_log_debug "-> wait $TIMOUT_WAIT_XST"   
    while [ ! -s  $WSREP_SST_OPT_DATA/XST_FILE.txt ];do 
          
         if [ "$TIMOUT_WAIT_XST" == "0" ]; then
            wsrep_log_error "TIMEOUT Waiting for the DONOR MESSAGE"
            break ;
         fi            
         sleep 1
         
        ((TIMOUT_WAIT_XST-=1))  
    done 
    
    wsrep_log_debug "-> WAIT DIR DONOR MESSAGE DONE"
    
    if [[ ! `cat $WSREP_SST_OPT_DATA/XST_FILE.txt` =~ "SST@" ]];then
        wsrep_log_info "DONOR SAY IST"
        wsrep_log_debug "-> RECOVER POSITION TO SEND OUT DONOR IST"
         RP_PURGED=`cat $WSREP_SST_OPT_DATA/XST_FILE.txt`   
         wsrep_log_debug "-> POSITION: $RP_PURGED"         
         echo $RP_PURGED
#         rm -f $CLONE_ERR || :
         rm -f  $WSREP_SST_OPT_DATA/XST_FILE.txt || :
         kill -15 $NC_PID || : > /dev/null 2>&1
         exit 0
    else
         wsrep_log_info "DONOR SAY SST"
         RP_PURGED=`cat $WSREP_SST_OPT_DATA/XST_FILE.txt`   
         RP_PURGED_EMERGENCY=${RP_PURGED#"SST@"}
         wsrep_log_debug "-> recovered position from DONOR: $RP_PURGED_EMERGENCY"
         
    fi    
##################################################################################################



    if [ ! -d "$WSREP_SST_OPT_DATA/mysql" -o \
         ! -r "$WSREP_SST_OPT_DATA/mysql.ibd" ]
    then
        # No data dir, need to initialize one first, to make connections to
        # this node possible.
        # We need to use a temporary empty data directory, because the
        # actual datadir may already contain some wsrep-related files    
        wsrep_log_info "Initializing data directory at $tmp_datadir"

        wsrep_log_info "INITIALIZE DB"  
        #reset CLONE log
        echo "" > $CLONE_ERR
        eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS \
             $SKIP_NETWORKING_OPTIONS \
             --initialize-insecure --wsrep_provider=none --datadir="$tmp_datadir" >> $CLONE_ERR 2>&1 || \
        ( wsrep_log_error "Failed to initialize data directory. Some hints below:"
          grep '[ERROR]' $CLONE_ERR | cut -d ']' -f 4- | while read msg
          do
              wsrep_log_error "> $msg"
          done
          wsrep_log_error "Full log at $CLONE_ERR"
          exit 1 )
          
                # move initialized data directory structure to real datadir and cleanup
        mv --force "$tmp_datadir"/* "$WSREP_SST_OPT_DATA/"
        sleep 2
        wsrep_log_debug "-> REMOVE $tmp_datadir"  
        rm -rf "$tmp_datadir"
  
        wsrep_log_info "INITIALIZE DB DONE"  
    fi 

# Need to create an extra user for 'localhost' because in some installations
# by default exists user ''@'localhost' and shadows every user with '%'.
 cat << EOF > "$CLONE_SQL"
 SET SESSION sql_log_bin=OFF;
 CREATE USER '$CLONE_USER'@'%' IDENTIFIED $WITH_OPTION BY '$CLONE_PSWD';
 GRANT CLONE_ADMIN, SYSTEM_VARIABLES_ADMIN, SUPER, SHUTDOWN, EXECUTE ON *.* to '$CLONE_USER'@'%';
 GRANT INSERT ON mysql.plugin to '$CLONE_USER';
 GRANT SELECT,UPDATE,INSERT ON performance_schema.* TO '$CLONE_USER';
 CREATE USER '$CLONE_USER'@'localhost' IDENTIFIED $WITH_OPTION BY '$CLONE_PSWD';
 GRANT CLONE_ADMIN, SYSTEM_VARIABLES_ADMIN, SUPER, SHUTDOWN, EXECUTE ON *.* TO '$CLONE_USER'@'localhost';
 GRANT EXECUTE ON *.* to '$CLONE_USER'@'localhost';
 GRANT INSERT ON mysql.plugin to '$CLONE_USER'@'localhost';
 GRANT SELECT,UPDATE,INSERT ON performance_schema.* TO '$CLONE_USER'@'localhost';
EOF

    wsrep_log_info "Launching clone recipient daemon"
    wsrep_log_info "-> using: $CLONE_ENV $CLONE_BINARY_SAFE $DEFAULT_OPTIONS "
    wsrep_log_debug "-> Test connection as: -u$CLONE_USER -p$CLONE_PSWD -h $CLONE_HOST -P $CLONE_PORT"
    
    #Define client
    MYSQL_ACLIENT="$MYSQL_CLIENT -u$CLONE_USER -S$CLONE_SOCK --batch --skip_column_names --silent"
    
    
    #HERE We start the instance that will get the clone
    eval WSREP_SST_CLONE=1 $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS \
        --wsrep_provider=none --init_file="$CLONE_SQL" >> $CLONE_ERR &
    
    wsrep_log_info "-> Initialization user returned $?"
     
    if [ ! "$?" == "0" ]; then
        wsrep_log_error "-> User Creation on Joiner node failed, possible permission denied. Check permissions for "
        #we will try to silently shutdown the instance
        `$MYSQL_ACLIENT -e "SHUTDOWN" 2> /dev/null` || : 
        exit 1
    fi

    # wait for the receiver process to start
    until [ -n "$(cat $CLONE_PID_FILE 2>/dev/null)" ]
    do
        sleep 0.2
    done
    CLONE_REAL_PID=`cat $CLONE_PID_FILE`
    
    if [[ -n "$WSREP_LOG_DEBUG" ]]; then
        GRANTS=`$MYSQL_ACLIENT -uroot -NB -e "show grants for '$CLONE_USER'@'localhost'"` 
        wsrep_log_debug "-> Clone user grants: $GRANTS"
    fi
    
    trap cleanup_joiner EXIT

    export MYSQL_PWD=$CLONE_PSWD
    wsrep_log_debug "-> Exported MySQL password $MYSQL_PWD"  
    
    wsrep_log_info "Waiting for clone recipient daemon to be ready for connections at port $CLONE_PORT"
    wsrep_log_debug "-> connecting as: $MYSQL_CLIENT -u$CLONE_USER -h$CLONE_HOST -P$CLONE_PORT"
    to_wait=60 # 1 minute Should we put this in the config?? 
    until $MYSQL_CLIENT -u$CLONE_USER -h$CLONE_HOST -P$CLONE_PORT \
          -e 'SELECT USER()' > /dev/null
    do
        if [ $to_wait -eq 0 ]
        then
            wsrep_log_error "Timeout waiting for clone recipient daemon"
            kill -9 $CLONE_REAL_PID || :
            exit $ETIMEDOUT
        fi
        to_wait=$(( $to_wait - 1 ))
        sleep 1
    done

#    MYSQL_ACLIENT="$MYSQL_CLIENT -u$CLONE_USER -S$CLONE_SOCK --batch --skip_column_names --silent"
    wsrep_log_info "-> MYSQL_ACLIENT: $MYSQL_ACLIENT"
    
    wsrep_log_info "Joiner setup."
    setup_clone_plugin "recipient"
  
    wsrep_log_info "Waiting for clone recipient daemon to finish"
   
   #getting clone process status report
    while check_pid_file "$CLONE_PID_FILE"
    do
        STATUS=`$MYSQL_ACLIENT -NB -e "select format(((data/estimate)*100),2) 'completed%' from performance_schema.clone_progress where stage like 'FILE_COPY';" 2> /dev/null` || :  
        wsrep_log_info "Copy at: ${STATUS}%"
        #wsrep_log_debug "Checking parent: $WSREP_SST_OPT_PARENT"
        check_parent $WSREP_SST_OPT_PARENT
        sleep 1 # Should this be in the config as well? 
    done
       
    CLONE_REAL_PID=0
    wsrep_log_info "clone recepient daemon finished"
 
    wsrep_log_info "Performing data recovery"

    wsrep_log_debug "-> RECOVERY COMMAND LINE: $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS --wsrep_provider=none"

    wsrep_log_info "FIRST RESTART for fixing dictionary"
    echo > $CLONE_PID_FILE
    eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS --wsrep_provider=none --init_file="$CLONE_SQL" &

   # wait for the receiver process to start
   until [ -f "${CLONE_SOCK}.lock" ]
   do
       sleep 2
       wsrep_log_debug "-> waiting"
   done
   # we want to be sure not only that the daemon is up but that all recovery is done 
   # correctly and we can connect before shutdon it again
   export MYSQL_PWD=$CLONE_PSWD
   until $MYSQL_CLIENT -u$CLONE_USER -h$CLONE_HOST -P$CLONE_PORT -e 'SHUTDOWN' 2> /dev/null
    do
        if [ $to_wait -eq 0 ]
        then
            wsrep_log_error "Timeout waiting for clone recipient daemon"
            kill -9 $CLONE_REAL_PID || :
            exit $ETIMEDOUT
        fi
        to_wait=$(( $to_wait - 1 ))
        sleep 1
    done
   CLONE_PID=`cat $CLONE_PID_FILE`  

   wsrep_log_debug "-> SHUTDOWN $CLONE_PID"
   kill -15 $CLONE_PID 2> /dev/null || :
   
   # wait for the receiver process to end
   wsrep_log_info "Waiting service to go down"
   until [ ! -f "${CLONE_SOCK}.lock" ]
   do
       sleep 2
       wsrep_log_info "waiting"
   done
#
   wsrep_log_info "SHUTDOWN done"
    
    wsrep_log_info "Second restart for recovery position"

    wsrep_log_debug "-> Second restart: $CLONE_BINARY $DEFAULT_OPTIONS $SKIP_NETWORKING_OPTIONS --wsrep_recover"
     eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS $SKIP_NETWORKING_OPTIONS --wsrep_recover >> $CLONE_ERR 2>&1

        
    RP="$(grep -a '\[WSREP\] Recovered position:' $CLONE_ERR || :)"
    RP_PURGED=`echo $RP | sed 's/.*WSREP\]\ Recovered\ position://' | sed 's/^[ \t]*//'`
    wsrep_log_info "Recovered POSITION: $RP_PURGED"
    
    #If an invalid recovery position is returned we report the one we get from Donor
    if [ "$RP_PURGED" == "00000000-0000-0000-0000-000000000000:-1" ]; then
        RP_PURGED=$RP_PURGED_EMERGENCY
    fi
    if [ ! "$RP_PURGED" == "" ]; then
        
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
        
    fi
    
    
    # Remove created clone user
cat << EOF > "$CLONE_SQL"
SET SESSION sql_log_bin=OFF;
DROP USER IF EXISTS $CLONE_USER;
DROP USER IF EXISTS $CLONE_USER@'localhost';
SHUTDOWN;
EOF

     wsrep_log_info "Third restart for cleanup"
      eval $CLONE_ENV $CLONE_BINARY $DEFAULT_OPTIONS $SKIP_NETWORKING_OPTIONS \
       --wsrep_provider=none  --init-file="$CLONE_SQL" >> $CLONE_ERR 2>&1

      if [ "$?" == "0" ]; then
        wsrep_log_info "CLEAN UP DONE" 
        if [ -z "$RP" ]
        then
            echo "Failed to recover position from $CLONE_ERR";
        else
            wsrep_log_debug "->SENDING MESSAGE: $RP_PURGED " 
            echo $RP | sed 's/.*WSREP\]\ Recovered\ position://' | sed 's/^[ \t]*//'
            CLEANUP_FILES=1
            exit 0 
        fi
        #forcing restart of parent
        CLEAN FILES=1
        wsrep_cleanup_progress_file 
        cleanup_joiner
        
        wsrep_log_debug "-> RESTARTING PARENT $PARENT_PID"
        kill -9 $PARENT_PID
      else
       wsrep_log_ERROR "Something failed in the cleanup"
      fi
   
else
    wsrep_log_error "Unrecognized role: '$WSREP_SST_OPT_ROLE'"
    exit $EINVAL
fi

wsrep_log_debug "-> SST PROCESS FINISHED for $WSREP_SST_OPT_ROLE $?"


exit 0