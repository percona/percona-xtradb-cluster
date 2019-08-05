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

# Documentation: http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html
# Make sure to read that before proceeding!


#-------------------------------------------------------------------------------
#
# Step-1: Parse and read input params arguments. These are mainly
# related to role/username/password/etc...
#
. $(dirname $0)/wsrep_sst_common

#-------------------------------------------------------------------------------
#
# Step-2: Setup default global variable that we plan to use during processing.
#

# encryption specific variables.
ealgo=""
ekey=""
ekeyfile=""
encrypt=0
ieopts=""
xbstreameopts=""
xbstreameopts_sst=""
xbstreameopts_other=""

nproc=1
ecode=0
ssyslog=""
ssystag=""
SST_PORT=""
REMOTEIP=""
tca=""
tcert=""
tkey=""
sockopt=""
ncsockopt=""
progress=""
ttime=0
totime=0
lsn=""
ecmd=""
ecmd_other=""
rlimit=""
stagemsg="${WSREP_SST_OPT_ROLE}"
cpat=""
ib_home_dir=""
ib_log_dir=""
ib_undo_dir=""
sfmt=""
strmcmd=""
tfmt=""
tcmd=""
rebuild=0
rebuildcmd=""
payload=0
pvformat="-F '%N => Rate:%r Avg:%a Elapsed:%t %e Bytes: %b %p' "
pvopts="-f  -i 10 -N $WSREP_SST_OPT_ROLE "

uextra=0
disver=""

# keyring specific variables.

# Till PXC-5.7.22, only supporting keyring plugin was keyring_file plugin.

keyring_plugin=0

keyring_file_data=""
keyring_vault_config=""

keyring_server_id=""

keyringbackupopt=""
keyringapplyopt=""

XB_DONOR_KEYRING_FILE="donor-keyring"
XB_DONOR_KEYRING_FILE_PATH=""
KEYRING_FILE_DIR=""

# encrpytion options
transition_key=""
encrypt_backup_options=""
encrypt_prepare_options=""
encrypt_move_options=""

# Starting XB-2.4.11, XB started shipping custom build keyring plugin
# This variable points it to the location of the same.
xtrabackup_plugin_dir=""

# Root directory for temporary files. This directory (and everything in it)
# will be removed upon exit.
tmpdirbase=""

# tmpdir used as target-dir for xtrabackup by the donor
itmpdir=""

# tmpdir used by the joiner
STATDIR=""

scomp=""
sdecomp=""
ssl_dhparams=""
pxc_encrypt_cluster_traffic=""

ssl_cert=""
ssl_ca=""
ssl_key=""

# thread options
backup_threads=-1
encrypt_threads=-1

# Required for backup locks
# For backup locks it is 1 sent by joiner
# 5.6.21 PXC and later can't donate to an older joiner
sst_ver=1

# pv is used to to meter the data passing through.
# more for statiscally reporting.
if which pv &>/dev/null && pv --help | grep -q FORMAT; then
    pvopts+=$pvformat
fi
pcmd=""
declare -a RC

# default XB (xtrabackup-binary) to use.
INNOBACKUPEX_BIN=xtrabackup
DATA="${WSREP_SST_OPT_DATA}"

# These files carry some important information in form of GTID of the data
# that is being backed up.
XB_GTID_INFO_FILE="xtrabackup_galera_info"
XB_GTID_INFO_FILE_PATH="${DATA}/${XB_GTID_INFO_FILE}"
IST_FILE="xtrabackup_ist"

# Used to send a file containing info about the SST
# Extend this if you want to send additional information
# from the donor to the joiner
SST_INFO_FILE="sst_info"

# Setting the path for ss and ip
# ss: is utility to investigate socket, ip for network routes.
export PATH="/usr/sbin:/sbin:$PATH"

#-------------------------------------------------------------------------------
#
# Step-3: Helper function to carry-out the main workload.
#

#
# time each of the command that we run.
timeit()
{
    local stage=$1
    shift
    local cmd="$@"
    local x1 x2 took extcode

    if [[ $ttime -eq 1 ]]; then
        x1=$(date +%s)
        wsrep_log_debug "Evaluating (@ $stage) $cmd"
        eval "$cmd"
        extcode=$?
        x2=$(date +%s)
        took=$(( x2-x1 ))
        wsrep_log_debug "NOTE: $stage took $took seconds"
        totime=$(( totime+took ))
    else
        wsrep_log_debug "Evaluating (@ $stage) $cmd"
        eval "$cmd"
        extcode=$?
    fi
    return $extcode
}

