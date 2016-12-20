# Tinyproxy

Tinyproxy is a small, efficient HTTP/SSL proxy daemon released under the
GNU General Public License.  Tinyproxy is very useful in a small network
setting, where a larger proxy would either be too resource intensive, or
a security risk.  One of the key features of Tinyproxy is the buffering
connection concept.  In effect, Tinyproxy will buffer a high speed
response from a server, and then relay it to a client at the highest
speed the client will accept.  This feature greatly reduces the problems
with sluggishness on the Internet.  If you are sharing an Internet
connection with a small network, and you only want to allow HTTP
requests to be allowed, then Tinyproxy is a great tool for the network
administrator.

For more info, please visit [the Tinyproxy web site](https://tinyproxy.github.io/).


## Installation


To install this package under a UNIX derivative, read the INSTALL file.
Tinyproxy uses a standard GNU `configure` script. Basically you should
be able to do:


```
./configure
make
make install
```

in the top level directory to compile and install Tinyproxy. There are
additional command line arguments you can supply to `configure`. They
include:

- `--enable-debug`: 
If you would like to turn on full debugging support.

- `--enable-xtinyproxy`: 
Compile in support for the XTinyproxy header, which is sent to any
web server in your domain.

- `--enable-filter`: 
Allows Tinyproxy to filter out certain domains and URLs.

- `--enable-upstream`: 
Enable support for proxying connections through another proxy server.

- `--enable-transparent`: 
Allow Tinyproxy to be used as a transparent proxy daemon.

- `--enable-static`: 
Compile a static version of Tinyproxy.

- `--with-stathost=HOST`: 
Set the default name of the stats host.


## Support


If you are having problems with Tinyproxy, please raise an
[issue on github](https://github.com/tinyproxy/tinyproxy/issues).


## Contributing

If you would like to contribute a feature, or a bug fix to the Tinyproxy
source, please clone the
[git repository from github](https://github.com/tinyproxy/tinyproxy.git)
and create a [pull request](https://github.com/tinyproxy/tinyproxy/pulls).


## Community

You can meet developers and users to discuss development,
patches and deployment issues in the `#tinyproxy` IRC channel on
Freenode (`irc.freenode.net`).
