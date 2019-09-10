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

# bail out on errors, be strict
set -ue

#-------------------------------------------------------------------------------
#
# Step-0: Set default configuration.
#

# check the machine type (x86_64, i686) and accordingly configure
# the compliation flag.
MACHINE_SPECS="$(uname -m)"
MACHINE_SPECS_CFLAGS=''

# define verbosity while running make. default is off.
QUIET=''

# link jemalloc (needed for tokudb). For now pxc doesn't build tokudb
# if in future pxc supports tokudb we will need it.
WITH_JEMALLOC=''
DEBUG_EXTRA=''

# Galera can be build locally as part of the script if the directory is placed
# at said location. Alternatively, skip building galera instead just copy
# the pre-build binary/so file to TARGETDIR (target-directory specified) and this
# script will install it at said location while making file tar.gz.
COPYGALERA=0

WITH_MECAB_OPTION=''

# build with ssl. configuration related to ssl.
WITH_SSL='/usr'
WITH_SSL_TYPE='system'
OPENSSL_INCLUDE=''
OPENSSL_LIBRARY=''
CRYPTO_LIBRARY=''
GALERA_SSL=''
SSL_OPT=''
STAG=${STAG:-}

# tag build with this name.
TAG='1'

# cmake build option to preset. based on configuration default options are used.
CMAKE_BUILD_TYPE='RelWithDebInfo'
COMMON_FLAGS=''
#
TOKUDB_BACKUP_VERSION=''
#
# Some programs that may be overriden
TAR=${TAR:-tar}
SCONS_ARGS=${SCONS_ARGS:-""}

# keep build files
KEEP_BUILD=0

# enable asan
ENABLE_ASAN=0

#-------------------------------------------------------------------------------
#
# Step-1: Set default configuration.
#

#
# parse input option and configure build enviornment acccordingly.
if ! getopt --test
then
    go_out="$(getopt --options=iqGadvjmt: \
        --longoptions=i686,verbose,copygalera,asan,debug,valgrind,with-jemalloc:,with-mecab:,with-yassl,keep-build,with-ssl:,tag: \
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
        MACHINE_SPECS="i686"
        MACHINE_SPECS_CFLAGS="-m32 -march=i686"
        ;;
    -d | --debug )
        shift
        CMAKE_BUILD_TYPE='Debug'
        BUILD_COMMENT="${BUILD_COMMENT:-}-debug"
        DEBUG_EXTRA="-DDEBUG_EXTNAME=ON"
        SCONS_ARGS+=' debug=0'
        ;;
    -G | --copygalera )
        shift
        COPYGALERA=1
        ;;
    -a | --asan )
        shift
        ENABLE_ASAN=1
        ;;
    -v | --valgrind )
        shift
        CMAKE_OPTS="${CMAKE_OPTS:-} -DWITH_VALGRIND=ON"
        BUILD_COMMENT="${BUILD_COMMENT:-}-valgrind"
        ;;
    -q | --verbose )
        shift
        QUIET='VERBOSE=1'
        ;;
    -j | --with-jemalloc )
        shift
        WITH_JEMALLOC="$1"
        shift
        ;;
    -m | --with-mecab )
        shift
        WITH_MECAB_OPTION="-DWITH_MECAB=$1"
        shift
        ;;
    --with-yassl )
        shift
        WITH_SSL_TYPE="bundled"
        ;;
    --keep-build )
        shift
        KEEP_BUILD=1
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

#-------------------------------------------------------------------------------
#
# Step-2: Set working/source/galera directories.
#

#
# Set working directory (default = current directory).
# if working directory is not empty the it will bail out.
if test "$#" -eq 0
then
    TARGETDIR="$(pwd)"

    # Check that the current directory is not empty
    if test "x$(echo *)" != "x*"
    then
        echo >&2 \
            "Current directory is not empty. Use $0 . to force build in ."
        exit 1
    fi
elif test "$#" -eq 1
then
    TARGETDIR="$1"

    # Check that the provided directory exists and is a directory
    if ! test -d "$TARGETDIR"
    then
        echo >&2 "$TARGETDIR is not a directory"
        exit 1
    fi

else
    echo >&2 "Usage: $0 [target dir]"
    exit 1
fi

#
# target directory path should be absolute
TARGETDIR="$(cd "$TARGETDIR"; pwd)"
echo "Using $TARGETDIR as target/output directory"

