# Generate percona-xtradb-cluster.spec from template
# This file is processed by CMake's CONFIGURE_FILE with @ONLY mode,
# substituting @VARIABLE@ markers with CMake variable values.
# Mirrors the Percona Server packaging template-generation pattern.

IF(NOT LINUX)
  RETURN()
ENDIF()

# Derive Percona XtraDB Cluster version from MYSQL_VERSION_EXTRA (strip leading dash)
STRING(REGEX REPLACE "^-" "" PERCONA_SERVER_VERSION "${MYSQL_VERSION_EXTRA}")

# Backwards-compat alias: the spec template uses @PERCONA_VERSION@
# (matches the historical sed-substitution pattern). PS migrated to
# PERCONA_SERVER_VERSION; PXC keeps both as aliases during the transition.
SET(PERCONA_VERSION "${PERCONA_SERVER_VERSION}")

# Git revision for the spec
IF(NOT DEFINED PERCONA_REVISION)
  EXECUTE_PROCESS(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PERCONA_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  IF(NOT PERCONA_REVISION)
    SET(PERCONA_REVISION "unknown")
  ENDIF()
ENDIF()
# Backwards-compat alias for spec's @REVISION@ marker
SET(REVISION "${PERCONA_REVISION}")

# RPM release number
IF(NOT DEFINED RPM_RELEASE)
  SET(RPM_RELEASE "1")
ENDIF()

# DEB release number
IF(NOT DEFINED DEB_RELEASE)
  SET(DEB_RELEASE "1")
ENDIF()

# WSREP version: derived from wsrep-API header + cmake/wsrep-lib.cmake.
# Prefer caller-provided value; fall back to extraction at configure time.
IF(NOT DEFINED WSREP_VERSION)
  SET(WSREP_API_HEADER "${CMAKE_SOURCE_DIR}/wsrep-lib/wsrep-API/v26/wsrep_api.h")
  SET(WSREP_LIB_CMAKE  "${CMAKE_SOURCE_DIR}/cmake/wsrep-lib.cmake")
  IF(EXISTS "${WSREP_API_HEADER}" AND EXISTS "${WSREP_LIB_CMAKE}")
    EXECUTE_PROCESS(
      COMMAND grep WSREP_INTERFACE_VERSION ${WSREP_API_HEADER}
      COMMAND cut -d "\"" -f 2
      OUTPUT_VARIABLE WSREP_INTERFACE_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    EXECUTE_PROCESS(
      COMMAND grep "SET(WSREP_PATCH_VERSION" ${WSREP_LIB_CMAKE}
      COMMAND cut -d "\"" -f 2
      OUTPUT_VARIABLE WSREP_PATCH_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    IF(WSREP_INTERFACE_VERSION AND WSREP_PATCH_VERSION)
      SET(WSREP_VERSION "${WSREP_INTERFACE_VERSION}.${WSREP_PATCH_VERSION}")
    ENDIF()
  ENDIF()
  IF(NOT WSREP_VERSION)
    SET(WSREP_VERSION "unknown")
  ENDIF()
ENDIF()

# Copyright year (matches PS spec.in's @MYSQL_COPYRIGHT_YEAR@)
STRING(TIMESTAMP MYSQL_COPYRIGHT_YEAR "%Y")

# Generate the RPM spec
SET(SPECFILENAME "percona-xtradb-cluster.spec")
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster.spec.in")
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster.spec.in
    ${CMAKE_CURRENT_BINARY_DIR}/${SPECFILENAME}
    @ONLY
  )
  MESSAGE(STATUS "Generated ${SPECFILENAME} (PERCONA_VERSION=${PERCONA_SERVER_VERSION}, REVISION=${PERCONA_REVISION}, WSREP=${WSREP_VERSION})")
ENDIF()

# Generate debian/rules from template if .in exists
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debian/rules.in")
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/debian/rules.in
    ${CMAKE_CURRENT_BINARY_DIR}/debian/rules
    @ONLY
  )
  MESSAGE(STATUS "Generated debian/rules from rules.in")
ENDIF()
