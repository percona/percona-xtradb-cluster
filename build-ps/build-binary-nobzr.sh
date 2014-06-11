#!/bin/sh
#
# Execute this tool to setup the environment and build binary releases
# for Percona XtraDB Cluster starting from a fresh tree.
#
# Usage: build-binary.sh [target dir]
# The default target directory is the current directory. If it is not
# supplied and the current directory is not empty, it will issue an error in
# order to avoid polluting the current directory after a test run.
#

# Bail out on errors, be strict
set -ue

# Examine parameters
TARGET="$(uname -m)"
TARGET_CFLAGS=''
QUIET='VERBOSE=1'
CMAKE_BUILD_TYPE='RelWithDebInfo'
DEBUG_COMMENT=''
WITH_JEMALLOC=''
COPYGALERA=0
DEBUG_EXTNAME=''

# Some programs that may be overriden
TAR=${TAR:-tar}

# Check if we have a functional getopt(1)
if ! getopt --test
then
    go_out="$(getopt --options=iqGdvj: \
        --longoptions=i686,quiet,copygalera,debug,valgrind,with-jemalloc: \
        --name="$(basename "$0")" -- "$@")"
    test $? -eq 0 || exit 1
    eval set -- $go_out
fi

for arg
do
    case "$arg" in
    -- ) shift; break;;
    -i | --i686 )
        shift
        TARGET="i686"
        TARGET_CFLAGS="-m32 -march=i686"
        ;;
    -d | --debug )
        shift
        CMAKE_BUILD_TYPE='Debug'
        BUILD_COMMENT="${BUILD_COMMENT:-}-debug"
        DEBUG_EXTNAME='-DDEBUG_EXTNAME=OFF'
        ;;
    -G | --copygalera)
        shift
        COPYGALERA=1
        ;;
    -v | --valgrind )
        shift
        CMAKE_OPTS="${CMAKE_OPTS:-} -DWITH_VALGRIND=ON"
        BUILD_COMMENT="${BUILD_COMMENT:-}-valgrind"
        ;;
    -q | --quiet )
        shift
        QUIET=''
        ;;
    -j | --with-jemalloc )
        shift
        WITH_JEMALLOC="$1"
        shift
        ;;
    esac
done

# Working directory
if test "$#" -eq 0
then
    WORKDIR="$(pwd)"
    
    # Check that the current directory is not empty
    if test "x$(echo *)" != "x*"
    then
        echo >&2 \
            "Current directory is not empty. Use $0 . to force build in ."
        exit 1
    fi

elif test "$#" -eq 1
then
    WORKDIR="$1"

    # Check that the provided directory exists and is a directory
    if ! test -d "$WORKDIR"
    then
        echo >&2 "$WORKDIR is not a directory"
        exit 1
    fi

else
    echo >&2 "Usage: $0 [target dir]"
    exit 1

fi

WORKDIR="$(cd "$WORKDIR"; pwd)"

SOURCEDIR="$(cd $(dirname "$0"); cd ..; pwd)"
test -e "$SOURCEDIR/Makefile-pxc" || exit 2

# Test for the galera sources
if [[ $COPYGALERA -eq 0 ]] && ! test -d "$SOURCEDIR/percona-xtradb-cluster-galera"
then
    echo >&2 "Subdir percona-xtradb-cluster-galera not found"
    exit 1
fi

# The number of processors is a good default for -j
if test -e "/proc/cpuinfo"
then
    PROCESSORS="$(grep -c ^processor /proc/cpuinfo)"
else
    PROCESSORS=4
fi

# Extract version from the Makefile
MYSQL_VERSION="$(grep ^MYSQL_VERSION= "$SOURCEDIR/Makefile-pxc" \
    | cut -d = -f 2)"
RELEASE_TAG=''
PERCONA_SERVER_VERSION="$(grep ^PERCONA_SERVER_VERSION= \
    "$SOURCEDIR/Makefile-pxc" | cut -d = -f 2)"
WSREP_VERSION="$(grep WSREP_INTERFACE_VERSION \
    "$SOURCEDIR/wsrep/wsrep_api.h" |
    cut -d '"' -f2).$(grep 'SET(WSREP_PATCH_VERSION' \
    "$SOURCEDIR/cmake/wsrep.cmake" | cut -d '"' -f2)"
PRODUCT="Percona-XtraDB-Cluster-$MYSQL_VERSION"

# Build information
if [[ -z ${REVNO:-} ]];then 
    REVISION=500 
else 
    REVISION=$REVNO
    echo "Building with revision $REVISION"
fi 

WSREP_REV="$(cd "$SOURCEDIR";test -r WSREP-REVISION && cat WSREP-REVISION || echo "$REVISION")"
if [[ $COPYGALERA -eq 0 ]];then 
    GALERA_REVISION="$(cd "$SOURCEDIR/percona-xtradb-cluster-galera"; test -r GALERA-REVISION && cat GALERA-REVISION)"
