#!/usr/bin/env bash
# Copyright (C) 2009-2015 Codership Oy
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

# This is a reference script for mysqldump-based state snapshot tansfer

set -o nounset -o errexit

. $(dirname $0)/wsrep_sst_common

EINVAL=22

local_ip()
{
    PATH=$PATH:/usr/sbin:/usr/bin:/sbin:/bin

    [ "$1" = "127.0.0.1" ]      && return 0
    [ "$1" = "localhost" ]      && return 0
    [ "$1" = "[::1]" ]          && return 0
    [ "$1" = "$(hostname -s)" ] && return 0
    [ "$1" = "$(hostname -f)" ] && return 0
    [ "$1" = "$(hostname -d)" ] && return 0

    # Now if ip program is not found in the path, we can't return 0 since
    # it would block any address. Thankfully grep should fail in this case
    ip route get "$1" | grep local >/dev/null && return 0

    return 1
}

if test -z "$WSREP_SST_OPT_HOST";  then wsrep_log_error "HOST cannot be nil";  exit $EINVAL; fi
if test -z "$WSREP_SST_OPT_PORT";  then wsrep_log_error "PORT cannot be nil";  exit $EINVAL; fi
if test -z "$WSREP_SST_OPT_LPORT"; then wsrep_log_error "LPORT cannot be nil"; exit $EINVAL; fi
if test -z "$WSREP_SST_OPT_SOCKET";then wsrep_log_error "SOCKET cannot be nil";exit $EINVAL; fi
if test -z "$WSREP_SST_OPT_GTID";  then wsrep_log_error "GTID cannot be nil";  exit $EINVAL; fi

if local_ip $WSREP_SST_OPT_HOST && \
   [ "$WSREP_SST_OPT_PORT" = "$WSREP_SST_OPT_LPORT" ]
then
    wsrep_log_error \
    "destination address '$WSREP_SST_OPT_HOST:$WSREP_SST_OPT_PORT' matches source address."
    exit $EINVAL
fi

# Check client version
CLIENT_MINOR=$($MYSQL_CLIENT --version | cut -d ' ' -f 6 | cut -d '.' -f 2)
if [ $CLIENT_MINOR -lt "6" ]
then
    $MYSQL_CLIENT --version >&2
    wsrep_log_error "this operation requires MySQL client version 5.6.x"
    exit $EINVAL
fi

[ -n "${WSREP_SST_OPT_USER:-}" ] && AUTH="-u$WSREP_SST_OPT_USER" || AUTH=

# Refs https://github.com/codership/mysql-wsrep/issues/141
# Passing password in MYSQL_PWD environment variable is considered
# "extremely insecure" by MySQL Guidelines for Password Security
# (https://dev.mysql.com/doc/refman/5.6/en/password-security-user.html)
# that is even less secure than passing it on a command line! It is doubtful:
# the whole command line is easily observable by any unprivileged user via ps,
# whereas (at least on Linux) unprivileged user can't see process environment
# that he does not own. So while it may be not secure in the NSA sense of the
# word, it is arguably more secure than passing password on the command line.
[ -n "${WSREP_SST_OPT_PSWD:-}" ] && export MYSQL_PWD="$WSREP_SST_OPT_PSWD"
[ -n "${SST_PROGRESS_FILE:-}" ] && touch $SST_PROGRESS_FILE

STOP_WSREP="SET wsrep_on=OFF;"

MYSQLDUMP_GENERAL="$MYSQLDUMP --defaults-extra-file=$WSREP_SST_OPT_CONF \
$AUTH -S$WSREP_SST_OPT_SOCKET \
--add-drop-database --add-drop-table --skip-add-locks --create-options \
--disable-keys --extended-insert --skip-lock-tables --quick --set-charset \
--skip-comments --flush-privileges --all-databases --events --routines"

MYSQLDUMP_GTID_EXECUTED="$MYSQLDUMP --defaults-extra-file=$WSREP_SST_OPT_CONF \
$AUTH -S$WSREP_SST_OPT_SOCKET \
--add-drop-table --skip-add-locks --create-options \
--disable-keys --extended-insert --skip-lock-tables --quick --set-charset \
--skip-comments --flush-privileges mysql gtid_executed --set-gtid-purged=OFF"

