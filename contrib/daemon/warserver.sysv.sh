#! /bin/sh
#-----------------------------------------------------------------------------------
# file: /etc/init.d/warserver
#   sysv initscript to start warserverd
#
# Thomas Drillich <th@drillich.com> 12/7/2007
#-----------------------------------------------------------------------------------
#set -x
PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/games/warserverd
NAME=warserver
DESC=warserver

test -x $DAEMON || exit 0

OPTIONS="-q"
[ -f /etc/default/$NAME ] && . /etc/default/$NAME
[ "x$ENABLED" = "xyes" ] || { echo "$NAME not enabled"; exit 0; }

# if subprocess should dump core on crash
[ "x$CORE" = "xyes" ] && ulimit -c unlimited 

export CIVSERVER CIVPUBLISH SYSLOG CRASHRELOAD CORE ENGLISH DEBUG METASERVER SENDHOST

. /lib/lsb/init-functions

case "$1" in  
  start)
	shift
	log_daemon_msg "Starting $DESC at"
	# selective start ?
	if [ $# -gt 0 ] 
	then start=''
                for k in $@
                do echo $PORTS | grep -q $k
		     [ $? -eq 0 ] && start="$start $k" || log_progress_msg "$k not in portlist"
		done
	else start=$PORTS
	fi
	for k in $start
	do log_progress_msg "$k"
		wpid=''
		[ -f "/var/run/$NAME.$k.pid" ] && wpid=`cat /var/run/$NAME.$k.pid`
		if [ "x$wpid" != "x" ]; then ## checking for double start
			ps ax | grep -v grep | grep $DAEMON | grep -q $wpid
			if [ $? -eq 0 ] ; then
			 log_progress_msg "already running"
			 continue
			fi
		fi
		/sbin/start-stop-daemon  --start --quiet --make-pidfile --pidfile "/var/run/$NAME.$k.pid" --background --chdir $RUNDIR --group $RUNGROUP --chuid $RUNAS --exec $DAEMON $k >/dev/null 2>&1
		[ $? -eq 0 ] && log_progress_msg ok || log_progress_msg fail
	done
        log_end_msg 0
	;;
  stop)
	log_daemon_msg "Stopping $DESC"
	shift; # remove stop option
	[ $# -gt 0 ] && stop=$@ || stop=$PORTS 
	for k in $stop
        do log_progress_msg "$k"
            wpid=`cat /var/run/$NAME.$k.pid`
            cpid=`ps axl | grep -v grep | grep $CIVSERVER | grep $wpid | awk '{print $3;}'`
	    /sbin/start-stop-daemon --stop --quiet --pidfile "/var/run/$NAME.$k.pid" >/dev/null 2>&1
	    [ $? ] && log_progress_msg ok || log_progress_msg fail
            if [ "x$cpid" != "x" ] 
	    then # kill the civserver at port $k
		log_progress_msg "client $cpid:"
		kill -TERM $cpid 
		[ $? ] && log_progress_msg ok || log_progress_msg fail
            fi
	done
	log_end_msg 0
	;;
  restart|force-reload)
	shift # remove this argument
	echo "Restart $DESC"
	$0 stop $@
	sleep 1
	$0 start $@
	;;
  status)
	for k in $PORTS
	do wpid=`cat /var/run/$NAME.$k.pid`
             cpid=`ps axl | grep -v grep | grep $CIVSERVER | grep $wpid | awk '{print $3;}'`
             echo -n "$NAME @$k: pid $wpid " 
             ps ax | grep -q "^$wpid"
	     if [ $? -eq 0 ] 
             then echo -n "running " 
                    ps ax | grep -q "^$cpid"
                    [ $? -eq 0 ] && echo "client pid $cpid ok" || echo "no client"
            else echo not running
            fi
	done
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
