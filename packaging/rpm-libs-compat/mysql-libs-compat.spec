# Copyright (c) 2000, 2014, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston
# MA  02110-1301  USA.

# ==========
#
# This spec file is a subset of the Oracle spec file for MySQL RPMs
# for Enterprise Linux (RedHat or Oracle Linux) as published with MySQL 5.6.23
# The subset is limited to only create the "mysql-libs-compat" packages
# which are needed to satisfy dependencies of application packages
# when installing newer versions of MySQL.
#
# Subsetting is done by Jörg Brühe ( joerg.bruehe@fromdual.com )
# to support the installation of MySQL-Galera-Cluster RPMs
# as offered by FromDual ( www.fromdual.com ) and/or
# Codership ( www.codership.com / www.galeracluster.com ).
#
# ==========

%global mysql_vendor          Oracle and/or its affiliates

%{!?with_debuginfo:              %global nodebuginfo 1}

# Deal with RedHat 5 which doesn't properly identify itself
# This is meant to solve Oracle's problem:
#     Rebuild on OL5/RHEL5 needs following rpmbuild options:
#      rpmbuild --define 'dist .el5' --define 'rhel 5' --define 'el5 1' mysql.spec
%if %(test -f /etc/redhat-release && echo 1 || echo 0)
  %if 0%{?rhel}
    # "rhel" is defined, trust it
  %else
    %global rhel 5
  %endif
%endif

# Define dist tag if not given by platform
%if %{undefined dist}
  # RHEL 5
  %if 0%{?rhel} == 5
    %define dist .el5
  %endif
  # For suse versions see:
  # https://en.opensuse.org/openSUSE:Build_Service_cross_distribution_howto
  %if 0%{?suse_version} == 1110
    %define dist .sle11
  %endif
  %if 0%{?suse_version} == 1310
    %define dist .suse13.1
  %endif
  %if 0%{?suse_version} == 1315
    %define dist .sle12
  %endif
  %if 0%{?suse_version} == 1320
    %define dist .suse13.2
  %endif
  # Still missing?
  %if %{undefined dist}
    %define dist .DIST
  %endif
%endif

# Version for compat libs
%if 0%{?rhel} == 5
%global compatver             5.0.96
%global compatlib             15
%global compatsrc             http://downloads.mysql.com/archives/mysql-5.0/mysql-%{compatver}.tar.gz
%global compatch              mysql-5096-charset-dir.patch
%endif

%if 0%{?rhel} == 6
%global compatver             5.1.73
%global compatlib             16
%global compatsrc             https://cdn.mysql.com/Downloads/MySQL-5.1/mysql-%{compatver}.tar.gz
%global compatch              mysql-5173-charset-dir.patch
%endif

%if 0%{?rhel} == 7
%global compatver             5.5.37
%global compatlib             18
%global compatsrc             https://cdn.mysql.com/Downloads/MySQL-5.5/mysql-%{compatver}.tar.gz
%global compatch              mysql-5537-charset-dir.patch
%endif

# Hack to support el5 where __isa_bits not defined. Note: supports i386 and x86_64 only, sorry.
%if x%{?__isa_bits} == x
%ifarch %{ix86}
%global __isa_bits            32
%endif
%ifarch x86_64
%global __isa_bits            64
%endif
%endif

# Attention: "src_dir" is the old version (5.0.96 or 5.1.73), depends on platform
%global src_dir               mysql-%{compatver}

# No debuginfo for now, ships /usr/sbin/mysqld-debug and libmysqlcliet-debug.a
%if 0%{?nodebuginfo}
%global _enable_debug_package 0
%global debug_package         %{nil}
%global __os_install_post     /usr/lib/rpm/brp-compress %{nil}
%endif

%global license_files_server  %{src_dir}/COPYING %{src_dir}/README

