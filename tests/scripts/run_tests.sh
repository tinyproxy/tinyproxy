#!/bin/sh

# testsuite runner for tinyproxy
#
# Copyright (C) 2009 Michael Adam <obnox@samba.org>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.


SCRIPTS_DIR=$(cd "$(dirname "$0")" && pwd)
BASEDIR=$SCRIPTS_DIR/../..
TESTS_DIR=$SCRIPTS_DIR/..
TESTENV_DIR=$TESTS_DIR/env
LOG_DIR=$TESTENV_DIR/var/log

TINYPROXY_IP=127.0.0.2
TINYPROXY_PORT=12321
TINYPROXY_USER=$(id -un)
TINYPROXY_PID_DIR=$TESTENV_DIR/var/run/tinyproxy
TINYPROXY_PID_FILE=$TINYPROXY_PID_DIR/tinyproxy.pid
TINYPROXY_LOG_DIR=$LOG_DIR/tinyproxy
TINYPROXY_LOG_FILE=$TINYPROXY_LOG_DIR/tinyproxy.log
TINYPROXY_DATA_DIR=$TESTENV_DIR/usr/share/tinyproxy
TINYPROXY_CONF_DIR=$TESTENV_DIR/etc/tinyproxy
TINYPROXY_CONF_FILE=$TINYPROXY_CONF_DIR/tinyproxy.conf
TINYPROXY_FILTER_FILE=$TINYPROXY_CONF_DIR/filter
TINYPROXY_STDERR_LOG=$TINYPROXY_LOG_DIR/tinyproxy.stderr.log
TINYPROXY_BIN=$BASEDIR/src/tinyproxy
TINYPROXY_STATHOST_IP="127.0.0.127"

WEBSERVER_IP=127.0.0.3
WEBSERVER_PORT=32123
WEBSERVER_PID_DIR=$TESTENV_DIR/var/run/webserver
WEBSERVER_PID_FILE=$WEBSERVER_PID_DIR/webserver.pid
WEBSERVER_LOG_DIR=$TESTENV_DIR/var/log/webserver
WEBSERVER_BIN_FILE=webserver.pl
WEBSERVER_BIN=$SCRIPTS_DIR/$WEBSERVER_BIN_FILE

WEBCLIENT_LOG=$LOG_DIR/webclient.log
WEBCLIENT_BIN=$SCRIPTS_DIR/webclient.pl

provision_initial() {
	if test -e "$TESTENV_DIR" ; then
		TESTENV_DIR_OLD=$TESTENV_DIR.old
		if test -e "$TESTENV_DIR_OLD" ; then
			rm -rf "$TESTENV_DIR_OLD"
		fi
		mv "$TESTENV_DIR" "$TESTENV_DIR".old
	fi

	mkdir -p "$LOG_DIR"
}

provision_tinyproxy() {
	mkdir -p "$TINYPROXY_DATA_DIR"
	cp "$BASEDIR"/data/templates/default.html "$TINYPROXY_DATA_DIR"
	cp "$BASEDIR"/data/templates/debug.html "$TINYPROXY_DATA_DIR"
	cp "$BASEDIR"/data/templates/stats.html "$TINYPROXY_DATA_DIR"
	mkdir -p "$TINYPROXY_PID_DIR"
	mkdir -p "$TINYPROXY_LOG_DIR"
	mkdir -p "$TINYPROXY_CONF_DIR"

	cat >>"$TINYPROXY_CONF_FILE"<<EOF
User $TINYPROXY_USER
#Group $TINYPROXY_GROUP
Port $TINYPROXY_PORT
#Bind $TINYPROXY_IP
Listen $TINYPROXY_IP
Timeout 600
StatHost "$TINYPROXY_STATHOST_IP"
DefaultErrorFile "$TINYPROXY_DATA_DIR/debug.html"
ErrorFile 400 "$TINYPROXY_DATA_DIR/debug.html"
ErrorFile 403 "$TINYPROXY_DATA_DIR/debug.html"
ErrorFile 501 "$TINYPROXY_DATA_DIR/debug.html"
ErrorFile 502 "$TINYPROXY_DATA_DIR/debug.html"
StatFile "$TINYPROXY_DATA_DIR/stats.html"
Logfile "$TINYPROXY_LOG_FILE"
PidFile "$TINYPROXY_PID_FILE"
LogLevel Info
MaxClients 100
Allow 127.0.0.0/8
ViaProxyName "tinyproxy"
#DisableViaHeader Yes
ConnectPort 443
ConnectPort 563
#FilterURLs On
Filter "$TINYPROXY_FILTER_FILE"
XTinyproxy Yes
AddHeader "X-My-Header1" "Powered by Tinyproxy"
AddHeader "X-My-Header2" "Powered by Tinyproxy"
AddHeader "X-My-Header3" "Powered by Tinyproxy"
Upstream http 255.255.255.255:65535 ".invalid"
EOF

cat << 'EOF' > "$TINYPROXY_FILTER_FILE"
.*\.google-analytics\.com$
EOF
}

start_tinyproxy() {
	printf "starting tinyproxy..."
	$VALGRIND "$TINYPROXY_BIN" -c "$TINYPROXY_CONF_FILE" 2> "$TINYPROXY_STDERR_LOG"
	echo " done (listening on $TINYPROXY_IP:$TINYPROXY_PORT)"
}

