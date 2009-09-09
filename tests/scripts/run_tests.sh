#!/bin/sh

# testsuite runner for tinyproxy
#
# Copyright (C) 2009 Michael Adam
#
# License: GNU GPL


SCRIPTS_DIR=$(pwd)/$(dirname $0)
BASEDIR=$SCRIPTS_DIR/../..
TESTS_DIR=$SCRIPTS_DIR/..
TESTENV_DIR=$TESTS_DIR/env

TINYPROXY_IP=127.0.0.2
TINYPROXY_PORT=12321
TINYPROXY_USER=$USER
TINYPROXY_PID_DIR=$TESTENV_DIR/var/run/tinyproxy
TINYPROXY_PID_FILE=$TINYPROXY_PID_DIR/tinyproxy.pid
LOG_DIR=$TESTENV_DIR/var/log
TINYPROXY_LOG_DIR=$LOG_DIR
TINYPROXY_DATA_DIR=$TESTENV_DIR/usr/share/tinyproxy
TINYPROXY_CONF_DIR=$TESTENV_DIR/etc
TINYPROXY_CONF_FILE=$TINYPROXY_CONF_DIR/tinyproxy.conf

WEBSERVER_IP=127.0.0.3
WEBSERVER_PORT=32123
WEBSERVER_PID_DIR=$TESTENV_DIR/var/run/webserver
WEBSERVER_PID_FILE=$WEBSERVER_PID_DIR/webserver.pid
WEBSERVER_LOG_DIR=$TESTENV_DIR/var/log/webserver

TINYPROXY_STDERR_LOG=$TINYPROXY_LOG_DIR/tinyproxy.stderr.log
WEBCLIENT_LOG=$LOG_DIR/webclient.log

WEBSERVER_BIN_FILE=webserver.pl
WEBSERVER_BIN=$SCRIPTS_DIR/$WEBSERVER_BIN_FILE
WEBCLIENT_BIN=$SCRIPTS_DIR/webclient.pl
TINYPROXY_BIN=$BASEDIR/src/tinyproxy

STAMP=$(date +%Y%m%d-%H:%M:%S)

if test -e $TESTENV_DIR ; then
	TESTENV_DIR_OLD=$TESTENV_DIR.old
	if test -e $TESTENV_DIR_OLD ; then
		rm -rf $TESTENV_DIR_OLD
	fi
	mv $TESTENV_DIR $TESTENV_DIR.old
fi

mkdir -p $TINYPROXY_DATA_DIR
cp $BASEDIR/doc/default.html $TINYPROXY_DATA_DIR
cp $BASEDIR/doc/debug.html $TINYPROXY_DATA_DIR
cp $BASEDIR/doc/stats.html $TINYPROXY_DATA_DIR
mkdir -p $TINYPROXY_PID_DIR
mkdir -p $TINYPROXY_LOG_DIR
mkdir -p $TINYPROXY_CONF_DIR

mkdir -p $WEBSERVER_PID_DIR
mkdir -p $WEBSERVER_LOG_DIR

cat >>$TINYPROXY_CONF_FILE<<EOF
User $TINYPROXY_USER
#Group $TINYPROXY_GROUP
Port $TINYPROXY_PORT
#Bind $TINYPROXY_IP
Listen $TINYPROXY_IP
Timeout 600
DefaultErrorFile "$TINYPROXY_DATA_DIR/default.html"
StatFile "$TINYPROXY_DATA_DIR/stats.html"
Logfile "$TINYPROXY_LOG_DIR/tinyproxy.log"
PidFile "$TINYPROXY_PID_FILE"
LogLevel Info
MaxClients 100
MinSpareServers 5
MaxSpareServers 20
StartServers 10
MaxRequestsPerChild 0
Allow 127.0.0.0/8
ViaProxyName "tinyproxy"
ConnectPort 443
ConnectPort 563
EOF


echo -n "starting web server..."
$WEBSERVER_BIN --port $WEBSERVER_PORT --log-dir $WEBSERVER_LOG_DIR --pid-file $WEBSERVER_PID_FILE
echo " done"

echo -n "starting tinyproxy..."
$TINYPROXY_BIN -c $TINYPROXY_CONF_FILE 2> $TINYPROXY_STDERR_LOG
echo " done"

echo -n "waiting for 3 seconds."
sleep 1
echo -n "."
sleep 1
echo -n "."
sleep 1
echo -n "."
echo " done"

echo -n "checking direct connection to web server..."
$WEBCLIENT_BIN $WEBSERVER_IP:$WEBSERVER_PORT / >> $WEBCLIENT_LOG 2>&1
WEBCLIENT_EXIT_CODE=$?
if test "x$WEBCLIENT_EXIT_CODE" = "x0" ; then
	echo " ok"
else
	echo "ERROR ($EBCLIENT_EXIT_CODE)"
	echo "webclient output:"
	cat $WEBCLIENT_LOG
fi

echo -n "testing connection through tinyproxy..."
$WEBCLIENT_BIN $TINYPROXY_IP:$TINYPROXY_PORT http://$WEBSERVER_IP:$WEBSERVER_PORT/ >> $WEBCLIENT_LOG 2>&1
WEBCLIENT_EXIT_CODE=$?
if test "x$WEBCLIENT_EXIT_CODE" = "x0" ; then
	echo " ok"
else
	echo "ERROR ($WEBCLIENT_EXIT_CODE)"
	echo "webclient output:"
	cat $WEBCLIENT_LOG
fi

echo -n "hit <enter> to stop the servers and exit: "
read READ

echo -n "killing tinyproxy..."
kill $(cat $TINYPROXY_PID_FILE)
if test "x$?" = "x0" ; then echo "ok" ; else echo "error" ; fi

echo -n "killing webserver..."
# too crude still, need pid file handling for webserver:
kill $(cat $WEBSERVER_PID_FILE)
if test "x$?" = "x0" ; then echo "ok" ; else echo "error" ; fi

echo $0: done

exit 0