fi
PRODUCT_FULL="$PRODUCT-$RELEASE_TAG$WSREP_VERSION.$REVISION.$(uname -s).$TARGET"
COMMENT="Percona XtraDB Cluster (GPL) $MYSQL_VERSION-$RELEASE_TAG$WSREP_VERSION"
COMMENT="$COMMENT, Revision $REVISION"

# Compilation flags
export CC=${CC:-gcc}
export CXX=${CXX:-g++}
COMMON_FLAGS="-Wall -Wp,-D_FORTIFY_SOURCE=2 -DPERCONA_INNODB_VERSION=$PERCONA_SERVER_VERSION "
export CFLAGS=" $COMMON_FLAGS -static-libgcc $TARGET_CFLAGS ${CFLAGS:-}"
export CXXFLAGS=" $COMMON_FLAGS $TARGET_CFLAGS ${CXXFLAGS:-}"
export MAKE_JFLAG="${MAKE_JFLAG:--j$PROCESSORS}"

export WSREP_REV="$WSREP_REV"

# Test jemalloc directory
if test "x$WITH_JEMALLOC" != "x"
then
    if ! test -d "$WITH_JEMALLOC"
    then
        echo >&2 "Jemalloc dir $WITH_JEMALLOC does not exist"
        exit 1
    fi
    
    JEMALLOCDIR="$(cd "$WITH_JEMALLOC"; pwd)"

fi

if [[ -n ${LDFLAGS:-} ]];then 
   MLDFLAGS=-DWITH_MYSQLD_LDFLAGS=$LDFLAGS 
   WITHASAN=ON
else 
   MLDFLAGS=""
   WITHASAN=OFF
fi

# Build
(
    cd "$SOURCEDIR"
 
    # Build galera
    (
    if [[ $COPYGALERA -eq 0 ]];then 
        export CC=${GALERA_CC:-gcc}
        export CXX=${GALERA_CXX:-g++}

        cd "percona-xtradb-cluster-galera"
        scons --config=force boost_pool=0 revno="$GALERA_REVISION" $MAKE_JFLAG \
              garb/garbd libgalera_smm.so
        mkdir -p "$WORKDIR/usr/local/$PRODUCT_FULL/bin" \
             "$WORKDIR/usr/local/$PRODUCT_FULL/lib"
        cp garb/garbd "$WORKDIR/usr/local/$PRODUCT_FULL/bin"
        cp libgalera_smm.so "$WORKDIR/usr/local/$PRODUCT_FULL/lib"
    else 
        mkdir -p "$WORKDIR/usr/local/$PRODUCT_FULL/bin" \
             "$WORKDIR/usr/local/$PRODUCT_FULL/lib"
        cp $WORKDIR/garbd "$WORKDIR/usr/local/$PRODUCT_FULL/bin"
        cp $WORKDIR/libgalera_smm.so "$WORKDIR/usr/local/$PRODUCT_FULL/lib"
    fi

    ) || exit 1


    #make -f Makefile-pxc all

    cmake . ${CMAKE_OPTS:-} $MLDFLAGS -DBUILD_CONFIG=mysql_release \
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo} \
        $DEBUG_EXTNAME \
        -DWITH_ASAN=$WITHASAN \
        -DWITH_EMBEDDED_SERVER=OFF \
        -DFEATURE_SET=community \
        -DENABLE_DTRACE=OFF \
        -DWITH_SSL=system \
        -DWITH_ZLIB=system \
        -DCMAKE_INSTALL_PREFIX="/usr/local/$PRODUCT_FULL" \
        -DMYSQL_DATADIR="/usr/local/$PRODUCT_FULL/data" \
        -DMYSQL_SERVER_SUFFIX="-$RELEASE_TAG$WSREP_VERSION" \
        -DWITH_INNODB_DISALLOW_WRITES=ON \
        -DWITH_WSREP=ON \
        -DCOMPILATION_COMMENT="$COMMENT" \
        -DWITH_PAM=ON

    make $MAKE_JFLAG $QUIET
    make DESTDIR="$WORKDIR" install


    (
       echo "Packaging the test files"
       # mkdir -p $WORKDIR/usr/local/$PRODUCT_FULL
       cp -R percona-xtradb-cluster-tests $WORKDIR/usr/local/$PRODUCT_FULL/
    )

    # Build jemalloc
    if test "x$WITH_JEMALLOC" != x
    then
    (
        cd "$JEMALLOCDIR"

        ./configure --prefix="/usr/local/$PRODUCT_FULL/" \
                --libdir="/usr/local/$PRODUCT_FULL/lib/mysql/"
        make $MAKE_JFLAG
        make DESTDIR="$WORKDIR" install_lib_shared

        # Copy COPYING file
        cp COPYING "$WORKDIR/usr/local/$PRODUCT_FULL/COPYING-jemalloc"

    )
    fi

)

# Package the archive
(
    cd "$WORKDIR/usr/local/"

    $TAR czf "$WORKDIR/$PRODUCT_FULL.tar.gz" \
        --owner=0 --group=0 "$PRODUCT_FULL/"
    
)
