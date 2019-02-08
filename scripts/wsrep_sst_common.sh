# Copyright (C) 2012-2015 Codership Oy
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

# This is a common command line parser to be sourced by other SST scripts

set -u

WSREP_SST_OPT_BYPASS=0
WSREP_SST_OPT_BINLOG=""
WSREP_SST_OPT_CONF_SUFFIX=""
WSREP_SST_OPT_DATA=""
WSREP_SST_OPT_AUTH=${WSREP_SST_OPT_AUTH:-}
WSREP_SST_OPT_USER=${WSREP_SST_OPT_USER:-}
WSREP_SST_OPT_PSWD=${WSREP_SST_OPT_PSWD:-}
WSREP_SST_OPT_VERSION=""

WSREP_LOG_DEBUG=""

# These are the 'names' of the commands
# The paths to these are stored separately (MYSQLD_PATH for MYSQLD_NAME)
# The NAMEs don't change, however the PATHs may be different (due to packaging)
readonly MYSQLD_NAME=mysqld
readonly MYSQLADMIN_NAME=mysqladmin
readonly MYSQL_UPGRADE_NAME=mysql_upgrade
readonly MYSQL_CLIENT_NAME=mysql

declare MYSQL_UPGRADE_TMPDIR=""

while [ $# -gt 0 ]; do
case "$1" in
    '--address')
        readonly WSREP_SST_OPT_ADDR="$2"
        #
        # Break address string into host:port/path parts
        #
        if echo $WSREP_SST_OPT_ADDR | grep -qe '^\[.*\]'
        then
            # IPv6 notation
            readonly WSREP_SST_OPT_HOST=${WSREP_SST_OPT_ADDR/\]*/\]}
            readonly WSREP_SST_OPT_HOST_UNESCAPED=$(echo $WSREP_SST_OPT_HOST | \
                 cut -d '[' -f 2 | cut -d ']' -f 1)
        else
            # "traditional" notation
            readonly WSREP_SST_OPT_HOST=${WSREP_SST_OPT_ADDR%%[:/]*}
        fi
        readonly WSREP_SST_OPT_PORT=$(echo $WSREP_SST_OPT_ADDR | \
                cut -d ']' -f 2 | cut -s -d ':' -f 2 | cut -d '/' -f 1)
        readonly WSREP_SST_OPT_PATH=${WSREP_SST_OPT_ADDR#*/}
        shift
        ;;
    '--bypass')
        WSREP_SST_OPT_BYPASS=1
        ;;
    '--datadir')
        readonly WSREP_SST_OPT_DATA="$2"
        shift
        ;;
    '--defaults-file')
        readonly WSREP_SST_OPT_CONF="$2"
        shift
        ;;
    '--defaults-group-suffix')
        WSREP_SST_OPT_CONF_SUFFIX="$2"
        shift
        ;;
    '--host')
        readonly WSREP_SST_OPT_HOST="$2"
        shift
        ;;
    '--local-port')
        readonly WSREP_SST_OPT_LPORT="$2"
        shift
        ;;
    '--parent')
        readonly WSREP_SST_OPT_PARENT="$2"
        shift
        ;;
    '--password')
        WSREP_SST_OPT_PSWD="$2"
        shift
        ;;
    '--port')
        readonly WSREP_SST_OPT_PORT="$2"
        shift
        ;;
    '--role')
        readonly WSREP_SST_OPT_ROLE="$2"
        shift
        ;;
    '--socket')
        readonly WSREP_SST_OPT_SOCKET="$2"
        shift
        ;;
    '--user')
        WSREP_SST_OPT_USER="$2"
        shift
        ;;
    '--mysqld-version')
        readonly WSREP_SST_OPT_VERSION="$2"
        shift
        ;;
    '--gtid')
        readonly WSREP_SST_OPT_GTID="$2"
        shift
        ;;
    '--binlog')
        WSREP_SST_OPT_BINLOG="$2"
        shift
        ;;
    *) # must be command
       # usage
       # exit 1
       ;;
esac
shift
done
readonly WSREP_SST_OPT_BYPASS
readonly WSREP_SST_OPT_BINLOG
readonly WSREP_SST_OPT_CONF_SUFFIX

