#!/bin/bash
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
DEBUG_EXTNAME=''
WITH_SSL='/usr'
WITH_SSL_TYPE='system'
OPENSSL_INCLUDE=''
OPENSSL_LIBRARY=''
CRYPTO_LIBRARY=''
GALERA_SSL=''
SSL_OPT=''
TAG=''
#
CMAKE_BUILD_TYPE=''
COMMON_FLAGS=''
#
# Some programs that may be overriden
TAR=${TAR:-tar}

# Check if we have a functional getopt(1)
if ! getopt --test
then
    go_out="$(getopt --options=iqdvjt: \
        --longoptions=i686,quiet,debug,valgrind,with-jemalloc:,with-yassl,with-ssl:,tag: \
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
    --with-yassl )
        shift
        WITH_SSL_TYPE="bundled"
        ;;
    --with-ssl )
        shift
        WITH_SSL="$1"
        shift
        # Set openssl and crypto library path
        if test -e "$WITH_SSL/lib/libssl.a"
        then
            OPENSSL_INCLUDE="-DOPENSSL_INCLUDE_DIR=$WITH_SSL/include"
            OPENSSL_LIBRARY="-DOPENSSL_LIBRARY=$WITH_SSL/lib/libssl.a"
            CRYPTO_LIBRARY="-DCRYPTO_LIBRARY=$WITH_SSL/lib/libcrypto.a"
            GALERA_SSL="$WITH_SSL/lib"
        elif test -e "$WITH_SSL/lib64/libssl.a"
        then
            OPENSSL_INCLUDE="-DOPENSSL_INCLUDE_DIR=$WITH_SSL/include"
            OPENSSL_LIBRARY="-DOPENSSL_LIBRARY=$WITH_SSL/lib64/libssl.a"
            CRYPTO_LIBRARY="-DCRYPTO_LIBRARY=$WITH_SSL/lib64/libcrypto.a"
            GALERA_SSL="$WITH_SSL/lib64"
        else
            echo >&2 "Cannot find libssl.a in $WITH_SSL"
            exit 3
        fi
        ;;
    -t | --tag )
        shift
        TAG="$1"
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

# Workdir path should be absolute
WORKDIR="$(cd "$WORKDIR"; pwd)"

SOURCEDIR="$(cd $(dirname "$0"); cd ..; pwd)"
test -e "$SOURCEDIR/VERSION" || exit 2

# Test for the galera sources
if ! test -d "$SOURCEDIR/percona-xtradb-cluster-galera"
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

if [[ -z ${STAG:-} ]];then 
    STAG=""
fi


# Build information
if [[ -z ${REVISION:-} ]];then 
    REVISION=500 
else 
    REVISION=$REVISION
    echo "Building with revision $REVISION"
fi

# Extract version from the VERSION file
source "$SOURCEDIR/VERSION"
MYSQL_VERSION="$MYSQL_VERSION_MAJOR.$MYSQL_VERSION_MINOR.$MYSQL_VERSION_PATCH"
# Extract version from the Makefile-pxc
PERCONA_XTRADB_CLUSTER_VERSION="$(echo $MYSQL_VERSION_EXTRA | sed 's/^-/rel/')"
RELEASE_TAG=''
PRODUCT="Percona-XtraDB-Cluster-$MYSQL_VERSION-$PERCONA_XTRADB_CLUSTER_VERSION"

# Build information
if test -e "$SOURCEDIR/Docs/INFO_SRC"
then
    REVISION="$(cd "$SOURCEDIR"; grep '^revno: ' Docs/INFO_SRC |sed -e 's/revno: //')"
elif test -e "$SOURCEDIR/.bzr/branch/last-revision"
then
    REVISION="$(cd "$SOURCEDIR"; cat .bzr/branch/last-revision | awk -F ' ' '{print $1}')"
else
    REVISION=""
