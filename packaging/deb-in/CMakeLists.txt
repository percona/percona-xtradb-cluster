# Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

IF(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  RETURN()
ENDIF()
IF (NOT DEFINED DEB_CODENAME)
  execute_process(
    COMMAND lsb_release -cs
    OUTPUT_VARIABLE DEB_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  SET (DEB_CODENAME ${DEB_CODENAME} CACHE STRING "")
ENDIF()
IF (DEFINED WITH_NDBCLUSTER_STORAGE_ENGINE)
  SET (DEB_BASE_PRODUCT "cluster-")
  SET (DEB_BASE_PRODUCTC "Cluster ")
  SET (DEB_BASE_VERSION ${NDBVERSION})
  SET (DEB_CONTROL_SERVER_CONFLICT_NDB "mysql-community-server, mysql-commercial-server")
  SET (DEB_CONTROL_CLIENT_CONFLICT_NDB "mysql-community-client, mysql-commercial-client")
ELSE()
  SET (DEB_BASE_PRODUCT "")
  SET (DEB_BASE_PRODUCTC "")
  SET (DEB_BASE_VERSION ${VERSION})
  SET (DEB_CONTROL_SERVER_CONFLICT_NDB "mysql-cluster-community-server, mysql-cluster-commercial-server")
  SET (DEB_CONTROL_CLIENT_CONFLICT_NDB "mysql-cluster-community-client, mysql-cluster-commercial-client")
ENDIF()

SET (DEB_BASE_VERSION "${DEB_BASE_VERSION}-${WSREP_VERSION}")
# Timestamp for use in debian/changelog
IF (NOT DEFINED DEB_CHANGELOG_TIMESTAMP)
  execute_process(
    COMMAND date --rfc-2822
    OUTPUT_VARIABLE DEB_CHANGELOG_TIMESTAMP
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  SET (DEB_CHANGELOG_TIMESTAMP ${DEB_CHANGELOG_TIMESTAMP} CACHE STRING "")
ENDIF()

# Commercial or community
IF (DEB_PRODUCT STREQUAL "commercial")
  message (FATAL_ERROR "mysql-wsrep is GPL only, no commercial builds")
  SET (DEB_COPYRIGHT_UPSTREAMNAME "MySQL Commercial Server ${MYSQL_BASE_VERSION}")
  SET (DEB_PRODUCTNAME "${DEB_BASE_PRODUCT}commercial")
  SET (DEB_PRODUCTNAMEC "${DEB_BASE_PRODUCTC}Commercial")
  SET (DEB_NOTPRODUCTNAME "${DEB_BASE_PRODUCT}community")
  SET (DEB_LICENSENAME "Advanced")
  SET (DEB_INSTALL_LICENSEFILE "LICENSE.mysql")
  SET (DEB_SERVERVERSION "${DEB_BASE_VERSION}+commercial.1")
  # List of plugins that are only in commercial packages
  # Plugins that are in both community and commercial should NOT be added here
  SET (DEB_INCLUDE_BINARIES_EXTRA
"
debian/extra/audit_log-plugin
debian/extra/authentication_pam-plugin
debian/extra/authentication_ldap_sasl-plugin
debian/extra/authentication_ldap_simple-plugin
debian/extra/firewall-plugin
debian/extra/keyring_okv-plugin
debian/extra/keyring_encrypted_file-plugin
debian/extra/openssl_udf-plugin
debian/extra/thread_pool-plugin
")
  SET (DEB_INSTALL_SERVER_PLUGINS
"
usr/lib/mysql/plugin/audit_log.so
usr/lib/mysql/plugin/authentication_pam.so
usr/lib/mysql/plugin/authentication_ldap_sasl.so
usr/lib/mysql/plugin/authentication_ldap_simple.so
usr/lib/mysql/plugin/data_masking.so
usr/lib/mysql/plugin/keyring_okv.so
usr/lib/mysql/plugin/keyring_encrypted_file.so
usr/lib/mysql/plugin/openssl_udf.so
usr/lib/mysql/plugin/thread_pool.so
usr/lib/mysql/plugin/firewall.so
usr/lib/mysql/plugin/debug/audit_log.so
usr/lib/mysql/plugin/debug/authentication_pam.so
usr/lib/mysql/plugin/debug/authentication_ldap_sasl.so
usr/lib/mysql/plugin/debug/authentication_ldap_simple.so
usr/lib/mysql/plugin/debug/data_masking.so
usr/lib/mysql/plugin/debug/keyring_okv.so
usr/lib/mysql/plugin/debug/keyring_encrypted_file.so
usr/lib/mysql/plugin/debug/openssl_udf.so
usr/lib/mysql/plugin/debug/thread_pool.so
usr/lib/mysql/plugin/debug/firewall.so
")
  IF (DEB_AWS_SDK)
    SET (DEB_CMAKE_EXTRAS "${DEB_CMAKE_EXTRAS} -DWITH_AWS_SDK=${DEB_AWS_SDK} -DDEB_AWS_SDK=${DEB_AWS_SDK}")
    SET (DEB_INCLUDE_BINARIES_EXTRA "${DEB_INCLUDE_BINARIES_EXTRA}
debian/extra/keyring_aws-plugin
")
    SET (DEB_INSTALL_SERVER_PLUGINS "${DEB_INSTALL_SERVER_PLUGINS}
usr/lib/mysql/plugin/keyring_aws.so
usr/lib/mysql/plugin/debug/keyring_aws.so
")
  ENDIF()
  IF (WITH_CURL)
    SET (DEB_CMAKE_EXTRAS "${DEB_CMAKE_EXTRAS} -DWITH_CURL=${WITH_CURL}")
  ENDIF()
  SET (DEB_REMOVEPATTERN "gpl.in")
ELSE()
  SET (DEB_PRODUCT "${DEB_BASE_PRODUCT}community")
  SET (DEB_COPYRIGHT_UPSTREAMNAME "MySQL Server ${MYSQL_BASE_VERSION}")
  SET (DEB_PRODUCTNAME "${DEB_BASE_PRODUCT}community")
  SET (DEB_PRODUCTNAMEC "${DEB_BASE_PRODUCTC}Community")
  SET (DEB_NOTPRODUCTNAME "${DEB_BASE_PRODUCT}commercial")
  SET (DEB_LICENSENAME "GPL")
  SET (DEB_INSTALL_LICENSEFILE "COPYING")
  SET (DEB_SERVERVERSION "${DEB_BASE_VERSION}+1")
  SET (DEB_REMOVEPATTERN "com.in")
ENDIF()

# mysql-wsrep !
SET (DEB_PRODUCTNAME "wsrep")
SET (DEB_PRODUCTNAMEC "Wsrep")
SET (DEB_PRODUCTSERIES "-5.7")

# For each startup mechanism, files are constant across distro version
SET (FILES_STARTUP_SYSTEMD
"
usr/bin/mysqld_pre_systemd
usr/bin/mysqld_bootstrap
lib/systemd/system/mysql@.service
lib/systemd/system/mysql.service
")
SET (FILES_STARTUP_SYSV
"
etc/init.d/mysql
usr/bin/mysqld_safe
")
SET (FILES_STARTUP_UPSTART
"
Not Yet Implememted: Upstart File List
")

SET (DEB_IGNORE_MISSING "\\")
# Generate debug symbol packages (this is done automatically in Debian9+)
SET (DEB_RULES_STRIP_DEFAULT
"
override_dh_strip:
	dh_strip -pmysql-${DEB_PRODUCTNAME}-server --dbg-package=mysql-${DEB_PRODUCTNAME}-server-dbgsym
	dh_strip -pmysql-${DEB_PRODUCTNAME}-client --dbg-package=mysql-${DEB_PRODUCTNAME}-client-dbgsym
	dh_strip -pmysql-${DEB_PRODUCTNAME}-test --dbg-package=mysql-${DEB_PRODUCTNAME}-test-dbgsym
	dh_strip -pmysql-server
	dh_strip -pmysql-client
	dh_strip -pmysql-testsuite
	dh_strip -plibmysqlclient20 --dbg-package=libmysqlclient20-dbgsym
	dh_strip -plibmysqlclient-dev
")
SET (DEB_CONTROL_DBGSYM_DEFAULT
"
Package: libmysqlclient20-dbgsym
Architecture: any
Section: debug
Depends: libmysqlclient20 (=\${binary:Version}), \${misc:Depends}
Description: Debugging symbols for client library

Package: mysql-${DEB_PRODUCTNAME}-test-dbgsym
Architecture: any
Section: debug
Depends: mysql-${DEB_PRODUCTNAME}-test (= \${binary:Version}), \${misc:Depends}
Description: Debugging symbols for test suite

Package: mysql-${DEB_PRODUCTNAME}-server-dbgsym
Architecture: any
Section: debug
Depends: mysql-${DEB_PRODUCTNAME}-server (= \${binary:Version}), \${misc:Depends}
Description: Debugging symbols for server

Package: mysql-${DEB_PRODUCTNAME}-client-dbgsym
Architecture: any
Section: debug
Depends: mysql-${DEB_PRODUCTNAME}-client (= \${binary:Version}), \${misc:Depends}
Description: Debugging symbols for client
")

# Platform specifics. The differences are generally only distro version
# and whether or not Systemd and/or apparmor are available
IF (DEB_CODENAME STREQUAL "wheezy")
  SET (DEB_PLATFORMRELEASE "debian7")
  # Following the Oracle example, we package for Debian 7
  # using only SysV init and no Apparmor.
  SET (DEB_CONTROL_BDEPS "")
  SET (DEB_INSTALL_SOURCE_XZ "")
  SET (DEB_RULES_INSTALL_SYSTEMD "")
  SET (DEB_RULES_INSTALL_APPARMOR "")
  SET (DEB_RULES_APPARMOR_LOAD "")
  SET (DEB_RULES_SYSTEMD_ENABLE "")
  SET (DEB_RULES_SYSTEMD_START "")
  SET (DEB_INSTALL_SERVER_SYSTEMD "")
  SET (DEB_INSTALL_SERVER_APPARMOR "")
  SET (DEB_SERVICE_SERVER_EXECPRE "")
  SET (DEB_SERVICE_SERVER_EXECPOST "")
  SET (DEB_INIT_APPARMOR "")
  SET (DEB_STARTUP "SYSV")
  SET (DEB_IGNORE_MISSING "-Xusr/bin/mysqld_multi \\")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "jessie")
  SET (DEB_PLATFORMRELEASE "debian8")
  # Improving on Oracle, we would prefer to package for Debian 8
  # using both SysV init and Systemd, but still no Apparmor.
  SET (DEB_CONTROL_BDEPS "dh-systemd")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR "")
  SET (DEB_RULES_APPARMOR_LOAD "")
  SET (DEB_SYSTEMD_SERVICE_NAME "mysql")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --no-enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start
  usr/lib/tmpfiles.d/mysql.conf")
  SET (DEB_INSTALL_SERVER_APPARMOR "")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_SERVICE_SERVER_EXECPOST
    "ExecStartPost=/usr/share/mysql/mysql-systemd-start post")
  SET (DEB_INIT_APPARMOR "")
  # Debian 8 comes with both SysV init and Systemd,
  # but "scripts/CMakeLists.txt" enforces an either-or.
  # SET (DEB_STARTUP "SYSV SYSTEMD")
  # For now, we go with Systemd only.
  SET (DEB_STARTUP "SYSTEMD")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "stretch")
  SET (DEB_PLATFORMRELEASE "debian9")
  SET (DEB_CONTROL_BDEPS "dh-systemd")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR "")
  SET (DEB_RULES_APPARMOR_LOAD "")
  SET (DEB_SYSTEMD_SERVICE_NAME "mysql")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start
  usr/lib/tmpfiles.d/mysql.conf")
  SET (DEB_INSTALL_SERVER_APPARMOR "")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_SERVICE_SERVER_EXECPOST
    "ExecStartPost=/usr/share/mysql/mysql-systemd-start post")
  SET (DEB_INIT_APPARMOR "")
  SET (DEB_STARTUP "SYSTEMD")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
ELSEIF(DEB_CODENAME STREQUAL "buster")
  SET (DEB_PLATFORMRELEASE "debian10")
  SET (DEB_CONTROL_BDEPS ", dh-systemd")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR "")
  SET (DEB_RULES_APPARMOR_LOAD "")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_INIT_APPARMOR "")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
  IF (DEFINED WITH_NDBCLUSTER_STORAGE_ENGINE)
    # Cluster code triggers a lot of new warnings with GCC 7.2
    SET (DEB_DEBUG_MAINTAINER "-DMYSQL_MAINTAINER_MODE=0")
  ENDIF()
ELSEIF(DEB_CODENAME STREQUAL "sid")
  IF (DEFINED DEB_GCC_SNAPSHOT)
    SET (DEB_CMAKE_EXTRAS "${DEB_CMAKE_EXTRAS} -DCMAKE_C_COMPILER=/usr/lib/gcc-snapshot/bin/gcc -DCMAKE_CXX_COMPILER=/usr/lib/gcc-snapshot/bin/g++ -DMYSQL_MAINTAINER_MODE=0 -DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
  ENDIF()
  SET (DEB_PLATFORMRELEASE "debianunstable")
  SET (DEB_CONTROL_BDEPS "dh-systemd")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR "")
  SET (DEB_RULES_APPARMOR_LOAD "")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_INIT_APPARMOR "")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
ELSEIF(DEB_CODENAME STREQUAL "trusty")
  SET (DEB_PLATFORMRELEASE "ubuntu14.04")
  SET (DEB_CONTROL_BDEPS "dh-apparmor")
  SET (DEB_INSTALL_SOURCE_XZ "")
  SET (DEB_RULES_INSTALL_SYSTEMD "")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld")
  SET (DEB_RULES_SYSTEMD_ENABLE "")
  SET (DEB_RULES_SYSTEMD_START "")
  SET (DEB_INSTALL_SERVER_SYSTEMD "")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE "")
  SET (DEB_SERVICE_SERVER_EXECPOST "")
  SET (DEB_INIT_APPARMOR "/lib/init/apparmor-profile-load usr.sbin.mysqld")
  SET (DEB_STARTUP "SYSV")
  SET (DEB_IGNORE_MISSING "-Xusr/bin/mysqld_multi \\")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "xenial")
  SET (DEB_PLATFORMRELEASE "ubuntu16.04")
  SET (DEB_CONTROL_BDEPS "dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld")
  SET (DEB_SYSTEMD_SERVICE_NAME "mysql")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --no-enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start
  usr/lib/tmpfiles.d/mysql.conf")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_SERVICE_SERVER_EXECPOST
    "ExecStartPost=/usr/share/mysql/mysql-systemd-start post")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_STARTUP "SYSTEMD")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "zesty")
  SET (DEB_PLATFORMRELEASE "ubuntu17.04")
  SET (DEB_CONTROL_BDEPS "dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld")
  SET (DEB_SYSTEMD_SERVICE_NAME "mysql")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --no-enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_SERVICE_SERVER_EXECPOST
    "ExecStartPost=/usr/share/mysql/mysql-systemd-start post")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "artful")
  SET (DEB_PLATFORMRELEASE "ubuntu17.10")
  SET (DEB_CONTROL_BDEPS "dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_CONTROL_DBGSYM ${DEB_CONTROL_DBGSYM_DEFAULT})
  SET (DEB_RULES_STRIP ${DEB_RULES_STRIP_DEFAULT})
ELSEIF(DEB_CODENAME STREQUAL "bionic")
  SET (DEB_PLATFORMRELEASE "ubuntu18.04")
  SET (DEB_CONTROL_BDEPS "dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server${DEB_PRODUCTSERIES} --profile-name=usr.sbin.mysqld")
  SET (DEB_SYSTEMD_SERVICE_NAME "mysql")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql --no-enable")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start
  usr/lib/tmpfiles.d/mysql.conf")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_SERVICE_SERVER_EXECPOST
    "ExecStartPost=/usr/share/mysql/mysql-systemd-start post")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_STARTUP "SYSTEMD")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