SET_START_POSITION="SET GLOBAL wsrep_start_position='$WSREP_SST_OPT_GTID';"

MYSQL="$MYSQL_CLIENT --defaults-extra-file=$WSREP_SST_OPT_CONF "\
"$AUTH -h${WSREP_SST_OPT_HOST_UNESCAPED:-$WSREP_SST_OPT_HOST} "\
"-P$WSREP_SST_OPT_PORT --disable-reconnect --connect_timeout=10"

# need to disable logging when loading the dump
# reason is that dump contains ALTER TABLE for log tables, and
# this causes an error if logging is enabled
GENERAL_LOG_OPT=`$MYSQL --skip-column-names -e "$STOP_WSREP SELECT @@GENERAL_LOG"`
SLOW_LOG_OPT=`$MYSQL --skip-column-names -e "$STOP_WSREP SELECT @@SLOW_QUERY_LOG"`
PXC_STRICT_MODE=`$MYSQL --skip-column-names -e "$STOP_WSREP SELECT @@pxc_strict_mode"`

$MYSQL -e "$STOP_WSREP SET GLOBAL GENERAL_LOG=OFF"
$MYSQL -e "$STOP_WSREP SET GLOBAL SLOW_QUERY_LOG=OFF"
$MYSQL -e "$STOP_WSREP SET GLOBAL pxc_strict_mode=DISABLED"

# commands to restore log settings
RESTORE_GENERAL_LOG="SET GLOBAL GENERAL_LOG=$GENERAL_LOG_OPT;"
RESTORE_SLOW_QUERY_LOG="SET GLOBAL SLOW_QUERY_LOG=$SLOW_LOG_OPT;"
RESTORE_PXC_STRICT_MODE="SET GLOBAL pxc_strict_mode='$PXC_STRICT_MODE';"

TURNOFF_SQL_LOG_BIN="SET SESSION sql_log_bin=0;"
if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
then
    # need to disable logging when loading the dump
    # reason is that dump contains ALTER TABLE for log tables, and
    # this causes an error if logging is enabled
    GENERAL_LOG_OPT=`$MYSQL --skip-column-names -e "$STOP_WSREP SELECT @@GENERAL_LOG"`
    SLOW_LOG_OPT=`$MYSQL --skip-column-names -e "$STOP_WSREP SELECT @@SLOW_QUERY_LOG"`
    $MYSQL -e "$STOP_WSREP SET GLOBAL GENERAL_LOG=OFF"
    $MYSQL -e "$STOP_WSREP SET GLOBAL SLOW_QUERY_LOG=OFF"

    # commands to restore log settings
    RESTORE_GENERAL_LOG="SET GLOBAL GENERAL_LOG=$GENERAL_LOG_OPT;"
    RESTORE_SLOW_QUERY_LOG="SET GLOBAL SLOW_QUERY_LOG=$SLOW_LOG_OPT;"

    # reset master for 5.6 to clear GTID_EXECUTED
    RESET_MASTER="RESET MASTER;"

    # commented out from dump command for 5.6: && echo $CSV_TABLES_FIX \
    # error is ignored because joiner binlog might be disabled.
    # and if joiner binlog is disabled, 'RESET MASTER' returns error
    # ERROR 1186 (HY000) at line 2: Binlog closed, cannot RESET MASTER
    (echo $STOP_WSREP && echo $RESET_MASTER) | $MYSQL || true

    (echo $STOP_WSREP \
        && $MYSQLDUMP_GENERAL \
        && echo $RESTORE_GENERAL_LOG \
        && echo $RESTORE_SLOW_QUERY_LOG \
        && echo $RESTORE_PXC_STRICT_MODE \
        && echo $SET_START_POSITION \
        || echo "SST failed to complete;") | $MYSQL
    (echo $STOP_WSREP \
        && echo $TURNOFF_SQL_LOG_BIN \
        && $MYSQLDUMP_GTID_EXECUTED \
        || echo "SST failed to complete;") | $MYSQL -D mysql
else
    wsrep_log_info "Bypassing state dump (SST)."
    echo $SET_START_POSITION | $MYSQL
    (echo $RESTORE_GENERAL_LOG \
     && echo $RESTORE_SLOW_QUERY_LOG \
     && echo $RESTORE_PXC_STRICT_MODE) | $MYSQL
fi
wsrep_cleanup_progress_file
#
