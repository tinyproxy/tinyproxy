#!/usr/bin/perl -w

# Simple command line web client.
# Initially loosely based on examples from the perlipc manpage.
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

use strict;

use IO::Socket;

my $EOL = "\015\012";
my $BLANK = $EOL x 2;

unless (@ARGV > 1) {
	die "usage: $0 host[:port] document ...";
}

my $host = shift(@ARGV);
my $port = "http(80)";

if ($host =~ /^([^:]+):(.*)/) {
	$port = $2;
	$host = $1;
}

foreach my $document (@ARGV) {
	my $remote = IO::Socket::INET->new(
						Proto     => "tcp",
						PeerAddr  => $host,
						PeerPort  => $port,
					);
	unless ($remote) {
		die "cannot connect to http daemon on $host (port $port)";
	}

	$remote->autoflush(1);

	print $remote "GET $document HTTP/1.0" . $BLANK;
	while (<$remote>) {
		print;
	}
	close $remote;
}