#
# user can specify xtrabackup specific settings that will be used during sst
# process like encryption, etc.....
# parse such configuration option. (group for xb settings is [sst] in my.cnf
#
# 1st param: group : name of the config file section, e.g. mysqld
# 2nd param: var : name of the variable in the section, e.g. server-id
# 3rd param: - : default value for the param
parse_cnf()
{
    local group=$1
    local var=$2
    local reval=""

    # normalize the variable name by replacing all '_' with '-'
    var=${var//_/-}

    # print the default settings for given group using my_print_default.
    # normalize the variable names specified in cnf file (user can use _ or - for example log-bin or log_bin)
    # then grep for needed variable
    # finally get the variable value (if variables has been specified multiple time use the last value only)

    # look in group+suffix
    if [[ -n $WSREP_SST_OPT_CONF_SUFFIX ]]; then
        reval=$($MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF "${group}${WSREP_SST_OPT_CONF_SUFFIX}" | awk -F= '{if ($1 ~ /_/) { gsub(/_/,"-",$1); print $1"="$2 } else { print $0 }}' | grep -- "--$var=" | cut -d= -f2- | tail -1)
    fi

    # look in group
    if [[ -z $reval ]]; then
        reval=$($MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF $group | awk -F= '{if ($1 ~ /_/) { gsub(/_/,"-",$1); print $1"="$2 } else { print $0 }}' | grep -- "--$var=" | cut -d= -f2- | tail -1)
    fi

    # use default if we haven't found a value
    if [[ -z $reval ]]; then
        [[ -n $3 ]] && reval=$3
    fi
    echo $reval
}


#
# Converts an input string into a boolean "on", "off"
# Converts:
#   "on", "true", "1" -> "on" (case insensitive)
#   "off", "false", "0" -> "off" (case insensitive)
# All other values : -> default_value
#
# 1st param: input value
# 2nd param: default value
normalize_boolean()
{
    local input_value=$1
    local default_value=$2
    local return_value=$default_value

    if [[ "$input_value" == "1" ]]; then
        return_value="on"
    elif [[ "$input_value" =~ ^[Oo][Nn]$ ]]; then
        return_value="on"
    elif [[ "$input_value" =~ ^[Tt][Rr][Uu][Ee]$ ]]; then
        return_value="on"
    elif [[ "$input_value" == "0" ]]; then
        return_value="off"
    elif [[ "$input_value" =~ ^[Oo][Ff][Ff]$ ]]; then
        return_value="off"
    elif [[ "$input_value" =~ ^[Ff][Aa][Ll][Ss][Ee]$ ]]; then
        return_value="off"
    fi

    echo $return_value
}


# try to use my_print_defaults, mysql and mysqldump that come with the sources
# (for MTR suite)
SCRIPTS_DIR="$(cd $(dirname "$0"); pwd -P)"
EXTRA_DIR="$SCRIPTS_DIR/../extra"
CLIENT_DIR="$SCRIPTS_DIR/../client"

if [ -x "$CLIENT_DIR/mysql" ]; then
    MYSQL_CLIENT="$CLIENT_DIR/mysql"
else
    MYSQL_CLIENT=$(which mysql)
fi

if [ -x "$CLIENT_DIR/mysqldump" ]; then
    MYSQLDUMP="$CLIENT_DIR/mysqldump"
else
    MYSQLDUMP=$(which mysqldump)
fi

if [ -x "$SCRIPTS_DIR/my_print_defaults" ]; then
    MY_PRINT_DEFAULTS="$SCRIPTS_DIR/my_print_defaults"
elif [ -x "$EXTRA_DIR/my_print_defaults" ]; then
    MY_PRINT_DEFAULTS="$EXTRA_DIR/my_print_defaults"
else
    MY_PRINT_DEFAULTS=$(which my_print_defaults)
fi

wsrep_auth_not_set()
{
    [ -z "$WSREP_SST_OPT_AUTH" -o "$WSREP_SST_OPT_AUTH" = "(null)" ]
}

# For Bug:1200727
if wsrep_auth_not_set
then
    WSREP_SST_OPT_AUTH=$(parse_cnf sst wsrep-sst-auth "")
fi
readonly WSREP_SST_OPT_AUTH

# Splitting AUTH into potential user:password pair
if ! wsrep_auth_not_set
then
    readonly AUTH_VEC=(${WSREP_SST_OPT_AUTH//:/ })
    WSREP_SST_OPT_USER="${AUTH_VEC[0]:-}"
    WSREP_SST_OPT_PSWD="${AUTH_VEC[1]:-}"
fi
readonly WSREP_SST_OPT_USER
readonly WSREP_SST_OPT_PSWD

if [ -n "${WSREP_SST_OPT_DATA:-}" ]
then
    SST_PROGRESS_FILE="$WSREP_SST_OPT_DATA/sst_in_progress"
else
    SST_PROGRESS_FILE=""
fi


WSREP_LOG_DEBUG=$(parse_cnf sst wsrep-debug "")
if [ -z "$WSREP_LOG_DEBUG" ]; then
    WSREP_LOG_DEBUG=$(parse_cnf mysqld wsrep-debug "")
fi
WSREP_LOG_DEBUG=$(normalize_boolean "$WSREP_LOG_DEBUG" "off")
if [[ "$WSREP_LOG_DEBUG" == "off" ]]; then
    WSREP_LOG_DEBUG=""
fi


# Determine the timezone to use for error logging
LOGGING_TZ=$(parse_cnf mysqld log-timestamps "")
if [ -z "$LOGGING_TZ" ]; then
    LOGGING_TZ="UTC"
fi
# Set the options to the 'date' command based on the timezone
if [[ "$LOGGING_TZ" == "SYSTEM" ]]; then
    LOG_DATE_COMMAND="+%Y-%m-%dT%H:%M:%S.%6N%:z"
else
    LOG_DATE_COMMAND="-u +%Y-%m-%dT%H:%M:%S.%6NZ"
fi

wsrep_log()
{
    # echo everything to stderr so that it gets into common error log
    # deliberately made to look different from the rest of the log
    local readonly tst="$(date $LOG_DATE_COMMAND)"
    echo -e "\t$tst WSREP_SST: $*" >&2
}

wsrep_log_error()
{
    wsrep_log "[ERROR] $*"
}

wsrep_log_warning()
{
    wsrep_log "[WARNING] $*"
}

wsrep_log_info()
{
    wsrep_log "[INFO] $*"
}

wsrep_log_debug()
{
    if [[ -n "$WSREP_LOG_DEBUG" ]]; then
        wsrep_log "[DEBUG] $*"
    fi
}

wsrep_cleanup_progress_file()
{
    [ -n "${SST_PROGRESS_FILE:-}" ] && rm -f "$SST_PROGRESS_FILE" 2>/dev/null || true
}

wsrep_check_program()
{
    local prog=$1

    if ! which $prog >/dev/null
    then
        echo "'$prog' not found in PATH"
        return 2 # no such file or directory
    fi
}

wsrep_check_programs()
{
    local ret=0

    while [ $# -gt 0 ]
    do
        wsrep_check_program $1 || ret=$?
        shift
    done

    return $ret
}


# Runs mysql_upgrade on the datadir (argument 1)
#
# Globals
#   MYSQLD_NAME
#   MYSQLADMIN_NAME
#   MYSQL_UPGRADE_NAME
#   MYSQL_CLIENT_NAME
#   WSREP_SST_OPT_PARENT
#   WSREP_SST_OPT_USER
#   WSREP_SST_OPT_PSWD
#   WSREP_SST_OPT_CONF
#   WSREP_SST_OPT_CONF_SUFFIX
#   MYSQL_UPGRADE_TMPDIR    (This path will be deleted on exit)
#
# Arguments
#   Argument 1: Path to the datadir
#   Argument 2: Port to be used by mysqld
#
function run_mysql_upgrade()
{
    local datadir=$1
    local port=$2

    # Path to the binary locations
    local mysqld_path=""
    local mysql_upgrade_path
    local mysql_client_path
    local mysqladmin_path

    local upgrade_tmpdir
    local mysql_upgrade_dir_path
    local use_mysql_upgrade_conf_suffix

    # Locate mysqld
    # mysqld (depending on the distro) may be in a different
    # directory from the SST scripts (it may be in /usr/sbin not /usr/bin)
    # So, first, try to use readlink to read from /proc/<pid>/exe
    if which readlink >/dev/null; then

        # Check to see if the symlink for the exe exists
        if [[ -L /proc/${WSREP_SST_OPT_PARENT}/exe ]]; then
            mysqld_path=$(readlink -f /proc/${WSREP_SST_OPT_PARENT}/exe)
        fi
    fi
    if [[ -z $mysqld_path ]]; then
        # We don't have readlink, so look for mysqld in the path
        mysqld_path=$(which ${MYSQLD_NAME})
    fi
    if [[ -z $mysqld_path ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQLD_NAME} (needed to run mysql_upgrade)"
        wsrep_log_error "Please ensure that ${MYSQLD_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        exit 2
    fi
    wsrep_log_debug "Found the ${MYSQLD_NAME} binary in ${mysqld_path}"
    wsrep_log_debug "--$("$mysqld_path" --version | cut -d' ' -f2-)"

    # Verify any other needed programs
    wsrep_check_program "${MYSQL_UPGRADE_NAME}"
    if [[ $? -ne 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQLD_UPGRADE_NAME} (needed for upgrade)"
        wsrep_log_error "Please ensure that ${MYSQLD_UPGRADE_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        exit 2
    fi
    wsrep_check_program "${MYSQLADMIN_NAME}"
    if [[ $? -ne 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQLADMIN_NAME} (needed for upgrade)"
        wsrep_log_error "Please ensure that ${MYSQLADMIN_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        exit 2
    fi
    wsrep_check_program "${MYSQL_CLIENT_NAME}"
    if [[ $? -ne 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQL_CLIENT_NAME} (needed for upgrade)"
        wsrep_log_error "Please ensure that ${MYSQL_CLIENT_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        exit 2
    fi

    mysql_upgrade_path=$(which ${MYSQL_UPGRADE_NAME})
    mysqladmin_path=$(which ${MYSQLADMIN_NAME})
    mysql_client_path=$(which ${MYSQL_CLIENT_NAME})

    wsrep_log_debug "Found the ${MYSQL_UPGRADE_NAME} binary in ${mysql_upgrade_path}"
    wsrep_log_debug "--$("$mysql_upgrade_path" --version | cut -d' ' -f2-)"
    wsrep_log_debug "Found the ${MYSQLADMIN_NAME} binary in ${mysqladmin_path}"
    wsrep_log_debug "--$("$mysqladmin_path" --version | cut -d' ' -f2-)"
    wsrep_log_debug "Found the ${MYSQL_CLIENT_NAME} binary in ${mysql_client_path}"
    wsrep_log_debug "--$("$mysql_client_path" --version | cut -d' ' -f2-)"

    # Create a temp directory for upgrade logs/output
    upgrade_tmpdir=$(parse_cnf sst tmpdir "")
    if [[ -z "${upgrade_tmpdir}" ]]; then
        upgrade_tmpdir=$(parse_cnf xtrabackup tmpdir "")
    fi
    if [[ -z "${upgrade_tmpdir}" ]]; then
        upgrade_tmpdir=$(parse_cnf mysqld tmpdir "")
    fi
    if [[ -z "${upgrade_tmpdir}" ]]; then
        mysql_upgrade_dir_path=$(mktemp -dt sst_mysql_upgrade.${port}.XXXX)
    else
        mysql_upgrade_dir_path=$(mktemp -p "${upgrade_tmpdir}" -dt sst_mysql_upgrade.${port}.XXXX)
    fi

    # Set this so that it will be cleaned up on exit
    MYSQL_UPGRADE_TMPDIR=$mysql_upgrade_dir_path

    # validation check to ensure that the auth param are specified.
    if [[ -z "$WSREP_SST_OPT_USER" ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Authentication params missing. Check wsrep_sst_auth"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "****************************************************** "
        exit 3
    fi

    if [[ -n $WSREP_SST_OPT_CONF_SUFFIX ]]; then
      use_mysql_upgrade_conf_suffix="--defaults-group-suffix=${WSREP_SST_OPT_CONF_SUFFIX}"
    fi

    #-----------------------------------------------------------------------
    # start server in standalone mode with the my.cnf configuration
    # for upgrade purpose using the data-directory that was just SST from
    # the DONOR node.
    #
    # This is a internal SST process, so isolate this mysqld instance
    # from the rest of the system.
    #
    # Disable wsrep
    #   --wsrep-provider=none
    # Disable networking
    #   --skip-networking
    # Disable slave start (avoid starting slave threads)
    #   --skip-slave-start
    # Turn off logging
    #   --log-syslog
    #   --log-output
    #   --general-log
    #   --slow-query-log
    # Turn off logging to syslog
    #   --log-error-services
    # Redirect files (to avoid conflict with parent mysqld)
    #   --pid-file
    #   --socket
    #   --log-error
    #   --datadir
    local upgrade_socket="${mysql_upgrade_dir_path}/mysqld.sock"

    wsrep_log_debug "Starting the MySQL server used by mysql_upgrade"
    local mysqld_cmdline="--defaults-file=${WSREP_SST_OPT_CONF} ${use_mysql_upgrade_conf_suffix} \
        --skip-networking --skip-slave-start \
        --auto_generate_certs=OFF \
        --general-log=0 --slow-query-log=0 \
        --pxc-maint-transition-period=1 \
        --log-error-services='log_filter_internal;log_sink_internal' \
        --log-error=${mysql_upgrade_dir_path}/err.log \
        --log_output=NONE \
        --pid-file=${mysql_upgrade_dir_path}/mysqld.pid \
        --socket=$upgrade_socket \
        --datadir=$datadir --wsrep_provider=none"

    $mysqld_path $mysqld_cmdline &> ${mysql_upgrade_dir_path}/err.log &
    mysql_pid=$!

    #-----------------------------------------------------------------------
    # wait for the mysql to come up
    local -i timeout
    local -i timeout_threshold

    timeout_threshold=300
    timeout=$timeout_threshold
    sleep 5

    while [ true ]; do
        if ! ps --pid $mysql_pid >/dev/null; then
            # If the process doesn't exist, then it shut down, so exit
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to start the mysql server that executes mysql-upgrade."
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO  pid:$mysql_pid"
            echo "--------------- err.log (START) --------------------" >&2
            cat ${mysql_upgrade_dir_path}/err.log >&2
            echo "--------------- err.log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 3
        fi

        $mysqladmin_path ping --socket=$upgrade_socket &> /dev/null
        errcode=$?
        if [ $errcode -eq 0 ]; then
            break;
        fi

        if [ $timeout -eq $timeout_threshold ]; then
            wsrep_log_info "Waiting for server instance to start....." \
                           " This may take some time"
        fi

        sleep 1
        ((timeout--))
        if [ $timeout -eq 0 ]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to start the MySQL server that executes mysql-upgrade."
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO"
            echo "--------------- mysql error log  (START) --------------------" >&2
            cat ${mysql_upgrade_dir_path}/err.log >&2
            echo "--------------- mysql error log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 3
        fi
    done
    wsrep_log_debug "MySQL server started"

    #-----------------------------------------------------------------------
    # run mysql-upgrade
    wsrep_log_debug "Starting mysql_upgrade"
    $mysql_upgrade_path \
        --defaults-file=/dev/stdin \
        --force \
        --socket=$upgrade_socket \
        &> ${mysql_upgrade_dir_path}/upgrade.out <<EOF
[mysql_upgrade]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
    errcode=$?
    if [ $errcode -ne 0 ]; then
        kill -9 $mysql_pid
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Failed to execute mysql-upgrade. Check the parameters and retry"
        wsrep_log_error "Line $LINENO errcode:${errcode}"
        echo "--------------- mysql_upgrade log (START) --------------------" >&2
        cat ${mysql_upgrade_dir_path}/upgrade.out >&2
        echo "--------------- mysql_upgrade log (END) ----------------------" >&2
        wsrep_log_error "****************************************************** "
        exit 3
    fi
    wsrep_log_debug "mysql_upgrade completed"

    #-----------------------------------------------------------------------
    # shutdown mysql instance started.
    wsrep_log_debug "Shutting down the MySQL server"
    $mysqladmin_path \
        --defaults-file=/dev/stdin \
        --socket=$upgrade_socket \
        shutdown \
        &> ${mysql_upgrade_dir_path}/upgrade_shutdown.out <<EOF
[mysqladmin]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
    errcode=$?
    if [ $errcode -ne 0 ]; then
        kill -9 $mysql_pid
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Failed to shutdown the MySQL instance started for upgrade."
        wsrep_log_error "Check the parameters and retry"
        wsrep_log_error "Line $LINENO errcode:${errcode}"
        echo "--------------- shutdown log (START) --------------------" >&2
        cat ${mysql_upgrade_dir_path}/upgrade_shutdown.out >&2
        echo "--------------- shutdown log (END) ----------------------" >&2
        wsrep_log_error "****************************************************** "
        exit 3
    fi

    sleep 1

    #-----------------------------------------------------------------------
    # cleanup
    rm -rf $datadir/*.pem || true
    rm -rf $datadir/auto.cnf || true

}