#
# Configures the commands for encrypt=1
# Sets up the parameters (algo/keys) for XB-based encryption
# If the version of PXB is >= 2.4.7, changes the XB parameters
# If the version of PXB is < 2.4.7, use xbcrypt instead
#
get_keys()
{
    # $encrypt -eq -1 is for internal purposes only
    if [[ $encrypt -ge 2 || $encrypt -eq -1 ]]; then
        return
    fi

    # TODO: it seems like encrypt=1/2/3/4 was not provided
    if [[ $encrypt -eq 0 ]]; then
        if $MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF xtrabackup | grep -q encrypt; then
            wsrep_log_warning "Encryption configuration mis-match. SST may fail. Refer to http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html for more details."
        fi
        return
    fi

    if [[ $sfmt == 'tar' ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "* Xtrabackup-based encryption (encrypt = 1) cannot be  "
        wsrep_log_error "* enabled with the tar format.                         "
        wsrep_log_error "****************************************************** "
        exit 22
    fi

    wsrep_log_debug "Xtrabackup based encryption enabled in my.cnf"

    if [[ -z $ealgo ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "FATAL: Encryption algorithm empty from my.cnf, bailing out"
        wsrep_log_error "****************************************************** "
        exit 3
    fi

    if [[ -z $ekey && ! -r $ekeyfile ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "FATAL: Either key or keyfile must be readable"
        wsrep_log_error "****************************************************** "
        exit 3
    fi

    if [[ -n "$ekey" ]]; then
        wsrep_log_warning "Using the 'encrypt-key' option causes the encryption key"
        wsrep_log_warning "to be set via the command-line and is considered insecure."
        wsrep_log_warning "It is recommended to use the 'encrypt-key-file' option instead."
    fi

    local readonly encrypt_opts=""
    if [[ -z $ekey ]]; then
        encrypt_opts=" --encrypt-key-file=$ekeyfile "
    else
        encrypt_opts=" --encrypt-key=$ekey "
    fi

    # Setup the command for all non-SST transfers
    # Encryption is done by xbcrypt for all other (non-SST) transfers
    #
    ecmd_other="xbcrypt --encrypt-algo=$ealgo $encrypt_opts "
    if [[ "$WSREP_SST_OPT_ROLE" == "joiner" ]]; then
        # Decryption is done by xbcrypt for all other (non-SST) transfers
        ecmd_other+=" -d"
    fi

    #
    # PXB version >= 2.4.7 added encryption support directly in PXB
    #
    if check_for_version $XB_VERSION "2.4.7"; then
        # ensure that ecmd is clear because SST encryption
        # goes through xtrabackup, not a separate program
        ecmd=""

        if [[ "$WSREP_SST_OPT_ROLE" == "joiner" ]]; then
            # Decryption is done by xbstream for SST
            xbstreameopts_sst=" --decrypt=$ealgo $encrypt_opts --encrypt-threads=$encrypt_threads "
            xbstreameopts_other=""
            ieopts=""
        else
            # Encryption is done by xtrabackup for SST
            xbstreameopts_sst=""
            xbstreameopts_other=""
            ieopts=" --encrypt=$ealgo $encrypt_opts --encrypt-threads=$encrypt_threads "
        fi
    else
        #
        # For older versions, use xbcrypt for SST encryption/decryption
        # (same as for non-SST transfers)
        #
        ecmd=$ecmd_other
    fi

    stagemsg+="-XB-Encrypted"
}

#
# If the ssl_dhparams variable is already set, uses that as a source
# of dh parameters for OpenSSL. Otherwise, looks for dhparams.pem in the
# datadir, and creates it there if it can't find the file.
# No input parameters
#
check_for_dhparams()
{
    if [[ -z "$ssl_dhparams" ]]; then
        if ! [[ -r "$DATA/dhparams.pem" ]]; then
            wsrep_check_programs openssl
            wsrep_log_info "Could not find dhparams file, creating $DATA/dhparams.pem"

            if ! openssl dhparam -out "$DATA/dhparams.pem" 2048 >/dev/null 2>&1
            then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "* Could not create the dhparams.pem file with OpenSSL. "
                wsrep_log_error "****************************************************** "
                exit 22
            fi
        fi
        ssl_dhparams="$DATA/dhparams.pem"
    fi
}

#
# verifies that the certificate matches the private key
# doing this will save us having to wait for a timeout that would
# otherwise occur.
#
# 1st param: path to the cert
# 2nd param: path to the private key
#
verify_cert_matches_key()
{
    local cert_path=$1
    local key_path=$2

    wsrep_check_programs openssl diff

    # generate the public key from the cert and the key
    # they should match (otherwise we can't create an SSL connection)
    if ! diff <(openssl x509 -in "$cert_path" -pubkey -noout) <(openssl pkey -in "$key_path" -pubout 2>/dev/null) >/dev/null 2>&1
    then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "* The certifcate and private key do not match. "
        wsrep_log_error "* Please check your certificate and key files. "
        wsrep_log_error "****************************************************** "
        exit 22
    fi
}

#
# verifies that the CA file verifies the certificate
# doing this here lets us generate better error messages
#
# 1st param: path to the CA file
# 2nd param: path to the cert
#
verify_ca_matches_cert()
{
    local ca_path=$1
    local cert_path=$2

    wsrep_check_programs openssl

    if ! openssl verify -verbose -CAfile "$ca_path" "$cert_path" >/dev/null  2>&1
    then
        wsrep_log_error "******** FATAL ERROR ****************************************** "
        wsrep_log_error "* The certifcate and CA (certificate authority) do not match.   "
        wsrep_log_error "* It does not appear that the certificate was issued by the CA. "
        wsrep_log_error "* Please check your certificate and CA files.                   "
        wsrep_log_error "*************************************************************** "
        exit 22
    fi

}

#
# Checks to see if the file exists
# If the file does not exist (or cannot be read), issues an error
# and exits
#
# 1st param: file name to be checked (for read access)
# 2nd param: 1st error message (header)
# 3rd param: 2nd error message (footer, optional)
#
verify_file_exists()
{
    local file_path=$1
    local error_message1=$2
    local error_message2=$3

    if ! [[ -r "$file_path" ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "* $error_message1 "
        wsrep_log_error "* Could not find/access : $file_path "

        if ! [[ -z "$error_message2" ]]; then
            wsrep_log_error "* $error_message2 "
        fi

        wsrep_log_error "****************************************************** "
        exit 22
    fi
}

#
# Setup stream to transfer. (Alternative: nc or socat)
get_transfer()
{
    if [[ -z $SST_PORT ]]; then
        TSST_PORT=4444
    else
        TSST_PORT=$SST_PORT
    fi

    if [[ $tfmt == 'nc' ]]; then
        if [[ ! -x `which nc` ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "nc(netcat) not found in path: $PATH"
            wsrep_log_error "****************************************************** "
            exit 2
        fi

        if [[ $encrypt -eq 2 || $encrypt -eq 3 || $encrypt -eq 4 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "* Using SSL encryption (encrypt= 2, 3, or 4) "
            wsrep_log_error "* is not supported when using nc(netcat).    "
            wsrep_log_error "****************************************************** "
            exit 22
        fi

        wsrep_log_debug "Using netcat as streamer"
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
            if nc -h 2>&1 | grep -q ncat; then
                tcmd="nc $ncsockopt -l ${TSST_PORT}"
            else 
                tcmd="nc $ncsockopt -dl ${TSST_PORT}"
            fi
        else
            # netcat doesn't understand [] around IPv6 address
            tcmd="nc ${REMOTEIP//[\[\]]/} ${TSST_PORT}"
        fi
    else
        tfmt='socat'
        wsrep_log_debug "Using socat as streamer"
        if [[ ! -x `which socat` ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "socat not found in path: $PATH"
            wsrep_log_error "****************************************************** "
            exit 2
        fi

        donor_extra=""
        joiner_extra=""
        if [[ $encrypt -eq 2 || $encrypt -eq 3 || $encrypt -eq 4 ]]; then
            if ! socat -V | grep -q WITH_OPENSSL; then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "* socat is not openssl enabled.         "
                wsrep_log_error "* Unable to encrypt SST communications. "
                wsrep_log_error "****************************************************** "
                exit 2
            fi

            # Determine the socat version
            SOCAT_VERSION=`socat -V 2>&1 | grep -oe '[0-9]\.[0-9][\.0-9]*' | head -n1`
            if [[ -z "$SOCAT_VERSION" ]]; then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "* Cannot determine the socat version. "
                wsrep_log_error "****************************************************** "
                exit 2
            fi

            # socat versions < 1.7.3 will have 512-bit dhparams (too small)
            #       so create 2048-bit dhparams and send that as a parameter
            # socat version >= 1.7.3, checks to see if the peername matches the hostname
            #       set commonname="" to disable the peername checks
            #
            if ! check_for_version "$SOCAT_VERSION" "1.7.3"; then
                if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
                    # dhparams check (will create ssl_dhparams if needed)
                    check_for_dhparams
                    joiner_extra=",dhparam=$ssl_dhparams"
                fi
            fi
            if check_for_version "$SOCAT_VERSION" "1.7.3"; then
                donor_extra=',commonname=""'
            fi
        fi

        # prepend a comma if it's not already there
        if [[ -n "${sockopt}" ]] && [[ "${sockopt}" != ","* ]]; then
            sockopt=",${sockopt}"
        fi

        if [[ $encrypt -eq 2 ]]; then
            wsrep_log_warning "**** WARNING **** encrypt=2 is deprecated and will be removed in a future release"
            wsrep_log_debug "Using openssl based encryption with socat: with crt and ca"

            verify_file_exists "$tcert" "Both certificate and CA files are required." \
                                        "Please check the 'tcert' option.           "
            verify_file_exists "$tca" "Both certificate and CA files are required." \
                                      "Please check the 'tca' option.             "
            verify_ca_matches_cert $tca $tcert

            stagemsg+="-OpenSSL-Encrypted-2"
            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
                wsrep_log_debug "Decrypting with CERT: $tcert, CA: $tca"
                tcmd="socat -u openssl-listen:${TSST_PORT},reuseaddr,cert=${tcert},cafile=${tca}${joiner_extra}${sockopt} stdio"
            else
                wsrep_log_debug "Encrypting with CERT: $tcert, CA: $tca"
                tcmd="socat -u stdio openssl-connect:${REMOTEIP}:${TSST_PORT},cert=${tcert},cafile=${tca}${donor_extra}${sockopt}"
            fi
        elif [[ $encrypt -eq 3 ]];then
            wsrep_log_warning "**** WARNING **** encrypt=3 is deprecated and will be removed in a future release"
            wsrep_log_debug "Using openssl based encryption with socat: with key and crt"

            verify_file_exists "$tcert" "Both certificate and key files are required." \
                                        "Please check the 'tcert' option.            "
            verify_file_exists "$tkey" "Both certificate and key files are required." \
                                       "Please check the 'tkey' option.             "
            verify_cert_matches_key $tcert $tkey

            stagemsg+="-OpenSSL-Encrypted-3"
            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]];then
                wsrep_log_debug "Decrypting with CERT: $tcert, KEY: $tkey"
                tcmd="socat -u openssl-listen:${TSST_PORT},reuseaddr,cert=${tcert},key=${tkey},verify=0${joiner_extra}${sockopt} stdio"
            else
                wsrep_log_debug "Encrypting with CERT: $tcert, KEY: $tkey"
                tcmd="socat -u stdio openssl-connect:${REMOTEIP}:${TSST_PORT},cert=${tcert},key=${tkey},verify=0${sockopt}"
            fi
        elif [[ $encrypt -eq 4 ]]; then
            wsrep_log_debug "Using openssl based encryption with socat: with key, crt, and ca"

            verify_file_exists "$ssl_ca" "CA, certificate, and key files are required." \
                                         "Please check the 'ssl-ca' option.           "
            verify_file_exists "$ssl_cert" "CA, certificate, and key files are required." \
                                           "Please check the 'ssl-cert' option.         "
            verify_file_exists "$ssl_key" "CA, certificate, and key files are required." \
                                          "Please check the 'ssl-key' option.          "

            verify_cert_matches_key $ssl_cert $ssl_key
            verify_ca_matches_cert $ssl_ca $ssl_cert

            stagemsg+="-OpenSSL-Encrypted-4"
            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
                wsrep_log_debug "Decrypting with CERT: $ssl_cert, KEY: $ssl_key, CA: $ssl_ca"
                tcmd="socat -u openssl-listen:${TSST_PORT},reuseaddr,cert=${ssl_cert},key=${ssl_key},cafile=${ssl_ca},verify=1${joiner_extra}${sockopt} stdio"
            else
                wsrep_log_debug "Encrypting with CERT: $ssl_cert, KEY: $ssl_key, CA: $ssl_ca"
                tcmd="socat -u stdio openssl-connect:${REMOTEIP}:${TSST_PORT},cert=${ssl_cert},key=${ssl_key},cafile=${ssl_ca},verify=1${donor_extra}${sockopt}"
            fi

        else
            if [[ $encrypt -eq 1 ]]; then
                wsrep_log_warning "**** WARNING **** encrypt=1 is deprecated and will be removed in a future release"
            fi

            if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
                tcmd="socat -u TCP-LISTEN:${TSST_PORT},reuseaddr${sockopt} stdio"
            else
                tcmd="socat -u stdio TCP:${REMOTEIP}:${TSST_PORT}${sockopt}"
            fi
        fi
    fi
}

#
# read the sst specific options.
read_cnf()
{
    sfmt=$(parse_cnf sst streamfmt "xbstream")
    tfmt=$(parse_cnf sst transferfmt "socat")
    tca=$(parse_cnf sst tca "")
    tcert=$(parse_cnf sst tcert "")
    tkey=$(parse_cnf sst tkey "")
    encrypt=$(parse_cnf sst encrypt 0)
    sockopt=$(parse_cnf sst sockopt "")
    ncsockopt=$(parse_cnf sst ncsockopt "")
    rebuild=$(parse_cnf sst rebuild 0)
    ttime=$(parse_cnf sst time 0)
    scomp=$(parse_cnf sst compressor "")
    sdecomp=$(parse_cnf sst decompressor "")
    xbstreameopts=$(parse_cnf sst xbstreameopts "")

    # if wsrep_node_address is not set raise a warning
    wsrep_node_address=$(parse_cnf mysqld wsrep-node-address "")
    wsrep_sst_receive_address=$(parse_cnf mysqld wsrep-sst-receive-address "")
    if [[ -z $wsrep_node_address && -z $wsrep_sst_receive_address ]]; then
        wsrep_log_warning "wsrep_node_address or wsrep_sst_receive_address not set." \
                          "Consider setting them if SST fails."
    fi

    # If pv is not in the PATH, then disable the 'progress'
    # and 'rlimit' options
    progress=$(parse_cnf sst progress "")
    rlimit=$(parse_cnf sst rlimit "")
    if [[ -n "$progress" ]] || [[ -n "$rlimit" ]]; then
        pcmd="pv $pvopts"
        if [[ ! -x `which pv` ]]; then
            wsrep_log_error "pv not found in path: $PATH"
            wsrep_log_error "Disabling all progress/rate-limiting"
            pcmd=""
            rlimit=""
            progress=""
        fi
    fi

    #------- KEYRING config parsing

    #======================================================================
    # Parse for keyring plugin. Only 1 plugin can be enabled at given time.
    keyring_file_data=$(parse_cnf mysqld keyring-file-data "")
    keyring_vault_config=$(parse_cnf mysqld keyring-vault-config "")
    if [[ -n $keyring_file_data && -n $keyring_vault_config ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Can't have keyring_file and keyring_vault enabled at same time"
        wsrep_log_error "****************************************************** "
        exit 32
    fi

    if [[ -n $keyring_file_data || -n $keyring_vault_config ]]; then
        keyring_plugin=1
    fi

    if [[ -n $keyring_file_data ]]; then
        KEYRING_FILE_DIR=$(dirname "${keyring_file_data}")
    fi

    #======================================================================
    # If we can't find a server-id, assume a default of 0
    keyring_server_id=$(parse_cnf mysqld server-id "")
    if [[ -z $keyring_server_id ]]; then
        keyring_server_id=0
    fi

    #======================================================================
    # Starting PXB-2.4.11, XB needs plugin directory as it has its own
    # compiled version of keyring plugins. (if no specified it will look at
    # default location and fail if not found there)
    xtrabackup_plugin_dir=$(parse_cnf xtrabackup xtrabackup-plugin-dir "")
    if [[ $keyring_plugin -eq 1 && -z $xtrabackup_plugin_dir ]]; then
        wsrep_log_warning "WARNING: xtrabackup-plugin-dir (under xtrabackup section) missing"
        wsrep_log_warning "xtrabackup installation will lookout for plugin at default location"
    fi

    # Refer to http://www.percona.com/doc/percona-xtradb-cluster/manual/xtrabackup_sst.html
    ealgo=$(parse_cnf xtrabackup encrypt "")
    ekey=$(parse_cnf xtrabackup encrypt-key "")
    ekeyfile=$(parse_cnf xtrabackup encrypt-key-file "")
    if [[ -z $ealgo ]]; then
        ealgo=$(parse_cnf sst encrypt-algo "")
        ekey=$(parse_cnf sst encrypt-key "")
        ekeyfile=$(parse_cnf sst encrypt-key-file "")
    fi

    # Pull the parameters needed for encrypt=4
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

    pxc_encrypt_cluster_traffic=$(parse_cnf mysqld pxc-encrypt-cluster-traffic "")
    pxc_encrypt_cluster_traffic=$(normalize_boolean "$pxc_encrypt_cluster_traffic" "off")

    ssl_dhparams=$(parse_cnf sst ssl-dhparams "")

    uextra=$(parse_cnf sst use-extra 0)
    iopts=$(parse_cnf sst inno-backup-opts "")
    iapts=$(parse_cnf sst inno-apply-opts "")
    impts=$(parse_cnf sst inno-move-opts "")
    stimeout=$(parse_cnf sst sst-initial-timeout 100)
    ssyslog=$(parse_cnf sst sst-syslog 0)
    ssystag=$(parse_cnf mysqld_safe syslog-tag "${SST_SYSLOG_TAG:-}")
    ssystag+="-"

    # thread options
    encrypt_threads=$(parse_cnf sst encrypt-threads -1)
    if [[ $encrypt_threads -le 0 ]]; then
        encrypt_threads=4
    fi

    backup_threads=$(parse_cnf sst backup-threads -1)
    if [[ $backup_threads -le 0 ]]; then
        backup_threads=4
    fi

    if [[ $ssyslog -ne -1 ]]; then
        if $MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF mysqld_safe | tr '_' '-' | grep -q -- "--syslog"; then
            ssyslog=1
        fi
    fi

    # Pull out the buffer pool size to be used by PXB
    # and set it in use-memory (if not specified in inno-apply-opts)
    #
    # Is the use-memory option already specified in inno-apply-opts?
    if ! [[ "$iapts" =~ --use-memory= ]]; then
        bufferpoolsize=$(parse_cnf xtrabackup use-memory "")
        if [[ -z "$bufferpoolsize" ]]; then
            bufferpoolsize=$(parse_cnf mysqld innodb-buffer-pool-size "")
        fi
        if [[ -n "$bufferpoolsize" ]]; then
            iapts="$iapts --use-memory=$bufferpoolsize"
        fi
    fi

    # Setup # of threads (if not already specified)
    if [[ ! "$iopts" =~ --parallel= ]]; then
        iopts+=" --parallel=$backup_threads"
    fi

    # Buildup the list of files to keep in the datadir
    # (These files will not be REMOVED on the joiner node)
    cpat=$(parse_cnf sst cpat '.*\.pem$\|.*init\.ok$\|.*galera\.cache$\|.*sst_in_progress$\|.*\.sst$\|.*gvwstate\.dat$\|.*grastate\.dat$\|.*\.err$\|.*\.log$\|.*RPM_UPGRADE_MARKER$\|.*RPM_UPGRADE_HISTORY$')

    # Keep the donor's keyring file
    cpat+="\|.*/${XB_DONOR_KEYRING_FILE}$"

    # Keep the KEYRING_FILE_DIR if it is a subdir of the datadir
    # Normalize the datadir for comparison
    local readonly DATA_TEMP=$(dirname "$DATA/xxx")
    if [[ -n "$KEYRING_FILE_DIR" && "$KEYRING_FILE_DIR" != "$DATA_TEMP" && "$KEYRING_FILE_DIR/" =~ $DATA ]]; then

        # Add the path to each subdirectory, this will ensure that
        # the path to the keyring is kept
        local CURRENT_DIR=$(dirname "$KEYRING_FILE_DIR/xx")
        while [[ $CURRENT_DIR != "." && $CURRENT_DIR != "/" && $CURRENT_DIR != "$DATA_TEMP" ]]; do
            cpat+="\|${CURRENT_DIR}$"
            CURRENT_DIR=$(dirname "$CURRENT_DIR")
        done
    fi

    # Retry the connection 30 times (at 1-second intervals)
    if [[ ! "$sockopt" =~ retry= ]]; then
        sockopt+=",retry=30"
    fi

}

#
# get a feel of how big payload is being used to estimate progress.
get_footprint()
{
    if [[ -z "$pcmd" ]]; then
        return
    fi
    pushd $WSREP_SST_OPT_DATA 1>/dev/null
    payload=$(find . -regex '.*\.ibd$\|.*\.MYI$\|.*\.MYD$\|.*ibdata1$' -type f -print0 | xargs -0 du --block-size=1 -c | awk 'END { print $1 }')
    if $MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF xtrabackup | grep -q -- "--compress"; then
        # QuickLZ has around 50% compression ratio
        # When compression/compaction used, the progress is only an approximate.
        payload=$(( payload*1/2 ))
    fi
    popd 1>/dev/null
    pcmd+=" -s $payload"
    adjust_progress
}

#
# progress emitting function. This is just for user to monitor there is
# no programatic use of this monitoring.
adjust_progress()
{
    if [[ -z "$pcmd" ]]; then
        return
    fi
    if [[ -n $progress && $progress != '1' ]]; then
        if [[ -e $progress ]]; then
            pcmd+=" 2>>$progress"
        else
            pcmd+=" 2>$progress"
        fi
    elif [[ -z $progress && -n $rlimit  ]]; then
            # When rlimit is non-zero
            pcmd="pv -q"
    fi

    if [[ -n $rlimit && "$WSREP_SST_OPT_ROLE"  == "donor" ]]; then
        wsrep_log_debug "Rate-limiting SST to $rlimit"
        pcmd+=" -L \$rlimit"
    fi
}

#
# Fills in strmcmd, which holds the command used for streaming
#
# Note:
#   This code creates a command that uses FILE_TO_STREAM
#
get_stream()
{
    if [[ $sfmt == 'xbstream' ]]; then
        wsrep_log_debug "Streaming with xbstream"
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
            strmcmd="xbstream \$xbstreameopts -x"
        else
            strmcmd="xbstream \$xbstreameopts -c \${FILE_TO_STREAM}"
        fi
    else
        sfmt="tar"
        wsrep_log_debug "Streaming with tar"
        if [[ "$WSREP_SST_OPT_ROLE"  == "joiner" ]]; then
            strmcmd="tar xfi - "
        else
            strmcmd="tar cf - \${FILE_TO_STREAM} "
        fi

    fi
}

#
# Some more monitoring and cleanup functions.
get_proc()
{
    set +e
    nproc=$(grep -c processor /proc/cpuinfo)
    [[ -z $nproc || $nproc -eq 0 ]] && nproc=1
    set -e
}

sig_joiner_cleanup()
{
    wsrep_log_error "Removing $XB_GTID_INFO_FILE_PATH file due to signal"
    rm -f "$XB_GTID_INFO_FILE_PATH" 2> /dev/null
    wsrep_log_error "Removing $XB_DONOR_KEYRING_FILE_PATH file due to signal"
    rm -f "$XB_DONOR_KEYRING_FILE_PATH" 2> /dev/null || true
}

cleanup_joiner()
{
    # Since this is invoked just after exit NNN
    local estatus=$?
    if [[ $estatus -ne 0 ]]; then
        wsrep_log_error "Cleanup after exit with status:$estatus"
    elif [ "${WSREP_SST_OPT_ROLE}" = "joiner" ]; then
        wsrep_log_debug "Removing the sst_in_progress file"
        wsrep_cleanup_progress_file
    fi
    if [[ -n $progress && -p $progress ]]; then
        wsrep_log_debug "Cleaning up fifo file $progress"
        rm $progress
    fi
    if [[ -n "${tmpdirbase}" ]]; then
        [[ -d "${tmpdirbase}" ]] && rm -rf "${tmpdirbase}" || true
    fi

    if [[ -r "${XB_DONOR_KEYRING_FILE_PATH}" ]]; then
        rm -f "${XB_DONOR_KEYRING_FILE_PATH}"
    fi

    # Final cleanup
    pgid=$(ps -o pgid= $$ | grep -o '[0-9]*')

    # This means no setsid done in mysqld.
    # We don't want to kill mysqld here otherwise.
    if [[ $$ -eq $pgid ]]; then

        # This means a signal was delivered to the process.
        # So, more cleanup.
        if [[ $estatus -ge 128 ]]; then
            kill -KILL -$$ || true
        fi

    fi

    exit $estatus
}

cleanup_donor()
{
    # Since this is invoked just after exit NNN
    local estatus=$?
    if [[ $estatus -ne 0 ]]; then
        wsrep_log_error "Cleanup after exit with status:$estatus"
    fi

    rm -f ${DATA}/${IST_FILE} || true

    if [[ -n $progress && -p $progress ]]; then
        wsrep_log_debug "Cleaning up fifo file $progress"
        rm -f $progress || true
    fi

    wsrep_log_debug "Cleaning up temporary directories"

    if [[ -n "${tmpdirbase}" ]]; then
        [[ -d "${tmpdirbase}" ]] && rm -rf "${tmpdirbase}" || true
    fi

    # Final cleanup
    pgid=$(ps -o pgid= $$ | grep -o '[0-9]*')

    # This means no setsid done in mysqld.
    # We don't want to kill mysqld here otherwise.
    if [[ $$ -eq $pgid ]]; then

        # This means a signal was delivered to the process.
        # So, more cleanup.
        if [[ $estatus -ge 128 ]]; then
            kill -KILL -$$ || true
        fi

    fi

    rm -rf "${KEYRING_FILE_DIR}/${XB_DONOR_KEYRING_FILE}" || true

    exit $estatus

}

setup_ports()
{
    if [[ "$WSREP_SST_OPT_ROLE"  == "donor" ]];then
        SST_PORT=$WSREP_SST_OPT_PORT
        REMOTEIP=$WSREP_SST_OPT_HOST
        lsn=$(echo $WSREP_SST_OPT_PATH | awk -F '[/]' '{ print $2 }')
        sst_ver=$(echo $WSREP_SST_OPT_PATH | awk -F '[/]' '{ print $3 }')
    else
        SST_PORT=$WSREP_SST_OPT_PORT
    fi
}

# Returns a list of parent pids, until we reach pid=1 or any process
# that starts with 'mysql'.  The list is returned as a string
# separated by spaces.
#
# Parameter 1: the child pid
#
# This function will stop going up the parent process chain
# when a process starting with 'mysql' is found.
#
function get_parent_pids()
{
    local mypid=$1
    local list_of_pids=" "

    while [[ $mypid -ne 1 ]]; do
        local ps_out=$(ps -h -o ppid= -o comm $mypid 2>/dev/null)
        if [[ $? -ne 0 || -z $ps_out ]]; then
            break
        fi
        list_of_pids+="$mypid "

        # If we reach a process that starts with mysql, exit
        # such as mysqld, mysqld-debug, mysqld-safe
        if [[ $(echo $ps_out | awk '{ print $2 }') =~ ^mysql ]]; then
            break
        fi

        mypid=$(echo $ps_out | awk '{ print $1 }')
    done

    if [[ $list_of_pids = " " ]]; then
        list_of_pids=""
    fi
    echo "$list_of_pids"
}

# waits ~1 minute for nc/socat to open the port and then reports ready
# (regardless of timeout)
#
# Assumptions:
#   1. The socat/nc processes do not launch subprocesses to handle
#      the connections.  Note that socat can be configured to do so.
#   2. socat is not bound to a specific interface/address, so the
#      IP portion of the local address is all zeros (0000...000).
#
# Parameter 1: the pid of the wsrep_sst_xtrabackup-v2 process. We are looking
#              for the socat process that was started by this script.
# Parameter 2: the IP address of the SST host
# Parameter 3: the Port of the SST host
# Parameter 4: a descriptive name for what we are doing at this point
#
wait_for_listen()
{
    local parentpid=$1
    local host=$2
    local port=$3
    local module=$4

    #
    # Check to see if the OS supports /proc/<pid>/net/tcp
    #
    if [[ ! -r /proc/$$/net/tcp && ! -r /proc/$$/net/tcp6 ]]; then
        wsrep_log_debug "$LINENO: Using ss for socat/nc discovery"

        # Revert to using ss to check if socat/nc is listening
        wsrep_check_program ss
        if [[ $? -ne 0 ]]; then
            wsrep_log_error "******** FATAL ERROR *********************** "
            wsrep_log_error "* Could not find 'ss'.  Check that it is installed and in the path."
            wsrep_log_error "******************************************** "
            return 2
        fi

        for i in {1..300}
        do
            ss -p state listening "( sport = :${port} )" | grep -qE 'socat|nc' && break
            sleep 0.2
        done

        echo "ready ${host}:${port}/${module}//$sst_ver"
        return 0
    fi

    wsrep_log_debug "$LINENO: Using /proc/pid/net/tcp for socat/nc discovery"

    # Get the index for the 'local_address' column in /proc/xxxx/net/tcp
    # We expect this to be the same for IPv4 (net/tcp) and IPv6 (net/tcp6)
    local ip_index=0
    local header
    if [[ -r /proc/$$/net/tcp ]]; then
        read -ra header <<< $(head -n 1 /proc/$$/net/tcp)
    elif [[ -r /proc/$$/net/tcp6 ]]; then
        read -ra header <<< $(head -n 1 /proc/$$/net/tcp6)
    else
        wsrep_log_error "******** FATAL ERROR *********************** "
        wsrep_log_error "* Cannot find /proc/$$/net/tcp (or tcp6)"
        wsrep_log_error "******************************************** "
        exit 1
    fi
    for i in "${!header[@]}"; do
        if [[ ${header[$i]} = "local_address" ]]; then
            # Add one to the index since arrays are 0-based
            # but awk is 1-based
            ip_index=$(( i + 1 ))
            break
        fi
    done
    if [[ $ip_index -eq 0 ]]; then
        wsrep_log_error "******** FATAL ERROR *********************** "
        wsrep_log_error "* Unexpected /proc/xx/net/tcp layout: cannot find local_address"
        wsrep_log_error "******************************************** "
        exit 1
    fi

    wsrep_log_debug "$LINENO: local_address index is $ip_index"
    local port_in_hex
    port_in_hex=$(printf "%04X" $port)

    local user_id
    user_id=$(id -u)

    for i in {1..300}
    do
        # List all socat/nc processes started by the user of this script
        # Then look for processes that have the script pid as a parent prcoess
        # somewhere in the process tree

        wsrep_log_debug "$LINENO: Entering loop body : $i"

        # List only socat/nc processes started by this user to avoid triggering SELinux
        for pid in $(ps -u $user_id -o pid,comm | grep -E 'socat|nc' | awk '{ printf $1 " " }')
        do
            if [[ -z $pid || $pid = " " ]]; then
                continue
            fi

            wsrep_log_debug "$LINENO: Examining pid: $pid"

            # Now get the processtree for this pid
            # If the parentpid is NOT in the process tree, then ignore
            if ! echo "$(get_parent_pids $pid)" | grep -qw "$parentpid"; then
                wsrep_log_debug "$LINENO: $parentpid is not in the process tree: $(get_parent_pids $pid)"
                continue
            fi

            # get the sockets for the pid
            # Note: may not need to get the list of sockets, is it ok to
            # just look at the list of local addresses in tcp?
            local sockets
            sockets=$(ls -l /proc/$pid/fd | grep socket | cut -d'[' -f2 | cut -d ']' -f1 | tr '\n' '|')

            # remove the trailing '|'
            sockets=${sockets%|}
            wsrep_log_debug "$LINENO: sockets: $sockets"

            if [[ -n $sockets ]]; then
                # For the network addresses, we expect to be listening
                # on all interfaces, thus the address should be
                # 00..000:PORT (all zeros for the IP address).

                # Dumping the data in the lines
                #if [[ -n "$WSREP_LOG_DEBUG" ]]; then
                #    lines=$(grep -E "\s(${sockets})\s" /proc/$pid/net/tcp)
                #    if [[ -n $lines ]]; then
                #        while read -r line; do
                #            if [[ -z $line ]]; then
                #                continue
                #            fi
                #            wsrep_log_debug "$LINENO: $line"
                #        done <<< "$lines\n"
                #    fi
                #fi

                # Checking IPv4
                if grep -E "\s(${sockets})\s" /proc/$pid/net/tcp |
                        awk "{print \$${ip_index}}" |
                        grep -q "^00*:${port_in_hex}$"; then
                    wsrep_log_debug "$LINENO: found a match for pid: $pid"
                    break 2
                fi

                # Also check IPv6
                if grep -E "\s(${sockets})\s" /proc/$pid/net/tcp6 |
                        awk "{print \$${ip_index}}" |
                        grep -q "^00*:${port_in_hex}$"; then
                    break 2
                fi
            fi
        done

        sleep 0.2
    done

    wsrep_log_debug "$LINENO: wait_for_listen() exiting"
    echo "ready ${host}:${port}/${module}//$sst_ver"
    return 0
}

#
# check if there are any extra options to parse.
# Note: if port is specified socket is not used.
check_extra()
{
    local use_socket=1
    if [[ $uextra -eq 1 ]]; then
        if $MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF mysqld | tr '_' '-' | grep -- "--thread-handling=" | grep -q 'pool-of-threads'; then
            local eport=$($MY_PRINT_DEFAULTS -c $WSREP_SST_OPT_CONF mysqld | tr '_' '-' | grep -- "--extra-port=" | cut -d= -f2)
            if [[ -n $eport ]]; then
                # Xtrabackup works only locally.
                # Hence, setting host to 127.0.0.1 unconditionally.
                wsrep_log_debug "SST through extra_port $eport"
                INNOEXTRA+=" --host=127.0.0.1 --port=$eport "
                use_socket=0
            else
                wsrep_log_error "Extra port $eport null, failing"
                exit 1
            fi
        else
            wsrep_log_warning "Thread pool not set, ignore the option use_extra"
        fi
    fi
    if [[ $use_socket -eq 1 ]] && [[ -n "${WSREP_SST_OPT_SOCKET}" ]]; then
        INNOEXTRA+=" --socket=${WSREP_SST_OPT_SOCKET}"
    fi
}

#
# JOINER function to recieve data from DONOR.
#
# 1st param: dir
# 2nd param: msg
# 3rd param: tmt : timeout
# 4th param: checkf : file check
#   If this is -2, skip the file checks (but still check the return code)
#   If this is -1, skip all error checking
#   If this is 0, do nothing (no additional checks)
#   If this is 1, check to see if the gtid info file exists
#   If this is 2, check to see if the keyring file exists
recv_data_from_donor_to_joiner()
{
    local dir=$1
    local msg=$2
    local tmt=$3
    local checkf=$4
    local ltcmd

    if [[ ! -d ${dir} ]]; then
        # This indicates that IST is in progress
        return
    fi

    pushd ${dir} 1>/dev/null
    set +e

    if [[ $tmt -gt 0 && -x `which timeout` ]]; then
        if timeout --help | grep -q -- '-k'; then
            ltcmd="timeout -k $(( tmt+10 )) $tmt $tcmd"
        else
            ltcmd="timeout -s9 $tmt $tcmd"
        fi
        timeit "$msg" "$ltcmd | $strmcmd; RC=( "\${PIPESTATUS[@]}" )"
    else
        timeit "$msg" "$tcmd | $strmcmd; RC=( "\${PIPESTATUS[@]}" )"
    fi

    set -e
    popd 1>/dev/null

    if [[ $checkf -eq -1 ]]; then
        # we don't care about errors (or we expect an error to occur)
        # just return
        return
    fi

    if [[ ${RC[0]} -eq 124 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Possible timeout in receving first data from donor in gtid/keyring stage"
        wsrep_log_error "****************************************************** "
        exit 32
    fi

    for ecode in "${RC[@]}";do
        if [[ $ecode -ne 0 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Error while getting data from donor node: " \
                            "exit codes: ${RC[@]}"
            wsrep_log_error "****************************************************** "
            exit 32
        fi
    done

    if [[ $checkf -eq -2 ]]; then
        # no file checking
        return
    fi

    if [[ $checkf -eq 1 && ! -r "${XB_GTID_INFO_FILE_PATH}" ]]; then
        # this message should cause joiner to abort
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "xtrabackup process ended without creating '${XB_GTID_INFO_FILE_PATH}'"
        wsrep_log_error "****************************************************** "
        wsrep_log_debug "Contents of datadir"
        wsrep_log_debug "$(ls -l ${dir}/*)"
        exit 32
    fi

    if [[ $checkf -eq 2 && ! -r "${XB_DONOR_KEYRING_FILE_PATH}" ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "FATAL: xtrabackup could not find '${XB_DONOR_KEYRING_FILE_PATH}'"
        wsrep_log_error "The joiner is using a keyring file but the donor has not sent"
        wsrep_log_error "a keyring file.  Please check your configuration to ensure that"
        wsrep_log_error "both sides are using a keyring file"
        wsrep_log_error "****************************************************** "
        exit 32
    fi
}

#
# Process to send data from DONOR to JOINER
#
# Parameters:
#   1 : dir - the base directory (paths are based on this)
#   2 : msg - descriptive message
#
send_data_from_donor_to_joiner()
{
    local dir=$1
    local msg=$2

    pushd ${dir} 1>/dev/null
    set +e
    timeit "$msg" "$strmcmd | $tcmd; RC=( "\${PIPESTATUS[@]}" )"
    set -e
    popd 1>/dev/null


    for ecode in "${RC[@]}";do
        if [[ $ecode -ne 0 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Error while sending data to joiner node: " \
                            "exit codes: ${RC[@]}"
            wsrep_log_error "****************************************************** "
            exit 32
        fi
    done
}

# Returns the version string in a standardized format
# Input "1.2.3" => echoes "010203"
# Wrongly formatted values => echoes "000000"
normalize_version()
{
    local major=0
    local minor=0
    local patch=0

    # Only parses purely numeric version numbers, 1.2.3 
    # Everything after the first three values are ignored
    if [[ $1 =~ ^([0-9]+)\.([0-9]+)\.?([0-9]*)([\.0-9])*$ ]]; then
        major=${BASH_REMATCH[1]}
        minor=${BASH_REMATCH[2]}
        patch=${BASH_REMATCH[3]}
    fi

    printf %02d%02d%02d $major $minor $patch
}

# Compares two version strings
# The first parameter is the version to be checked
# The second parameter is the minimum version required
# Returns 0 (success) if $1 >= $2, 1 (failure) otherwise
check_for_version()
{
    local local_version_str="$( normalize_version $1 )"
    local required_version_str="$( normalize_version $2 )"

    if [[ "$local_version_str" < "$required_version_str" ]]; then
        return 1
    else
        return 0
    fi
}

#
# Initiailizes the tmpdir
# Reads the info from the config file and creates the tmpdir as needed.
#
# Sets the $tmpdirbase variable to the root of the temporary directory
# to be used by SST. This directory will be removed upon exiting the script.
#
initialize_tmpdir()
{
    local tmpdir_path=""

    tmpdir_path=$(parse_cnf sst tmpdir "")
    if [[ -z "${tmpdir_path}" ]]; then
        tmpdir_path=$(parse_cnf xtrabackup tmpdir "")
    fi
    if [[ -z "${tmpdir_path}" ]]; then
        tmpdir_path=$(parse_cnf mysqld tmpdir "")
    fi
    if [[ -n "${tmpdir_path}" ]]; then
        if [[ ! -d "${tmpdir_path}" ]]; then
            wsrep_log_error "Cannot find the directory, ${tmpdir_path}, the tmpdir must exist before startup."
            exit 2
        fi
        if [[ ! -r "${tmpdir_path}" ]]; then
            wsrep_log_error "The temporary directory, ${tmpdir_path}, is not readable.  Please check the directory permissions."
            exit 22
        fi
        if [[ ! -w "${tmpdir_path}" ]]; then
            wsrep_log_error "The temporary directory, ${tmpdir_path}, is not writable.  Please check the directory permissions."
            exit 22
        fi
    fi

    if [[ -z "${tmpdir_path}" ]]; then
        tmpdir_path=$(mktemp --tmpdir --directory pxc_sst_XXXX)
    else
        tmpdir_path=$(mktemp --tmpdir="${tmpdir_path}" --directory pxc_sst_XXXX)
    fi

    # This directory (and everything in it), will be removed upon exit
    tmpdirbase=$tmpdir_path
}


#
# Parses the passed in config file and returns the option in the
# specified group.
#
# 1st param: source_path : path the the source file
# 2nd param: group : name of the config file section, e.g. mysqld
# 3rd param: var : name of the variable in the section, e.g. server-id
# 4th param: - : default value for the param
#
parse_sst_info()
{
    local source_path=$1
    local group=$2
    local var=$3
    local reval=""

    # print the default settings for given group using my_print_default.
    # normalize the variable names specified in cnf file (user can use _ or -
    # for example log-bin or log_bin) then grep for needed variable
    # finally get the variable value (if variables has been specified
    # multiple time use the last value only)

    reval=$($MY_PRINT_DEFAULTS -c "$source_path" $group | awk -F= '{if ($1 ~ /_/) { gsub(/_/,"-",$1); print $1"="$2 } else { print $0 }}' | grep -- "--$var=" | cut -d= -f2- | tail -1)

    # use default if we haven't found a value
    if [[ -z $reval ]]; then
        [[ -n $4 ]] && reval=$4
    fi

    echo $reval
}

#-------------------------------------------------------------------------------
#
# Step-4: Main workload logic starts here.
#

#
# Validation check for xtrabackup binary.
if [[ ! -x `which $INNOBACKUPEX_BIN` ]]; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "$INNOBACKUPEX_BIN not in path: $PATH"
    wsrep_log_error "****************************************************** "
    exit 2
fi

# XB_REQUIRED_VERSION requires at least major.minor version (e.g. 2.4.1 or 3.0)
#
# 2.4.3 : Starting with 5.7, the redo log format has changed and so XB-2.4.3 or higher is needed
# for performing backup (read SST)
#
# 2.4.4 : needed to support the keyring option
#
# 2.4.11: XB now has its own keyring plugin complied and added support for vault plugin
#         in addition to existing keyring_file plugin.
#
# 2.4.12: XB fixed bugs like keyring is empty + move-back stage now uses params from
#         my.cnf.

XB_REQUIRED_VERSION="2.4.12"

XB_VERSION=`$INNOBACKUPEX_BIN --version 2>&1 | grep -oe '[0-9]\.[0-9][\.0-9]*' | head -n1`
if [[ -z $XB_VERSION ]]; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "Cannot determine the $INNOBACKUPEX_BIN version. Needs xtrabackup-$XB_REQUIRED_VERSION or higher to perform SST"
    wsrep_log_error "****************************************************** "
    exit 2
fi

if ! check_for_version $XB_VERSION $XB_REQUIRED_VERSION; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "The $INNOBACKUPEX_BIN version is $XB_VERSION. Needs xtrabackup-$XB_REQUIRED_VERSION or higher to perform SST"
    wsrep_log_error "****************************************************** "
    exit 2
fi

wsrep_log_debug "The $INNOBACKUPEX_BIN version is $XB_VERSION"

# Get our MySQL version
MYSQL_VERSION=$WSREP_SST_OPT_VERSION
if [[ -z $MYSQL_VERSION ]]; then
    wsrep_log_error "FATAL: Cannot determine the mysqld server version"
    exit 2
fi

rm -f "${XB_GTID_INFO_FILE_PATH}"

#
# establish roles. Either it should be JOINER or DONOR.
if [[ ! ${WSREP_SST_OPT_ROLE} == 'joiner' && ! ${WSREP_SST_OPT_ROLE} == 'donor' ]]; then
    wsrep_log_error "******************* FATAL ERROR ********************** "
    wsrep_log_error "Invalid role ${WSREP_SST_OPT_ROLE}"
    wsrep_log_error "****************************************************** "
    exit 22
fi

#
# read configuration and setup ports for streaming data.
read_cnf
setup_ports

#
# If pxc_encrypt_cluster_traffic is set, do the SSL autoconfig
# (overriding any values already in the config file)
if [[ "$pxc_encrypt_cluster_traffic" == "on" ]]; then
    encrypt=4

    # Look for the [mysqld] files only, not the ones in [sst]
    ssl_ca=$(parse_cnf mysqld ssl-ca "")
    ssl_cert=$(parse_cnf mysqld ssl-cert "")
    ssl_key=$(parse_cnf mysqld ssl-key "")

    # Check that we have all the files
    # If they have not been explicitly specified, check the datadir
    if [[ -z "$ssl_ca" ]]; then
        if [[ -r "$DATA/ca.pem" ]]; then
            ssl_ca="$DATA/ca.pem"
        else
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "* Could not find a CA (Certificate Authority) file.  "
            wsrep_log_error "* Please specify a CA file with the 'ssl-ca' option. "
            wsrep_log_error "**************************************************** "
            return 2
        fi
    fi
    if [[ -z "$ssl_cert" ]]; then
        if [[ -r "$DATA/server-cert.pem" ]]; then
            ssl_cert="$DATA/server-cert.pem"
        else
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "* Could not find a certificate file.                            "
            wsrep_log_error "* Please specify a certificate file with the 'ssl-cert' option. "
            wsrep_log_error "****************************************************** "
            return 2
        fi
    fi
    if [[ -z "$ssl_key" ]]; then
        if [[ -r "$DATA/server-key.pem" ]]; then
            ssl_key="$DATA/server-key.pem"
        else
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "* Could not find a key file.                           "
            wsrep_log_error "* Please specify a key file with the 'ssl-key' option. "
            wsrep_log_error "****************************************************** "
            return 2
        fi
    fi

    wsrep_log_debug "pxc_encrypt_cluster_traffic is enabled, using PXC auto-ssl configuration"
    wsrep_log_debug "with encrypt=4  ssl_ca=$ssl_ca  ssl_cert=$ssl_cert  ssl_key=$ssl_key"
fi

if ${INNOBACKUPEX_BIN} /tmp --help 2>/dev/null | grep -q -- '--version-check'; then
    disver="--no-version-check"
fi

if [[ ${FORCE_FTWRL:-0} -eq 1 ]]; then
    wsrep_log_warning "Forcing FTWRL due to environment variable FORCE_FTWRL equal to $FORCE_FTWRL"
    iopts+=" --no-backup-locks "
fi

INNOEXTRA=""

#
# Use syslogger (for logging) if configured else continue
# to use file-based logging.
if [[ $ssyslog -eq 1 ]]; then

    if [[ ! -x `which logger` ]]; then
        wsrep_log_error "logger not in path: $PATH. Ignoring"
    else

        wsrep_log_info "Logging all stderr of SST/XtraBackup to syslog"

        exec 2> >(logger -p daemon.err -t ${ssystag}wsrep-sst-$WSREP_SST_OPT_ROLE)

        wsrep_log_error()
        {
            logger -p daemon.err -t ${ssystag}wsrep-sst-$WSREP_SST_OPT_ROLE "$@"
        }

        wsrep_log_warning()
        {
            logger -p daemon.warn -t ${ssystag}wsrep-sst-$WSREP_SST_OPT_ROLE "$@"
        }

        wsrep_log_info()
        {
            logger -p daemon.info -t ${ssystag}wsrep-sst-$WSREP_SST_OPT_ROLE "$@"
        }

        wsrep_log_debug()
        {
            if [[ -n "$WSREP_LOG_DEBUG" ]]; then
                logger -p daemon.debug -t ${ssystag}wsrep-sst-$WSREP_SST_OPT_ROLE "$@"
            fi
        }

        # prepare doesn't look at my.cnf instead it has its own generated backup-my.cnf
        INNOPREPARE="${INNOBACKUPEX_BIN} $disver $iapts --prepare --binlog-info=ON \$rebuildcmd \$keyringapplyopt \$encrypt_prepare_options --target-dir=\${DATA} 2>&1  | logger -p daemon.err -t ${ssystag}innobackupex-apply "
        INNOMOVE="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF} --datadir=\${TDATA} --defaults-group=mysqld${WSREP_SST_OPT_CONF_SUFFIX} $disver $impts  --move-back --binlog-info=ON --force-non-empty-directories \$encrypt_move_options --target-dir=\${DATA} 2>&1 | logger -p daemon.err -t ${ssystag}innobackupex-move "
        INNOBACKUP="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF} --defaults-group=mysqld${WSREP_SST_OPT_CONF_SUFFIX} $disver $iopts \$ieopts \$INNOEXTRA \$keyringbackupopt --lock-ddl --backup --galera-info  --binlog-info=ON \$encrypt_backup_options --stream=\$sfmt --target-dir=\$itmpdir 2> >(logger -p daemon.err -t ${ssystag}innobackupex-backup)"
    fi
else
    # prepare doesn't look at my.cnf instead it has its own generated backup-my.cnf
    INNOPREPARE="${INNOBACKUPEX_BIN} $disver $iapts --prepare --binlog-info=ON \$rebuildcmd \$keyringapplyopt \$encrypt_prepare_options --target-dir=\${DATA} &>\${DATA}/innobackup.prepare.log"
    INNOMOVE="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF}  --defaults-group=mysqld${WSREP_SST_OPT_CONF_SUFFIX} --datadir=\${TDATA} $disver $impts  --move-back --binlog-info=ON --force-non-empty-directories \$encrypt_move_options --target-dir=\${DATA} &>\${DATA}/innobackup.move.log"
    INNOBACKUP="${INNOBACKUPEX_BIN} --defaults-file=${WSREP_SST_OPT_CONF}  --defaults-group=mysqld${WSREP_SST_OPT_CONF_SUFFIX} $disver $iopts \$ieopts \$INNOEXTRA \$keyringbackupopt --lock-ddl --backup --galera-info --binlog-info=ON \$encrypt_backup_options --stream=\$sfmt --target-dir=\$itmpdir 2>\${DATA}/innobackup.backup.log"
fi

#
# Setup stream for transfering and streaming.
get_stream
get_transfer

if [ "$WSREP_SST_OPT_ROLE" = "donor" ]
then
    #
    # signal handler for cleanup-based-exit.
    trap cleanup_donor EXIT

    initialize_tmpdir

    # main temp directory for SST (non-XB) related files
    donor_tmpdir=$(mktemp --tmpdir="${tmpdirbase}" --directory donor_tmp_XXXX)

    # raise error if keyring_plugin is enabled but transit encryption is not
    if [[ $keyring_plugin -eq 1 && $encrypt -le 0 ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "FATAL: keyring plugin is enabled but transit channel" \
                        "is unencrypted. Enable encryption for SST traffic"
        wsrep_log_error "****************************************************** "
        exit 22
    fi

    # Create the SST info file
    # This file contains SST information that is passed from the
    # donor to the joiner.
    #
    # Add more parameters to the file here as needed
    # This file has the same format as a cnf file.
    #
    sst_info_file_path="${donor_tmpdir}/${SST_INFO_FILE}"
    transition_key=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
    echo "[sst]" > "$sst_info_file_path"
    echo "galera-gtid=$WSREP_SST_OPT_GTID" >> "$sst_info_file_path"
    echo "binlog-name=$(basename "$WSREP_SST_OPT_BINLOG")" >> "$sst_info_file_path"
    echo "mysql-version=$MYSQL_VERSION" >> "$sst_info_file_path"
    # append transition_key only if keyring is being used.
    if [[ $keyring_plugin -eq 1 ]]; then
        echo "transition-key=$transition_key" >> "$sst_info_file_path"
        encrypt_backup_options="--transition-key=\$transition_key"
    fi

    #
    # SST is not needed. IST would suffice. By-pass SST.
    if [ $WSREP_SST_OPT_BYPASS -eq 0 ]
    then
        usrst=0
        if [[ -z $sst_ver ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Upgrade joiner to 5.6.21 or higher for backup locks support"
            wsrep_log_error "The joiner is not supported for this version of donor"
            wsrep_log_error "****************************************************** "
            exit 93
        fi

        # main temp directory for xtrabackup (target-dir)
        itmpdir=$(mktemp --tmpdir="${tmpdirbase}" --directory donor_xb_XXXX)

        if [[ -n "${WSREP_SST_OPT_USER:-}" && "$WSREP_SST_OPT_USER" != "(null)" ]]; then
           INNOEXTRA+=" --user=$WSREP_SST_OPT_USER"
           usrst=1
        fi

        if [ -n "${WSREP_SST_OPT_PSWD:-}" ]; then
           INNOEXTRA+=" --password=$WSREP_SST_OPT_PSWD"
        elif [[ $usrst -eq 1 ]]; then
           # Empty password, used for testing, debugging etc.
           INNOEXTRA+=" --password="
        fi

        get_keys
        check_extra

        ttcmd="$tcmd"

        # Add compression to the head of the stream (if specified)
        if [[ -n "$scomp" ]]; then
            tcmd=" $scomp | $tcmd "
        fi
        # Add encryption to the head of the stream (if specified)
        if [[ $encrypt -eq 1 && -n "$ecmd_other" ]]; then
            tcmd=" \$ecmd_other | $tcmd "
        fi
        if [[ $encrypt -eq 1 ]]; then
            xbstreameopts=$xbstreameopts_other
        fi

        # Before the real SST,send the sst-info
        wsrep_log_debug "Streaming SST meta-info file before SST"

        FILE_TO_STREAM=$SST_INFO_FILE
        send_data_from_donor_to_joiner "$donor_tmpdir" "${stagemsg}-sst-info"

        # Restore the transport commmand to its original state
        tcmd="$ttcmd"
        if [[ -n "$progress" ]];then
            get_footprint
        elif [[ -n "$rlimit" ]];then
            adjust_progress
        fi
        if [[ -n "$pcmd" ]]; then
            tcmd="$pcmd | $tcmd"
        fi

        wsrep_log_debug "Sleeping before data transfer for SST"
        sleep 10

        wsrep_log_info "Streaming the backup to joiner at ${REMOTEIP} ${SST_PORT:-4444}"

        # Add compression to the head of the stream (if specified)
        if [[ -n $scomp ]]; then
            tcmd="$scomp | $tcmd"
        fi
        # Add encryption to the head of the stream (if specified)
        if [[ $encrypt -eq 1 && -n "$ecmd" ]]; then
            tcmd=" \$ecmd | $tcmd "
        fi
        if [[ $encrypt -eq 1 ]]; then
            xbstreameopts=$xbstreameopts_sst
        fi

        set +e
        timeit "${stagemsg}-SST" "$INNOBACKUP | $tcmd; RC=( "\${PIPESTATUS[@]}" )"
        set -e

        if [ ${RC[0]} -ne 0 ]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "${INNOBACKUPEX_BIN} finished with error: ${RC[0]}. " \
                            "Check ${DATA}/innobackup.backup.log"
            echo "--------------- innobackup.backup.log (START) --------------------" >&2
            cat ${DATA}/innobackup.backup.log >&2
            echo "--------------- innobackup.backup.log (END) ----------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 22
        elif [[ ${RC[$(( ${#RC[@]}-1 ))]} -eq 1 ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "$tcmd finished with error: ${RC[1]}"
            wsrep_log_error "****************************************************** "
            exit 22
        fi

    else # BYPASS FOR IST

        wsrep_log_info "Bypassing SST. Can work it through IST"
        echo "continue" # now server can resume updating data
        echo "1" > "${donor_tmpdir}/${IST_FILE}"
        get_keys
        # Add compression to the head of the stream (if specified)
        if [[ -n "$scomp" ]]; then
            tcmd=" $scomp | $tcmd "
        fi
        # Add encryption to the head of the stream (if specified)
        if [[ $encrypt -eq 1 && -n "$ecmd_other" ]]; then
            tcmd=" \$ecmd_other | $tcmd "
        fi
        if [[ $encrypt -eq 1 ]]; then
            xbstreameopts=$xbstreameopts_other
        fi

        strmcmd+=" \${IST_FILE}"

        FILE_TO_STREAM=$SST_INFO_FILE
        send_data_from_donor_to_joiner "$donor_tmpdir" "${stagemsg}-IST"
    fi

    echo "done ${WSREP_SST_OPT_GTID}"
    wsrep_log_debug "Total time on donor: $totime seconds"

elif [ "${WSREP_SST_OPT_ROLE}" = "joiner" ]
then

    [[ -e $SST_PROGRESS_FILE ]] && wsrep_log_warning "Found a stale sst_in_progress file: $SST_PROGRESS_FILE"
    [[ -n $SST_PROGRESS_FILE ]] && touch $SST_PROGRESS_FILE

    ib_home_dir=$(parse_cnf mysqld innodb-data-home-dir "")
    ib_log_dir=$(parse_cnf mysqld innodb-log-group-home-dir "")
    ib_undo_dir=$(parse_cnf mysqld innodb-undo-directory "")

    stagemsg="Joiner-Recv"

    nthreads=1

    MODULE="xtrabackup_sst"

    rm -f "${DATA}/${IST_FILE}"

    # May need xtrabackup_checkpoints later on
    rm -f ${DATA}/xtrabackup_binary ${DATA}/xtrabackup_galera_info  ${DATA}/xtrabackup_logfile
    if [[ -n $KEYRING_FILE_DIR ]]; then
        rm -f "${KEYRING_FILE_DIR}/${XB_DONOR_KEYRING_FILE}"
    fi

    # Note: this is started as a background process
    # So it has to wait for processes that are started by THIS process
    wait_for_listen $$ ${WSREP_SST_OPT_HOST} ${WSREP_SST_OPT_PORT:-4444} ${MODULE} &

    trap sig_joiner_cleanup HUP PIPE INT TERM
    trap cleanup_joiner EXIT

    if [[ -n $progress ]]; then
        adjust_progress
        tcmd+=" | $pcmd"
    fi

    get_keys
    if [[ $encrypt -eq 1 && -n "$ecmd_other" ]]; then
        strmcmd=" \$ecmd_other | $strmcmd"
    fi
    if [[ -n $sdecomp ]]; then
        strmcmd=" $sdecomp | $strmcmd"
    fi
    if [[ $encrypt -eq 1 ]]; then
        xbstreameopts=$xbstreameopts_other
    fi

    initialize_tmpdir
    STATDIR=$(mktemp --tmpdir="${tmpdirbase}" --directory joiner_XXXX)

    sst_file_info_path="${STATDIR}/${SST_INFO_FILE}"

    recv_data_from_donor_to_joiner $STATDIR "${stagemsg}-sst-info" $stimeout -2

    #
    # Determine which file was received, the GTID or the SST_INFO
    #
    if [[ -r "${STATDIR}/${SST_INFO_FILE}" ]]; then
        #
        # Extract information from the sst-info file that was just received
        #
        XB_GTID_INFO_FILE_PATH="${STATDIR}/${XB_GTID_INFO_FILE}"
        parse_sst_info "$sst_file_info_path" sst galera-gtid "" > "$XB_GTID_INFO_FILE_PATH"

        DONOR_BINLOGNAME=$(parse_sst_info "$sst_file_info_path" sst binlog-name "")
        DONOR_MYSQL_VERSION=$(parse_sst_info "$sst_file_info_path" sst mysql-version "")

        transition_key=$(parse_sst_info "$sst_file_info_path" sst transition-key "")
        if [[ -n $transition_key ]]; then
            encrypt_prepare_options="--transition-key=\$transition_key"
            encrypt_move_options="--transition-key=\$transition_key --generate-new-master-key"
        fi
    elif [[ -r "${STATDIR}/${XB_GTID_INFO_FILE}" ]]; then
        #
        # For compatibility, we have received the gtid file
        #
        XB_GTID_INFO_FILE_PATH="${STATDIR}/${XB_GTID_INFO_FILE}"

        DONOR_BINLOGNAME=""
        DONOR_MYSQL_VERSION=""

    else
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "Did not receive expected file from donor: '${SST_INFO_FILE}' or '${XB_GTID_INFO_FILE}'"
        wsrep_log_error "****************************************************** "
        exit 32
    fi

    if [ ! -r "${STATDIR}/${IST_FILE}" ]
    then
        if [[ -n "$DONOR_MYSQL_VERSION" ]]; then
            local_version_str=""
            donor_version_str=""

            # Truncate the version numbers (we want the major.minor version like "5.6", not "5.6.35-...")
            local_version_str=$(expr match "$MYSQL_VERSION" '\([0-9]\+\.[0-9]\+\)')
            donor_version_str=$(expr match "$DONOR_MYSQL_VERSION" '\([0-9]\+\.[0-9]\+\)')

            # Is this node's pxc version < donor's pxc version?
            if ! check_for_version $local_version_str $donor_version_str; then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "FATAL: PXC is receiving an SST from a node with a higher version."
                wsrep_log_error "This node's PXC version is $local_version_str.  The donor's PXC version is $donor_version_str."
                wsrep_log_error "Upgrade this node before joining the cluster."
                wsrep_log_error "****************************************************** "
                exit 2
            fi

            # Is the donor's pxc version < this node's pxc version?
            if ! check_for_version $donor_version_str $local_version_str; then
                wsrep_log_warning "WARNING: PXC is receiving an SST from a node with a lower version."
                wsrep_log_warning "This node's PXC version is $local_version_str. The donor's PXC version is $donor_version_str."
                wsrep_log_warning "Run mysql_upgrade in non-cluster (standalone mode) to upgrade."
                wsrep_log_warning "Check the upgrade process here:"
                wsrep_log_warning "    https://www.percona.com/doc/percona-xtradb-cluster/LATEST/howtos/upgrade_guide.html"
            fi
        fi

        # server-id is already part of backup-my.cnf so avoid appending it.
        # server-id is the id of the node that is acting as donor and not joiner node.

        # if keyring_plugin is enabled on JOINER and DONOR failed to send transition_key
        # this means either of the 2 things:
        # a. DONOR is at version < 5.7.22
        # b. DONOR is not configured to use keyring_plugin

        if [[ $keyring_plugin -eq 1 && -z $transition_key ]]; then

            # case-a: DONOR is at version < 5.7.22. We should expect keyring file
            #         and not transition_key.
            if ! check_for_version $DONOR_MYSQL_VERSION "5.7.22"; then

                 if [[ -n $keyring_file_data ]]; then
                     # joiner needs to wait to receive the file.
                     sleep 3

                     # Ensure that the destination directory exists and is R/W by us
                     if [[ ! -d "$KEYRING_FILE_DIR" || ! -r "$KEYRING_FILE_DIR" || ! -w "$KEYRING_FILE_DIR" ]]; then
                         wsrep_log_error "******************* FATAL ERROR ********************** "
                         wsrep_log_error "FATAL: Cannot acccess the keyring directory"
                         wsrep_log_error "  $KEYRING_FILE_DIR"
                         wsrep_log_error "It must already exist and be readable/writeable by MySQL"
                         wsrep_log_error "****************************************************** "
                         exit 22
                     fi

                     XB_DONOR_KEYRING_FILE_PATH="${KEYRING_FILE_DIR}/${XB_DONOR_KEYRING_FILE}"
                     recv_data_from_donor_to_joiner "${KEYRING_FILE_DIR}" "${stagemsg}-keyring" 0 2
                     keyringapplyopt=" --keyring-file-data=$XB_DONOR_KEYRING_FILE_PATH "
                     wsrep_log_info "donor keyring received at: '${XB_DONOR_KEYRING_FILE_PATH}'"
                 else
                     # There shouldn't be a keyring file, since we do not have a keyring here
                     # This will error out if a keyring file IS found
                     XB_DONOR_KEYRING_FILE_PATH="${KEYRING_FILE_DIR}/${XB_DONOR_KEYRING_FILE}"
                     recv_data_from_donor_to_joiner "${KEYRING_FILE_DIR}" "${stagemsg}-keyring" 5 -1

                     if [[ -r "${XB_DONOR_KEYRING_FILE_PATH}" ]]; then
                         wsrep_log_error "******************* FATAL ERROR ********************** "
                         wsrep_log_error "FATAL: xtrabackup found '${XB_DONOR_KEYRING_FILE_PATH}'"
                         wsrep_log_error "The joiner is not using a keyring file but the donor has sent"
                         wsrep_log_error "a keyring file.  Please check your configuration to ensure that"
                         wsrep_log_error "both sides are using a keyring file"
                         wsrep_log_error "****************************************************** "
                         exit 32
                     fi
                 fi
            # case-b: JOINER is configured to use keyring but DONOR is not
            else
                 wsrep_log_error "******************* FATAL ERROR ********************** "
                 wsrep_log_error "FATAL: JOINER is configured to use keyring_plugin" \
                                 "(file/vault) but DONOR is not"
                 wsrep_log_error "****************************************************** "
                 exit 32
            fi
        fi

        if [[ $keyring_plugin -eq 0 && -n $transition_key ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "FATAL: DONOR is configured to use keyring_plugin" \
                            "(file/vault) but JOINER is not"
            wsrep_log_error "****************************************************** "
            exit 32
        fi

        if ! ps -p ${WSREP_SST_OPT_PARENT} &>/dev/null
        then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Parent mysqld process (PID:${WSREP_SST_OPT_PARENT}) terminated unexpectedly."
            wsrep_log_error "****************************************************** "
            exit 32
        fi

        get_stream
        if [[ $encrypt -eq 1 && -n "$ecmd" ]]; then
            strmcmd=" \$ecmd | $strmcmd"
        fi
        if [[ -n $sdecomp ]]; then
            strmcmd=" $sdecomp | $strmcmd"
        fi
        if [[ $encrypt -eq 1 ]]; then
            xbstreameopts=$xbstreameopts_sst
        fi

        # For compatibility, if the tmpdir is not specified, then use
        # the datadir to hold the .sst directory
        if [[ -z "$(parse_cnf sst tmpdir "")" ]]; then
            if [[ -d ${DATA}/.sst ]]; then
                wsrep_log_info "WARNING: Stale temporary SST directory: ${DATA}/.sst from previous state transfer. Removing"
                rm -rf ${DATA}/.sst
            fi
            mkdir -p ${DATA}/.sst
            JOINER_SST_DIR=$DATA/.sst
        else
            JOINER_SST_DIR=$(mktemp -p "${tmpdirbase}" -dt sst_XXXX)
        fi

        (recv_data_from_donor_to_joiner "$JOINER_SST_DIR" "${stagemsg}-SST" 0 0) &
        jpid=$!
        wsrep_log_info "Proceeding with SST........."

        wsrep_log_debug "Cleaning the existing datadir and innodb-data/log directories"
        # Avoid emitting the find command output to log file. It just fill the
        # with ever increasing number of files and achieve nothing.
        find $ib_home_dir $ib_log_dir $ib_undo_dir $DATA -mindepth 1  -regex $cpat  -prune  -o -exec rm -rfv {} 1>/dev/null \+

        # Clean the binlog dir (if it's explicitly specified)
        # By default it'll be in the datadir
        tempdir=$(parse_cnf mysqld log-bin "")
        if [[ -n "$tempdir" ]]; then
            binlog_dir=$(dirname "$tempdir")
            binlog_file=$(basename "$tempdir")
            if [[ -n ${binlog_dir:-} && "$binlog_dir" != '.' && "$binlog_dir" != "$DATA" ]];then
                pattern="$binlog_dir/$binlog_file\.[0-9]+$"
                wsrep_log_debug "Cleaning the binlog directory $binlog_dir as well"
                find "$binlog_dir" -maxdepth 1 -type f -regex $pattern -exec rm -fv {} 1>&2 \+ || true
                rm -f $binlog_dir/*.index || true
            fi
        fi

        TDATA=$DATA
        DATA=$JOINER_SST_DIR

        XB_GTID_INFO_FILE_PATH="${DATA}/${XB_GTID_INFO_FILE}"
        wsrep_log_info "............Waiting for SST streaming to complete!"
        wait $jpid

        get_proc

        if [[ ! -s ${DATA}/xtrabackup_checkpoints ]]; then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "xtrabackup_checkpoints missing. xtrabackup/SST failed on DONOR. Check DONOR log"
            wsrep_log_error "****************************************************** "
            exit 2
        fi

        # Rebuild indexes for compact backups
        if grep -q 'compact = 1' ${DATA}/xtrabackup_checkpoints; then
            wsrep_log_info "Index compaction detected"
            rebuild=1
        fi

        if [[ $rebuild -eq 1 ]]; then
            nthreads=$(parse_cnf xtrabackup rebuild-threads $nproc)
            wsrep_log_debug "Rebuilding during prepare with $nthreads threads"
            rebuildcmd="--rebuild-indexes --rebuild-threads=$nthreads"
        fi

        if test -n "$(find ${DATA} -maxdepth 1 -type f -name '*.qp' -print -quit)"; then

            wsrep_log_info "Compressed qpress files found"

            if [[ ! -x `which qpress` ]]; then
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "qpress not found in path: $PATH"
                wsrep_log_error "****************************************************** "
                exit 22
            fi

            if [[ -n $progress ]] && pv --help | grep -q 'line-mode'; then
                count=$(find "${DATA}" -type f -name '*.qp' | wc -l)
                count=$(( count*2 ))
                if pv --help | grep -q FORMAT; then
                    pvopts="-f -s $count -l -N Decompression -F '%N => Rate:%r Elapsed:%t %e Progress: [%b/$count]'"
                else
                    pvopts="-f -s $count -l -N Decompression"
                fi

                pcmd="pv $pvopts"
                adjust_progress
                dcmd="$pcmd | xargs -n 2 qpress -T${nproc}d"
            else
                dcmd="xargs -n 2 qpress -T${nproc}d"
            fi


            # Decompress the qpress files
            wsrep_log_debug "Decompression with $nproc threads"
            timeit "Joiner-Decompression" "find ${DATA} -type f -name '*.qp' -printf '%p\n%h\n' | $dcmd"
            extcode=$?

            if [[ $extcode -eq 0 ]]; then
                wsrep_log_debug "Removing qpress files after decompression"
                find "${DATA}" -type f -name '*.qp' -delete
                if [[ $? -ne 0 ]]; then
                    wsrep_log_error "******************* FATAL ERROR ********************** "
                    wsrep_log_error "Something went wrong with deletion of qpress files. Investigate"
                    wsrep_log_error "****************************************************** "
                fi
            else
                wsrep_log_error "******************* FATAL ERROR ********************** "
                wsrep_log_error "Decompression failed. Exit code: $extcode"
                wsrep_log_error "****************************************************** "
                exit 22
            fi
        fi

        if  [[ -n "$WSREP_SST_OPT_BINLOG" && -n "$DONOR_BINLOGNAME" ]]; then

            binlog_dir=$(dirname "$WSREP_SST_OPT_BINLOG")
            binlog_file=$(basename "$WSREP_SST_OPT_BINLOG")
            donor_binlog_file=$DONOR_BINLOGNAME

            # rename the donor binlog to the name of the binlogs on the joiner
            if [[ "$binlog_file" != "$donor_binlog_file" ]]; then
                pushd "$DATA" &>/dev/null
                for f in $donor_binlog_file.*; do
                    if [[ ! -e "$f" ]]; then continue; fi
                    f_new=$(echo $f | sed "s/$donor_binlog_file/$binlog_file/")
                    mv "$f" "$f_new" 2>/dev/null || true
                done
                popd &> /dev/null
            fi

            # To avoid comparing data directory and BINLOG_DIRNAME 
            mv $DATA/${binlog_file}.* "$binlog_dir"/ 2>/dev/null || true

            pushd "$binlog_dir" &>/dev/null
            for bfiles in $binlog_file.*; do
                if [[ ! -e "$bfiles" ]]; then continue; fi
                echo "${binlog_dir}/${bfiles}" >> "${binlog_file}.index"
            done
            popd &> /dev/null

        fi

        wsrep_log_info "Preparing the backup at ${DATA}"
        timeit "Xtrabackup prepare stage" "$INNOPREPARE"

        if [ $? -ne 0 ];
        then
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "${INNOBACKUPEX_BIN} apply finished with errors." \
                            "Check ${DATA}/innobackup.prepare.log"
            echo "--------------- innobackup.prepare.log (START) --------------------" >&2
            cat "${DATA}/innobackup.prepare.log" >&2
            echo "--------------- innobackup.prepare.log (END) --------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 22
        fi

        XB_GTID_INFO_FILE_PATH="${TDATA}/${XB_GTID_INFO_FILE}"
        set +e
        rm -f $TDATA/innobackup.prepare.log $TDATA/innobackup.move.log
        set -e
        wsrep_log_info "Moving the backup to ${TDATA}"
        timeit "Xtrabackup move stage" "$INNOMOVE"
        if [[ $? -eq 0 ]]; then

            # Did we receive a keyring file?
            if [[ -r "${XB_DONOR_KEYRING_FILE_PATH}" ]]; then
                wsrep_log_info "Moving sst keyring into place: moving $XB_DONOR_KEYRING_FILE_PATH to $keyring_file_data"
                mv "${XB_DONOR_KEYRING_FILE_PATH}" "$keyring_file_data"
                wsrep_log_debug "Keyring move successful"
            fi

            wsrep_log_debug "Move successful, removing ${DATA}"
            rm -rf "$DATA"
            DATA=${TDATA}
        else
            if [[ -r "${XB_DONOR_KEYRING_FILE_PATH}" ]]; then
                rm -f "${XB_DONOR_KEYRING_FILE_PATH}"
            fi
            wsrep_log_error "******************* FATAL ERROR ********************** "
            wsrep_log_error "Move failed, keeping ${DATA} for further diagnosis" \
                            "Check ${DATA}/innobackup.move.log for details"
            echo "--------------- innobackup.move.log (START) --------------------" >&2
            cat "${DATA}/innobackup.move.log" >&2
            echo "--------------- innobackup.move.log (END) --------------------" >&2
            wsrep_log_error "****************************************************** "
            exit 22
        fi

    else
        wsrep_log_info "${IST_FILE} received from donor: Running IST"
    fi

    if [[ ! -r ${XB_GTID_INFO_FILE_PATH} ]]; then
        wsrep_log_error "******************* FATAL ERROR ********************** "
        wsrep_log_error "SST magic file ${XB_GTID_INFO_FILE_PATH} not found/readable"
        wsrep_log_error "****************************************************** "
        exit 2
    fi
    wsrep_log_info "Galera co-ords from recovery: $(cat "${XB_GTID_INFO_FILE_PATH}")"
    cat "${XB_GTID_INFO_FILE_PATH}" # output UUID:seqno
    wsrep_log_debug "Total time on joiner: $totime seconds"
fi

exit 0