fi
REVISION="$(cd "$SOURCEDIR"; grep '^revno: ' Docs/INFO_SRC |sed -e 's/revno: //')"
WSREP_VERSION="$(grep WSREP_INTERFACE_VERSION wsrep/wsrep_api.h | cut -d '"' -f2).$(grep 'SET(WSREP_PATCH_VERSION'  "cmake/wsrep.cmake" | cut -d '"' -f2)"
GALERA_REVISION="$(cd "$SOURCEDIR/percona-xtradb-cluster-galera"; test -r GALERA-REVISION && cat GALERA-REVISION)"
PRODUCT_FULL="$PRODUCT-$RELEASE_TAG$WSREP_VERSION.$REVISION${BUILD_COMMENT:-}$TAG.$(uname -s).$TARGET"
COMMENT="Percona XtraDB Cluster binary (GPL) $MYSQL_VERSION-$RELEASE_TAG$WSREP_VERSION"
COMMENT="$COMMENT, Revision $REVISION${BUILD_COMMENT:-}"

# Compilation flags
export CC=${CC:-gcc}
export CXX=${CXX:-g++}

# TokuDB cmake flags
if test -d "$SOURCEDIR/storage/tokudb"
then
    CMAKE_OPTS="${CMAKE_OPTS:-} -DBUILD_TESTING=OFF -DUSE_GTAGS=OFF -DUSE_CTAGS=OFF -DUSE_ETAGS=OFF -DUSE_CSCOPE=OFF"
    
    if test "x$CMAKE_BUILD_TYPE" != "xDebug"
    then
        CMAKE_OPTS="${CMAKE_OPTS:-} -DTOKU_DEBUG_PARANOID=OFF"
    else
        CMAKE_OPTS="${CMAKE_OPTS:-} -DTOKU_DEBUG_PARANOID=ON"
    fi

    if [[ $CMAKE_OPTS == *WITH_VALGRIND=ON* ]]
    then
        CMAKE_OPTS="${CMAKE_OPTS:-} -DUSE_VALGRIND=ON"
    fi
fi

#
if [ -n "$(which rpm)" ]; then
  export COMMON_FLAGS=$(rpm --eval %optflags | sed -e "s|march=i386|march=i686|g")
else
  COMMON_FLAGS="-Wall -Wp,-D_FORTIFY_SOURCE=2 -DPERCONA_INNODB_VERSION=$MYSQL_RELEASE "
fi
export CFLAGS=" $COMMON_FLAGS -static-libgcc $TARGET_CFLAGS ${CFLAGS:-}"
export CXXFLAGS=" $COMMON_FLAGS $TARGET_CFLAGS ${CXXFLAGS:-}"
export MAKE_JFLAG="${MAKE_JFLAG:--j$PROCESSORS}"
#
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

# Build
(
    cd "$SOURCEDIR"
 
    # Build galera
    (
        export CC=${GALERA_CC:-gcc}
        export CXX=${GALERA_CXX:-g++}

        cd "percona-xtradb-cluster-galera"
        if grep builtin <<< "$STAG";then 
            # No builtin SSL in galera yet.
            scons $MAKE_JFLAG --config=force ssl=0 revno="$GALERA_REVISION" boost_pool=0 \
                garb/garbd libgalera_smm.so
        elif grep static <<< "$STAG";then 
            # Disable SSL in galera for now
            scons $MAKE_JFLAG --config=force static_ssl=1 with_ssl=$GALERA_SSL \
            revno="$GALERA_REVISION" boost_pool=0 garb/garbd libgalera_smm.so
        else 
            scons $MAKE_JFLAG --config=force revno="$GALERA_REVISION" \
                garb/garbd libgalera_smm.so
        fi
        mkdir -p "$WORKDIR/usr/local/$PRODUCT_FULL/bin" \
             "$WORKDIR/usr/local/$PRODUCT_FULL/lib"
        cp garb/garbd "$WORKDIR/usr/local/$PRODUCT_FULL/bin"
        cp libgalera_smm.so "$WORKDIR/usr/local/$PRODUCT_FULL/lib"

    ) || exit 1

    #make -f Makefile-pxc all

    if grep -q builtin <<< "$STAG" || [[ $WITH_SSL_TYPE == 'bundled' ]];then 
        # builtin
        SSL_OPT='-DWITH_SSL=bundled -DWITH_ZLIB=bundled'
    else 
        SSL_OPT='-DWITH_SSL=system -DWITH_ZLIB=system'
    fi
    cmake . ${CMAKE_OPTS:-} -DBUILD_CONFIG=mysql_release \
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo} \
        $DEBUG_EXTNAME \
        -DWITH_EMBEDDED_SERVER=OFF \
        -DFEATURE_SET=community \
        -DENABLE_DTRACE=OFF \
         $SSL_OPT \
        -DCMAKE_INSTALL_PREFIX="/usr/local/$PRODUCT_FULL" \
        -DMYSQL_DATADIR="/usr/local/$PRODUCT_FULL/data" \
        -DMYSQL_SERVER_SUFFIX="-$RELEASE_TAG$WSREP_VERSION" \
        -DWITH_INNODB_DISALLOW_WRITES=ON \
        -DWITH_WSREP=ON \
        -DCOMPILATION_COMMENT="$COMMENT" \
        -DWITH_PAM=ON \
        -DWITH_INNODB_MEMCACHED=ON \
        $OPENSSL_INCLUDE $OPENSSL_LIBRARY $CRYPTO_LIBRARY

    make $MAKE_JFLAG $QUIET
    make DESTDIR="$WORKDIR" install

    if [[ $CMAKE_BUILD_TYPE != 'Debug' ]];then
        make clean
        cmake . ${CMAKE_OPTS:-} -DBUILD_CONFIG=mysql_release \
            -DCMAKE_BUILD_TYPE=Debug \
            -DDEBUG_EXTNAME=ON \
            -DWITH_EMBEDDED_SERVER=OFF \
            -DFEATURE_SET=community \
            -DENABLE_DTRACE=OFF \
            $SSL_OPT \
            -DCMAKE_INSTALL_PREFIX="/usr/local/$PRODUCT_FULL" \
            -DMYSQL_DATADIR="/usr/local/$PRODUCT_FULL/data" \
            -DMYSQL_SERVER_SUFFIX="-$RELEASE_TAG$WSREP_VERSION" \
            -DWITH_INNODB_DISALLOW_WRITES=ON \
            -DWITH_WSREP=ON \
            -DCOMPILATION_COMMENT="$COMMENT - UNIV_DEBUG ON" \
            -DWITH_PAM=ON \
            -DWITH_INNODB_MEMCACHED=ON \
            $OPENSSL_INCLUDE $OPENSSL_LIBRARY $CRYPTO_LIBRARY
        make $MAKE_JFLAG $QUIET

        echo "Copying mysqld-debug"
        cp -v sql/mysqld-debug $WORKDIR/usr/local/$PRODUCT_FULL/bin/mysqld-debug
    fi


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

        CFLAGS= ./autogen.sh --disable-valgrind --prefix="/usr/local/$PRODUCT_FULL/" \
            --libdir="/usr/local/$PRODUCT_FULL/lib/mysql/"
        make $MAKE_JFLAG
        make DESTDIR="$WORKDIR" install_lib_shared
        strip lib/libjemalloc* || true
        # Copy COPYING file
        cp COPYING "$WORKDIR/usr/local/$PRODUCT_FULL/COPYING-jemalloc"

    )
    fi

)

# Package the archive
(
    cd "$WORKDIR/usr/local/"

    find $PRODUCT_FULL ! -type d  ! \( -iname '*toku*' -o -iwholename '*/tokudb*/*'  \) | sort > $WORKDIR/tokudb_server.list
    $TAR --owner=0 --group=0 -czf "$WORKDIR/$PRODUCT_FULL.tar.gz" -T $WORKDIR/tokudb_server.list
    rm -f $WORKDIR/tokudb_server.list

    if test -e "$PRODUCT_FULL/lib/mysql/plugin/ha_tokudb.so"
    then
        TARGETTOKU=$(echo $PRODUCT_FULL | sed 's/.Linux/.TokuDB.Linux/')
	find $PRODUCT_FULL ! -type d \( -iname '*toku*' -o -iwholename '*/tokudb*/*' \) > $WORKDIR/tokudb_plugin.list
        $TAR --owner=0 --group=0 -czf "$WORKDIR/$TARGETTOKU.tar.gz" -T $WORKDIR/tokudb_plugin.list
        rm -f $WORKDIR/tokudb_plugin.list
    fi
)

