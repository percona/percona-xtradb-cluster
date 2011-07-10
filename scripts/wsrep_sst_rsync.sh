#!/bin/bash -ue
# Copyright (C) 2010 Codership Oy
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

trap "exit 32" SIGHUP SIGPIPE
trap kill_rsync EXIT


RSYNC_PID=
RSYNC_CONF=

kill_rsync()
{
#set -x
    local PID=$(cat "$RSYNC_PID" 2>/dev/null || echo 0)
    [ "0" != "$PID" ] && kill $PID && (kill $PID && kill -9 $PID) || :
    rm -rf "$RSYNC_CONF"
    rm -rf "$MAGIC_FILE"
#set +x
}

check_pid()
{
    local pid_file=$1
    [ -r $pid_file ] && ps -p $(cat $pid_file) | grep rsync > /dev/null 2>&1
}

ROLE=$1
ADDR=$2
AUTH=$3
DATA=$4

MAGIC_FILE="$DATA/rsync_sst_complete"
rm -rf "$MAGIC_FILE"

FLUSHED="$DATA/tables_flushed"
rm -rf "$FLUSHED"

if [ "$ROLE" = "donor" ]
then
    # Use deltaxfer only for WAN
    [ "$0" == "wsrep_sst_rsync_wan" ] && WHOLE_FILE_OPT="" \
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

    rsync -rlpgoDqI $WHOLE_FILE_OPT --inplace --delete \
          --exclude '*.err' --exclude '*.pid' --exclude '*.sock' --exclude '*.conf' \
          --exclude 'core' --exclude 'galera.*' --exclude 'grastate.txt' \
          "$DATA/" rsync://$ADDR

# it looks like copying logfiles is mandatory at least for fresh nodes.
#          --exclude 'ib_logfile*' \

    echo "$STATE" > "$MAGIC_FILE"
    rsync -aqc "$MAGIC_FILE" rsync://$ADDR

    echo "done $STATE"

elif [ "$ROLE" = "joiner" ]
then
    MODULE="rsync_sst"

    RSYNC_PID="$DATA/$MODULE.pid"

    if check_pid $RSYNC_PID
    then
        echo "rsync daemon already running."
        exit 114 # EALREADY
    fi
    rm -rf "$RSYNC_PID"

    RSYNC_PORT=$(echo $ADDR | awk -F ':' '{ print $2 }')
    if [ -z "$RSYNC_PORT" ]
    then
        RSYNC_PORT=4444
        ADDR="$(echo $ADDR | awk -F ':' '{ print $1 }'):$RSYNC_PORT"
    fi

    MYUID=$(id -u)
    MYGID=$(id -g)
    RSYNC_CONF="$DATA/$MODULE.conf"
    echo "pid file = $RSYNC_PID" >  "$RSYNC_CONF"
    echo "use chroot = no"       >> "$RSYNC_CONF"
    echo "[${MODULE}]"           >> "$RSYNC_CONF"
    echo "	path = $DATA"    >> "$RSYNC_CONF"
    echo "	read only = no"  >> "$RSYNC_CONF"
    echo "	timeout = 300"   >> "$RSYNC_CONF"
    echo "	uid = $MYUID"    >> "$RSYNC_CONF"
    echo "	gid = $MYGID"    >> "$RSYNC_CONF"

    rm -rf "$DATA"/ib_logfile* # we don't want old logs around

    # listen at all interfaces (for firewalled setups)
    rsync --daemon --port $RSYNC_PORT --config "$RSYNC_CONF"

    until [ -r "$RSYNC_PID" ]
    do
        sleep 0.2
    done

    trap "exit 32" SIGHUP SIGPIPE SIGCHLD
    trap kill_rsync EXIT

    echo "ready $ADDR/$MODULE"

    # wait for SST to complete by monitoring magic file
    while [ ! -r "$MAGIC_FILE" ] && check_pid "$RSYNC_PID"
    do
        sleep 1
    done

    if [ -r "$MAGIC_FILE" ]
    then
        cat "$MAGIC_FILE" # output UUID:seqno
    else
        # this message should cause joiner to abort
        echo "rsync process ended without creating '$MAGIC_FILE'"
    fi

#    kill_rsync
else
    echo "Unrecognized role: $ROLE"
    exit 22 # EINVAL
fi

exit 0
