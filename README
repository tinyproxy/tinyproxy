
 DESCRIPTION
 -----------

 tinyproxy is a small, efficient HTTP/SSL proxy daemon released under
 the GNU General Public License (GPL).  tinyproxy is very useful in a
 small network setting, where a larger proxy like Squid would either
 be too resource intensive, or a security risk.  One of the key
 features of tinyproxy is the buffering connection concept.  In
 effect, tinyproxy will buffer a high speed response from a server,
 and then relay it to a client at the highest speed the client will
 accept.  This feature greatly reduces the problems with sluggishness
 on the Internet.  If you are sharing an Internet connection with a
 small network, and you only want to allow HTTP requests to be
 allowed, then tinyproxy is a great tool for the network
 administrator.


 INSTALLATION
 ------------

 To install this package under a Unix derivative, read the INSTALL
 file.  tinyproxy uses a standard GNU configure script (basically you
 should be able to do:

	 ./configure ; make ; make install

 in the top level directory to compile and install tinyproxy).  There
 are additional command line arguments you can supply to configure.
 They include:

	--enable-debug		If you would like to turn on full
				debugging support
	--enable-socks		This turns on SOCKS support for using
				tinyproxy across a fire wall.
	--enable-xtinyproxy	Compile in support for the XTinyproxy
				header, which is sent to any web
				server in your domain.
	--enable-filter		Allows tinyproxy to filter out certain
				domains and URLs.
	--enable-upstream	Enable support for proxying connections
				through another proxy server.
	--enable-transparent-proxy
				Allow tinyproxy to be used as a
				transparent proxy daemon
	--enable-static		Compile a static version of tinyproxy


     Options for file locations etc.
        --with-stathost=HOST	Set the default name of the stats host
	--with-config=FILE	Set the default location of the
				configuration file

 Once you have completed your installation, if you would like to
 report your success please execute the report.sh script in the doc
 directory.  This will send an email to the authors reporting your
 version, and a few bits of information concerning the memory usage of 
 tinyproxy.  Alternatively, you could just send an email stating the
 version, whichever you prefer.


 SUPPORT
 -------

 If you are having problems with tinyproxy, please submit a bug to the
 tinyproxy Bug Tracking system hosted by SourceForge and located at:

	http://sourceforge.net/tracker/?group_id=2632

 You may also wish to subscribe to the tinyproxy-user mailing list. To
 do so please visit:

        http://lists.sourceforge.net/lists/listinfo/tinyproxy-users

 for more information on how to subscribe and post messages to the
 list.

 Please recompile tinyproxy with full debug support (--enable-debug)
 and include a copy of the log file, and any assert errors reported by
 tinyproxy.  Note that tinyproxy will output memory statistics to
 standard error if compiled with debugging support so you might want
 to redirect the output to a file for later examination.  Also, if you
 feel up to it, try running tinyproxy under your debugger and report
 the error your received and a context listing of the location.  Under
 gdb you would run tinyproxy like so:

	 gdb tinyproxy

	 (gdb) run -c location_of_tinyproxy_conf -d 2>/dev/null

 Now access the port tinyproxy is on until you receive a break in the
 gdb. You can now type:

	 (gbd) l

 to produce a context listing of the location of the error.  Send a
 copy to the authors.


 HOW TO CONTRIBUTE TO tinyproxy
 ------------------------------

 If you would like to contribute a feature, or a bug fix to the
 tinyproxy source, please send a diff (preferable a unified
 diff. i.e. "diff -u") against the latest release of tinyproxy.  Also, 
 if you could include a brief description of what your patch does.