#
# source directory hold target directory as immediately child.
# as in cd source-dir/target-dir should be true
SOURCEDIR="$(cd $(dirname "$0"); cd ..; pwd)"
test -e "$SOURCEDIR/VERSION" || exit 2
echo "Using $SOURCEDIR as source directory"

#
# Check if galera sources are part of source directory and needs to be build
# or it is just a copy-over operation.
if [[ $COPYGALERA -eq 0 ]] && ! test -d "$SOURCEDIR/percona-xtradb-cluster-galera"
then
    echo >&2 "Subdir percona-xtradb-cluster-galera not found"
    exit 1
fi

#
# Find the number of processor to parallelize build (make -j<parallel>)
if test -e "/proc/cpuinfo"
then
    PROCESSORS="$(grep -c ^processor /proc/cpuinfo)"
else
    PROCESSORS=4
fi

#-------------------------------------------------------------------------------
#
# Step-3: Extract and set PXC version
# (It is made up from all contributing components)
#

#
# PXC is made up from multiple upstream component.
# PS: Percona Server Version
# WSREP: wsrep plugin.
# (Galera: This is standalone so nothing to consider from here)
# Extract the version from of each component.

source "$SOURCEDIR/VERSION"
MYSQL_VERSION="$MYSQL_VERSION_MAJOR.$MYSQL_VERSION_MINOR.$MYSQL_VERSION_PATCH"
PERCONA_SERVER_EXTENSION="$(echo $MYSQL_VERSION_EXTRA | sed 's/^-/rel/')"
PS_VERSION="$MYSQL_VERSION-$PERCONA_SERVER_EXTENSION"