ELSEIF(DEB_CODENAME STREQUAL "cosmic")
  SET (DEB_PLATFORMRELEASE "ubuntu18.10")
  SET (DEB_CONTROL_BDEPS "dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
    "install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_RULES_APPARMOR_LOAD
    "dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
ELSEIF(DEB_CODENAME STREQUAL "disco")
  SET (DEB_PLATFORMRELEASE "ubuntu19.04")
  SET (DEB_CONTROL_BDEPS ", dh-apparmor, dh-systemd (>=1.5)")
  SET (DEB_INSTALL_SOURCE_XZ "../*.tar.xz usr/src/mysql/")
  SET (DEB_RULES_INSTALL_SYSTEMD
    "install -m 0755 debian/extra/mysql-systemd-start debian/tmp/usr/share/mysql/")
  SET (DEB_RULES_INSTALL_APPARMOR
  "
	install -g root -o root -m 0644 -D debian/extra/apparmor-profile debian/tmp/etc/apparmor.d/usr.sbin.mysqld
  ")
  SET (DEB_RULES_APPARMOR_LOAD
  "
	dh_apparmor -pmysql-${DEB_PRODUCTNAME}-server --profile-name=usr.sbin.mysqld
  ")
  SET (DEB_RULES_SYSTEMD_ENABLE "dh_systemd_enable --name=mysql")
  SET (DEB_RULES_SYSTEMD_START "dh_systemd_start --restart-after-upgrade")
  SET (DEB_INSTALL_SERVER_SYSTEMD "usr/share/mysql/mysql-systemd-start")
  SET (DEB_INSTALL_SERVER_APPARMOR "etc/apparmor.d/usr.sbin.mysqld")
  SET (DEB_SERVICE_SERVER_EXECPRE
    "ExecStartPre=/usr/share/mysql/mysql-systemd-start pre")
  SET (DEB_INIT_APPARMOR "/lib/apparmor/profile-load usr.sbin.mysqld")
  SET (DEB_CONTROL_DBGSYM "")
  SET (DEB_RULES_STRIP "")
  IF (DEFINED WITH_NDBCLUSTER_STORAGE_ENGINE)
    # Cluster code triggers a lot of new warnings with GCC 7.2
    SET (DEB_DEBUG_MAINTAINER "-DMYSQL_MAINTAINER_MODE=0")
  ENDIF()
ELSE()
  MESSAGE(STATUS
    "Skipping deb packaging on unsupported platform ${DEB_CODENAME}.")
  RETURN()
ENDIF()

IF (DEFINED WITH_NDBCLUSTER_STORAGE_ENGINE)
  INCLUDE(deb_ndb.cmake)
ENDIF()
# All files are configured and copied to the debian/ directory, which is used 
# by debuild to make the packages
# 5.7.11 builds out-of-source. Hack: have duplicate debian/
SET (DEB_ROOT ${CMAKE_SOURCE_DIR}/packaging/deb-in)
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/debian)
FILE(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/debian)
FILE(GLOB_RECURSE SOURCEFILES RELATIVE "${DEB_ROOT}" "${DEB_ROOT}/*")
# Skip the copyright file from the "other" product type
LIST(REMOVE_ITEM SOURCEFILES "copyright.${DEB_REMOVEPATTERN}")
LIST(REMOVE_ITEM SOURCEFILES "CMakeLists.txt")
# Combine the list of all startup files to be included,
# skip Debian magical init files unless needed
IF(DEB_PRODUCTNAME STREQUAL "wsrep")
    SET(DEB_SERVER_INSTALL_WSREP "usr/share/mysql/README-wsrep /usr/share/doc/mysql-${DEB_PRODUCTNAME}-server")
    LIST(REMOVE_ITEM SOURCEFILES "mysql-common.install.in")
    LIST(REMOVE_ITEM SOURCEFILES "mysql-common.dirs.in")
    LIST(REMOVE_ITEM SOURCEFILES "mysql-common.postinst.in")
    LIST(REMOVE_ITEM SOURCEFILES "mysql-common.postrm.in")
ENDIF()
IF(DEB_STARTUP MATCHES ".*SYSV.*")
  SET (DEB_FILES_STARTUP ${FILES_STARTUP_SYSV})
ELSE()
  LIST(REMOVE_ITEM SOURCEFILES
  "mysql-packagesource-server-SERIES.mysql.init.in")
ENDIF()
IF(DEB_STARTUP MATCHES ".*SYSTEMD.*")
  SET (DEB_FILES_STARTUP
    ${DEB_FILES_STARTUP}
    ${FILES_STARTUP_SYSTEMD})
ELSE()
  LIST(REMOVE_ITEM SOURCEFILES
  "mysql-packagesource-server-SERIES.mysql.service.in")
ENDIF()
# Don't install systemd file on systems without systemd
IF(DEB_INSTALL_SERVER_SYSTEMD STREQUAL "")
	LIST(REMOVE_ITEM SOURCEFILES "mysql-packagesource-server.mysql.service.in")
ENDIF()

FOREACH (SOURCEFILE ${SOURCEFILES})
  STRING(REGEX REPLACE "-packagesource-" "-${DEB_PRODUCTNAME}-"
    TMPFILE ${SOURCEFILE})
  STRING(REGEX REPLACE "-SERIES" "${DEB_PRODUCTSERIES}" TMPFILE ${TMPFILE})
  # Strip away the input file endings from the destination filenames
  STRING(REGEX REPLACE ".in$" "" TMPFILE ${TMPFILE})
  STRING(REGEX REPLACE ".gpl$" "" TMPFILE ${TMPFILE})
  STRING(REGEX REPLACE ".com$" "" DESTFILE ${TMPFILE})
  CONFIGURE_FILE("${DEB_ROOT}/${SOURCEFILE}"
    "${CMAKE_BINARY_DIR}/debian/${DESTFILE}" @ONLY)
  CONFIGURE_FILE("${DEB_ROOT}/${SOURCEFILE}"
    "${CMAKE_SOURCE_DIR}/debian/${DESTFILE}" @ONLY)
ENDFOREACH()
EXECUTE_PROCESS(
  COMMAND chmod +x ${CMAKE_BINARY_DIR}/debian/rules
)
IF(DEB_PRODUCTNAME STREQUAL "wsrep")
FILE(APPEND "${CMAKE_BINARY_DIR}/debian/control" "
Package: mysql-common
Architecture: any
Pre-depends: debconf (>= 0.2.17),
             \${misc:Pre-Depends}
Multi-Arch: foreign
Depends: \${misc:Depends},
         \${shlibs:Depends},
         mysql-wsrep-common-5.7
Description: MySQL Common
 The MySQL(TM) software delivers a very fast, multi-threaded, multi-user,
 and robust SQL (Structured Query Language) database server. MySQL Server
 is intended for mission-critical, heavy-load production systems as well
 as for embedding into mass-deployed software. MySQL is a trademark of
 Oracle. This package contains common files needed by MySQL client
 library and the MySQL database server.
 This package is a dummy replacement for the case when using mysql WSREP
 implementation to satisfy dependencies of other packages
 like libmysqlclient18
")
ENDIF()


