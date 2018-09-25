#!/bin/sh
# Copyright Abandoned 1996 TCX DataKonsult AB & Monty Program KB & Detron HB
# This file is public domain and comes with NO WARRANTY of any kind

# MySQL (Percona XtraDB Cluster) daemon start/stop script.

# Usually this is put in /etc/init.d (at least on machines SYSV R4 based
# systems) and linked to /etc/rc3.d/S99mysql and /etc/rc0.d/K01mysql.
# When this is done the mysql server will be started when the machine is
# started and shut down when the systems goes down.

# Comments to support chkconfig on RedHat Linux
# chkconfig: 2345 64 36
# description: A very fast and reliable SQL database engine.

# Comments to support LSB init script conventions
### BEGIN INIT INFO
# Provides: mysql
# Required-Start: $local_fs $network $remote_fs
# Should-Start: ypbind nscd ldap ntpd xntpd
# Required-Stop: $local_fs $network $remote_fs
# Default-Start:  2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: start and stop MySQL (Percona XtraDB Cluster)
# Description: Percona-Server is a SQL database engine with focus on high performance.
### END INIT INFO

# Source function library.
. /etc/rc.d/init.d/functions
 
# Prevent OpenSUSE's init scripts from calling systemd, so that
# both 'bootstrap' and 'start' are handled entirely within this script

SYSTEMD_NO_WRAP=1

# Prevent Debian's init scripts from calling systemctl

SYSTEMCTL_SKIP_REDIRECT=true
_SYSTEMCTL_SKIP_REDIRECT=true
 
# If you install MySQL on some other places than @prefix@, then you
# have to do one of the following things for this script to work:
#
# - Run this script from within the MySQL installation directory
# - Create a /etc/my.cnf file with the following information:
#   [mysqld]
#   basedir=<path-to-mysql-installation-directory>
# - Add the above to any other configuration file (for example ~/.my.ini)
#   and copy my_print_defaults to /usr/bin
# - Add the path to the mysql-installation-directory to the basedir variable
#   below.
#
# If you want to affect other MySQL variables, you should make your changes
# in the /etc/my.cnf, ~/.my.cnf or other MySQL configuration files.

# If you change base dir, you must also change datadir. These may get
# overwritten by settings in the MySQL configuration files.

# source the defauts file /etc/sysconfig/mysql if it exists for any environment variables.
# set -a is done so that  'FOO=BAR' just be used than 'export FOO=BAR' in the /etc/sysconfig/mysql.
# However, export FOO=BAR in that file also should work.
set -a
[ -r /etc/sysconfig/mysql ] && . /etc/sysconfig/mysql
set +a

basedir=
datadir=

# Default value, in seconds, afterwhich the script should timeout waiting
# for server start. 
# Value here is overriden by value in my.cnf. 
# 0 means don't wait at all
# Negative numbers mean to wait indefinitely
service_startup_timeout=900
startup_sleep=1

# Lock directory for RedHat / SuSE.
lockdir='/var/lock/subsys'
lock_file_path="$lockdir/mysql"

# The following variables are only set for letting mysql.server find things.

# Set some defaults
mysqld_pid_file_path=
if test -z "$basedir"
then
  basedir=@prefix@
  bindir=@bindir@
  if test -z "$datadir"
  then
    datadir=@localstatedir@
  fi
  sbindir=@sbindir@
  libexecdir=@libexecdir@
else
  bindir="$basedir/bin"
  if test -z "$datadir"
  then
    datadir="$basedir/data"
  fi
  sbindir="$basedir/sbin"
  libexecdir="$basedir/libexec"
fi

# datadir_set is used to determine if datadir was set (and so should be
# *not* set inside of the --basedir= handler.)
datadir_set=

#
# Use LSB init script functions for printing messages, if possible
#
lsb_functions="/lib/lsb/init-functions"
if test -f $lsb_functions ; then
  . $lsb_functions
else
  log_success_msg()
  {
    echo " SUCCESS! $@"
  }
  log_failure_msg()
  {
    echo " ERROR! $@"
  }
fi

PATH="/sbin:/usr/sbin:/bin:/usr/bin:$basedir/bin"
export PATH

mode=$1    # start or stop

