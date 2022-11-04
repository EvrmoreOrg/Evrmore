%define bdbv 4.8.30
%global selinux_variants mls strict targeted

%if 0%{?_no_gui:1}
%define _buildqt 0
%define buildargs --with-gui=no
%else
%define _buildqt 1
%if 0%{?_use_qt4}
%define buildargs --with-qrencode --with-gui=qt4
%else
%define buildargs --with-qrencode --with-gui=qt5
%endif
%endif

Name:		evrmore
Version:	0.12.0
Release:	2%{?dist}
Summary:	Peer to Peer Cryptographic Currency

Group:		Applications/System
License:	MIT
URL:		https://evrmorecoin.org/
Source0:	https://evrmorecoin.org/bin/evrmore-core-%{version}/evrmore-%{version}.tar.gz
Source1:	http://download.oracle.com/berkeley-db/db-%{bdbv}.NC.tar.gz

Source10:	https://raw.githubusercontent.com/evrmoreorg/evrmore/v%{version}/contrib/debian/examples/evrmore.conf

#man pages
Source20:	https://raw.githubusercontent.com/evrmoreorg/evrmore/v%{version}/doc/man/evrmored.1
Source21:	https://raw.githubusercontent.com/evrmoreorg/evrmore/v%{version}/doc/man/evrmore-cli.1
Source22:	https://raw.githubusercontent.com/evrmoreorg/evrmore/v%{version}/doc/man/evrmore-qt.1

#selinux
Source30:	https://raw.githubusercontent.com/evrmoreorg/evrmore/v%{version}/contrib/rpm/evrmore.te
# Source31 - what about evrmore-tx and bench_evrmore ???
Source31:	https://raw.githubusercontent.com/evrmore/evrmoreorg/v%{version}/contrib/rpm/evrmore.fc
Source32:	https://raw.githubusercontent.com/evrmore/evrmoreorg/v%{version}/contrib/rpm/evrmore.if

Source100:	https://upload.wikimedia.org/wikipedia/commons/4/46/Bitcoin.svg

%if 0%{?_use_libressl:1}
BuildRequires:	libressl-devel
%else
BuildRequires:	openssl-devel
%endif
BuildRequires:	boost-devel
BuildRequires:	miniupnpc-devel
BuildRequires:	autoconf automake libtool
BuildRequires:	libevent-devel


Patch0:		evrmore-0.12.0-libressl.patch


%description
Evrmore is a digital cryptographic currency that uses peer-to-peer technology to
operate with no central authority or banks; managing transactions and the
issuing of EVRs is carried out collectively by the network.

%if %{_buildqt}
%package core
Summary:	Peer to Peer Cryptographic Currency
Group:		Applications/System
Obsoletes:	%{name} < %{version}-%{release}
Provides:	%{name} = %{version}-%{release}
%if 0%{?_use_qt4}
BuildRequires:	qt-devel
%else
BuildRequires:	qt5-qtbase-devel
# for /usr/bin/lrelease-qt5
BuildRequires:	qt5-linguist
%endif
BuildRequires:	protobuf-devel
BuildRequires:	qrencode-devel
BuildRequires:	%{_bindir}/desktop-file-validate
# for icon generation from SVG
BuildRequires:	%{_bindir}/inkscape
BuildRequires:	%{_bindir}/convert

%description core
Evrmore is a digital cryptographic currency that uses peer-to-peer technology to
operate with no central authority or banks; managing transactions and the
issuing of EVRs is carried out collectively by the network.

This package contains the Qt based graphical client and node. If you are looking
to run a Evrmore wallet, this is probably the package you want.
%endif


%package libs
Summary:	Evrmore shared libraries
Group:		System Environment/Libraries

%description libs
This package provides the evrmoreconsensus shared libraries. These libraries
may be used by third party software to provide consensus verification
functionality.

Unless you know need this package, you probably do not.