reload_config() {
	printf "signaling tinyproxy to reload config..."
	pid=$(cat "$TINYPROXY_PID_FILE")
	#1: SIGHUP
	kill -1 "$pid" && echo "ok" || echo "fail"
}

stop_tinyproxy() {
	printf "killing tinyproxy..."
	pid=$(cat "$TINYPROXY_PID_FILE")
	kill "$pid"
	if test "$?" = "0" ; then
		echo " ok"
	else
		echo " error killing pid $pid"
		pgrep tinyproxy
		echo "### printing logfile"
		cat "$TINYPROXY_LOG_FILE"
		echo "### printing stderr logfile"
		cat "$TINYPROXY_STDERR_LOG"
	fi
}

provision_webserver() {
	mkdir -p "$WEBSERVER_PID_DIR"
	mkdir -p "$WEBSERVER_LOG_DIR"
}

start_webserver() {
	printf "starting web server..."
	"$WEBSERVER_BIN" --port "$WEBSERVER_PORT" --log-dir "$WEBSERVER_LOG_DIR" --pid-file "$WEBSERVER_PID_FILE"
	echo " done. listening on $WEBSERVER_IP:$WEBSERVER_PORT"
}

stop_webserver() {
	printf  "killing webserver..."
	kill "$(cat "$WEBSERVER_PID_FILE")"
	if test "$?" = "0" ; then
		echo " ok"
	else
		echo " error"
    fi
}

wait_for_some_seconds() {
	seconds=$1
	if test "$seconds" = "" ; then
		seconds=1
	fi

	printf 'waiting for %s seconds.' "$seconds"

	for COUNT in $(seq 1 "$seconds") ; do
		sleep 1
		printf ' %s ' "$COUNT"
	done
	echo ' done'
}

run_basic_webclient_request() {
	"$WEBCLIENT_BIN" "$1" "$2" > "$WEBCLIENT_LOG" 2>&1
	WEBCLIENT_EXIT_CODE=$?
	if test "$WEBCLIENT_EXIT_CODE" = "0" ; then
		echo " ok"
	else
		echo "ERROR: $WEBCLIENT_EXIT_CODE"
		echo "webclient output:"
		cat "$WEBCLIENT_LOG"
		echo "######################################"
	fi

	return $WEBCLIENT_EXIT_CODE
}

run_failure_webclient_request() {
	ec=$1
	expected_error=$(($1 - 399))
	shift
	"$WEBCLIENT_BIN" "$1" "$2" "$3" "$4" > "$WEBCLIENT_LOG" 2>&1
	WEBCLIENT_EXIT_CODE=$?
	if test "$WEBCLIENT_EXIT_CODE" = "$expected_error" ; then
		echo " ok, got expected error code $ec"
		return 0
	else
		echo "ERROR: $WEBCLIENT_EXIT_CODE"
		echo "webclient output:"
		cat "$WEBCLIENT_LOG"
		echo "######################################"
	fi

	return 1
}

# "main"

provision_initial
provision_tinyproxy
provision_webserver

start_webserver
start_tinyproxy

wait_for_some_seconds 1

FAILED=0

basic_test() {
printf "checking direct connection to web server..."
run_basic_webclient_request "$WEBSERVER_IP:$WEBSERVER_PORT" /
test "$?" = "0" || FAILED=$((FAILED + 1))

printf "testing connection through tinyproxy..."
run_basic_webclient_request "$TINYPROXY_IP:$TINYPROXY_PORT" "http://$WEBSERVER_IP:$WEBSERVER_PORT/"
test "$?" = "0" || FAILED=$((FAILED + 1))

printf "requesting statspage via stathost url..."
run_basic_webclient_request "$TINYPROXY_IP:$TINYPROXY_PORT" "http://$TINYPROXY_STATHOST_IP"
test "$?" = "0" || FAILED=$((FAILED + 1))
}

ext_test() {
printf "checking bogus request..."
run_failure_webclient_request 400 --method="BIG FART" "$TINYPROXY_IP:$TINYPROXY_PORT" "http://$WEBSERVER_IP:$WEBSERVER_PORT"
test "$?" = "0" || FAILED=$((FAILED + 1))

printf "testing connection to filtered domain..."
run_failure_webclient_request 403 "$TINYPROXY_IP:$TINYPROXY_PORT" "http://badgoy.google-analytics.com/"
test "$?" = "0" || FAILED=$((FAILED + 1))

printf "requesting connect method to denied port..."
run_failure_webclient_request 403 --method=CONNECT "$TINYPROXY_IP:$TINYPROXY_PORT" "localhost:12345"
test "$?" = "0" || FAILED=$((FAILED + 1))

printf "testing unavailable backend..."
run_failure_webclient_request 502 "$TINYPROXY_IP:$TINYPROXY_PORT" "http://bogus.invalid"
test "$?" = "0" || FAILED=$((FAILED + 1))
}

basic_test
reload_config
basic_test
ext_test

echo "$FAILED errors"

if test "$TINYPROXY_TESTS_WAIT" = "yes"; then
	echo "You can continue using the webserver and tinyproxy."
	 rintf "hit <enter> to stop the servers and exit: "
	read -r READ
    echo "$READ" > /dev/null
fi

stop_tinyproxy
stop_webserver

echo "done"

exit $FAILED
