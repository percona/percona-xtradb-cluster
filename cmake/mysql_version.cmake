# Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA 

#
# Global constants, only to be changed between major releases.
#

SET(SHARED_LIB_MAJOR_VERSION "21")
SET(SHARED_LIB_MINOR_VERSION "0")
SET(PROTOCOL_VERSION "10")

# Generate "something" to trigger cmake rerun when VERSION changes
CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/VERSION
  ${CMAKE_BINARY_DIR}/VERSION.dep
)

# Read value for a variable from VERSION.

MACRO(MYSQL_GET_CONFIG_VALUE keyword var)
 IF(NOT ${var})
   FILE (STRINGS ${CMAKE_SOURCE_DIR}/VERSION str REGEX "^[ ]*${keyword}=")
   IF(str)
     STRING(REPLACE "${keyword}=" "" str ${str})
     STRING(REGEX REPLACE  "[ ].*" ""  str "${str}")
     SET(${var} ${str})
   ENDIF()
 ENDIF()
ENDMACRO()


# Read mysql version for configure script

MACRO(GET_MYSQL_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_MAJOR" MAJOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_MINOR" MINOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_PATCH" PATCH_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_EXTRA" EXTRA_VERSION)

  IF(NOT DEFINED MAJOR_VERSION OR
     NOT DEFINED MINOR_VERSION OR
     NOT DEFINED PATCH_VERSION)
    MESSAGE(FATAL_ERROR "VERSION file cannot be parsed.")
  ENDIF()

  SET(VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}${EXTRA_VERSION}")
  MESSAGE(STATUS "MySQL ${VERSION}")
  SET(MYSQL_BASE_VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}" CACHE INTERNAL "MySQL Base version")
  SET(MYSQL_NO_DASH_VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
  STRING(REGEX REPLACE "^-" "." MYSQL_VERSION_EXTRA_DOT "${EXTRA_VERSION}")

  # Use NDBVERSION irregardless of whether this is Cluster or not, if not
  # then the regex will be ignored anyway.
  STRING(REGEX REPLACE "^.*-ndb-" "" NDBVERSION "${VERSION}")
  STRING(REPLACE "-" "_" MYSQL_RPM_VERSION "${NDBVERSION}")
  MATH(EXPR MYSQL_VERSION_ID
    "10000*${MAJOR_VERSION} + 100*${MINOR_VERSION} + ${PATCH_VERSION}")
  MARK_AS_ADVANCED(VERSION MYSQL_VERSION_ID MYSQL_BASE_VERSION)
  SET(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
ENDMACRO()

# Get mysql version and other interesting variables
GET_MYSQL_VERSION()

SET(SHARED_LIB_PATCH_VERSION ${PATCH_VERSION})

SET(MYSQL_TCP_PORT_DEFAULT "3306")

IF(NOT MYSQL_TCP_PORT)
  SET(MYSQL_TCP_PORT ${MYSQL_TCP_PORT_DEFAULT})
  SET(MYSQL_TCP_PORT_DEFAULT "0")
ELSEIF(MYSQL_TCP_PORT EQUAL MYSQL_TCP_PORT_DEFAULT)
  SET(MYSQL_TCP_PORT_DEFAULT "0")
ENDIF()


IF(NOT MYSQL_UNIX_ADDR)
  SET(MYSQL_UNIX_ADDR "/tmp/mysql.sock")
ENDIF()
IF(NOT COMPILATION_COMMENT)
  SET(COMPILATION_COMMENT "Source distribution")
ENDIF()

# Get the sys schema version from the mysql_sys_schema.sql file
# however if compiling without performance schema, always use version 1.0.0
MACRO(GET_SYS_SCHEMA_VERSION)
  FILE (STRINGS ${CMAKE_SOURCE_DIR}/scripts/mysql_sys_schema.sql str REGEX "SELECT \\'([0-9]+\\.[0-9]+\\.[0-9]+)\\' AS sys_version")
  IF(str)
    STRING(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" SYS_SCHEMA_VERSION "${str}")
  ENDIF()
ENDMACRO()

GET_SYS_SCHEMA_VERSION()

