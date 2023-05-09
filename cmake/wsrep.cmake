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

# PXC needs this because it still support execvpe approach for forking
# while initiating SST.
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(execvpe HAVE_EXECVPE)

IF (NOT WITH_WSREP)
  RETURN()
ENDIF()

# The code section which fetches the wsrep version
# is moved to its own wsrep_version.cmake file.

SET(WSREP_C_FLAGS   "-DWITH_WSREP -DWSREP_PROC_INFO -DMYSQL_MAX_VARIABLE_VALUE_LEN=4096")
ADD_DEFINITIONS(-DWSREP_PS_VAR_VALUE_BUFF_SIZE=4096)
ADD_DEFINITIONS(-DWSREP_PS_VAR_VALUE_BUFF_SIZE_STR="4096")
SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${WSREP_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSREP_C_FLAGS}")
SET(COMPILATION_COMMENT "${COMPILATION_COMMENT}, wsrep_${WSREP_VERSION}")
SET(WITH_EMBEDDED_SERVER OFF)
SET(WITH_INNODB_DISALLOW_WRITES ON)
SET(WITH_INNODB_MEMCACHED ON)
