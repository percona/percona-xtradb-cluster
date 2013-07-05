#!/bin/bash -ue
# Copyright (C) 2013 Percona Inc
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

#############################################################################################################
#     This is a reference script for Percona XtraBackup-based state snapshot transfer                       #
#     Dependencies:  (depending on configuration)                                                           #
#     xbcrypt for encryption/decryption.                                                                    #
#     qpress for decompression. Download from http://www.quicklz.com/qpress-11-linux-x64.tar till           #
#     https://blueprints.launchpad.net/percona-xtrabackup/+spec/package-qpress is fixed.                    #
#     my_print_defaults to extract values from my.cnf.                                                      #
#     netcat or socat for transfer.                                                                         #
#     xbstream/tar for streaming. (and xtrabackup ofc)                                                      #
#     ss from iproute package                                                                               #
#     pv if progress is enabled                                                                             #
#                                                                                                           #
#############################################################################################################

###############################################################################################################
# Following SST specific options (under [sst]) are allowed in my.cnf:                                         #
#                                                                                                             #
# a) streamfmt=xbstream|tar : default is tar (till lp:1193240 is fixed).                                      #
#      You need to use xbstream for encryption, compression etc., however,                                    #
#      lp:1193240 requires you to manually cleanup the directory prior to SST                                 #
#                                                                                                             #
# b) transferfmt=socat|nc : default is socat and recommended.                                                 #
#                           Recommended because it allows for socket options like transfer buffer sizes       #
#                                                                                                             #
# c) tca = CA file for openssl based encryption with socat.                                                   #
#                                                                                                             #
# d) tcert = PEM for openssl based encryption with socat.                                                     #
#                                                                                                             #
# e) encrypt = 0|1|2 : decides whether encryption is to be done or not, if                                    #
#    this is zero, no encryption is done.                                                                     #
#    1 - Xtrabackup based encryption                                                                          #
#    2 - OpenSSL based encryption                                                                             # 
#                                                                                                             #
# f) sockopt = comma separated key/value pairs of socket options. Must                                        #
# begin with a comma. Refer to socat manual for details.                                                      #
#                                                                                                             #
# g) progress = 1|path-to-file|path-to-fifo:                                                                  #
#     If 1 it writes to mysql stderr                                                                          #
#        path-to-file writes to that file                                                                     #
#    If path is to a fifo (full path), it will be created and cleaned up at exit. This is the preferred way.  #
#        You need to cat the fifo file to monitor the progress, not tailf it.                                 #
#     Note: Value of 0 is not valid                                                                           #
#                                                                                                             #
# h) rebuild = 1|0 - 1 implies rebuild indexes. Note this is independent of compaction, though compaction     #
# enables it.                                                                                                 #
#    Rebuild of indexes may be used as an optimization.                                                       #
#                                                                                                             #
# i) time = 0|1  - enabling it instruments key stages of backup/restore in SST                                #
#                                                                                                             #
# j) incremental=0|1  -  To be set on joiner only, supersedes IST if set. Read documentation before setting   #
#                         this.                                                                               #
#                                                                                                             #
# For c) and d), refer to http://www.dest-unreach.org/socat/doc/socat-openssltunnel.html for an example.      #
#                                                                                                             #
# Values of a), b) and e) must match on donor and joiner.                                                     #
#                                                                                                             #
# socat based openssl encryption: encrypt=2 is recommended for now.                                           #
#                                                                                                             #
# socat must be built with openSSL for encryption: socat -V | grep OPENSSL                                    #
#                                                                                                             #
###############################################################################################################

. $(dirname $0)/wsrep_sst_common

ealgo=""
ekey=""
ekeyfile=""
encrypt=0
nproc=1
ecode=0
XTRABACKUP_PID=""
SST_PORT=""
REMOTEIP=""
tcert=""
tpem=""
sockopt=""
progress=""
ttime=0
totime=0
lsn=""
incremental=0
ecmd=""

sfmt="tar"
strmcmd=""
tfmt=""
tcmd=""
rebuild=0
rebuildcmd=""
payload=0
pvopts="-f -i 10 -N $WSREP_SST_OPT_ROLE -F '%N => Rate:%r Avg:%a Elapsed:%t %e Bytes: %b %p'"
pcmd="pv $pvopts"
declare -a RC