%package devel
Summary:	Development files for evrmore
Group:		Development/Libraries
Requires:	%{name}-libs = %{version}-%{release}

%description devel
This package contains the header files and static library for the
evrmoreconsensus shared library. If you are developing or compiling software
that wants to link against that library, then you need this package installed.

Most people do not need this package installed.

%package server
Summary:	The evrmore daemon
Group:		System Environment/Daemons
Requires:	evrmore-utils = %{version}-%{release}
Requires:	selinux-policy policycoreutils-python
Requires(pre):	shadow-utils
Requires(post):	%{_sbindir}/semodule %{_sbindir}/restorecon %{_sbindir}/fixfiles %{_sbindir}/sestatus
Requires(postun):	%{_sbindir}/semodule %{_sbindir}/restorecon %{_sbindir}/fixfiles %{_sbindir}/sestatus
BuildRequires:	systemd
BuildRequires:	checkpolicy
BuildRequires:	%{_datadir}/selinux/devel/Makefile

%description server
This package provides a stand-alone evrmore-core daemon. For most users, this
package is only needed if they need a full-node without the graphical client.

Some third party wallet software will want this package to provide the actual
evrmore-core node they use to connect to the network.

If you use the graphical evrmore-core client then you almost certainly do not
need this package.

%package utils
Summary:	Evrmore utilities
Group:		Applications/System

%description utils
This package provides several command line utilities for interacting with a
evrmore-core daemon.

The evrmore-cli utility allows you to communicate and control a evrmore daemon
over RPC, the evrmore-tx utility allows you to create a custom transaction, and
the bench_evrmore utility can be used to perform some benchmarks.

This package contains utilities needed by the evrmore-server package.


%prep
%setup -q
%patch0 -p1 -b .libressl
cp -p %{SOURCE10} ./evrmore.conf.example
tar -zxf %{SOURCE1}
cp -p db-%{bdbv}.NC/LICENSE ./db-%{bdbv}.NC-LICENSE
mkdir db4 SELinux
cp -p %{SOURCE30} %{SOURCE31} %{SOURCE32} SELinux/


%build
CWD=`pwd`
cd db-%{bdbv}.NC/build_unix/
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=${CWD}/db4
make install
cd ../..

./autogen.sh
%configure LDFLAGS="-L${CWD}/db4/lib/" CPPFLAGS="-I${CWD}/db4/include/" --with-miniupnpc --enable-glibc-back-compat %{buildargs}
make %{?_smp_mflags}

pushd SELinux
for selinuxvariant in %{selinux_variants}; do
	make NAME=${selinuxvariant} -f %{_datadir}/selinux/devel/Makefile
	mv evrmore.pp evrmore.pp.${selinuxvariant}
	make NAME=${selinuxvariant} -f %{_datadir}/selinux/devel/Makefile clean
done
popd


%install
make install DESTDIR=%{buildroot}

mkdir -p -m755 %{buildroot}%{_sbindir}
mv %{buildroot}%{_bindir}/evrmored %{buildroot}%{_sbindir}/evrmored

# systemd stuff
mkdir -p %{buildroot}%{_tmpfilesdir}
cat <<EOF > %{buildroot}%{_tmpfilesdir}/evrmore.conf
d /run/evrmored 0750 evrmore evrmore -
EOF
touch -a -m -t 201504280000 %{buildroot}%{_tmpfilesdir}/evrmore.conf

mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
cat <<EOF > %{buildroot}%{_sysconfdir}/sysconfig/evrmore
# Provide options to the evrmore daemon here, for example
# OPTIONS="-testnet -disable-wallet"

OPTIONS=""

# System service defaults.
# Don't change these unless you know what you're doing.
CONFIG_FILE="%{_sysconfdir}/evrmore/evrmore.conf"
DATA_DIR="%{_localstatedir}/lib/evrmore"
PID_FILE="/run/evrmored/evrmored.pid"
EOF
touch -a -m -t 201504280000 %{buildroot}%{_sysconfdir}/sysconfig/evrmore

