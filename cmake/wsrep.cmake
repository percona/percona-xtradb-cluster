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

# Obtain wsrep API version
EXECUTE_PROCESS(
  COMMAND sh -c "grep WSREP_INTERFACE_VERSION wsrep/wsrep_api.h | cut -d '\"' -f 2"
  OUTPUT_VARIABLE WSREP_API_VERSION
  RESULT_VARIABLE RESULT
)
#FILE(WRITE "wsrep_config" "Debug: WSREP_VERSION result: ${RESULT}\n")
STRING(REGEX REPLACE "(\r?\n)+$" "" WSREP_API_VERSION "${WSREP_API_VERSION}")

#SET(WSREP_VERSION "grep WSREP_INTERFACE_VERSION wsrep/wsrep_api.h | cut -d '\"' -f 2")

# Set the patch version
SET(WSPATCH_VERSION 1)

# Obtain patch revision number
SET(WSPATCH_REVNO $ENV{WSREP_REV})
IF(NOT WSPATCH_REVNO)
  EXECUTE_PROCESS(
    COMMAND bzr revno
    OUTPUT_VARIABLE WSPATCH_REVNO
    RESULT_VARIABLE RESULT
  )
#  FILE(APPEND "wsrep_config" "Debug: WSPATCH_REVNO result: ${RESULT}\n")
ENDIF()
IF(NOT WSPATCH_REVNO)
  SET(WSPATCH_REVNO "XXXX")
ENDIF()

SET(WSREP_VERSION "${WSREP_API_VERSION}.${WSPATCH_VERSION}.r${WSPATCH_REVNO}")
