Summary: A small, efficient HTTP/SSL proxy daemon.
Name: tinyproxy
Version: 1.5.2
Release: 1
License: GPL
Group: System Environment/Daemons
URL: http://tinyproxy.sourceforge.net
Prefix: %{_prefix}
Packager: S. A. Hutchins <sh4d0wstr1f3@yahoo.com>
Source: tinyproxy-1.5.2.tar.gz
Source1: tinyproxy-initd
Patch0: tinyproxy-1.5.2-config-patch
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
tinyproxy is a small, efficient HTTP/SSL proxy daemon released under the GNU
General Public License (GPL).  tinyproxy is very useful in a small network
setting, where a larger proxy like Squid would either be too resource
intensive, or a security risk.

%prep
%setup
%patch

%build
    ./configure  --enable-transparent-proxy --prefix=%{_prefix} \
		 --mandir=%{_mandir}
    make

%install
    if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
    fi

    pwd

    cd $RPM_BUILD_DIR/%{name}-%{version}
    make install prefix=%{_prefix} DESTDIR="$RPM_BUILD_ROOT"

    mkdir -p $RPM_BUILD_ROOT/etc/init.d
    cp -f $RPM_SOURCE_DIR/tinyproxy-initd $RPM_BUILD_ROOT/etc/init.d/tinyproxy

%files
%defattr(-, root, root)
    %{_sbindir}/tinyproxy
    %{_mandir}/*

%defattr(0755, root, root)
    /etc/init.d/tinyproxy

%defattr(0600, root, root)
    /etc/tinyproxy/tinyproxy.conf
    /etc/tinyproxy/tinyproxy.conf-dist

%doc AUTHORS COPYING INSTALL NEWS README TODO
%doc ChangeLog
%doc doc/filter-howto.txt
%doc doc/HTTP_ERROR_CODES
%doc doc/releases.txt
%doc doc/RFC_INFO
%doc doc/report.sh

%clean
    if [ "$RPM_BUILD_DIR" != "/" ]; then
	rm -rf $RPM_BUILD_DIR/%{name}-%{version}
    fi
    if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
    fi

%changelog
* Sat Feb 01 2003 S. A. Hutchins
- From the depths of the void this beast I spawn. I added an initrd script for
  this so it can be started/stopped from /sbin/service. My version of RedHat
  doesn't have a 'nogroup' so used nobody instead.

