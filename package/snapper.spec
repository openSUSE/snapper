#
# spec file for package snapper (Version 0.0.1)
#
# norootforbuild

Name:		snapper
Version:	0.0.1
Release:	0
License:	GPL
Group:		System/Libraries
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Source:		snapper-%{version}.tar.bz2
Source1:	snapper-rpmlintrc

prefix:		/usr

BuildRequires:	gcc-c++ boost-devel blocxx-devel doxygen dejagnu libxml2-devel

Requires:	btrfs-progs

PreReq:		%fillup_prereq
Summary:	Library for snapper management

%description
This package contains snapper, a library for filesystem snapshot management.

Authors:
--------
    Arvin Schnell <aschnell@suse.de>

%prep
%setup -n snapper-%{version}

%build
export CFLAGS="$RPM_OPT_FLAGS -DNDEBUG"
export CXXFLAGS="$RPM_OPT_FLAGS -DNDEBUG"

aclocal
libtoolize --force --automake --copy
autoheader
automake --add-missing --copy
autoconf

%{?suse_update_config:%{suse_update_config -f}}
./configure --libdir=%{_libdir} --prefix=%{prefix} --mandir=%{_mandir} --disable-silent-rules
make %{?jobs:-j%jobs}

%check
LOCALEDIR=$RPM_BUILD_ROOT/usr/share/locale make check

%install
make install DESTDIR="$RPM_BUILD_ROOT"

install -d -m 755 $RPM_BUILD_ROOT/var/lock/snapper

%{find_lang} snapper

%clean
rm -rf "$RPM_BUILD_ROOT"

%files -f snapper.lang
%defattr(-,root,root)
%{_libdir}/libsnapper.so.*
%dir /var/lock/libsnapper
/var/adm/fillup-templates/sysconfig.snapper-libsnapper
%doc %dir %{prefix}/share/doc/packages/libsnapper
%doc %{prefix}/share/doc/packages/snapper/AUTHORS
%doc %{prefix}/share/doc/packages/snapper/COPYING

%post
/sbin/ldconfig
%{fillup_only -an snapper}

%postun
/sbin/ldconfig

%package devel
Requires:	libsnapper = %version
Requires:	gcc-c++ libstdc++-devel boost-devel blocxx-devel libxml2-devel
Summary:	Header files and documentation for libsnapper
Group:		Development/Languages/C and C++

%description devel
This package contains header files and documentation for developing with
libsnapper.

Authors:
--------
    Arvin Schnell <aschnell@suse.de>

%files devel
%defattr(-,root,root)
%{_libdir}/libsnapper.la
%{_libdir}/libsnapper.so
%{prefix}/include/snapper
%doc %{prefix}/share/doc/packages/libsnapper/autodocs
%doc %{prefix}/share/doc/packages/libsnapper/examples