# Note: WSREP_INTERFACE_VERSION act as compatibility check between wsrep-plugin
# and galera plugin so we include it as part of our component.
WSREP_VERSION="$(grep WSREP_INTERFACE_VERSION wsrep-lib/wsrep-API/v26/wsrep_api.h | cut -d '"' -f2).$(grep 'SET(WSREP_PATCH_VERSION'  "cmake/wsrep-lib.cmake" | cut -d '"' -f2)"

if [[ $COPYGALERA -eq 0 ]];then
    #GALERA_REVISION="$(cd "$SOURCEDIR/percona-xtradb-cluster-galera"; test -r GALERA-REVISION && cat GALERA-REVISION)"
    GALERA_REVISION="$(cd "$SOURCEDIR/percona-xtradb-cluster-galera"; git log --pretty=oneline | head -1 | cut --delimiter=' ' -f 1 | cut -b 1-7)"
fi
TOKUDB_BACKUP_VERSION="${MYSQL_VERSION}${MYSQL_VERSION_EXTRA}"

RELEASE_TAG=''
PRODUCT_NAME="Percona-XtraDB-Cluster-$MYSQL_VERSION-$PERCONA_SERVER_EXTENSION"
PRODUCT_FULL_NAME="$PRODUCT_NAME-$RELEASE_TAG$WSREP_VERSION${BUILD_COMMENT:-}.$TAG.$(uname -s)${DIST_NAME:-}.$MACHINE_SPECS${SSL_VER:-}"

#
# This corresponds to GIT revision when the build/package is created.
if test -e "$SOURCEDIR/Docs/INFO_SRC"
then
    REVISION="$(cd "$SOURCEDIR"; grep '^short: ' Docs/INFO_SRC |sed -e 's/short: //')"
elif [ -n "$(which git)" -a -d "$SOURCEDIR/.git" ];
then
    REVISION="$(git rev-parse --short HEAD)"
else
    REVISION=""
fi
COMMENT="Percona XtraDB Cluster binary (GPL) $MYSQL_VERSION-$RELEASE_TAG$WSREP_VERSION"
COMMENT="$COMMENT, Revision $REVISION${BUILD_COMMENT:-}"

#-------------------------------------------------------------------------------
#
# Step-4: Set compilation options.
#

export CC=${CC:-gcc}
export CXX=${CXX:-g++}

# If gcc >= 4.8 we can use ASAN in debug build but not if valgrind build also
if [[ $ENABLE_ASAN -eq 1 ]]; then
    if [[ "$CMAKE_BUILD_TYPE" == "Debug" ]] && [[ "${CMAKE_OPTS:-}" != *WITH_VALGRIND=ON* ]]; then
        GCC_VERSION=$(${CC} -dumpversion)
        GT_VERSION=$(echo -e "4.8.0\n${GCC_VERSION}" | sort -t. -k1,1nr -k2,2nr -k3,3nr | head -1)
        if [ "${GT_VERSION}" = "${GCC_VERSION}" ]; then
            DEBUG_EXTRA="${DEBUG_EXTRA} -DWITH_ASAN=ON"
            echo "Build with ASAN on"
        fi
    fi
fi

COMMON_FLAGS="-DPERCONA_INNODB_VERSION=$PERCONA_SERVER_EXTENSION"
export CFLAGS=" $COMMON_FLAGS -static-libgcc $MACHINE_SPECS_CFLAGS ${CFLAGS:-}"
export CXXFLAGS=" $COMMON_FLAGS $MACHINE_SPECS_CFLAGS ${CXXFLAGS:-}"
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

#-------------------------------------------------------------------------------
#
# Step-4: Finally Build.
#

(
    cd "$SOURCEDIR"

    # Build/Copy galera as configured
    (
    if [[ $COPYGALERA -eq 0 ]];then
        export CC=${GALERA_CC:-gcc}
        export CXX=${GALERA_CXX:-g++}

        # Look for boost_program_options static library
        # (use if possible to avoid additional installation requirements)
        BOOST_PROGRAM_OPTIONS_ARGS=""
        BOOST_PROGRAM_OPTIONS_LIB=""
        if [[ -r /usr/lib/x86_64-linux-gnu/libboost_program_options.a ]]; then
            BOOST_PROGRAM_OPTIONS_LIB="/usr/lib/x86_64-linux-gnu/libboost_program_options.a"
        elif [[ -r /usr/lib64/libboost_program_options.a ]]; then
            BOOST_PROGRAM_OPTIONS_LIB="/usr/lib64/libboost_program_options.a"
        else
            BOOST_PROGRAM_OPTIONS_LIB=$(locate libboost_program_options.a)
        fi
        if [[ -n $BOOST_PROGRAM_OPTIONS_LIB ]]; then
            BOOST_PROGRAM_OPTIONS_ARGS="bpostatic=$BOOST_PROGRAM_OPTIONS_LIB"
        fi

        cd "percona-xtradb-cluster-galera"
        if grep builtin <<< "$STAG";then
            # No builtin SSL in galera yet.
            scons $MAKE_JFLAG psi=1 --config=force ssl=0 revno="$GALERA_REVISION" ${SCONS_ARGS} boost_pool=0 \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                libgalera_smm.so
            scons $MAKE_JFLAG --config=force ssl=0 revno="$GALERA_REVISION" ${SCONS_ARGS} boost_pool=0 \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                garb/garbd
        elif grep static <<< "$STAG";then
            # Disable SSL in galera for now
            scons $MAKE_JFLAG psi=1 --config=force static_ssl=1 with_ssl=$GALERA_SSL \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                revno="$GALERA_REVISION" ${SCONS_ARGS} boost_pool=0 libgalera_smm.so
            scons $MAKE_JFLAG --config=force static_ssl=1 with_ssl=$GALERA_SSL \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                revno="$GALERA_REVISION" ${SCONS_ARGS} boost_pool=0 garb/garbd
        else
            scons $MAKE_JFLAG psi=1 --config=force revno="$GALERA_REVISION" ${SCONS_ARGS} \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                libgalera_smm.so
            scons $MAKE_JFLAG --config=force revno="$GALERA_REVISION" ${SCONS_ARGS} \
                $BOOST_PROGRAM_OPTIONS_ARGS \
                garb/garbd
        fi
        mkdir -p "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin" \
             "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/lib"
        cp garb/garbd "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin"
        cp libgalera_smm.so "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/lib"
    else
        mkdir -p "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin" \
             "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/lib"
        cp $TARGETDIR/garbd "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin"
        cp $TARGETDIR/libgalera_smm.so "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/lib"
    fi
    ) || exit 1

    if grep -q builtin <<< "$STAG" || [[ $WITH_SSL_TYPE == 'bundled' ]];then
        # builtin
        SSL_OPT='-DWITH_SSL=bundled -DWITH_ZLIB=bundled'
    else
        SSL_OPT='-DWITH_SSL=system -DWITH_ZLIB=system'
    fi

    mkdir "$TARGETDIR/bld"
    cd "$TARGETDIR/bld"
    echo "$(pwd)"

    if [[ $CMAKE_BUILD_TYPE == 'Debug' ]]; then
        cmake ../../ ${CMAKE_OPTS:-} -DBUILD_CONFIG=mysql_release \
            -DCMAKE_BUILD_TYPE=Debug \
            $DEBUG_EXTRA \
            -DFEATURE_SET=community \
            -DCMAKE_INSTALL_PREFIX="$TARGETDIR/usr/local/$PRODUCT_FULL_NAME" \
            -DMYSQL_DATADIR="$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/data" \
            -DCOMPILATION_COMMENT="$COMMENT - UNIV_DEBUG ON" \
            -DWITH_PAM=ON \
            -DWITHOUT_ROCKSDB=ON \
            -DWITHOUT_TOKUDB=ON \
            -DWITH_INNODB_MEMCACHED=ON \
            -DDOWNLOAD_BOOST=1 \
            -DFORCE_INSOURCE_BUILD=1 \
            -DWITH_SYSTEM_LIBS=ON \
            -DWITH_PROTOBUF=bundled \
            -DWITH_RAPIDJSON=bundled \
            -DWITH_ICU=bundled \
            -DWITH_LZ4=bundled \
            -DWITH_RE2=bundled \
            -DWITH_LIBEVENT=bundled \
            -DWITH_EDITLINE=bundled \
            -DWITH_NUMA=ON \
            -DWITH_BOOST="$TARGETDIR/libboost" \
            -DMYSQL_SERVER_SUFFIX="-$WSREP_VERSION" \
            -DWITH_WSREP=ON \
            -DWITH_UNIT_TESTS=0 \
            -DWITH_DEBUG=ON \
            $WITH_MECAB_OPTION $OPENSSL_INCLUDE $OPENSSL_LIBRARY $CRYPTO_LIBRARY

        (make $MAKE_JFLAG $QUIET) || exit 1
        (make install) || exit 1
        (cp -v runtime_output_directory/mysqld-debug $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/mysqld) || true
        echo "mysqld in build in debug mode"
    else
        cmake ../../ ${CMAKE_OPTS:-} -DBUILD_CONFIG=mysql_release \
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo} \
            -DFEATURE_SET=community \
            -DCMAKE_INSTALL_PREFIX="$TARGETDIR/usr/local/$PRODUCT_FULL_NAME" \
            -DMYSQL_DATADIR="$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/data" \
            -DCOMPILATION_COMMENT="$COMMENT - UNIV_DEBUG ON" \
            -DWITH_PAM=ON \
            -DWITHOUT_ROCKSDB=ON \
            -DWITHOUT_TOKUDB=ON \
            -DWITH_INNODB_MEMCACHED=ON \
            -DDOWNLOAD_BOOST=1 \
            -DFORCE_INSOURCE_BUILD=1 \
            -DWITH_SYSTEM_LIBS=ON \
            -DWITH_PROTOBUF=bundled \
            -DWITH_RAPIDJSON=bundled \
            -DWITH_ICU=bundled \
            -DWITH_LZ4=bundled \
            -DWITH_RE2=bundled \
            -DWITH_EDITLINE=bundled \
            -DWITH_LIBEVENT=bundled \
            -DWITH_NUMA=ON \
            -DWITH_BOOST="$TARGETDIR/libboost" \
            -DMYSQL_SERVER_SUFFIX="-$WSREP_VERSION" \
            -DWITH_WSREP=ON \
            -DWITH_UNIT_TESTS=0 \
            $WITH_MECAB_OPTION $OPENSSL_INCLUDE $OPENSSL_LIBRARY $CRYPTO_LIBRARY

        (make $MAKE_JFLAG $QUIET) || exit 1
        (make install) || exit 1
        echo "mysqld in build in release mode"
    fi

    (
       echo "Packaging the test files"
       cp -R $SOURCEDIR/percona-xtradb-cluster-tests $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/
    ) || exit 1

    # Build jemalloc
    if test "x$WITH_JEMALLOC" != x
    then
    (
        cd "$JEMALLOCDIR"

        CFLAGS= ./autogen.sh --disable-valgrind --prefix="/usr/local/$PRODUCT_FULL_NAME/" \
            --libdir="/usr/local/$PRODUCT_FULL_NAME/lib/mysql/"
        make $MAKE_JFLAG
        make DESTDIR="$TARGETDIR" install_lib_shared
        strip lib/libjemalloc* || true
        # Copy COPYING file
        cp COPYING "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/COPYING-jemalloc"

    ) || exit 1
    fi

    # Extract the Percona Xtrabackup binaries
    # Look for the pxb 2.4 tarball
    (
        cd "$TARGETDIR"
        pxb_tar=$(ls -1td percona-xtrabackup-2.4.* | grep ".tar" | sort --version-sort | tail -n1)
        if [[ -z $pxb_tar ]]; then
            echo "Could not find percona-xtrabackup-2.4 tarball in $TARGETDIR.  Terminating."
            exit 1
        fi
        pxb_dir=$(echo $pxb_tar | grep -oe ".*x86_64" )

        mkdir -p pxc_extra
        cd pxc_extra
        if [[ -d ${pxb_dir} ]]; then
            echo "Using existing pxb 2.4 directory"
        else
            echo "Removing existing percona-xtrabackup-2.4 basedir (if found)"
            find . -maxdepth 1 -type d -name 'percona-xtrabackup-2.*' -exec rm -rf {} \+

            echo "Extracting pxb 2.4 tarball"
            tar -xzf "../$pxb_tar"
        fi
        rm -f "pxb-2.4"
        echo "Creating symlink pxc_extra/pxb-2.4 --> $pxb_dir"
        ln -s "$pxb_dir" pxb-2.4
    ) || exit 1

    pxb_dir=$(ls -1td $TARGETDIR/pxc_extra/percona-xtrabackup-2.* | sort --version-sort | tail -n1)
    pxb2_version=$(echo $pxb_dir | grep -oe "[1-9]\.[0-9][0-9]*\.[0-9][0-9]*")

    # Look for the pxb 8.0 tarball
    (
        cd "$TARGETDIR"
        pxb_tar=$(ls -1td percona-xtrabackup-8.0.* | grep ".tar" | sort --version-sort | tail -n1)
        if [[ -z $pxb_tar ]]; then
            echo "Could not find percona-xtrabackup-8.0.x tarball in $TARGETDIR.  Terminating."
            exit 1
        fi
        pxb_dir=$(echo $pxb_tar | grep -oe ".*x86_64" )

        mkdir -p pxc_extra
        cd pxc_extra
        if [[ -d ${pxb_dir} ]]; then
            echo "Using existing pxb 8.0 directory"
        else
            echo "Removing existing percona-xtrabackup-8.0 basedir (if found)"
            find . -maxdepth 1 -type d -name 'percona-xtrabackup-8.*' -exec rm -rf {} \+

            echo "Extracting pxb 8.0 tarball"
            tar -xzf "../$pxb_tar"
        fi
        rm -f "pxb-8.0"
        echo "Creating symlink pxc_extra/pxb-8.0 --> $pxb_dir"
        ln -s "$pxb_dir" pxb-8.0
    ) || exit 1

    pxb_dir=$(ls -1td $TARGETDIR/pxc_extra/percona-xtrabackup-8.* | sort --version-sort | tail -n1)
    pxb8_version=$(echo $pxb_dir | grep -oe "[1-9]\.[0-9][0-9]*\.[0-9][0-9]*")

    # Only copy over the bin and lib portions of the xtrabackup packages
    # Test cases and other files are not copied
    mkdir -p "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-2.4"
    (cp -v -r $TARGETDIR/pxc_extra/pxb-2.4/bin/  $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-2.4) || true
    (cp -v -r $TARGETDIR/pxc_extra/pxb-2.4/lib/  $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-2.4) || true
    echo "percona xtrabackup $pxb2_version in build in release mode"

    mkdir -p "$TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-8.0"
    (cp -v -r $TARGETDIR/pxc_extra/pxb-8.0/bin/ $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-8.0) || true
    (cp -v -r $TARGETDIR/pxc_extra/pxb-8.0/lib/ $TARGETDIR/usr/local/$PRODUCT_FULL_NAME/bin/pxc_extra/pxb-8.0) || true
    echo "percona xtrabackup $pxb8_version in build in release mode"

) || exit 1

# Package the archive
(
    cd "$TARGETDIR/usr/local/"

    $TAR --owner=0 --group=0 -czf "$TARGETDIR/$PRODUCT_FULL_NAME.tar.gz" $PRODUCT_FULL_NAME
) || exit 1

if [[ $KEEP_BUILD -eq 0 ]]
then
    rm -rf $TARGETDIR/libboost
    rm -rf $TARGETDIR/bld
    rm -rf $TARGETDIR/usr
fi

echo "Build Complete"
