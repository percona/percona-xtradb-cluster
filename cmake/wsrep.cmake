# Copyright (c) 2011, Codership Oy <info@codership.com>.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA 

# We need to generate a proper spec file even without --with-wsrep flag,
# so WSREP_VERSION is produced regardless

# Set the patch version
SET(WSREP_PATCH_VERSION "39")
# PXC needs this because it still support execvpe approach for forking
# while initiating SST.
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(execvpe HAVE_EXECVPE)

# Obtain patch revision number
SET(WSREP_REVISION $ENV{WSREP_REV})
IF(NOT WSREP_REVISION)
  SET(WSREP_REVISION "XXXX" CACHE STRING "WSREP revision")
ENDIF()


# Obtain wsrep API version
EXECUTE_PROCESS(
  COMMAND sh -c "grep WSREP_INTERFACE_VERSION ${MySQL_SOURCE_DIR}/wsrep/src/wsrep_api.h | cut -d '\"' -f 2"
  OUTPUT_VARIABLE WSREP_API_VERSION
  RESULT_VARIABLE RESULT
)
#FILE(WRITE "wsrep_config" "Debug: WSREP_API_VERSION result: ${RESULT}\n")
STRING(REGEX REPLACE "(\r?\n)+$" "" WSREP_API_VERSION "${WSREP_API_VERSION}")

SET(WSREP_VERSION "${WSREP_API_VERSION}.${WSREP_PATCH_VERSION}"
  CACHE STRING "WSREP version")

OPTION(WITH_WSREP "WSREP replication API (to use, e.g. Galera Replication library)" ON)
IF (WITH_WSREP)
  SET(WSREP_C_FLAGS   "-DWITH_WSREP -DWSREP_PROC_INFO -DMYSQL_MAX_VARIABLE_VALUE_LEN=4096")
  SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${WSREP_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSREP_C_FLAGS}")
  SET(COMPILATION_COMMENT "${COMPILATION_COMMENT}, wsrep_${WSREP_VERSION}")
  SET(WITH_EMBEDDED_SERVER OFF)
  SET(WITH_INNODB_DISALLOW_WRITES ON)
  SET(WITH_INNODB_MEMCACHED ON)
ENDIF()

#
