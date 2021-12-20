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
WSREP_SST_OPT_BASEDIR=""
WSREP_SST_OPT_PLUGINDIR=""
WSREP_SST_OPT_USER=""
WSREP_SST_OPT_PSWD=""
WSREP_SST_OPT_VERSION=""
WSREP_SST_OPT_DEBUG=""

WSREP_LOG_DEBUG=""

# These are the 'names' of the commands
# The paths to these are stored separately (MYSQLD_PATH for MYSQLD_NAME)
# The NAMEs don't change, however the PATHs may be different (due to packaging)
readonly MYSQLD_NAME=mysqld
readonly MYSQLADMIN_NAME=mysqladmin
readonly MYSQL_CLIENT_NAME=mysql

declare MYSQL_UPGRADE_TMPDIR=""
declare WSREP_LOG_DIR=""

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
    '--basedir')
        readonly WSREP_SST_OPT_BASEDIR="$2"
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
        # Ignore (read value from stdin)
        shift
        ;;
    '--plugindir')
        readonly WSREP_SST_OPT_PLUGINDIR="$2"
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
        # Ignore (read value from stdin)
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
    '--debug')
        WSREP_SST_OPT_DEBUG="$2"
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
        reval=$($MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF "${group}${WSREP_SST_OPT_CONF_SUFFIX}" | awk -F= '{st=index($0,"="); cur=$0; if ($1 ~ /_/) { gsub(/_/,"-",$1);} if (st != 0) { print $1"="substr(cur,st+1) } else { print cur }}' | grep -- "--$var=" | cut -d= -f2- | tail -1)
    fi

    # look in group
    if [[ -z $reval ]]; then
        reval=$($MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF "${group}" | awk -F= '{st=index($0,"="); cur=$0; if ($1 ~ /_/) { gsub(/_/,"-",$1);} if (st != 0) { print $1"="substr(cur,st+1) } else { print cur }}' | grep -- "--$var=" | cut -d= -f2- | tail -1)
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
#   "on", "true", positive number -> "on" (case insensitive)
#   "off", "false", "0" -> "off" (case insensitive)
# All other values : -> default_value
#
# 1st param: input value
# 2nd param: default value
normalize_boolean()
{
    local input_value=$1
    local default_value=$2

    [[ "$input_value" =~ ^[1-9][0-9]*$ ]]       && echo "on"    && return 0
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

wsrep_log()
{
    echo -e "$*" >&2
}

wsrep_log_error()
{
    wsrep_log "ERR:$*"
}

wsrep_log_warning()
{
    wsrep_log "WRN:$*"
}

wsrep_log_info()
{
    wsrep_log "INF:$*"
}

wsrep_log_debug()
{
    if [[ -n "$WSREP_LOG_DEBUG" ]]; then
        wsrep_log "DBG:(debug) $*"
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
#   Parameter 1: The left-side of the comparison (for example: "5.7.25")
#   Parameter 2: the comparison operation
#                   '>', '>=', '=', '==', '<', '<=', "!="
#   Parameter 3: The right-side of the comparison (for example: "5.7.24")
#
# Returns:
#   Returns 0 (success) if param1 op param2
#   Returns 1 (failure) otherwise
#
function compare_versions()
{
    local version_1="$1"
    local op=$2
    local version_2="$3"

    if [[ -z $version_1 || -z $version_2 ]]; then
        wsrep_log_error "******************* ERROR ********************** "
        wsrep_log_error "Missing version string in comparison"
        wsrep_log_error "left-side:$version_1  operation:$op  right-side:$version_2"
        wsrep_log_error "******************* ERROR ********************** "
        return 1
    fi

    version_1="$( normalize_version "$version_1" )"
    version_2="$( normalize_version "$version_2" )"

    if [[ ! " = == > >= < <= != " =~ " $op " ]]; then
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
    [[ $op == "!=" &&   $version_1 != $version_2 ]] && return 0

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
            cat_file_to_stderr "${mysqld_err_log}" "ERR" "mysql error log"
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
    local sst_user=$5
    local sst_password=$6
    local description=$7

    local -i timeout
    timeout=$threshold

    while [ true ]; do
        if ! ps --pid $mysqld_pid >/dev/null; then
            # If the process doesn't exist, then it shut down, so exit
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to start the $description."
            wsrep_log_error "Check the parameters and retry"
            wsrep_log_error "Line $LINENO  pid:$mysqld_pid"
            cat_file_to_stderr "${err_log}" "ERR" "mysql error log"
            wsrep_log_error "****************************************************** "
            return 3
        fi

        $mysqladmin_path \
            --defaults-file=/dev/stdin \
            --socket=$mysqld_socket \
            ping &> /dev/null <<EOF
[mysqladmin]
user=${sst_user}
password="${sst_password}"
EOF
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
            cat_file_to_stderr "${err_log}" "ERR" "mysql error log"
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
#   MYSQL_CLIENT_NAME
#   WSREP_SST_OPT_PARENT
#   WSREP_SST_OPT_CONF
#   WSREP_SST_OPT_CONF_SUFFIX
#   WSREP_SST_OPT_BASEDIR
#   WSREP_SST_OPT_PLUGINDIR
#   MYSQL_UPGRADE_TMPDIR    (This path will be deleted on exit)
#
# Arguments
#   Argument 1: Path to the datadir
#   Argument 2: Port to be used by mysqld
#   Argument 3: The DONOR's version string
#   Argument 4: The local (JOINER) version string
#   Argument 5: The transfer method ('rsync' or 'xtrabackup')
#   Argument 6: The transfer type ('sst' or 'ist')
#   Argument 7: The value of 'force_upgrade' ('on'' or 'off')
#   Argument 8: The value of 'auto_upgrade' ('on' or 'off')
#
# Returns:
#   0 if successful (no errors encountered)
#   <non-zero> otherwise
#
# Overview: The basic flow is:
#   - start up a MySQL server (creates the SST user and upgrades mysql)
#   - run 'RESET SLAVE ALL' (if needed)
#   - shutdown the MySQL Server
#
function run_post_processing_steps()
{
    local datadir=$1
    local port=$2
    local donor_version=$3
    local local_version=$4
    local transfer_method=$5
    local transfer_type=$6
    local force_upgrade=$7
    local auto_upgrade=$8

    local local_version_str
    local donor_version_str

    # A value of 'no'   : do not run mysql_upgrade
    #            'yes'  : run mysql_upgrade
    local run_mysql_upgrade

    # A value of 'no'   : do not run 'reset slave all'
    #            'yes'  : run 'reset slave all'
    #            'check': check the slave status before running 'reset slave all'
    local run_reset_slave

    # Path to the binary locations
    local mysqld_path=""
    local mysql_client_path
    local mysqladmin_path

    local upgrade_tmpdir
    local mysql_upgrade_dir_path
    local use_mysql_upgrade_conf_suffix=""

    local_version_str=$(expr match "$local_version" '\([0-9]\+\.[0-9]\+\.[0-9]\+\)')
    donor_version_str=$(expr match "$donor_version" '\([0-9]\+\.[0-9]\+\.[0-9]\+\)')
    donor_version_str=${donor_version_str:-"0.0.0"}

    if [[ $force_upgrade == "on" ]]; then
        wsrep_log_info "Forcing mysql_upgrade"
        run_mysql_upgrade='yes'
    elif [[ $auto_upgrade == "on" ]]; then
        if [[ $transfer_type == "sst" ]]; then
            # For an SST, we know the version of the datadir from the donor
            # thus, we can skip mysql_upgrade if it's the same version
            if compare_versions "$local_version_str" "!=" "$donor_version_str"; then
                # Check the sys and wsrep schema versions to determine
                # if mysql_upgrade is needed
                wsrep_log_info "Opting for mysql_upgrade (sst): local version ($local_version_str) != donor version ($donor_version_str)"
                run_mysql_upgrade='yes'
            else
                wsrep_log_info "Skipping mysql_upgrade (sst): local version ($local_version_str) == donor version ($donor_version_str)"
                run_mysql_upgrade='no'
            fi
        else
            # For an IST, skip the upgrade step. Let the normal processing
            # do the upgrade (since we don't need to do an 'RESET SLAVE ALL',
            # there's no need for us to startup mysqld).
            run_mysql_upgrade='no'
            wsrep_log_info "Skipping mysql_upgrade (ist)"
        fi
    else
        wsrep_log_info "Skipping mysql_upgrade: auto-upgrade disabled by configuration"
        run_mysql_upgrade='no'
    fi

    # If the donor version is less than 8.0, remove the redo logs
    # Otherwise mysqld will not start up on the 5.7 datadir
    if [[ -n $WSREP_LOG_DIR && $run_mysql_upgrade == "yes" ]] && compare_versions "${donor_version_str}" "<" "8.0.0"; then
        wsrep_log_info "Removing the redo logs (older version:$donor_version_str)"
        remove_redo_logs "${WSREP_LOG_DIR}"
        errcode=$?
        if [[ $errcode -ne 0 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "FATAL: Failed to remove the redo logs that were received"
            wsrep_log_error "       from an older version."
            wsrep_log_error "       donor:${donor_version_str}"
            wsrep_log_error "****************************************************** "
            return $errcode
        fi
    fi

    # If we have received an IST, do NOT reset the async slave info
    # If we have received an SST, we always try to reset the slave info
    #
    # Side-effect! For an SST, this ensures that the SST user gets removed
    # during the shutdown of the mysqld started to run the commands.
    # We do not need to remove the SST user for an IST, since the datadir
    # is not sent for an IST.
    if [[ $transfer_type == "ist" ]]; then
        run_reset_slave='no'
    else
        run_reset_slave='check'
    fi

    # Check if we have anything to do
    if [[ $run_reset_slave == 'no' && $run_mysql_upgrade == 'no' ]]; then
        # nothing to do here, just return
        return 0
    fi

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
    if [[ $mysqld_path == *"memcheck"* ]]; then
      wsrep_log_debug "Detected valgrind, adjusting mysqld path accordingly"
      while read -r line
      do
        if [[ ${line::1} != "-" ]]; then
          mysqld_path=$line
          wsrep_log_debug "Adjusted mysqld to $line"
        fi
      done < <(cat /proc/${WSREP_SST_OPT_PARENT}/cmdline | strings -1)
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

    local mysqld_err_log="${datadir}/mysqld.post.processing.log"
    local mysqld_upgrade_log="${datadir}/mysqld.upgrade.log"
    local upgrade_socket="${mysql_upgrade_dir_path}/my.sock"

    rm -f ${mysqld_err_log} 2> /dev/null || true
    rm -f ${mysqld_upgrade_log} 2> /dev/null || true

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
        --log-error-services=log_filter_internal;log_sink_internal \
        --log-error=${mysqld_err_log} \
        --log_output=NONE \
        --server-id=1 \
        --read_only=OFF --super_read_only=OFF \
        --pid-file=${mysql_upgrade_dir_path}/mysqld.pid \
        --socket=$upgrade_socket \
        --datadir=$datadir --wsrep_provider=none"

    if [[ -n $WSREP_SST_OPT_BASEDIR ]]; then
        mysqld_cmdline+=" --basedir=${WSREP_SST_OPT_BASEDIR}"
    fi

    if [[ -n $WSREP_SST_OPT_PLUGINDIR ]]; then
        mysqld_cmdline+=" --plugin-dir=${WSREP_SST_OPT_PLUGINDIR}"
    fi

    # Generate a new random password to be used by the JOINER
    local sst_user="mysql.pxc.sst.user"
    local sst_password="aA!9$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)"

    local -i timeout_threshold
    timeout_threshold=300

    #-----------------------------------------------------------------------
    # Reuse the SST User (use it to run mysql_upgrade)
    # Recreate with root permissions and a new password
    wsrep_log_debug "Starting the MySQL server used for post-processing"
    echo "---- Starting the MySQL server used for post-processing ----" >> ${mysqld_err_log}
    cat <<EOF | $mysqld_path $mysqld_cmdline --init-file=/dev/stdin &>> ${mysqld_err_log} &
SET sql_log_bin=OFF;
DROP USER IF EXISTS '${sst_user}'@localhost;
CREATE USER '${sst_user}'@localhost IDENTIFIED BY '${sst_password}';
GRANT ALL ON *.* TO '${sst_user}'@localhost;
EOF
    mysql_pid=$!


    #-----------------------------------------------------------------------
    # wait for mysql to come up
    wait_for_mysqld_startup "${mysql_pid}" "${upgrade_socket}" "${timeout_threshold}" \
        "${mysqld_err_log}" "${sst_user}" "${sst_password}" \
        "mysql server that checks for async replication"
    errcode=$?
    [[ $errcode -ne 0 ]] && return $errcode
    wsrep_log_debug "MySQL server($mysql_pid) started"


    #-----------------------------------------------------------------------
    # Stop replication activity

    if [[ $run_reset_slave == 'check' ]]; then
        wsrep_log_debug "Checking slave status"

        local slave_status=""
        slave_status=$($mysql_client_path \
                        --defaults-file=/dev/stdin \
                        --socket=$upgrade_socket \
                        --unbuffered --batch --silent --skip-column-names \
                        -e "SHOW SLAVE STATUS;" \
                        2> ${mysql_upgrade_dir_path}/show_slave_status.out <<EOF
[client]
user=${sst_user}
password="${sst_password}"
EOF
)
        errcode=$?
        if [[ $errcode -ne 0 ]]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to execute mysql 'SHOW SLAVE STATUS'. Check the parameters and retry"
            wsrep_log_error "Line $LINENO errcode:${errcode}"
            cat_file_to_stderr "${mysql_upgrade_dir_path}/show_slave_status.out" "ERR" "show slave status log"
            cat_file_to_stderr "${mysqld_err_log}" "ERR" "mysql error log"
            wsrep_log_error "****************************************   ************** "
            return 3
        fi
        if [[ -n $slave_status ]]; then
            run_reset_slave='yes'
        else
            run_reset_slave='no'
        fi
    fi

    if [[ $run_reset_slave == 'yes' ]]; then
        wsrep_log_debug "Resetting Async Slave"

        $mysql_client_path \
            --defaults-file=/dev/stdin \
            --socket=$upgrade_socket \
            --unbuffered --batch --silent \
            -e "SET sql_log_bin=OFF; RESET SLAVE ALL;" \
            &> ${mysql_upgrade_dir_path}/reset_slave.out <<EOF
[client]
user=${sst_user}
password="${sst_password}"
EOF
        errcode=$?
        if [[ $errcode -ne 0 ]]; then
            kill -9 $mysql_pid
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Failed to execute mysql 'RESET SLAVE ALL'. Check the parameters and retry"
            wsrep_log_error "Line $LINENO errcode:${errcode}"
            cat_file_to_stderr "${mysql_upgrade_dir_path}/reset_slave.out" "ERR" "reset slave log"
            cat_file_to_stderr "${mysqld_err_log}" "ERR" "mysql error log"
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
        -e "SET sql_log_bin=OFF; ALTER USER '${sst_user}'@localhost ACCOUNT LOCK; SHUTDOWN;" \
        &> ${mysql_upgrade_dir_path}/upgrade_shutdown.out <<EOF
[client]
user=${sst_user}
password="${sst_password}"
EOF
    errcode=$?
    if [[ $errcode -ne 0 ]]; then
        sleep 3
        kill -9 $mysql_pid
        echo "---- Killed the MySQL server used for post-processing ----" >> ${mysqld_err_log}
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Failed to shutdown the MySQL instance started for upgrade."
        wsrep_log_error "Check the parameters and retry.  Killing the process."
        wsrep_log_error "Line $LINENO errcode:${errcode}"
        cat_file_to_stderr "${mysql_upgrade_dir_path}/upgrade_shutdown.out" "ERR" "shutdown log"
        cat_file_to_stderr "${mysqld_err_log}" "ERR" "mysql error log"
        wsrep_log_error "****************************************************** "
        return 3
    fi

    wait_for_mysqld_shutdown $mysql_pid $timeout_threshold "MySQL server started for the upgrade process"
    [[ $? -ne 0 ]] && return 1

    wsrep_log_debug "MySQL server shut down"
    echo "---- Stopped the MySQL server used for post-processing ----" >> ${mysqld_err_log}

    #-----------------------------------------------------------------------
    # cleanup

    return 0
}


# Reads incoming data from STDIN and sets the variables
#
# Globals:
#   WSREP_SST_OPT_USER (sets this variable)
#   WSREP_SST_OPT_PSWD (sets this variable)
#
# Parameters:
#   None
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
    return 0
}


# Takes a file and writes it to stderr in a way that allows
# for better formatting by the server
#
# Globals:
#   None
#
# Parameters:
#   Argument 1: Path to the file
#   Argument 2: Priority level (DBG, INF, WRN, ERR)
#   Argument 3: File description
#
function cat_file_to_stderr()
{
    local file_path=$1
    local level=$2
    local description=$3

    echo "FIL:${level}:${description}" >&2
    cat $file_path >&2
    echo "EOF:" >&2
}


# Returns the absolute path from a path to a file (with a filename)
#   If a relative path is given as an argument, the absolute path
#   is generated from the current path.
#
# Globals:
#   None
#
# Parameters:
#   Argument 1: path to a file
#
# Returns 0 if successful (path exists) and the absolute path is output.
# Returns non-zero otherwise
#
function get_absolute_path()
{
    local path="$1"
    local abs_path retvalue
    local filename

    filename=$(basename "${path}")
    abs_path=$(cd "$(dirname "${path}")" && pwd)
    retvalue=$?
    [[ $retvalue -ne 0 ]] && return $retvalue

    printf "%s/%s" "${abs_path}" "${filename}"
    return 0
}