[ $# -ge 1 ] && shift


other_args="$*"   # uncommon, but needed when called from an RPM upgrade action
           # Expected: "--skip-networking --skip-grant-tables"
           # They are not checked here, intentionally, as it is the resposibility
           # of the "spec" file author to give correct arguments only.

case `echo "testing\c"`,`echo -n testing` in
    *c*,-n*) echo_n=   echo_c=     ;;
    *c*,*)   echo_n=-n echo_c=     ;;
    *)       echo_n=   echo_c='\c' ;;
esac

parse_server_arguments() {
  for arg do
    case "$arg" in
      --basedir=*)  basedir=`echo "$arg" | sed -e 's/^[^=]*=//'`
                    bindir="$basedir/bin"
                    if test -z "$datadir_set"; then
                      datadir="$basedir/data"
                    fi
                    sbindir="$basedir/sbin"
                    libexecdir="$basedir/libexec"
        ;;
      --datadir=*)  datadir=`echo "$arg" | sed -e 's/^[^=]*=//'`
                    datadir_set=1
        ;;
      --pid-file=*|--pid_file=*) mysqld_pid_file_path=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --service-startup-timeout=*|--service_startup_timeout=*) service_startup_timeout=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
    esac
  done
}

wait_for_pid () {
  verb="$1"           # created | removed
  pid="$2"            # process ID of the program operating on the pid-file
  pid_file_path="$3" # path to the PID file.

  sst_progress_file=$datadir/sst_in_progress
  i=0
  avoid_race_condition="by checking again"

  while test $i -ne $service_startup_timeout ; do

    case "$verb" in
      'created')
        # wait for a PID-file to pop into existence.
        test -s "$pid_file_path" && i='' && break
        ;;
      'removed')
        # wait for this PID-file to disappear
        test ! -s "$pid_file_path" && i='' && break
        ;;
      *)
        echo "wait_for_pid () usage: wait_for_pid created|removed pid pid_file_path"
        exit 1
        ;;
    esac

    # if server isn't running, then pid-file will never be updated
    if test -n "$pid"; then
      if kill -0 "$pid" 2>/dev/null; then
        :  # the server still runs
      else
        # The server may have exited between the last pid-file check and now.  
        if test -n "$avoid_race_condition"; then
          avoid_race_condition=""
          continue  # Check again.
        fi

        # there's nothing that will affect the file.
        log_failure_msg "The server quit without updating PID file ($pid_file_path)."
        return 1  # not waiting any more.
      fi
    fi

    if (test -e $sst_progress_file || [ $mode = 'bootstrap-pxc' ]) && [ $startup_sleep -ne 10 ];then
        echo "State transfer in progress, setting sleep higher"
        startup_sleep=10
    fi

    echo $echo_n ".$echo_c"
    i=`expr $i + 1`
    sleep $startup_sleep

  done

  if test -z "$i" ; then
    log_success_msg
    return 0
  elif test -e $sst_progress_file; then 
    log_failure_msg
    return 2
  else
    log_failure_msg
    return 1
  fi
}

install_validate_password_sql_file () {
    local dir
    local initfile
    dir=/var/lib/mysql
    initfile="$(mktemp $dir/install-validate-password-plugin.XXXXXX.sql)"
    chown mysql:mysql "$initfile"
    echo "INSERT INTO mysql.plugin (name, dl) VALUES ('validate_password', 'validate_password.so');" > "$initfile"
    echo "SHUTDOWN;" >> "$initfile"
    echo "$initfile"
}

# Get arguments from the my.cnf file,
# the only group, which is read from now on is [mysqld]
if test -x ./bin/my_print_defaults
then
  print_defaults="./bin/my_print_defaults"
elif test -x $bindir/my_print_defaults
then
  print_defaults="$bindir/my_print_defaults"
elif test -x $bindir/mysql_print_defaults
then
  print_defaults="$bindir/mysql_print_defaults"
else
  # Try to find basedir in /etc/my.cnf
  conf=/etc/my.cnf
  print_defaults=
  if test -r $conf
  then
    subpat='^[^=]*basedir[^=]*=\(.*\)$'
    dirs=`sed -e "/$subpat/!d" -e 's//\1/' $conf`
    for d in $dirs
    do
	d=`echo $d | sed -e 's/[ 	]//g'`
      if test -x "$d/bin/my_print_defaults"
      then
        print_defaults="$d/bin/my_print_defaults"
        break
      fi
      if test -x "$d/bin/mysql_print_defaults"
      then
        print_defaults="$d/bin/mysql_print_defaults"
        break
      fi
    done
  fi

  # Hope it's in the PATH ... but I doubt it
  test -z "$print_defaults" && print_defaults="my_print_defaults"