INNOBACKUPEX_BIN=innobackupex
readonly AUTH=(${WSREP_SST_OPT_AUTH//:/ })
DATA="${WSREP_SST_OPT_DATA}"
INFO_FILE="xtrabackup_galera_info"
IST_FILE="xtrabackup_ist"
MAGIC_FILE="${DATA}/${INFO_FILE}"

timeit(){
    local stage=$1
    shift
    local cmd="$@"
    local x1 x2 took extcode

    if [[ $ttime -eq 1 ]];then 
        x1=$(date +%s)
        eval "$cmd"
        extcode=$?
        x2=$(date +%s)
        took=$(( x2-x1 ))
        wsrep_log_info "NOTE: $stage took $took seconds"
        totime=$(( totime+took ))
    else 
        eval "$cmd"
        extcode=$?
    fi
    return $extcode
}

get_keys()
{
    if [[ $encrypt -eq 0 || $encrypt -eq 2 ]];then 
        return 
    fi

    if [[ $sfmt == 'tar' ]];then
        wsrep_log_info "NOTE: Encryption cannot be enabled with tar format"
        encrypt=0
        return
    fi

    wsrep_log_info "Xtrabackup based encryption enabled in my.cnf - not supported at the moment - lp:1190343"
    wsrep_log_info "Use socat-based openSSL encryption with encrypt=2 under [sst]"

    if [[ -z $ealgo ]];then
        wsrep_log_error "FATAL: Encryption algorithm empty from my.cnf, bailing out"
        exit 3
    fi

    if [[ -z $ekey && ! -r $ekeyfile ]];then
        wsrep_log_error "FATAL: Either key or keyfile must be readable"
        exit 3
    fi

    if [[ -z $ekey ]];then
        ecmd="xbcrypt --encrypt-algo=$ealgo --encrypt-key-file=$ekeyfile"
    else
        ecmd="xbcrypt --encrypt-algo=$ealgo --encrypt-key=$ekey"
    fi

    if [[ "$WSREP_SST_OPT_ROLE" == "joiner" ]];then
        ecmd+=" -d"
    fi
}

get_transfer()
{
    if [[ $tfmt == 'nc' ]];then
        if [[ ! -x `which nc` ]];then 
            wsrep_log_error "nc(netcat) not found in path: $PATH"
            exit 2
        fi
        wsrep_log_info "Using netcat as streamer"
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
            tcmd="nc -dl ${SST_PORT}"
        else
            tcmd="nc ${REMOTEIP} ${SST_PORT}"
        fi
    else
        tfmt='socat'
        wsrep_log_info "Using socat as streamer"
        if [[ ! -x `which socat` ]];then 
            wsrep_log_error "socat not found in path: $PATH"
            exit 2
        fi

        if ! socat -V | grep -q OPENSSL && [[ $encrypt -eq 2 ]];then 
            wsrep_log_info "NOTE: socat is not openssl enabled, falling back to plain transfer"
            encrypt=0
        fi

        if [[ $encrypt -eq 2 ]];then 
            wsrep_log_info "Using openssl based encryption with socat"
            if [[ -z $tpem || -z $tcert ]];then 
                wsrep_log_error "Both PEM and CRT files required"
                exit 22
            fi
            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
                tcmd="socat openssl-listen:${SST_PORT},reuseaddr,cert=$tpem,cafile=${tcert}${sockopt} stdio"
            else
                tcmd="socat stdio openssl-connect:${REMOTEIP}:${SST_PORT},cert=$tpem,cafile=${tcert}${sockopt}"
            fi
        else 
            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
                tcmd="socat TCP-LISTEN:${SST_PORT},reuseaddr${sockopt} stdio"
            else
                tcmd="socat stdio TCP:${REMOTEIP}:${SST_PORT}${sockopt}"
            fi
        fi
    fi
}

parse_cnf()
{
    local group=$1
    local var=$2
    reval=$(my_print_defaults -c $WSREP_SST_OPT_CONF $group | grep -- "--$var" | cut -d= -f2)
    if [[ -z $reval ]];then 
        [[ -n $3 ]] && reval=$3
    fi
    echo $reval
}

get_footprint()
{
    pushd $WSREP_SST_OPT_DATA 1>/dev/null
    payload=$(du --block-size=1 -c  **/*.ibd **/*.MYI **/*.MYI ibdata1  | awk 'END { print $1 }')
    if my_print_defaults -c $WSREP_SST_OPT_CONF xtrabackup | grep -q -- "--compress";then 
        # QuickLZ has around 50% compression ratio
        payload=$(( payload*1/2 ))
    fi
    popd 1>/dev/null
    pcmd+=" -s $payload"
    adjust_progress
}

adjust_progress()
{
    if [[ -n $progress && $progress -ne 1 ]];then 
        if [[ $progress == /*.fif ]];then 
            wsrep_log_info "Creating fifo file $progress for tracking progress"
            mkfifo $progress
        fi
        
        if [[ -e $progress ]];then 
            pcmd+=" 2>>$progress"
        else 
            pcmd+=" 2>$progress"
        fi
    fi
}

read_cnf()
{
    sfmt=$(parse_cnf sst streamfmt "tar")
    tfmt=$(parse_cnf sst transferfmt "socat")
    tcert=$(parse_cnf sst tca "")
    tpem=$(parse_cnf sst tcert "")
    encrypt=$(parse_cnf sst encrypt 0)
    sockopt=$(parse_cnf sst sockopt "")
    progress=$(parse_cnf sst progress "")
    rebuild=$(parse_cnf sst rebuild 0)
    ttime=$(parse_cnf sst time 0)
    incremental=$(parse_cnf sst incremental 0)
    ealgo=$(parse_cnf xtrabackup encrypt "")
    ekey=$(parse_cnf xtrabackup encrypt-key "")
    ekeyfile=$(parse_cnf xtrabackup encrypt-key-file "")
}

get_stream()
{
    if [[ $sfmt == 'xbstream' ]];then 
        wsrep_log_info "Streaming with xbstream"
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
            wsrep_log_info "xbstream requires manual cleanup of data directory before SST - lp:1193240"
            strmcmd="xbstream -x -C \${DATA}"
        else
            strmcmd="xbstream -c \${INFO_FILE} \${IST_FILE}"
        fi
    else
        sfmt="tar"
        wsrep_log_info "Streaming with tar"
        wsrep_log_info "Note: Advanced xtrabackup features - encryption,compression etc. not available with tar."
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
            wsrep_log_info "However, xbstream requires manual cleanup of data directory before SST - lp:1193240."
            strmcmd="tar xfi - -C \${DATA}"
        else
            strmcmd="tar cf - \${INFO_FILE} \${IST_FILE}"
        fi

    fi
}

get_proc()
{
    set +e
    nproc=$(grep -c processor /proc/cpuinfo)
    [[ -z $nproc || $nproc -eq 0 ]] && nproc=1
    set -e
}

cleanup_joiner()
{
    # Since this is invoked just after exit NNN
    local estatus=$?
    if [[ $estatus -ne 0 ]];then 
        wsrep_log_error "Cleanup after exit with status:$estatus"
    fi
    local PID=$(ps -aef | grep nc | grep $SST_PORT  | awk '{ print $2 }')
    if [[ $estatus -ne 0 ]];then 
        wsrep_log_error "Killing nc pid $PID"
    else 
        wsrep_log_info "Killing nc pid $PID"
    fi
    [ -n "$PID" -a "0" != "$PID" ] && kill $PID && (kill $PID && kill -9 $PID) || :
    rm -f "$MAGIC_FILE"
    if [ "${WSREP_SST_OPT_ROLE}" = "joiner" ];then
        wsrep_log_info "Removing the sst_in_progress file"
        wsrep_cleanup_progress_file
    fi
    if [[ -p $progress ]];then 
        wsrep_log_info "Cleaning up fifo file"
        rm $progress
    fi
}

check_pid()
{
    local pid_file="$1"
    [ -r "$pid_file" ] && ps -p $(cat "$pid_file") >/dev/null 2>&1
}

cleanup_donor()
{
    # Since this is invoked just after exit NNN
    local estatus=$?
    if [[ $estatus -ne 0 ]];then 
        wsrep_log_error "Cleanup after exit with status:$estatus"
    fi
    local pid=$XTRABACKUP_PID
    if check_pid "$pid"
    then
        wsrep_log_error "xtrabackup process is still running. Killing... "
        kill_xtrabackup
    fi

    rm -f "$pid"
    rm -f ${DATA}/${IST_FILE}

    if [[ -p $progress ]];then 
        wsrep_log_info "Cleaning up fifo file $progress"
        rm $progress
    fi
}

kill_xtrabackup()
{
    local PID=$(cat $XTRABACKUP_PID)
    [ -n "$PID" -a "0" != "$PID" ] && kill $PID && (kill $PID && kill -9 $PID) || :
    rm -f "$XTRABACKUP_PID"
}

setup_ports()
{
    if [[ "$WSREP_SST_OPT_ROLE"  == "donor" ]];then
        SST_PORT=$(echo $WSREP_SST_OPT_ADDR | awk -F '[:/]' '{ print $2 }')
        REMOTEIP=$(echo $WSREP_SST_OPT_ADDR | awk -F ':' '{ print $1 }')
        lsn=$(echo $WSREP_SST_OPT_ADDR | awk -F '[:/]' '{ print $4 }')
    else
        SST_PORT=$(echo ${WSREP_SST_OPT_ADDR} | awk -F ':' '{ print $2 }')
    fi
}

# waits ~10 seconds for nc to open the port and then reports ready
# (regardless of timeout)
wait_for_listen()
{
    local PORT=$1
    local ADDR=$2
    local MODULE=$3
    for i in {1..50}
    do
        ss -p state listening "( sport = :$PORT )" | grep -qE 'socat|nc' && break
        sleep 0.2
    done
    if [[ $incremental -eq 1 ]];then 
        echo "ready ${ADDR}/${MODULE}/$lsn"
    else 
    echo "ready ${ADDR}/${MODULE}"
    fi
}

if [[ ! -x `which innobackupex` ]];then 
    wsrep_log_error "innobackupex not in path: $PATH"
    exit 2
fi

rm -f "${MAGIC_FILE}"

if [[ ! ${WSREP_SST_OPT_ROLE} == 'joiner' && ! ${WSREP_SST_OPT_ROLE} == 'donor' ]];then 
    wsrep_log_error "Invalid role ${WSREP_SST_OPT_ROLE}"
    exit 22
fi

read_cnf
setup_ports
get_stream
get_transfer

INNOAPPLY="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF} --redo-only --apply-log $rebuildcmd ${DATA} &>${DATA}/innobackup.prepare.log"
INNOBACKUP="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF} --galera-info --stream=$sfmt \${TMPDIR} 2>${DATA}/innobackup.backup.log"

if [ "$WSREP_SST_OPT_ROLE" = "donor" ]
then
    trap cleanup_donor EXIT

    if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
    then
        TMPDIR="/tmp"

        if [ "${AUTH[0]}" != "(null)" ]; then
           INNOBACKUP+=" --user=${AUTH[0]}"
        fi

        if [ ${#AUTH[*]} -eq 2 ]; then
           INNOBACKUP+=" --password=${AUTH[1]}"
       else
           # Empty password, used for testing, debugging etc.
           INNOBACKUP+=" --password="
       fi

        get_keys
        if [[ $encrypt -eq 1 ]];then
            if [[ -n $ekey ]];then
                INNOBACKUP+=" --encrypt=$ealgo --encrypt-key=$ekey"
            else 
                INNOBACKUP+=" --encrypt=$ealgo --encrypt-key-file=$ekeyfile"
            fi
        fi

        if [[ -n $lsn ]];then 
                INNOBACKUP+=" --incremental --incremental-lsn=$lsn"
        fi

        wsrep_log_info "Streaming the backup to joiner at ${REMOTEIP} ${SST_PORT}"

        if [[ -n $progress ]];then 
            get_footprint
            tcmd="$pcmd | $tcmd"
        fi

        set +e
        timeit "Donor-Transfer" "$INNOBACKUP | $tcmd; RC=( "\${PIPESTATUS[@]}" )"
        set -e

        if [ ${RC[0]} -ne 0 ]; then
          wsrep_log_error "${INNOBACKUPEX_BIN} finished with error: ${RC[0]}. " \
                          "Check ${DATA}/innobackup.backup.log"
          exit 22
        elif [[ ${RC[$(( ${#RC[@]}-1 ))]} -eq 1 ]];then 
          wsrep_log_error "$tcmd finished with error: ${RC[1]}"
          exit 22
        fi

        # innobackupex implicitly writes PID to fixed location in ${TMPDIR}
        XTRABACKUP_PID="${TMPDIR}/xtrabackup_pid"


    else # BYPASS FOR IST

        wsrep_log_info "Bypassing the SST for IST"
        STATE="${WSREP_SST_OPT_GTID}"
        echo "continue" # now server can resume updating data
        echo "${STATE}" > "${MAGIC_FILE}"
        echo "1" > "${DATA}/${IST_FILE}"
        get_keys
        pushd ${DATA} 1>/dev/null
        set +e
        if [[ $encrypt -eq 1 ]];then
            tcmd=" $ecmd | $tcmd"
        fi
        timeit "Donor-IST-Unencrypted-transfer" "$strmcmd | $tcmd; RC=( "\${PIPESTATUS[@]}" )"
        set -e
        popd 1>/dev/null

        for ecode in "${RC[@]}";do 
            if [[ $ecode -ne 0 ]];then 
                wsrep_log_error "Error while streaming data to joiner node: " \
                                "exit codes: ${RC[@]}"
                exit 1
            fi
        done
    fi

    echo "done ${WSREP_SST_OPT_GTID}"
    wsrep_log_info "Total time on donor: $totime seconds"

elif [ "${WSREP_SST_OPT_ROLE}" = "joiner" ]
then
    [[ -e $SST_PROGRESS_FILE ]] && wsrep_log_info "Stale sst_in_progress file: $SST_PROGRESS_FILE"
    touch $SST_PROGRESS_FILE

    if [[ ! -e ${DATA}/ibdata1 ]];then 
        incremental=0
    fi

    if [[ $incremental -eq 1 ]];then 
        wsrep_log_info "Incremental SST enabled"
        #lsn=$(/pxc/bin/mysqld --defaults-file=$WSREP_SST_OPT_CONF  --basedir=/pxc  --wsrep-recover 2>&1 | grep -o 'log sequence number .*' | cut -d " " -f 4 | head -1)
        lsn=$(grep to_lsn xtrabackup_checkpoints | cut -d= -f2 | tr -d ' ')
        wsrep_log_info "Recovered LSN: $lsn"
    fi

    sencrypted=1
    nthreads=1

    MODULE="xtrabackup_sst"

    rm -f ${DATA}/xtrabackup_binary ${DATA}/xtrabackup_galera_info  ${DATA}/xtrabackup_logfile

    ADDR=${WSREP_SST_OPT_ADDR}
    if [ -z "${SST_PORT}" ]
    then
        SST_PORT=4444
        ADDR="$(echo ${WSREP_SST_OPT_ADDR} | awk -F ':' '{ print $1 }'):${SST_PORT}"
    fi

    wait_for_listen ${SST_PORT} ${ADDR} ${MODULE} &

    trap "exit 32" HUP PIPE
    trap "exit 3"  INT TERM
    trap cleanup_joiner EXIT

    if [[ -n $progress ]];then 
        adjust_progress
        tcmd+=" | $pcmd"
    fi

    if [[ $incremental -eq 1 ]];then 
        BDATA=$DATA
        DATA=$(mktemp -d)
        MAGIC_FILE="${DATA}/${INFO_FILE}"
    fi

    get_keys
    set +e
    if [[ $encrypt -eq 1 && $sencrypted -eq 1 ]];then
        strmcmd=" $ecmd | $strmcmd"
    fi
    timeit "Joiner-Recv-Unencrypted" "$tcmd | $strmcmd; RC=( "\${PIPESTATUS[@]}" )"
    set -e

    if [[ $sfmt == 'xbstream' ]];then 
        # Special handling till lp:1193240 is fixed"
        if [[ ${RC[$(( ${#RC[@]}-1 ))]} -eq 1 ]];then 
            wsrep_log_error "Xbstream failed"
            wsrep_log_error "Data directory ${DATA} needs to be empty for SST: lp:1193240" \
                            "Manual intervention required in that case"
            exit 32
        fi
    fi

    wait %% # join for wait_for_listen thread

    for ecode in "${RC[@]}";do 
        if [[ $ecode -ne 0 ]];then 
            wsrep_log_error "Error while getting data from donor node: " \
                            "exit codes: ${RC[@]}"
            exit 32
        fi
    done

    if [ ! -r "${MAGIC_FILE}" ]
    then
        # this message should cause joiner to abort
        wsrep_log_error "xtrabackup process ended without creating '${MAGIC_FILE}'"
        exit 32
    fi

    if ! ps -p ${WSREP_SST_OPT_PARENT} &>/dev/null
    then
        wsrep_log_error "Parent mysqld process (PID:${WSREP_SST_OPT_PARENT}) terminated unexpectedly." 
        exit 32
    fi

    if [ ! -r "${DATA}/${IST_FILE}" ]
    then
        wsrep_log_info "Proceeding with SST"
        wsrep_log_info "Removing existing ib_logfile files"
        if [[ $incremental -ne 1 ]];then 
            rm -f ${DATA}/ib_logfile*
        else
            rm -f ${BDATA}/ib_logfile*
        fi

        get_proc

        # Rebuild indexes for compact backups
        if grep -q 'compact = 1' ${DATA}/xtrabackup_checkpoints;then 
            wsrep_log_info "Index compaction detected"
            rebuild=1
        fi

        if [[ $rebuild -eq 1 ]];then 
            nthreads=$(parse_cnf xtrabackup rebuild-threads $nproc)
            wsrep_log_info "Rebuilding during prepare with $nthreads threads"
            rebuildcmd="--rebuild-indexes --rebuild-threads=$nthreads"
        fi

        if test -n "$(find ${DATA} -maxdepth 1 -type f -name '*.qp' -print -quit)";then

            wsrep_log_info "Compressed qpress files found"

            if [[ ! -x `which qpress` ]];then 
                wsrep_log_error "qpress not found in path: $PATH"
                exit 22
            fi

            if [[ -n $progress ]];then
                count=$(find ${DATA} -type f -name '*.qp' | wc -l)
                count=$(( count*2 ))
                pvopts="-f -s $count -l -N Decompression -F '%N => Rate:%r Elapsed:%t %e Progress: [%b/$count]'"
                pcmd="pv $pvopts"
                adjust_progress
                dcmd="$pcmd | xargs -n 2 qpress -T${nproc}d"
            else 
                dcmd="xargs -n 2 qpress -T${nproc}d"
            fi

            wsrep_log_info "Removing existing ibdata1 file"
            rm -f ${DATA}/ibdata1

            # Decompress the qpress files 
            wsrep_log_info "Decompression with $nproc threads"
            timeit "Decompression" "find ${DATA} -type f -name '*.qp' -printf '%p\n%h\n' | $dcmd"
            extcode=$?

            if [[ $extcode -eq 0 ]];then
                wsrep_log_info "Removing qpress files after decompression"
                find ${DATA} -type f -name '*.qp' -delete 
                if [[ $? -ne 0 ]];then 
                    wsrep_log_error "Something went wrong with deletion of qpress files. Investigate"
                fi
            else
                wsrep_log_error "Decompression failed. Exit code: $extcode"
                exit 22
            fi
        fi

        if [[ $incremental -eq 1 ]];then 
            INNOAPPLY="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF} \
                --ibbackup=xtrabackup_55 --apply-log $rebuildcmd --redo-only $BDATA --incremental-dir=${DATA} &>>${BDATA}/innobackup.prepare.log"
        fi

        wsrep_log_info "Preparing the backup at ${DATA}"
        timeit "Xtrabackup prepare stage" "$INNOAPPLY"

        if [[ $incremental -eq 1 ]];then 
            wsrep_log_info "Cleaning up ${DATA} after incremental SST"
            [[ -d ${DATA} ]] && rm -rf ${DATA}
            DATA=$BDATA
        fi

        if [ $? -ne 0 ];
        then
            wsrep_log_error "${INNOBACKUPEX_BIN} finished with errors. Check ${DATA}/innobackup.prepare.log" 
            exit 22
        fi
    else 
        wsrep_log_info "${IST_FILE} received from donor: Running IST"
    fi

    cat "${MAGIC_FILE}" # output UUID:seqno
    wsrep_log_info "Total time on joiner: $totime seconds"
fi

exit 0
