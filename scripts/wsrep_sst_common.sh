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
WSREP_SST_OPT_USER=""
WSREP_SST_OPT_PSWD=""
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

    [[ "$input_value" == "1" ]]                 && echo "on"    && return 0
    [[ "$input_value" =~ ^[Oo][Nn]$ ]]          && echo "on"    && return 0
    [[ "$input_value" =~ ^[Tt][Rr][Uu][Ee]$ ]]  && echo "on"    && return 0
    [[ "$input_value" == "0" ]]                 && echo "off"   && return 0
    [[ "$input_value" =~ ^[Oo][Ff][Ff]$ ]]      && echo "off"   && return 0
    [[ "$input_value" =~ ^[Ff][Aa][Ll][Ss][Ee]$ ]] && echo "off"   && return 0

    echo $default_value
    return 0
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

# Returns the length of the string in bytes
#
# Globals:
#   None
#
# Arguments:
#   Parameter 1: the string
#
# Outputs:
#   The length of the string in bytes (not characters!)
#
function get_length_in_bytes()
{
    local str=$1
    local byte_len=0

    # Run this in a sub-shell
    (
        LANG=C LC_ALL=C
        byte_len=${#str}
        echo $byte_len
    ) || true
}

# Returns the version string in a standardized format
# Input "1.2.3" => echoes "010203"
# Wrongly formatted values => echoes "000000"
#
# Globals:
#   None
#
# Arguments:
#   Parameter 1: a version string
#                like "5.1.12"
#                anything after the major.minor.revision is ignored
# Outputs:
#   A string that can be used directly with string comparisons.
#   So, the string "5.1.12" is transformed into "050112"
#   Note that individual version numbers can only go up to 99.
#
function normalize_version()
{
    local major=0
    local minor=0
    local patch=0

    # Only parses purely numeric version numbers, 1.2.3
    # Everything after the first three values are ignored
    if [[ $1 =~ ^([0-9]+)\.([0-9]+)\.?([0-9]*)([^ ])* ]]; then
        major=${BASH_REMATCH[1]}
        minor=${BASH_REMATCH[2]}
        patch=${BASH_REMATCH[3]}
    fi

    printf %02d%02d%02d $major $minor $patch
}

# Compares two version strings
#   The version strings passed in will be normalized to a
#   string-comparable version.
#
# Globals:
#   None
#
# Arguments:
#   Parameter 1: The left-side of the comparison
#   Parameter 2: the comparison operation
#                   '>', '>=', '=', '==', '<', '<='
#   Parameter 3: The right-side of the comparison
#
# Returns:
#   Returns 0 (success) if param1 op param2
#   Returns 1 (failure) otherwise
#
function compare_versions()
{
    local version_1="$( normalize_version $1 )"
    local op=$2
    local version_2="$( normalize_version $3 )"

    if [[ ! " = == > >= < <= " =~ " $op " ]]; then
        wsrep_log_error "******************* ERROR ********************** "
        wsrep_log_error "Unknown operation : $op"
        wsrep_log_error "Must be one of : = == > >= < <="
        wsrep_log_error "******************* ERROR ********************** "
        return 1
    fi

    [[ $op == "<"  &&   $version_1 <  $version_2 ]] && return 0
    [[ $op == "<=" && ! $version_1 >  $version_2 ]] && return 0
    [[ $op == "="  &&   $version_1 == $version_2 ]] && return 0
    [[ $op == "==" &&   $version_1 == $version_2 ]] && return 0
    [[ $op == ">"  &&   $version_1 >  $version_2 ]] && return 0
    [[ $op == ">=" && ! $version_1 <  $version_2 ]] && return 0

    return 1
}


# Waits until $timeout_threshold for mysqld to shutdown
#
# Globals:
#   None
#
# Arguments:
#   Parameter 1 : the mysqld PID
#   Parameter 2 : threshold (in seconds) before killing mysqld
#   Parameter 3 : descriptive string to use upon error
#
# Returns:
#   0 (success) if the process exits
#   1 (failure) if the process was still running and had to be killed
function wait_for_mysqld_shutdown()
{
    local mysqld_pid=$1
    local threshold=$2
    local description=$3
    local timeout=0

    # Wait until the mysqld instance has shut down
    timeout=$threshold
    while [ true ]; do
        if ! ps --pid $mysqld_pid >/dev/null; then
            # If the process doesn't exist, then it shut down, so we're good
            break
        fi

        sleep 0.5
        ((timeout--))
        if [[ $timeout -eq 0 ]]; then
            kill -9 $mysqld_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "The $description"
            wsrep_log_error "has failed to shutdown.  Killing the process."
            wsrep_log_error "Line $LINENO"
            echo "--------------- mysql error log  (START) --------------------" >&2
            cat ${mysqld_err_log} >&2
            echo "--------------- mysql error log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            return 1
        fi
    done
    return 0
}

function wait_for_mysqld_startup()
{
    local mysqld_pid=$1
    local mysqld_socket=$2
    local threshold=$3
    local err_log=$4
    local description=$5

    local -i timeout
    timeout=$threshold

    while [ true ]; do
        if ! ps --pid $mysqld_pid >/dev/null; then
            # If the process doesn't exist, then it shut down, so exit
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to start the $description."
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO  pid:$mysqld_pid"
            echo "--------------- err.log (START) --------------------" >&2
            cat ${err_log} >&2
            echo "--------------- err.log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            return 3
        fi

        $mysqladmin_path ping --socket=$mysqld_socket &> /dev/null
        errcode=$?
        [[ $errcode -eq 0 ]] && break

        if [[ $timeout -eq $threshold ]]; then
            wsrep_log_info "Waiting for server instance to start....." \
                           " This may take some time"
        fi

        sleep 1
        ((timeout--))
        if [[ $timeout -eq 0 ]]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to start the $description. (timeout)"
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO  pid:$mysqld_pid"
            echo "--------------- mysql error log  (START) --------------------" >&2
            cat ${err_log} >&2
            echo "--------------- mysql error log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            return 3
        fi
    done

    return 0
}


# Runs any post-processing steps needed
# (after the datadir has been moved)
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
#   Argument 3: Set to 0 to skip mysql_upgrade, 1 to run mysql_upgrade
#   Argument 4: MySQL donor version
#
# Returns:
#   0 if successful (no errors encountered)
#   <non-zero> otherwise
#
# Overview: There are two code paths:
#   (1) Receiving from a < 8.0 donor (5.7 donor)
#   (2) Receiving from a >= 8.0 donor
#
#   (1) Receiving from a 5.7 donor
#       The basic flow is:
#           1. start up a MySQL server (#1) (creates the SST user)
#           2. shut down MySQL server
#               Because the system tables use MyISAM (5.7 donor), the
#               account mgmt commands (like CREATE USER, FLUSH PRIVILEGES)
#               will not work. So the only way to use this new user is
#               to restart the server.  Note that the SST user is not
#               removed on shutdown, because the 'DROP USER' call will fail.
#           3. start up MySQL Server (#2)
#           4. run mysql_upgrade with the SST user
#           5. shut down the MySQL server
#               The loading of any async replication info will have failed
#               because the tables are MyISAM. So we need to restart (again)
#               so that we can remove the async replication info. Note that
#               this time, the SST user will be removed on shutdown.
#           6. start up MySQL Server (#3) (creates the SST user)
#           7. run 'RESET SLAVE ALL' (if needed)
#           8. shutdown the MySQL Server
#
#   TODO: Steps (5) and (6) are needed due to 8.0.13 not restoring the
#         async info tables during mysql_upgrade. Check to see if this
#         is fixed in later versions.
#
#   (2) Receiving from an 8.0 donor
#       The basic flow is:
#           - start up a MySQL server (creates the SST user)
#           - run mysql_upgrade with the SST user
#           - run 'RESET SLAVE ALL' (if needed)
#           - shutdown the MySQL Server
#
function run_post_processing_steps()
{
    local datadir=$1
    local port=$2
    local run_mysql_upgrade=$3
    local donor_version_str=$4

    # Path to the binary locations
    local mysqld_path=""
    local mysql_upgrade_path
    local mysql_client_path
    local mysqladmin_path

    local upgrade_tmpdir
    local mysql_upgrade_dir_path
    local use_mysql_upgrade_conf_suffix=""

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
        wsrep_log_error "Could not locate ${MYSQLD_NAME} (needed for post-processing)"
        wsrep_log_error "Please ensure that ${MYSQLD_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        return 2
    fi
    wsrep_log_debug "Found the ${MYSQLD_NAME} binary in ${mysqld_path}"
    wsrep_log_debug "--$("$mysqld_path" --version | cut -d' ' -f2-)"

    # Verify any other needed programs
    if [[ $run_mysql_upgrade -eq 1 ]]; then
        wsrep_check_program "${MYSQL_UPGRADE_NAME}"
        if [[ $? -ne 0 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Could not locate ${MYSQLD_UPGRADE_NAME} (needed for upgrade)"
            wsrep_log_error "Please ensure that ${MYSQLD_UPGRADE_NAME} is in the path"
            wsrep_log_error "Line $LINENO"
            wsrep_log_error "******************* FATAL ERROR ********************** "
            return 2
        fi
    fi
    wsrep_check_program "${MYSQLADMIN_NAME}"
    if [[ $? -ne 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQLADMIN_NAME} (needed for post-processing)"
        wsrep_log_error "Please ensure that ${MYSQLADMIN_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        return 2
    fi
    wsrep_check_program "${MYSQL_CLIENT_NAME}"
    if [[ $? -ne 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Could not locate ${MYSQL_CLIENT_NAME} (needed for post-processing)"
        wsrep_log_error "Please ensure that ${MYSQL_CLIENT_NAME} is in the path"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "******************* FATAL ERROR ********************** "
        return 2
    fi


    if [[ $run_mysql_upgrade -eq 1 ]]; then
        mysql_upgrade_path=$(which ${MYSQL_UPGRADE_NAME})
        wsrep_log_debug "Found the ${MYSQL_UPGRADE_NAME} binary in ${mysql_upgrade_path}"
        wsrep_log_debug "--$("$mysql_upgrade_path" --version | cut -d' ' -f2-)"
    fi

    mysqladmin_path=$(which ${MYSQLADMIN_NAME})
    mysql_client_path=$(which ${MYSQL_CLIENT_NAME})

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
        mysql_upgrade_dir_path=$(mktemp -dt upgrd.XXXX)
    else
        mysql_upgrade_dir_path=$(mktemp -p "${upgrade_tmpdir}" -dt upgrd.XXXX)
    fi

    # Set this so that it will be cleaned up on exit
    MYSQL_UPGRADE_TMPDIR=$mysql_upgrade_dir_path

    if [[ -n $WSREP_SST_OPT_CONF_SUFFIX ]]; then
      use_mysql_upgrade_conf_suffix="--defaults-group-suffix=${WSREP_SST_OPT_CONF_SUFFIX}"
    fi

    local mysqld_err_log="${mysql_upgrade_dir_path}/err.log"
    local upgrade_socket="${mysql_upgrade_dir_path}/my.sock"

    # ---------------------------------------------------------
    # Check socket path length
    # AF_UNIX socket family restricts sockets path to 108 characters
    # (107 chars + 1 for terminating null character)
    if [[ $(get_length_in_bytes "$upgrade_socket") -gt 107 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Socket path is too long (must be less than 107 characters)."
        wsrep_log_error "  $upgrade_socket"
        wsrep_log_error "Line $LINENO"
        wsrep_log_error "****************************************************** "
        return 3
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
    local mysqld_cmdline="--defaults-file=${WSREP_SST_OPT_CONF} ${use_mysql_upgrade_conf_suffix} \
        --skip-networking --skip-slave-start \
        --auto_generate_certs=OFF \
        --general-log=0 --slow-query-log=0 \
        --pxc-maint-transition-period=1 \
        --log-error-services='log_filter_internal;log_sink_internal' \
        --log-error=${mysqld_err_log} \
        --log_output=NONE \
        --server-id=1 \
        --pid-file=${mysql_upgrade_dir_path}/mysqld.pid \
        --socket=$upgrade_socket \
        --datadir=$datadir --wsrep_provider=none"

    # Generate a new random password to be used by the JOINER
    WSREP_SST_OPT_USER="mysql.pxc.sst.user"
    WSREP_SST_OPT_PSWD=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)

    local -i timeout_threshold
    timeout_threshold=300

    #-----------------------------------------------------------------------
    # If the donor is >= 8.0.0, then we can use the 8.0 SQL commands
    # (specifically the account management, CREATE USER, etc...)
    #
    # Otherwise (for 5.7), we'll have to maniupulate the tables directly
    # and we'll have to restart the server to flush the privileges.
    # 'FLUSH PRIVILEGES' with 8.0 on a 5.7 datadir will fail because
    # the system tables (mysql.user) are MyISAM and not InnoDB.

    if compare_versions $donor_version_str ">=" "8.0.0"; then
        #-----------------------------------------------------------------------
        # Reuse the SST User (use it to run mysql_upgrade)
        # Recreate with root permissions and a new password
        wsrep_log_debug "Starting the MySQL server used for post-processing"
        cat <<EOF | $mysqld_path $mysqld_cmdline --init-file=/dev/stdin &> ${mysqld_err_log} &
SET sql_log_bin=OFF;
DROP USER IF EXISTS '${WSREP_SST_OPT_USER}'@localhost;
CREATE USER '${WSREP_SST_OPT_USER}'@localhost IDENTIFIED BY '${WSREP_SST_OPT_PSWD}';
GRANT ALL PRIVILEGES ON *.* TO '${WSREP_SST_OPT_USER}'@localhost;
FLUSH PRIVILEGES;
EOF
        mysql_pid=$!
    else
        #-----------------------------------------------------------------------
        # Reuse the SST User (use it to run mysql_upgrade)
        # Recreate with root permissions and a new password

        # Generate the authentication string for our new password
        # Use the same algorithm as mysql_native_password
        AUTH_STRING=$(echo -n "${WSREP_SST_OPT_PSWD}" | openssl sha1 -binary | openssl sha1 -hex)
        # Remove everything before the '='
        AUTH_STRING=${AUTH_STRING#*=}
        # Remove any leading spaces
        AUTH_STRING=${AUTH_STRING# }
        # add '*' at the beginning and convert to all uppercase
        AUTH_STRING=$(echo "${AUTH_STRING}" | awk '{print "*"toupper($0)}')

        wsrep_log_debug "Starting the MySQL server(#1) used for post-processing"
        cat <<EOF | timeout -k ${timeout_threshold}s ${timeout_threshold}s $mysqld_path $mysqld_cmdline --init-file=/dev/stdin &> ${mysqld_err_log} &
SET sql_log_bin=OFF;
DELETE FROM mysql.user WHERE User='${WSREP_SST_OPT_USER}' AND Host='localhost';
INSERT INTO mysql.user
    ( User, Host, Select_priv, Insert_priv, Update_priv,
      Delete_priv, Create_priv, Drop_priv, Reload_priv, Shutdown_priv,
      Process_priv, File_priv, Grant_priv, References_priv, Index_priv,
      Alter_priv, Show_db_priv, Super_priv, Create_tmp_table_priv, Lock_tables_priv,
      Execute_priv, Repl_slave_priv, Repl_client_priv, Create_view_priv, Show_view_priv,
      Create_routine_priv, Alter_routine_priv, Create_user_priv, Event_priv, Trigger_priv,
      Create_tablespace_priv,
      ssl_type, ssl_cipher, x509_issuer, x509_subject,
      max_questions, max_updates, max_connections, max_user_connections,
      plugin, authentication_string,
      password_expired, password_last_changed, password_lifetime, account_locked
    )
    VALUES
    ('${WSREP_SST_OPT_USER}', 'localhost', 'Y', 'Y', 'Y',
     'Y', 'Y', 'Y', 'Y', 'Y',
     'Y', 'Y', 'Y', 'Y', 'Y',
     'Y', 'Y', 'Y', 'Y', 'Y',
     'Y', 'Y', 'Y', 'Y', 'Y',
     'Y', 'Y', 'Y', 'Y', 'Y',
     'Y',
     '', '', '', '',
     0, 0, 0, 0,
     'mysql_native_password', '${AUTH_STRING}',
     'N', CURRENT_TIMESTAMP, NULL, 'N'
    );
SHUTDOWN;
EOF
        mysql_pid=$!
        wsrep_log_debug "MySQL server($mysql_pid) started"

        wait $mysql_pid
        errcode=$?
        if [[ $errcode -ne 0 ]]; then
            # If the process doesn't exist, then it shut down, so exit
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "The MySQL server failed and was terminated."
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO  errcode:$errcode  pid:$mysql_pid"
            echo "--------------- err.log (START) --------------------" >&2
            cat ${mysqld_err_log} >&2
            echo "--------------- err.log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            return 3
        fi
        wsrep_log_debug "MySQL server shut down"

        # The shutdown above does NOT remove the SST user, since it
        # relies on 'DROP USER' and those tables are still MyISAM

        #-----------------------------------------------------------------------
        # Restart the server (now that it has the needed user/password)
        wsrep_log_debug "Starting the MySQL server(#2) used for post-processing"
        echo "" > ${mysqld_err_log}
        $mysqld_path $mysqld_cmdline &> ${mysqld_err_log} &
        mysql_pid=$!
    fi


    #-----------------------------------------------------------------------
    # wait for mysql to come up
    wait_for_mysqld_startup $mysql_pid $upgrade_socket $timeout_threshold ${mysqld_err_log} \
        "mysql server that executes mysql-upgrade"
    errcode=$?
    [[ $errcode -ne 0 ]] && return $errcode
    wsrep_log_debug "MySQL server($mysql_pid) started"

    #-----------------------------------------------------------------------
    # run mysql-upgrade
    if [[ $run_mysql_upgrade -eq 1 ]]; then
        wsrep_log_info "Running mysql_upgrade"
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
        if [[ $errcode -ne 0 ]]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to execute mysql-upgrade. Check the parameters and retry"
            wsrep_log_error "Line $LINENO errcode:${errcode}"
            echo "--------------- mysql_upgrade log (START) --------------------" >&2
            cat ${mysql_upgrade_dir_path}/upgrade.out >&2
            echo "--------------- mysql_upgrade log (END) ----------------------" >&2
            echo "--------------- mysql error log  (START) --------------------" >&2
            cat ${mysqld_err_log} >&2
            echo "--------------- mysql error log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 3
        fi
        wsrep_log_debug "mysql_upgrade completed"

        # If this is for 5.7, we will have to restart the mysqld (again, sigh)
        # The async info tables will not have been loaded
        # so the SHOW SLAVE STATUS will fail
        if compare_versions $donor_version_str "<" "8.0.0"; then
            #-----------------------------------------------------------------------
            # Restart the server (to reload the async info tables
            # that failed to load during the mysql_upgrade from 5.7).
            wsrep_log_debug "Shutting down the MySQL server"
            $mysql_client_path \
                --defaults-file=/dev/stdin \
                --socket=$upgrade_socket \
                --unbuffered --batch --silent \
                -e "SHUTDOWN;" \
                &> ${mysql_upgrade_dir_path}/upgrade_shutdown.out <<EOF
[client]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
            errcode=$?
            if [[ $errcode -ne 0 ]]; then
                sleep 3
                kill -9 $mysql_pid
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "Failed to shutdown the MySQL instance started for upgrade."
                wsrep_log_error "Check the parameters and retry.  Killing the process."
                wsrep_log_error "Line $LINENO errcode:${errcode}"
                echo "--------------- shutdown log (START) --------------------" >&2
                cat ${mysql_upgrade_dir_path}/upgrade_shutdown.out >&2
                echo "--------------- shutdown log (END) ----------------------" >&2
                echo "--------------- mysql error log (START) --------------------" >&2
                cat ${mysqld_err_log} >&2
                echo "--------------- mysql error log (END) ----------------------" >&2
                wsrep_log_error "****************************************************** "
                return 3
            fi

            wait_for_mysqld_shutdown $mysql_pid $timeout_threshold "MySQL server started for the upgrade process"
            [[ $? -ne 0 ]] && return 1

            wsrep_log_debug "MySQL server shut down"

            #-----------------------------------------------------------------------
            # Starting up the server
            wsrep_log_debug "Starting the MySQL server(#3) used for post-processing"
            cat <<EOF | $mysqld_path $mysqld_cmdline --init-file=/dev/stdin &> ${mysqld_err_log} &
SET sql_log_bin=OFF;
DROP USER IF EXISTS '${WSREP_SST_OPT_USER}'@localhost;
CREATE USER '${WSREP_SST_OPT_USER}'@localhost IDENTIFIED BY '${WSREP_SST_OPT_PSWD}';
GRANT ALL PRIVILEGES ON *.* TO '${WSREP_SST_OPT_USER}'@localhost;
FLUSH PRIVILEGES;
EOF
            mysql_pid=$!
            wait_for_mysqld_startup $mysql_pid $upgrade_socket $timeout_threshold ${mysqld_err_log} \
                "mysql server that executes mysql-upgrade"
            errcode=$?
            [[ $errcode -ne 0 ]] && return $errcode
            wsrep_log_debug "MySQL server($mysql_pid) started"
        fi
    fi

    #-----------------------------------------------------------------------
    # Stop replication activity

    # First, check to see if we need to reset the slave
    local slave_status=""
    slave_status=$($mysql_client_path \
                    --defaults-file=/dev/stdin \
                    --socket=$upgrade_socket \
                    --unbuffered --batch --silent --skip-column-names \
                    -e "SHOW SLAVE STATUS;" \
                    2> ${mysql_upgrade_dir_path}/reset_slave.out <<EOF
[client]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
)
    if [[ -n $slave_status ]]; then
        wsrep_log_debug "Resetting Async Slave"

        $mysql_client_path \
            --defaults-file=/dev/stdin \
            --socket=$upgrade_socket \
            --unbuffered --batch --silent \
            -e "SET sql_log_bin=OFF; RESET SLAVE ALL;" \
            &> ${mysql_upgrade_dir_path}/reset_slave.out <<EOF
[client]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
        errcode=$?
        if [[ $errcode -ne 0 ]]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to execute mysql 'RESET SLAVE ALL'. Check the parameters and retry"
            wsrep_log_error "Line $LINENO errcode:${errcode}"
            echo "--------------- reset slave log (START) --------------------" >&2
            cat ${mysql_upgrade_dir_path}/reset_slave.out >&2
            echo "--------------- reset slave log (END) ----------------------" >&2
            echo "--------------- mysql error log  (START) --------------------" >&2
            cat ${mysqld_err_log} >&2
            echo "--------------- mysql error log (END) ----------------------" >&2
            wsrep_log_error "****************************************   ************** "
            return 3
        fi
        wsrep_log_debug "Async Slave Reset completed"
    fi

    #-----------------------------------------------------------------------
    # shutdown mysql instance started.
    #   (also does some account cleanup)
    #   LOCK the account to ensure that the user can't be used pass this
    #   point if the server fails to shutdown properly (or is killed).
    wsrep_log_debug "Shutting down the MySQL server"
    $mysql_client_path \
        --defaults-file=/dev/stdin \
        --socket=$upgrade_socket \
        --unbuffered --batch --silent \
        -e "SET sql_log_bin=OFF; ALTER USER '${WSREP_SST_OPT_USER}'@localhost ACCOUNT LOCK; SHUTDOWN;" \
        &> ${mysql_upgrade_dir_path}/upgrade_shutdown.out <<EOF
[client]
user=${WSREP_SST_OPT_USER}
password="${WSREP_SST_OPT_PSWD}"
EOF
    errcode=$?
    if [[ $errcode -ne 0 ]]; then
        sleep 3
        kill -9 $mysql_pid
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Failed to shutdown the MySQL instance started for upgrade."
        wsrep_log_error "Check the parameters and retry.  Killing the process."
        wsrep_log_error "Line $LINENO errcode:${errcode}"
        echo "--------------- shutdown log (START) --------------------" >&2
        cat ${mysql_upgrade_dir_path}/upgrade_shutdown.out >&2
        echo "--------------- shutdown log (END) ----------------------" >&2
        echo "--------------- mysql error log (START) --------------------" >&2
        cat ${mysqld_err_log} >&2
        echo "--------------- mysql error log (END) ----------------------" >&2
        wsrep_log_error "****************************************************** "
        return 3
    fi

    wait_for_mysqld_shutdown $mysql_pid $timeout_threshold "MySQL server started for the upgrade process"
    [[ $? -ne 0 ]] && return 1

    wsrep_log_debug "MySQL server shut down"

    #-----------------------------------------------------------------------
    # cleanup
    rm -rf $datadir/*.pem || true
    rm -rf $datadir/auto.cnf || true

    return 0
}


# Reads incoming data from STDIN and sets the variables
#
# Globals:
#   WSREP_SST_OPT_USER (sets this variable)
#   WSREP_SST_OPT_PSWD (sets this variable)
#
# Parameters:
#   Argument 1: the role ("donor" or "joiner")
#
# This function will exit if username or password is not received.
#
function read_variables_from_stdin()
{
    while read line; do
        name=$(echo "$line" | cut -d"=" -f1)
        value=$(echo "$line" | cut -d"=" -f2)

        case "$name" in
            'sst_user')
                WSREP_SST_OPT_USER="$value"
                ;;
            'sst_password')
                WSREP_SST_OPT_PSWD="$value"
                ;;
            *)
                wsrep_log_warning "Unrecognized input: $line"
        esac
    done
}
