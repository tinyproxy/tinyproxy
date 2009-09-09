#!/usr/bin/perl -w

# Simple command line web client.
# Initially loosely based on examples from the perlipc manpage.
#
# Copyright (C) 2009 Michael Adam
#
# License: GPL

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