Name:           mysql-wsrep-libs-compat
Summary:        Shared libraries for MySQL %{compatver} database client applications
Group:          Applications/Databases
# "Version = 5.6" because this libs-compat will fit all mysql-wsrep-server-5.6.* packages.
# "fullversion = 5.6.23" for better conflict handling.
Version:        5.6
%global fullversion        5.6.23
Release:        11%{?dist}
License:        Copyright (c) 2000, 2015, %{mysql_vendor}. All rights reserved. Under GPLv2 license as shown in the Description field.
URL:            http://www.mysql.com/
Packager:       FromDual GmbH <joerg.bruehe@fromdual.com>
Vendor:         %{mysql_vendor}
%if 0%{?compatlib}
Source7:        %{compatsrc}
Patch:          %{compatch}
%endif
BuildRequires:  gcc-c++
BuildRequires:  perl
%if 0%{?rhel}
# RedHat has it, SuSE doesn't
BuildRequires:  time
%endif
BuildRequires:  libaio-devel
BuildRequires:  ncurses-devel
BuildRequires:  openssl-devel
BuildRequires:  zlib-devel
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)


Obsoletes:      mysql-libs-compat < %{fullversion}-%{release}
Provides:       mysql-libs-compat = %{fullversion}-%{release}
Provides:       mysql-libs-compat%{?_isa} = %{fullversion}-%{release}
Provides:       MySQL-shared-compat%{?_isa} = %{fullversion}-%{release}
Obsoletes:      MySQL-shared-compat < %{fullversion}-%{release}
%if 0%{?rhel} < 6
# Dealing with RHEL 5 (and compatible ...)
# "mysql" without a subpackage specification is a RedHat convention,
# a package with client, lib, and server parts that blocks the server upgrade.
Obsoletes:      mysql <= %{compatver}
%endif
%if 0%{?rhel} > 5
# Dealing with RHEL 6 and upwards (and compatible ...)
# Directly, we replace "libs" only; but RedHat "client" and "server" need files from "libs"
Provides:       mysql-libs = %{compatver}-%{release}
Obsoletes:      mysql-libs < %{fullversion}-%{release}
Obsoletes:      mysql < %{fullversion}-%{release}
Obsoletes:      mysql-server < %{fullversion}-%{release}
%endif
%if 0%{?rhel} > 6
# Dealing with RHEL 7 and upwards (and compatible ...)
# Above section for RHEL 6 also applies, it deals with "mysql" packages.
# But with RHEL 7, we also get "mariadb" ...
Obsoletes:      mariadb-libs
%endif

