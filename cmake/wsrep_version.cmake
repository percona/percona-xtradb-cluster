# Copyright (c) 2023, Percona and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

# We need to generate a proper spec file even without --with-wsrep flag,
# so WSREP_VERSION is produced regardless

IF (NOT WITH_WSREP)
  RETURN()
ENDIF()

FILE(READ "${CMAKE_SOURCE_DIR}/WSREP_VERSION" wVER)
STRING(REGEX MATCH "WSREP_VERSION_API=([0-9]*)" _ ${wVER})
SET(WSREP_API_VERSION ${CMAKE_MATCH_1})
STRING(REGEX MATCH "WSREP_VERSION_PATCH=([0-9]*)" _ ${wVER})
SET(WSREP_PATCH_VERSION ${CMAKE_MATCH_1})

# Obtain patch revision number
SET(WSREP_REVISION $ENV{WSREP_REV})
IF(NOT WSREP_REVISION)
  SET(WSREP_REVISION "XXXX" CACHE STRING "WSREP revision")
ENDIF()

SET(WSREP_VERSION "${WSREP_API_VERSION}.${WSREP_PATCH_VERSION}"
  CACHE STRING "WSREP version")

MESSAGE(STATUS "* WSREP_API_VERSION is ${WSREP_API_VERSION}")
MESSAGE(STATUS "* WSREP_PATCH_VERSION is ${WSREP_PATCH_VERSION}")
MESSAGE(STATUS "* WSREP_VERSION is ${WSREP_VERSION}")