mkdir -p %{buildroot}%{_unitdir}
cat <<EOF > %{buildroot}%{_unitdir}/evrmore.service
[Unit]
Description=Evrmore daemon
After=syslog.target network.target

[Service]
Type=forking
ExecStart=%{_sbindir}/evrmored -daemon -conf=\${CONFIG_FILE} -datadir=\${DATA_DIR} -pid=\${PID_FILE} \$OPTIONS
EnvironmentFile=%{_sysconfdir}/sysconfig/evrmore
User=evrmore
Group=evrmore

Restart=on-failure
PrivateTmp=true
TimeoutStopSec=120
TimeoutStartSec=60
StartLimitInterval=240
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
EOF
touch -a -m -t 201504280000 %{buildroot}%{_unitdir}/evrmore.service
#end systemd stuff

mkdir %{buildroot}%{_sysconfdir}/evrmore
mkdir -p %{buildroot}%{_localstatedir}/lib/evrmore

#SELinux
for selinuxvariant in %{selinux_variants}; do
	install -d %{buildroot}%{_datadir}/selinux/${selinuxvariant}
	install -p -m 644 SELinux/evrmore.pp.${selinuxvariant} %{buildroot}%{_datadir}/selinux/${selinuxvariant}/evrmore.pp
done

%if %{_buildqt}
# qt icons
install -D -p share/pixmaps/evrmore.ico %{buildroot}%{_datadir}/pixmaps/evrmore.ico
install -p share/pixmaps/nsis-header.bmp %{buildroot}%{_datadir}/pixmaps/
install -p share/pixmaps/nsis-wizard.bmp %{buildroot}%{_datadir}/pixmaps/
install -p %{SOURCE100} %{buildroot}%{_datadir}/pixmaps/evrmore.svg
%{_bindir}/inkscape %{SOURCE100} --export-png=%{buildroot}%{_datadir}/pixmaps/evrmore16.png -w16 -h16
%{_bindir}/inkscape %{SOURCE100} --export-png=%{buildroot}%{_datadir}/pixmaps/evrmore32.png -w32 -h32
%{_bindir}/inkscape %{SOURCE100} --export-png=%{buildroot}%{_datadir}/pixmaps/evrmore64.png -w64 -h64
%{_bindir}/inkscape %{SOURCE100} --export-png=%{buildroot}%{_datadir}/pixmaps/evrmore128.png -w128 -h128
%{_bindir}/inkscape %{SOURCE100} --export-png=%{buildroot}%{_datadir}/pixmaps/evrmore256.png -w256 -h256
%{_bindir}/convert -resize 16x16 %{buildroot}%{_datadir}/pixmaps/evrmore256.png %{buildroot}%{_datadir}/pixmaps/evrmore16.xpm
%{_bindir}/convert -resize 32x32 %{buildroot}%{_datadir}/pixmaps/evrmore256.png %{buildroot}%{_datadir}/pixmaps/evrmore32.xpm
%{_bindir}/convert -resize 64x64 %{buildroot}%{_datadir}/pixmaps/evrmore256.png %{buildroot}%{_datadir}/pixmaps/evrmore64.xpm
%{_bindir}/convert -resize 128x128 %{buildroot}%{_datadir}/pixmaps/evrmore256.png %{buildroot}%{_datadir}/pixmaps/evrmore128.xpm
%{_bindir}/convert %{buildroot}%{_datadir}/pixmaps/evrmore256.png %{buildroot}%{_datadir}/pixmaps/evrmore256.xpm
touch %{buildroot}%{_datadir}/pixmaps/*.png -r %{SOURCE100}
touch %{buildroot}%{_datadir}/pixmaps/*.xpm -r %{SOURCE100}

# Desktop File - change the touch timestamp if modifying
mkdir -p %{buildroot}%{_datadir}/applications
cat <<EOF > %{buildroot}%{_datadir}/applications/evrmore-core.desktop
[Desktop Entry]
Encoding=UTF-8
Name=Evrmore
Comment=Evrmore P2P Cryptocurrency
Comment[fr]=Evrmore, monnaie virtuelle cryptographique pair à pair
Comment[tr]=Evrmore, eşten eşe kriptografik sanal para birimi
Exec=evrmore-qt %u
Terminal=false
Type=Application
Icon=evrmore128
MimeType=x-scheme-handler/evrmore;
Categories=Office;Finance;
EOF
# change touch date when modifying desktop
touch -a -m -t 201511100546 %{buildroot}%{_datadir}/applications/evrmore-core.desktop
%{_bindir}/desktop-file-validate %{buildroot}%{_datadir}/applications/evrmore-core.desktop

# KDE protocol - change the touch timestamp if modifying
mkdir -p %{buildroot}%{_datadir}/kde4/services
cat <<EOF > %{buildroot}%{_datadir}/kde4/services/evrmore-core.protocol
[Protocol]
exec=evrmore-qt '%u'
protocol=evrmore
input=none
output=none
helper=true
listing=
reading=false
writing=false
makedir=false
deleting=false
EOF
# change touch date when modifying protocol
touch -a -m -t 201511100546 %{buildroot}%{_datadir}/kde4/services/evrmore-core.protocol
%endif

# man pages
install -D -p %{SOURCE20} %{buildroot}%{_mandir}/man1/evrmored.1
install -p %{SOURCE21} %{buildroot}%{_mandir}/man1/evrmore-cli.1
%if %{_buildqt}
install -p %{SOURCE22} %{buildroot}%{_mandir}/man1/evrmore-qt.1
%endif

# nuke these, we do extensive testing of binaries in %%check before packaging
rm -f %{buildroot}%{_bindir}/test_*

%check
make check
srcdir=src test/evrmore-util-test.py
test/functional/test_runner.py --extended

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%pre server
getent group evrmore >/dev/null || groupadd -r evrmore
getent passwd evrmore >/dev/null ||
	useradd -r -g evrmore -d /var/lib/evrmore -s /sbin/nologin \
	-c "Evrmore wallet server" evrmore
exit 0

%post server
%systemd_post evrmore.service
# SELinux
if [ `%{_sbindir}/sestatus |grep -c "disabled"` -eq 0 ]; then
for selinuxvariant in %{selinux_variants}; do
	%{_sbindir}/semodule -s ${selinuxvariant} -i %{_datadir}/selinux/${selinuxvariant}/evrmore.pp &> /dev/null || :
done
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 8819
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 8820
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 18819
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 18820
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 18443
%{_sbindir}/semanage port -a -t evrmore_port_t -p tcp 18444
%{_sbindir}/fixfiles -R evrmore-server restore &> /dev/null || :
%{_sbindir}/restorecon -R %{_localstatedir}/lib/evrmore || :
fi

%posttrans server
%{_bindir}/systemd-tmpfiles --create

%preun server
%systemd_preun evrmore.service

%postun server
%systemd_postun evrmore.service
# SELinux
if [ $1 -eq 0 ]; then
	if [ `%{_sbindir}/sestatus |grep -c "disabled"` -eq 0 ]; then
	%{_sbindir}/semanage port -d -p tcp 8819
	%{_sbindir}/semanage port -d -p tcp 8820
	%{_sbindir}/semanage port -d -p tcp 18819
	%{_sbindir}/semanage port -d -p tcp 18820
	%{_sbindir}/semanage port -d -p tcp 18443
	%{_sbindir}/semanage port -d -p tcp 18444
	for selinuxvariant in %{selinux_variants}; do
		%{_sbindir}/semodule -s ${selinuxvariant} -r evrmore &> /dev/null || :
	done
	%{_sbindir}/fixfiles -R evrmore-server restore &> /dev/null || :
	[ -d %{_localstatedir}/lib/evrmore ] && \
		%{_sbindir}/restorecon -R %{_localstatedir}/lib/evrmore &> /dev/null || :
	fi
fi

%clean
rm -rf %{buildroot}

%if %{_buildqt}
%files core
%defattr(-,root,root,-)
%license COPYING db-%{bdbv}.NC-LICENSE
%doc COPYING evrmore.conf.example doc/README.md doc/bips.md doc/files.md doc/multiwallet-qt.md doc/reduce-traffic.md doc/release-notes.md doc/tor.md
%attr(0755,root,root) %{_bindir}/evrmore-qt
%attr(0644,root,root) %{_datadir}/applications/evrmore-core.desktop
%attr(0644,root,root) %{_datadir}/kde4/services/evrmore-core.protocol
%attr(0644,root,root) %{_datadir}/pixmaps/*.ico
%attr(0644,root,root) %{_datadir}/pixmaps/*.bmp
%attr(0644,root,root) %{_datadir}/pixmaps/*.svg
%attr(0644,root,root) %{_datadir}/pixmaps/*.png
%attr(0644,root,root) %{_datadir}/pixmaps/*.xpm
%attr(0644,root,root) %{_mandir}/man1/evrmore-qt.1*
%endif

%files libs
%defattr(-,root,root,-)
%license COPYING
%doc COPYING doc/README.md doc/shared-libraries.md
%{_libdir}/lib*.so.*

%files devel
%defattr(-,root,root,-)
%license COPYING
%doc COPYING doc/README.md doc/developer-notes.md doc/shared-libraries.md
%attr(0644,root,root) %{_includedir}/*.h
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/*.la
%attr(0644,root,root) %{_libdir}/pkgconfig/*.pc

%files server
%defattr(-,root,root,-)
%license COPYING db-%{bdbv}.NC-LICENSE
%doc COPYING evrmore.conf.example doc/README.md doc/REST-interface.md doc/bips.md doc/dnsseed-policy.md doc/files.md doc/reduce-traffic.md doc/release-notes.md doc/tor.md
%attr(0755,root,root) %{_sbindir}/evrmored
%attr(0644,root,root) %{_tmpfilesdir}/evrmore.conf
%attr(0644,root,root) %{_unitdir}/evrmore.service
%dir %attr(0750,evrmore,evrmore) %{_sysconfdir}/evrmore
%dir %attr(0750,evrmore,evrmore) %{_localstatedir}/lib/evrmore
%config(noreplace) %attr(0600,root,root) %{_sysconfdir}/sysconfig/evrmore
%attr(0644,root,root) %{_datadir}/selinux/*/*.pp
%attr(0644,root,root) %{_mandir}/man1/evrmored.1*

%files utils
%defattr(-,root,root,-)
%license COPYING
%doc COPYING evrmore.conf.example doc/README.md
%attr(0755,root,root) %{_bindir}/evrmore-cli
%attr(0755,root,root) %{_bindir}/evrmore-tx
%attr(0755,root,root) %{_bindir}/bench_evrmore
%attr(0644,root,root) %{_mandir}/man1/evrmore-cli.1*



%changelog
* Fri Feb 26 2016 Alice Wonder <buildmaster@librelamp.com> - 0.12.0-2
- Rename Qt package from evrmore to evrmore-core
- Make building of the Qt package optional
- When building the Qt package, default to Qt5 but allow building
-  against Qt4
- Only run SELinux stuff in post scripts if it is not set to disabled

* Wed Feb 24 2016 Alice Wonder <buildmaster@librelamp.com> - 0.12.0-1
- Initial spec file for 0.12.0 release

# This spec file is written from scratch but a lot of the packaging decisions are directly
# based upon the 0.11.2 package spec file from https://www.ringingliberty.com/evrmore/
