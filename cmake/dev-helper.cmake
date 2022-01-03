
OPTION(WITH_GALERA_DEV "Automatically build & copy & install debug galera library and garbd" OFF)
OPTION(WITH_PXB_DEV "Automatically build & copy & install PXB 2.4 and 8.0" OFF)

if(WITH_GALERA_DEV)
  INCLUDE(ProcessorCount)
  ProcessorCount(CPU_COUNT)

  IF(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG" OR WITH_DEBUG)
    SET(GALERA_DEBUG 0)
  ELSE()
    SET(GALERA_DEBUG 2)
  ENDIF()

  MESSAGE(STATUS "Configuring Galera to use debug=${GALERA_DEBUG}")

  # Add a custom target for later refreshes
  ADD_CUSTOM_TARGET(galera ALL scons -j ${CPU_COUNT} tests=0 debug=${GALERA_DEBUG}
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/garb/garbd"
    "${CMAKE_CURRENT_BINARY_DIR}/runtime_output_directory/garbd"
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/libgalera_smm.so"
    "${CMAKE_CURRENT_BINARY_DIR}/library_output_directory/libgalera_smm.so"
    WORKING_DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera"
    BYPRODUCTS
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/garb/garbd"
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/libgalera_smm.so"
  )

  INSTALL(PROGRAMS
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/garb/garbd"
    DESTINATION bin)
  INSTALL(PROGRAMS
    "${CMAKE_CURRENT_SOURCE_DIR}/percona-xtradb-cluster-galera/libgalera_smm.so"
    DESTINATION lib)
endif(WITH_GALERA_DEV)

if(WITH_PXB_DEV)
  include(ExternalProject)
  #Note: requires a different boost version than pxc8.0
  ExternalProject_Add(pxb24
    GIT_REPOSITORY https://github.com/percona/percona-xtrabackup.git
    GIT_TAG percona-xtrabackup-2.4.24
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    INSTALL_DIR "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-2.4/"
    CMAKE_ARGS 
    -DDOWNLOAD_BOOST=1
    -DWITH_BOOST=boost
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DMYSQL_MAINTAINER_MODE=OFF
    )
  INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-2.4/"
    DESTINATION "bin/pxc_extra/pxb-2.4" USE_SOURCE_PERMISSIONS)

  ExternalProject_Add(pxb80
    GIT_REPOSITORY https://github.com/percona/percona-xtrabackup.git
    GIT_TAG percona-xtrabackup-8.0.26-18
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    INSTALL_DIR "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-8.0/"
    CMAKE_ARGS 
    -DWITH_BOOST=${WITH_BOOST}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DMYSQL_MAINTAINER_MODE=OFF
    )
  INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-8.0/"
    DESTINATION "bin/pxc_extra/pxb-8.0" USE_SOURCE_PERMISSIONS)
endif()
