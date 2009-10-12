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
use Getopt::Long;
use Pod::Usage;

my $EOL = "\015\012";

my $VERSION = "0.1";
my $NAME = "Tinyproxy-Web-Client";
my $user_agent = "$NAME/$VERSION";
my $user_agent_header = "User-Agent: $user_agent$EOL";
my $http_version = "1.0";
my $method = "GET";
my $dry_run = 0;
my $help = 0;
my $entity = undef;

my $default_port = "80";
my $port = $default_port;

sub process_options() {
	my $result = GetOptions("help|?" => \$help,
				"http-version=s" => \$http_version,
				"method=s" => \$method,
				"dry-run" => \$dry_run,
				"entity=s" => \$entity);
	die "Error reading cmdline options! $!" unless $result;

	pod2usage(1) if $help;

	# some post-processing:
}


sub build_request($$$$$$)
{
	my ( $host, $port, $version, $method, $document, $entity ) = @_;
	my $request = "";

	$method = uc($method);

	if ($version eq '0.9') {
		if ($method ne 'GET') {
			die "invalid method '$method'";
		}
		$request = "$method $document$EOL";
	} elsif ($version eq '1.0') {
		$request = "$method $document HTTP/$version$EOL"
			 . $user_agent_header;
	} elsif ($version eq '1.1') {
		$request = "$method $document HTTP/$version$EOL"
			 . "Host: $host" . (($port and ($port ne $default_port))?":$port":"") . "$EOL"
			 . $user_agent_header
			 . "Connection: close$EOL";
	} else {
		die "invalid version '$version'";
	}

	$request .= $EOL;

	if ($entity) {
		$request .= $entity;
	}

	return $request;
}

# main

process_options();

unless (@ARGV > 1) {
	pod2usage(1);
}

my $hostarg = shift(@ARGV);
my $host = $hostarg;

if ($host =~ /^([^:]+):(.*)/) {
	$port = $2;
	$host = $1;
}

foreach my $document (@ARGV) {
	my $request = build_request($host, $port, $http_version, $method, $document, $entity);
	if ($dry_run) {
		print $request;
		exit(0);
	}

	my $remote = IO::Socket::INET->new(
						Proto     => "tcp",
						PeerAddr  => $host,
						PeerPort  => $port,
					);
	unless ($remote) {
		die "cannot connect to http daemon on $host (port $port)";
	}

	$remote->autoflush(1);

	print $remote $request;

	while (<$remote>) {
		print;
	}

	close $remote;
}

exit(0);

__END__

=head1 webclient.pl

A simple WEB client written in perl.

=head1 SYNOPSIS

webclient.pl [options] host[:port] document [document ...]

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exit.

=item B<--http-version>

Specify the HTTP protocol version to use (0.9, 1.0, 1.1). Default is 1.0.

=item B<--method>

Specify the HTTP request method ('GET', 'CONNECT', ...). Default is 'GET'.

=item B<--entity>

Add the provided string as entity (i.e. body) to the request.

=item B<--dry-run>

Don't actually connect to the server but print the request that would be sent.

=back

=head1 DESCRIPTION

This is a basic web client. It permits to send http request messages to
web servers or web proxy servers. The result is printed as is to standard output,
including headers. This is meant as a tool for diagnosing and testing
web servers and proxy servers.

=head1 COPYRIGHT

Copyright (C) 2009 Michael Adam <obnox@samba.org>

This program is distributed under the terms of the GNU General Public License
version 2 or above. See the COPYING file for additional information.

=cut