fi

#
# Read defaults file from 'basedir'.   If there is no defaults file there
# check if it's in the old (depricated) place (datadir) and read it from there
#

extra_args=""
if test -r "$basedir/my.cnf"
then
  extra_args="-e $basedir/my.cnf"
fi

parse_server_arguments `$print_defaults $extra_args mysqld server mysql_server mysql.server`

#
# Set pid file if not given
#
if test -z "$mysqld_pid_file_path"
then
  mysqld_pid_file_path=$datadir/`@HOSTNAME@`.pid
else
  case "$mysqld_pid_file_path" in
    /* ) ;;
    * )  mysqld_pid_file_path="$datadir/$mysqld_pid_file_path" ;;
  esac
fi

check_running() {

    local show_msg=$1

    # First, check to see if pid file exists
    if test -s "$mysqld_pid_file_path" ; then 
        read mysqld_pid < "$mysqld_pid_file_path"
        if kill -0 $mysqld_pid 2>/dev/null ; then 
            log_success_msg "MySQL (Percona XtraDB Cluster) running ($mysqld_pid)"
            return 0
        else
            log_failure_msg "MySQL (Percona XtraDB Cluster) is not running, but PID file exists"
            return 1
        fi
    else
        # Try to find appropriate mysqld process
        mysqld_pid=`pidof $libexecdir/mysqld`

        # test if multiple pids exist
        pid_count=`echo $mysqld_pid | wc -w`
        if test $pid_count -gt 1 ; then
            log_failure_msg "Multiple MySQL running but PID file could not be found ($mysqld_pid)"
            return 5
        elif test -z $mysqld_pid ; then 
            if test -f "$lock_file_path" ; then 
                log_failure_msg "MySQL (Percona XtraDB Cluster) is not running, but lock file ($lock_file_path) exists"
                return 3
            fi 
            test $show_msg -eq 1 && log_failure_msg "MySQL (Percona XtraDB Cluster) is not running"
            return 3
        else
            log_failure_msg "MySQL (Percona XtraDB Cluster) is running but PID file could not be found"
            return 4
        fi
    fi
}

case "$mode" in
  'start')

    check_running 0
    ext_status=$?
    if test $ext_status -ne 3;then 
        exit $ext_status
    fi
    if [ ! -d "$datadir/mysql" ] ; then
        # First, make sure $datadir is there with correct permissions
        if [ ! -e "$datadir" -a ! -h "$datadir" ]
        then
          mkdir -p "$datadir" || exit 1
        fi
        if [ -f "$datadir/sst_in_progress" ]; then
            rm -rf $datadir/*
        fi
        chown mysql:mysql "$datadir"
        chmod 0751 "$datadir"
        if [ -x /sbin/restorecon ] ; then
          /sbin/restorecon "$datadir"
          for dir in /var/lib/mysql-files /var/lib/mysql-keyring ; do
            if [ -x /usr/sbin/semanage -a -d /var/lib/mysql -a -d $dir ] ; then
              /usr/sbin/semanage fcontext -a -e /var/lib/mysql $dir >/dev/null 2>&1
              /sbin/restorecon $dir
            fi
          done
        fi
        # Now create the database
        action $"Initializing MySQL database: " /usr/sbin/mysqld --initialize --datadir="$datadir" --user=mysql
        ret=$?
        [ $ret -ne 0 ] && exit $ret
        chown -R mysql:mysql "$datadir"
        # Generate certs if needed
        if [ -x /usr/bin/mysql_ssl_rsa_setup -a ! -e "${datadir}/server-key.pem" ] ; then
          /usr/bin/mysql_ssl_rsa_setup --datadir="$datadir" --uid=mysql >/dev/null 2>&1
        fi
    fi
    # Start daemon
    if test -e $datadir/sst_in_progress;then 
        echo "Stale sst_in_progress file in datadir"
    fi

    # Safeguard (relative paths, core dumps..)
    cd $basedir

    echo $echo_n "Starting MySQL (Percona XtraDB Cluster)"
    if test -x $bindir/mysqld_safe
    then
      # Give extra arguments to mysqld with the my.cnf file. This script
      # may be overwritten at next upgrade.
      $bindir/mysqld_safe --datadir="$datadir" --pid-file="$mysqld_pid_file_path" $other_args >/dev/null &
      wait_for_pid created "$!" "$mysqld_pid_file_path"; return_value=$?
      if [ $return_value == 1 ];then 
          log_failure_msg "MySQL (Percona XtraDB Cluster) server startup failed!"
      elif [ $return_value == 2 ];then
          log_failure_msg "MySQL (Percona XtraDB Cluster) server startup failed! State transfer still in progress"
      fi

      # Make lock for RedHat / SuSE
      if test -w "$lockdir"
      then
        touch "$lock_file_path"
      fi

      exit $return_value
    else
      log_failure_msg "Couldn't find MySQL server ($bindir/mysqld_safe)"
    fi
    ;;

  'stop')
    # Stop daemon. We use a signal here to avoid having to know the
    # root password.
    echo $echo_n "Shutting down MySQL (Percona XtraDB Cluster)"
    if test -s "$mysqld_pid_file_path"
    then
      # signal mysqld_safe that it needs to stop
      touch "$mysqld_pid_file_path.shutdown"

      mysqld_pid=`cat "$mysqld_pid_file_path"`

      if (kill -0 $mysqld_pid 2>/dev/null)
      then
        kill $mysqld_pid
        # mysqld should remove the pid file when it exits, so wait for it.
        wait_for_pid removed "$mysqld_pid" "$mysqld_pid_file_path"; return_value=$?
        if [ $return_value != 0 ];then 
            log_failure_msg "MySQL (Percona XtraDB Cluster) server stop failed!"
        fi
      else
        log_failure_msg "MySQL (Percona XtraDB Cluster) server process #$mysqld_pid is not running!"
        rm "$mysqld_pid_file_path"
      fi

      # To avoid the race condition involved in restarts with PID file.
      if (kill -0 $mysqld_pid 2>/dev/null)
      then
          sleep 2
      fi

      # Delete lock for RedHat / SuSE
      if test -f "$lock_file_path"
      then
        rm -f "$lock_file_path"
      fi
      exit $return_value
    else
      log_failure_msg "MySQL (Percona XtraDB Cluster) PID file could not be found!"
    fi
    ;;

  'restart')
    # Stop the service and regardless of whether it was
    # running or not, start it again.
    if $0 stop  $other_args; then
      if ! $0 start $other_args; then 
         log_failure_msg "Failed to restart server."
         exit 1
      fi
    else
      log_failure_msg "Failed to stop running server, so refusing to try to start."
      exit 1
    fi
    ;;

  'restart-bootstrap')
    # Stop the service and regardless of whether it was
    # running or not, start it again.
    if $0 stop  $other_args; then
      if ! $0 bootstrap-pxc $other_args;then
         log_failure_msg "Failed to restart server with bootstrap."
         exit 1
      fi
    else
      log_failure_msg "Failed to stop running server, so refusing to try to start."
      exit 1
    fi
    ;;

  'reload'|'force-reload')
    if test -s "$mysqld_pid_file_path" ; then
      read mysqld_pid <  "$mysqld_pid_file_path"
      kill -HUP $mysqld_pid && log_success_msg "Reloading service MySQL (Percona XtraDB Cluster)"
      touch "$mysqld_pid_file_path"
    else
      log_failure_msg "MySQL (Percona XtraDB Cluster) PID file could not be found!"
      exit 1
    fi
    ;;
  'status')
      check_running 1
      exit $?
    ;;
  'bootstrap-pxc')
      # Bootstrap the Percona XtraDB Cluster, start the first node
      # that initiate the cluster
      echo $echo_n "Bootstrapping PXC (Percona XtraDB Cluster)"
      $0 start $other_args --wsrep-new-cluster
      ;;
    'bootstrap')
      # Bootstrap the cluster, start the first node
      # that initiate the cluster
      echo $echo_n "Bootstrapping the cluster"
      $0 start $other_args --wsrep-new-cluster
      ;;
    *)
      # usage
      basename=`basename "$0"`
      echo "Usage: $basename {start|stop|restart|restart-bootstrap|reload|force-reload|status|bootstrap-pxc}  [ MySQL (Percona XtraDB Cluster) options ]"
      exit 1
    ;;
esac

exit 0