%description
The MySQL(TM) software delivers a very fast, multi-threaded, multi-user,
and robust SQL (Structured Query Language) database server.
See the MySQL web site (http://www.mysql.com/) for further information,
including its dual licensing (GNU GPL or commercial licenses).

This package contains the shared libraries for (old) MySQL %{compatver} client
applications.
It is intended for RHEL %{?rhel} and compatible distributions,
to satisfy the dependencies of several applications shipping with that distro
while the MySQL software on the machine is updated to a newer release series.


%prep
%setup -q -T      -a 7 -c -n %{src_dir}
pushd %{src_dir}
%patch -p 1
popd

%build
# Build compat libs
(

%if 0%{?rhel} < 7

# RHEL 5 + 6: MySQL 5.0 / 5.1, using traditional "configure ; make"
export CC="gcc" CXX="g++"
export CFLAGS="%{optflags} -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -fno-strict-aliasing -fwrapv"
export CXXFLAGS="$CFLAGS %{?el6:-felide-constructors} -fno-rtti -fno-exceptions"
pushd %{src_dir}
%configure \
    --with-readline \
    --without-debug \
    --enable-shared \
    --localstatedir=/var/lib/mysql \
    --with-unix-socket-path=/var/lib/mysql/mysql.sock \
    --with-mysqld-user="mysql" \
    --with-extra-charsets=all \
    --enable-local-infile \
    --enable-largefile \
    --enable-thread-safe-client \
%if 0%{?rhel} == 5
    --with-openssl \
    --with-bench \
    --with-innodb \
    --with-berkeley-db \
    --enable-community-features \
    --enable-profiling \
    --with-named-thread-libs="-lpthread" \
%endif
%if 0%{?rhel} == 6
    --with-ssl=%{_prefix} \
    --with-embedded-server \
    --with-big-tables \
    --with-pic \
    --with-plugin-innobase \
    --with-plugin-innodb_plugin \
    --with-plugin-partition \
%endif
    --disable-dependency-tracking
make %{?_smp_mflags}
popd

%endif

)


%install
# Install compat libs
for dir in libmysql libmysql_r ; do
    pushd mysql-%{compatver}/$dir
    make DESTDIR=%{buildroot} install
    popd
done
rm -f %{buildroot}%{_libdir}/mysql/libmysqlclient{,_r}.{a,la,so}

# "charsets/"
MBD=$RPM_BUILD_DIR/mysql-%{mysql_version}/mysql-release-%{mysql_version}
install -d -m 0755 %{buildroot}/usr/share/mysql-%{compatver}/charsets/
install -m 644 mysql-%{compatver}/sql/share/charsets/* %{buildroot}/usr/share/mysql-%{compatver}/charsets/

# Add libdir to linker
install -d -m 0755 %{buildroot}%{_sysconfdir}/ld.so.conf.d
echo "%{_libdir}/mysql" > %{buildroot}%{_sysconfdir}/ld.so.conf.d/mysql-%{_arch}.conf

%check

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-, root, root, -)
%doc %{?license_files_server}
%dir %attr(755, root, root) %{_libdir}/mysql
%attr(644, root, root) %{_sysconfdir}/ld.so.conf.d/mysql-%{_arch}.conf
%{_libdir}/mysql/libmysqlclient.so.%{compatlib}
%{_libdir}/mysql/libmysqlclient.so.%{compatlib}.0.0
%{_libdir}/mysql/libmysqlclient_r.so.%{compatlib}
%{_libdir}/mysql/libmysqlclient_r.so.%{compatlib}.0.0
%dir %attr(755, root, root) %{_datadir}/mysql-%{compatver}
%attr(644, root, root) %{_datadir}/mysql-%{compatver}/charsets/


%changelog
* Thu Sep 03 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- Change the name from "mysql-libs-compat" to "mysql-wsrep-libs-compat".
- Change the compatibility version for RHEL 6 from MySQL 5.1.72 to 5.1.73.
- Start adding settings for RHEL 7, to replace the pre-installed "mariadb-libs 5.5.37".

* Tue Apr 28 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- Improve the dependency / conflicts settings.
- Drop the name component "community", 3rd party builds don't use it.

* Tue Mar 10 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- Sometimes, "libmysqlclient.so" needs the files in "/usr/share/mysql/charsets/",
  so the package should better contain them, but have the path name contain the
  version number to make it independent of other packages.

* Mon Feb 09 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- "g++: command not found" on RedHat 5 + 6, added "BuildRequires: gcc-c++".
- "time" is a RedHat-only package, not needed to build on SuSE.

* Fri Feb 06 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- Add code to identify RedHat 5 during build, "rhel" is not predefined there.
- Start adding "conflicts" directives to get rid of other MySQL library RPMs.
- Try with "CXX=g++".

* Thu Feb 05 2015 Joerg Bruehe <joerg.bruehe@fromdual.com>
- This spec file handles the "compat" RPM for the "libmysqlclient.so" only.
  It is created as a subset of the overall MySQL 5.6.23 spec file
  (file "packaging/rpm-oel/mysql.spec.in" in the source tree),
  see that file's "changelog" section for its history.
- Add "CC=gcc CXX=gcc" as used in 5.0 and 5.1 builds (missing in that 5.6.23 spec file)

