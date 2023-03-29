
OPTION(WITH_GALERA_DEV "Automatically build & copy & install debug galera library and garbd" OFF)
OPTION(WITH_PXB_DEV "Automatically build & copy & install PXB 2.4 and 8.0" OFF)

if(WITH_GALERA_DEV)
  MESSAGE(WARNING WITH_GALERA_DEV is no longer required, galera is built as part of the normal PXC build)
endif(WITH_GALERA_DEV)

if(WITH_PXB_DEV)
  include(ExternalProject)
  #Note: requires a different boost version than pxc8.0
  ExternalProject_Add(pxb24
    GIT_REPOSITORY https://github.com/percona/percona-xtrabackup.git
    GIT_TAG percona-xtrabackup-2.4.27
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    INSTALL_DIR "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-2.4/"
    CMAKE_ARGS 
    -DDOWNLOAD_BOOST=1
    -DWITH_BOOST=boost
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DMYSQL_MAINTAINER_MODE=OFF
    -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
    -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}
    )
  INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-2.4/"
    DESTINATION "bin/pxc_extra/pxb-2.4" USE_SOURCE_PERMISSIONS)

  ExternalProject_Add(pxb80
    GIT_REPOSITORY https://github.com/percona/percona-xtrabackup.git
    GIT_TAG percona-xtrabackup-8.0.32-25
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    INSTALL_DIR "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-8.0/"
    CMAKE_ARGS 
    -DWITH_BOOST=${WITH_BOOST}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DMYSQL_MAINTAINER_MODE=OFF
    -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}
    -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}
    )
  INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/scripts/pxc_extra/pxb-8.0/"
    DESTINATION "bin/pxc_extra/pxb-8.0" USE_SOURCE_PERMISSIONS)
endif()
