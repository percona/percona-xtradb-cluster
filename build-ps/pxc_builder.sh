#!/bin/sh

shell_quote_string() {
  echo "$1" | sed -e 's,\([^a-zA-Z0-9/_.=-]\),\\\1,g'
}

usage () {
    cat <<EOF
Usage: $0 [OPTIONS]
    The following options may be given :
        --builddir=DIR      Absolute path to the dir where all actions will be performed
        --get_sources       Source will be downloaded from github
        --build_src_rpm     If it is 1 src rpm will be built
        --build_source_deb  If it is 1 source deb package will be built
        --build_rpm         If it is 1 rpm will be built
        --build_deb         If it is 1 deb will be built
        --build_tarball     If it is 1 tarball will be built
        --install_deps      Install build dependencies(root previlages are required)
        --branch            Branch for build
        --repo              Repo for build
        --rpm_release       RPM version( default = 1)
        --deb_release       DEB version( default = 1)
        --bin_release       BIN version( default = 1)
        --debug             Build debug tarball
        --help) usage ;;
Example $0 --builddir=/tmp/PXC80 --get_sources=1 --build_src_rpm=1 --build_rpm=1
EOF
        exit 1
}

append_arg_to_args () {
  args="$args "$(shell_quote_string "$1")
}

parse_arguments() {
    pick_args=
    if test "$1" = PICK-ARGS-FROM-ARGV
    then
        pick_args=1
        shift
    fi
  
    for arg do
        val=$(echo "$arg" | sed -e 's;^--[^=]*=;;')
        case "$arg" in
            # these get passed explicitly to mysqld
            --builddir=*) WORKDIR="$val" ;;
            --build_src_rpm=*) SRPM="$val" ;;
            --build_source_deb=*) SDEB="$val" ;;
            --build_rpm=*) RPM="$val" ;;
            --build_deb=*) DEB="$val" ;;
            --get_sources=*) SOURCE="$val" ;;
            --build_tarball=*) TARBALL="$val" ;;
            --branch=*) BRANCH="$val" ;;
            --repo=*) REPO="$val" ;;
            --install_deps=*) INSTALL="$val" ;;
            --rpm_release=*) RPM_RELEASE="$val" ;;
            --deb_release=*) DEB_RELEASE="$val" ;;
            --bin_release=*) BIN_RELEASE="$val" ;;
            --no_clone=*) NO_CLONE="$val" ;;
            --debug=*) DEBUG="$val" ;;
            --help) usage ;;      
            *)
              if test -n "$pick_args"
              then
                  append_arg_to_args "$arg"
              fi
              ;;
        esac
    done
}

check_workdir(){
    if [ "x$WORKDIR" = "x$CURDIR" ]
    then
        echo >&2 "Current directory cannot be used for building!"
        exit 1
    else
        if ! test -d "$WORKDIR"
        then
            echo >&2 "$WORKDIR is not a directory."
            exit 1
        fi
    fi
    return
}

add_percona_yum_repo(){
    if [ ! -f /etc/yum.repos.d/percona-dev.repo ]
    then
        if [ "x$RHEL" = "x8" ]; then
            cat >/etc/yum.repos.d/percona-dev.repo <<EOL
[percona-rhel8-AppStream]
name=Percona internal YUM repository for RHEL8 AppStream
baseurl=http://jenkins.percona.com/yum-repo/rhel8/AppStream
gpgkey=https://jenkins.percona.com/yum-repo/rhel8/AppStream/RPM-GPG-KEY-redhat-beta
gpgcheck=0
enabled=1
[percona-rhel8-BaseOS]
name=Percona internal YUM repository for RHEL8 BaseOS
baseurl=https://jenkins.percona.com/yum-repo/rhel8/BaseOS/
gpgkey=https://jenkins.percona.com/yum-repo/rhel8/BaseOS/RPM-GPG-KEY-redhat-beta
gpgcheck=0
enabled=1
EOL
        else
            curl -o /etc/yum.repos.d/ https://jenkins.percona.com/yum-repo/percona-dev.repo
        fi
    fi
    return
}

get_sources(){
    cd "${WORKDIR}" || exit
    if [ "${SOURCE}" = 0 ]
    then
        echo "Sources will not be downloaded"
        return 0
    fi
    if [ ${NO_CLONE} = 0 ]; then
        git clone "$REPO"
        retval=$?
        if [ $retval != 0 ]
        then
            echo "There were some issues during repo cloning from github. Please retry one more time"
            exit 1
        fi

        cd percona-xtradb-cluster || exit
        if [ ! -z "$BRANCH" ]
        then
            git reset --hard
            git clean -xdf
            git checkout "$BRANCH"
        fi
        rm -rf wsrep-lib || true
        rm -rf percona-xtradb-cluster-galera || true
        git submodule deinit -f . || true
        git submodule init
        git submodule update

        for dir in 'wsrep-lib' 'percona-xtradb-cluster-galera'; do
            cd $dir || exit
            git submodule deinit -f . || true
            git submodule init
            git submodule update
            cd ../ || exit
        done
    else
        cd percona-xtradb-cluster || exit
    fi

    WSREP_VERSION="$(grep WSREP_INTERFACE_VERSION wsrep-lib/wsrep-API/v26/wsrep_api.h | cut -d '"' -f2).$(grep 'SET(WSREP_PATCH_VERSION'  "cmake/wsrep-lib.cmake" | cut -d '"' -f2)"
    WSREP_REV="$(test -r WSREP-REVISION && cat WSREP-REVISION)"
    REVISION=$(git rev-parse --short HEAD)
    GALERA_REVNO="$(test -r percona-xtradb-cluster-galera/GALERA-REVISION && cat percona-xtradb-cluster-galera/GALERA-REVISION)"
    if [ -f VERSION ]; then
        source VERSION
    elif [ -f MYSQL_VERSION ]; then
        source MYSQL_VERSION
    else
        echo "VERSION file does not exist"
       exit 1
    fi
    export MYSQL_VERSION="$MYSQL_VERSION_MAJOR.$MYSQL_VERSION_MINOR.$MYSQL_VERSION_PATCH"
    export MYSQL_RELEASE="$(echo $MYSQL_VERSION_EXTRA | sed 's/^-//')"

    PRODUCT=Percona-XtraDB-Cluster-80
    PRODUCT_FULL=Percona-XtraDB-Cluster-${MYSQL_VERSION}

    echo "WSREP_VERSION=${WSREP_VERSION}" > ${WORKDIR}/pxc-80.properties
    echo "WSREP_REV=${WSREP_REV}" >> ${WORKDIR}/pxc-80.properties
    echo "REVISION=${REVISION}" >> ${WORKDIR}/pxc-80.properties
    echo "MYSQL_VERSION=$MYSQL_VERSION" >> ${WORKDIR}/pxc-80.properties
    echo "MYSQL_RELEASE=$MYSQL_RELEASE" >> ${WORKDIR}/pxc-80.properties
    echo "BRANCH_NAME=${BRANCH}" >> ${WORKDIR}/pxc-80.properties
    echo "PRODUCT=${PRODUCT}" >> ${WORKDIR}/pxc-80.properties

    echo "PRODUCT_FULL=${PRODUCT_FULL}" >> ${WORKDIR}/pxc-80.properties
    echo "BUILD_NUMBER=${BUILD_NUMBER}" >> ${WORKDIR}/pxc-80.properties
    #
    if [ -z "${DESTINATION:-}" ]; then
    export DESTINATION=experimental
    fi
    DESTINATION="UPLOAD/${DESTINATION}/BUILDS/${PRODUCT}/${PRODUCT_FULL}/${BRANCH}/${REVISION}/${BUILD_NUMBER}"
    echo "DESTINATION=UPLOAD/${DESTINATION}/BUILDS/${PRODUCT}/${PRODUCT_FULL}/${BRANCH}/${REVISION}/${BUILD_NUMBER}" >> ${WORKDIR}/pxc-80.properties
    echo "GALERA_REVNO=${GALERA_REVNO}" >>${WORKDIR}/pxc-80.properties
    DEST=${DESTINATION}
    echo "DEST=${DEST}" >> ${WORKDIR}/pxc-80.properties
    if [ -f /etc/redhat-release ]; then
      export OS_RELEASE="centos$(lsb_release -sr | awk -F'.' '{print $1}')"
      RHEL=$(rpm --eval %rhel)
      source /opt/rh/devtoolset-8/enable
      if [ "x${RHEL}" = "x8" ]; then
          cmake . -DDOWNLOAD_BOOST=1 -DWITH_ROCKSDB=0 -DWITH_BOOST=build-ps/boost -DFORCE_INSOURCE_BUILD=1 -DWITH_ZLIB=bundled
      else
          cmake3 . -DDOWNLOAD_BOOST=1 -DWITH_ROCKSDB=0 -DWITH_BOOST=build-ps/boost -DFORCE_INSOURCE_BUILD=1 -DWITH_ZLIB=bundled
      fi
    else
      cmake . -DDOWNLOAD_BOOST=1 -DWITH_ROCKSDB=0 -DWITH_BOOST=build-ps/boost -DFORCE_INSOURCE_BUILD=1 -DWITH_ZLIB=bundled
    fi
    make dist
    mv *.tar.gz ${WORKDIR}/
    cd ${WORKDIR} || exit
    cat pxc-80.properties
    cd ${WORKDIR} || exit
    ls
    #EXPORTED_TAR=$(basename $(find . -type f -name "*.tar.gz" | sort  | grep -v new | tail -n 1))
    EXPORTED_TAR=$(ls | grep tar.gz )
    #
    PXCDIR=${EXPORTED_TAR%.tar.gz}
    rm -fr ${PXCDIR}
    tar xzf ${EXPORTED_TAR}
    rm -f ${EXPORTED_TAR}
    # add git submodules because make dist uses git archive which doesn't include them
    rsync -av ${WORKDIR}/percona-xtradb-cluster/percona-xtradb-cluster-galera/ ${PXCDIR}/percona-xtradb-cluster-galera --exclude .git
    rsync -av ${WORKDIR}/percona-xtradb-cluster/wsrep-lib/ ${PXCDIR}/wsrep-lib --exclude .git
    rsync -av ${WORKDIR}/percona-xtradb-cluster/extra/coredumper/ ${PXCDIR}/extra/coredumper --exclude .git

    sed -i 's:ROUTER_RUNTIMEDIR:/var/run/mysqlrouter/:g' ${PXCDIR}/packaging/rpm-common/*
    cd ${PXCDIR}/packaging/rpm-common || exit
        if [ ! -f mysqlrouter.service ]; then
            cp -p mysqlrouter.service.in mysqlrouter.service
        fi
        if [ ! -f mysqlrouter.tmpfiles.d ]; then
            cp -p mysqlrouter.tmpfiles.d.in mysqlrouter.tmpfiles.d
        fi
        if [ ! -f mysqlrouter.conf ]; then
            cp -p mysqlrouter.conf.in mysqlrouter.conf
        fi
        if [ ! -f mysql.logrotate ]; then
            cp -p mysql.logrotate.in mysql.logrotate
        fi

    cd ${WORKDIR} || exit
    #
    tar --owner=0 --group=0 --exclude=.bzr --exclude=.git -czf ${PXCDIR}.tar.gz ${PXCDIR}
    rm -fr ${PXCDIR}
    cat pxc-80.properties

    mkdir $WORKDIR/source_tarball
    mkdir $CURDIR/source_tarball
    cp ${PXCDIR}.tar.gz $WORKDIR/source_tarball
    cp ${PXCDIR}.tar.gz $CURDIR/source_tarball
    cd $CURDIR || exit
    rm -rf percona-xtradb-cluster
    return
}

get_system(){
    if [ -f /etc/redhat-release ]; then
        GLIBC_VER_TMP="$(rpm glibc -qa --qf %{VERSION})"
        RHEL=$(rpm --eval %rhel)
        ARCH=$(echo $(uname -m) | sed -e 's:i686:i386:g')
        OS_NAME="el$RHEL"
        OS="rpm"
    else
        GLIBC_VER_TMP="$(dpkg-query -W -f='${Version}' libc6 | awk -F'-' '{print $1}')"
        ARCH=$(uname -m)
        OS_NAME="$(lsb_release -sc)"
        OS="deb"
    fi
    export GLIBC_VER=".glibc${GLIBC_VER_TMP}"
    return
}

install_deps() {
    if [ $INSTALL = 0 ]
    then
        echo "Dependencies will not be installed"
        return;
    fi
    if [ $( id -u ) -ne 0 ]
    then
        echo "It is not possible to instal dependencies. Please run as root"
        exit 1
    fi
    CURPLACE=$(pwd)

    if [ "x$OS" = "xrpm" ]; then
        RHEL=$(rpm --eval %rhel)
        ARCH=$(echo $(uname -m) | sed -e 's:i686:i386:g')
        yum update -y
        yum install -y perl
        yum install -y https://repo.percona.com/yum/percona-release-latest.noarch.rpm
        percona-release enable tools testing
        add_percona_yum_repo
        if [ "x${RHEL}" = "x8" ]; then
            yum -y install dnf-plugins-core epel-release
            yum config-manager --set-enabled powertools
            yum -y install python2-scons || true
            yum -y install python2-pip python36-devel
            yum -y install autoconf automake binutils bison boost-static cmake gcc gcc-c++ make
            yum -y install git gperf glibc glibc-devel jemalloc jemalloc-devel libaio-devel
            yum -y install libstdc++-devel libtirpc-devel make ncurses-devel numactl-devel
            yum -y install openldap-devel openssl-devel pam-devel perl-Data-Dumper
            yum -y install perl-Dig perl-Digest perl-Digest-MD5 perl-Env perl-JSON perl-Time-HiRes
            yum -y install readline-devel rpm-build rsync tar time unzip wget zlib-devel selinux-policy-devel
            yum -y install bison boost-devel check-devel cmake gcc-c++ glibc-devel libaio-devel libcurl-devel
            wget https://archives.fedoraproject.org/pub/archive/fedora/linux/releases/30/Everything/x86_64/os/Packages/r/rpcgen-1.4-2.fc30.x86_64.rpm
            wget ftp://ftp.pbone.net/mirror/archive.fedoraproject.org/fedora/linux/releases/29/Everything/x86_64/os/Packages/g/gperf-3.1-6.fc29.x86_64.rpm
            yum -y install rpcgen-1.4-2.fc30.x86_64.rpm gperf-3.1-6.fc29.x86_64.rpm

            wget https://jenkins.percona.com/yum-repo/percona-dev.repo
            mv -vf percona-dev.repo /etc/yum.repos.d
            yum -y clean all
            yum -y install python2-scons python2-pip python36-devel
            yum -y install redhat-rpm-config python2-devel
            /usr/bin/pip3.6 install --user typing pyyaml regex Cheetah3
            /usr/bin/pip2.7 install --user typing pyyaml regex Cheetah
            dnf -y module disable mysql
        else
            yum -y install epel-release
            yum -y install git numactl-devel wget rpm-build gcc-c++ gperf ncurses-devel perl readline-devel openssl-devel jemalloc zstd zstd-devel
            yum -y install time zlib-devel libaio-devel bison cmake pam-devel libeatmydata autoconf automake jemalloc-devel make
            yum -y install perl-Time-HiRes openldap-devel unzip wget libcurl-devel boost-static selinux-policy-devel
            yum -y install perl-Env perl-Data-Dumper perl-JSON MySQL-python perl-Digest perl-Digest-MD5 perl-Digest-Perl-MD5 || true
            until yum -y install centos-release-scl; do
                echo "waiting"
                sleep 1
            done
            yum -y install  gcc-c++ devtoolset-8-gcc-c++ devtoolset-8-binutils
            source /opt/rh/devtoolset-8/enable
            yum -y install scons check-devel boost-devel cmake3
            yum -y install zstd libzstd libzstd-devel
            alternatives --install /usr/local/bin/cmake cmake /usr/bin/cmake 10 \
--slave /usr/local/bin/ctest ctest /usr/bin/ctest \
--slave /usr/local/bin/cpack cpack /usr/bin/cpack \
--slave /usr/local/bin/ccmake ccmake /usr/bin/ccmake \
--family cmake
            alternatives --install /usr/local/bin/cmake cmake /usr/bin/cmake3 20 \
--slave /usr/local/bin/ctest ctest /usr/bin/ctest3 \
--slave /usr/local/bin/cpack cpack /usr/bin/cpack3 \
--slave /usr/local/bin/ccmake ccmake /usr/bin/ccmake3 \
--family cmake
        fi
        yum -y install yum-utils patchelf
    else
        apt-get -y update
        DEBIAN_FRONTEND=noninteractive apt-get -y install curl lsb-release wget apt-transport-https software-properties-common
        apt-get -y install dirmngr || true
        apt-get update
        apt-get -y install dirmngr || true
        wget https://repo.percona.com/apt/percona-release_latest.$(lsb_release -sc)_all.deb && dpkg -i percona-release_latest.$(lsb_release -sc)_all.deb
        percona-release enable tools testing
        percona-release enable pxb-80 testing
        percona-release enable pxb-24 testing
        export DEBIAN_FRONTEND="noninteractive"
        export DIST="$(lsb_release -sc)"
            until apt-get update; do
            sleep 1
            echo "waiting"
        done
        apt-get -y purge eatmydata || true
        apt-get update
        apt-get -y install psmisc
        apt-get -y install libsasl2-modules:amd64 || apt-get -y install libsasl2-modules
        apt-get -y install dh-systemd || true
        apt-get -y install curl bison cmake perl libssl-dev gcc g++ libaio-dev libldap2-dev libwrap0-dev gdb unzip gawk
        apt-get -y install lsb-release libmecab-dev libncurses5-dev libreadline-dev libpam-dev zlib1g-dev libcurl4-gnutls-dev
        apt-get -y install libldap2-dev libnuma-dev libjemalloc-dev libeatmydata libc6-dbg valgrind libjson-perl libsasl2-dev
        apt-get -y install patchelf
        if [ x"${DIST}" = xfocal -o x"${DIST}" = xbullseye ]; then
            apt-get -y install python3-mysqldb
        else
            apt-get -y install python-mysqldb
        fi
        apt-get -y install libmecab2 mecab mecab-ipadic
        apt-get -y install build-essential devscripts
        apt-get -y install cmake autotools-dev autoconf automake build-essential devscripts debconf debhelper fakeroot
        apt-get -y install libtool libnuma-dev scons libboost-dev libboost-program-options-dev check
        apt-get -y install doxygen doxygen-gui graphviz rsync libcurl4-openssl-dev
        apt-get -y install libcurl4-openssl-dev libre2-dev pkg-config libtirpc-dev libev-dev
        apt-get -y install --download-only percona-xtrabackup-24=2.4.23-1.${DIST}
        apt-get -y install --download-only percona-xtrabackup-80=8.0.25-17-1.${DIST}
    fi
    return;
}

get_tar(){
    TARBALL=$1
    TARFILE=$(basename $(find $WORKDIR/$TARBALL -iname 'percona-xtradb-cluster*.tar.gz' | sort | tail -n1))
    if [ -z $TARFILE ]
    then
        TARFILE=$(basename $(find $CURDIR/$TARBALL -iname 'percona-xtradb-cluster*.tar.gz' | sort | tail -n1))
        if [ -z $TARFILE ]
        then
            echo "There is no $TARBALL for build"
            exit 1
        else
            cp $CURDIR/$TARBALL/$TARFILE $WORKDIR/$TARFILE
        fi
    else
        cp $WORKDIR/$TARBALL/$TARFILE $WORKDIR/$TARFILE
    fi
    return
}

get_deb_sources(){
    param=$1
    echo $param
    FILE=$(basename $(find $WORKDIR/source_deb -iname "percona-xtradb-cluster*.$param" | sort | tail -n1))
    if [ -z $FILE ]
    then
        FILE=$(basename $(find $CURDIR/source_deb -iname "percona-xtradb-cluster*.$param" | sort | tail -n1))
        if [ -z $FILE ]
        then
            echo "There is no sources for build"
            exit 1
        else
            cp $CURDIR/source_deb/$FILE $WORKDIR/
        fi
    else
        cp $WORKDIR/source_deb/$FILE $WORKDIR/
    fi
    return
}

build_srpm(){
    if [ $SRPM = 0 ]
    then
        echo "SRC RPM will not be created"
        return;
    fi
    if [ "x$OS" = "xdeb" ]
    then
        echo "It is not possible to build src rpm here"
        exit 1
    fi
    cd $WORKDIR || exit
    get_tar "source_tarball"
    rm -fr rpmbuild
    mkdir -vp rpmbuild/{SOURCES,SPECS,BUILD,SRPMS,RPMS}
    source ${WORKDIR}/pxc-80.properties
    TARFILE=$(basename $(find . -iname 'Percona-XtraDB-Cluster-*.tar.gz' | sort | tail -n1))
    NAME="Percona-XtraDB-Cluster"
    VERSION=$MYSQL_VERSION
    RELEASE=$MYSQL_RELEASE

    mkdir -vp rpmbuild/{SOURCES,SPECS,BUILD,SRPMS,RPMS}
    #
    cd ${WORKDIR}/rpmbuild/SPECS || exit
    tar vxzf ${WORKDIR}/${TARFILE} --wildcards '*/build-ps/*.spec' --strip=2
    #
    cd ${WORKDIR} || exit
    #
    mv -fv ${TARFILE} ${WORKDIR}/rpmbuild/SOURCES
    cd ${WORKDIR}/rpmbuild/SOURCES || exit
    tar -xzf ${TARFILE}
    rm -rf ${TARFILE}
    PXCDIR=$(ls | grep 'Percona-XtraDB-Cluster*' | sort | tail -n1)
    mv ${PXCDIR} Percona-XtraDB-Cluster-${MYSQL_VERSION}
    tar -zcf Percona-XtraDB-Cluster-${MYSQL_VERSION}.tar.gz Percona-XtraDB-Cluster-${MYSQL_VERSION}
    rm -rf ${PXCDIR}
    cd ${WORKDIR} || exit
    #
    sed -i "s:@@MYSQL_VERSION@@:${VERSION}:g" rpmbuild/SPECS/percona-xtradb-cluster.spec
    sed -i "s:@@PERCONA_VERSION@@:${RELEASE}:g" rpmbuild/SPECS/percona-xtradb-cluster.spec
    sed -i "s:@@WSREP_VERSION@@:${WSREP_VERSION}:g" rpmbuild/SPECS/percona-xtradb-cluster.spec
    sed -i "s:@@REVISION@@:${REVISION}:g" rpmbuild/SPECS/percona-xtradb-cluster.spec

    #

    SRCRPM=$(find . -name *.src.rpm)
    RHEL=$(rpm --eval %rhel)
    #
    ARCH=$(uname -m)
    if [ ${ARCH} = i686 ]; then
        ARCH=i386
    fi

    if test "x$(uname -m)" == "xx86_64"
    then
        SCONS_ARGS="bpostatic=/usr/lib64/libboost_program_options.a"
    else
        SCONS_ARGS=""
    fi
    source ${WORKDIR}/pxc-80.properties
    if [ -n ${SRCRPM} ]; then
        if test "x${SCONS_ARGS}" == "x"
        then
            rpmbuild -bs --define "_topdir ${WORKDIR}/rpmbuild" --define "rpm_version $MYSQL_RELEASE.$RPM_RELEASE" --define "galera_revision ${GALERA_REVNO}" --define "dist generic" rpmbuild/SPECS/percona-xtradb-cluster.spec
        else
            rpmbuild -bs --define "_topdir ${WORKDIR}/rpmbuild" --define "rpm_version $MYSQL_RELEASE.$RPM_RELEASE" --define "galera_revision ${GALERA_REVNO}" --define "dist generic" --define "scons_args ${SCONS_ARGS}" rpmbuild/SPECS/percona-xtradb-cluster.spec
        fi
    fi
    #
    echo "RPM_RELEASE=$RPM_RELEASE" >> pxc-80.properties

    mkdir -p ${WORKDIR}/srpm
    mkdir -p ${CURDIR}/srpm
    cp rpmbuild/SRPMS/*.src.rpm ${CURDIR}/srpm
    cp rpmbuild/SRPMS/*.src.rpm ${WORKDIR}/srpm
    return
}

build_mecab_lib(){
    MECAB_TARBAL="mecab-0.996.tar.gz"
    MECAB_LINK="http://jenkins.percona.com/downloads/mecab/${MECAB_TARBAL}"
    MECAB_DIR="${WORKDIR}/${MECAB_TARBAL%.tar.gz}"
    MECAB_INSTALL_DIR="${WORKDIR}/mecab-install"
    rm -f ${MECAB_TARBAL}
    rm -rf ${MECAB_DIR}
    rm -rf ${MECAB_INSTALL_DIR}
    mkdir ${MECAB_INSTALL_DIR}
    wget ${MECAB_LINK}
    tar xf ${MECAB_TARBAL}
    cd ${MECAB_DIR} || exit
    ./configure --with-pic --prefix=/usr
    make
    make check
    make DESTDIR=${MECAB_INSTALL_DIR} install
    cd ${WORKDIR} || exit
}

build_mecab_dict(){
    MECAB_IPADIC_TARBAL="mecab-ipadic-2.7.0-20070801.tar.gz"
    MECAB_IPADIC_LINK="http://jenkins.percona.com/downloads/mecab/${MECAB_IPADIC_TARBAL}"
    MECAB_IPADIC_DIR="${WORKDIR}/${MECAB_IPADIC_TARBAL%.tar.gz}"
    rm -f ${MECAB_IPADIC_TARBAL}
    rm -rf ${MECAB_IPADIC_DIR}
    wget ${MECAB_IPADIC_LINK}
    tar xf ${MECAB_IPADIC_TARBAL}
    cd ${MECAB_IPADIC_DIR} || exit
    # these two lines should be removed if proper packages are created and used for builds
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${MECAB_INSTALL_DIR}/usr/lib
    sed -i "/MECAB_DICT_INDEX=\"/c\MECAB_DICT_INDEX=\"${MECAB_INSTALL_DIR}\/usr\/libexec\/mecab\/mecab-dict-index\"" configure
    #
    ./configure --with-mecab-config=${MECAB_INSTALL_DIR}/usr/bin/mecab-config
    make
    make DESTDIR=${MECAB_INSTALL_DIR} install
    cd ../  || exit
    cd ${MECAB_INSTALL_DIR}  || exit
    if [ -d usr/lib64 ]; then
        mv usr/lib64/* usr/lib
    fi
    cd ${WORKDIR}  || exit
}

build_rpm(){
    if [ $RPM = 0 ]
    then
        echo "RPM will not be created"
        return;
    fi
    if [ "x$OS" = "xdeb" ]
    then
        echo "It is not possible to build rpm here"
        exit 1
    fi
    SRC_RPM=$(basename $(find $WORKDIR/srpm -iname 'percona-xtradb-cluster*.src.rpm' | sort | tail -n1))
    if [ -z $SRC_RPM ]
    then
        SRC_RPM=$(basename $(find $CURDIR/srpm -iname 'percona-xtradb-cluster*.src.rpm' | sort | tail -n1))
        if [ -z $SRC_RPM ]
        then
            echo "There is no src rpm for build"
            echo "You can create it using key --build_src_rpm=1"
            exit 1
        else
            cp $CURDIR/srpm/$SRC_RPM $WORKDIR
        fi
    else
        cp $WORKDIR/srpm/$SRC_RPM $WORKDIR
    fi
    cd $WORKDIR  || exit
    rm -fr rpmbuild
    mkdir -vp rpmbuild/{SOURCES,SPECS,BUILD,SRPMS,RPMS}
    cp $SRC_RPM rpmbuild/SRPMS/

    RHEL=$(rpm --eval %rhel)
    ARCH=$(echo $(uname -m) | sed -e 's:i686:i386:g')
    #
    echo "RHEL=${RHEL}" >> pxc-80.properties
    echo "ARCH=${ARCH}" >> pxc-80.properties
    #
    SRCRPM=$(basename $(find . -name '*.src.rpm' | sort | tail -n1))
    mkdir -vp rpmbuild/{SOURCES,SPECS,BUILD,SRPMS,RPMS}
    #
    mv *.src.rpm rpmbuild/SRPMS
    source /opt/rh/devtoolset-8/enable
    build_mecab_lib
    build_mecab_dict

    cd ${WORKDIR}  || exit
    source /opt/rh/devtoolset-8/enable
    source ${WORKDIR}/pxc-80.properties
    source ${CURDIR}/srpm/pxc-80.properties
    #
    if [ ${ARCH} = x86_64 ]; then
        rpmbuild --define "_topdir ${WORKDIR}/rpmbuild" --define "dist el${RHEL}" --define "rpm_version $MYSQL_RELEASE.$RPM_RELEASE" --define "rel $RPM_RELEASE" --define "galera_revision ${GALERA_REVNO}" --define "with_mecab ${MECAB_INSTALL_DIR}/usr" --rebuild rpmbuild/SRPMS/${SRCRPM}
    else
        rpmbuild --define "_topdir ${WORKDIR}/rpmbuild" --define "dist el${RHEL}" --define "rpm_version $MYSQL_RELEASE.$RPM_RELEASE" --define "rel $RPM_RELEASE" --define "galera_revision ${GALERA_REVNO}" --define "with_tokudb 0" --define "with_rocksdb 0" --define "with_mecab ${MECAB_INSTALL_DIR}/usr" --rebuild rpmbuild/SRPMS/${SRCRPM}
    fi
    return_code=$?
    if [ $return_code != 0 ]; then
        exit $return_code
    fi
    mkdir -p ${WORKDIR}/rpm
    mkdir -p ${CURDIR}/rpm
    cp rpmbuild/RPMS/*/*.rpm ${WORKDIR}/rpm
    cp rpmbuild/RPMS/*/*.rpm ${CURDIR}/rpm
}

build_source_deb(){
    if [ $SDEB = 0 ]
    then
        echo "source deb package will not be created"
        return;
    fi
    if [ "x$OS" = "xrpm" ]
    then
        echo "It is not possible to build source deb here"
        exit 1
    fi
     source ${WORKDIR}/pxc-80.properties
    rm -rf percona-server*
    get_tar "source_tarball"
    rm -f *.dsc *.orig.tar.gz *.debian.tar.gz *.changes
    #

    TARFILE=$(basename $(find . -iname 'percona-xtradb-cluster*.tar.gz' | grep -v tokudb | sort | tail -n1))
    NAME='percona-xtradb-cluster'
    HNAME='Percona-XtraDB-Cluster'

    VERSION=$MYSQL_VERSION
    SHORTVER=$(echo ${VERSION} | awk -F '.' '{print $1"."$2}')
    RELEASE=$MYSQL_RELEASE
    rm -fr ${HNAME}-${VERSION}

    #
    NEWTAR=${NAME}_${VERSION}-${RELEASE}.orig.tar.gz
    mv ${TARFILE} ${NEWTAR}

    DEBIAN_VERSION="$(lsb_release -sc)"

    #
    tar xzf ${NEWTAR}
    cd ${HNAME}-${VERSION}-${MYSQL_RELEASE} || exit
    cp -ap build-ps/debian/ .
    sed -i "s:@@MYSQL_VERSION@@:${VERSION}:g" debian/changelog
    sed -i "s:@@PERCONA_VERSION@@:${RELEASE}:g" debian/changelog

    sed -i "s:@@REVISION@@:${REVISION}:g" debian/rules
    sed -i "s:@@PERCONA_VERSION@@:${RELEASE}:g" debian/rules
    sed -i "s:@@WSREP_VERSION@@:${WSREP_VERSION}:g" debian/rules
    sed -i "s:@@DEB_RELEASE@@:${DEB_RELEASE}:g" debian/rules

    sed -i "s:libcurl4-gnutls-dev:libcurl4-openssl-dev:g" debian/control

    dch -D UNRELEASED --force-distribution -v "$MYSQL_VERSION-$MYSQL_RELEASE-$DEB_RELEASE" "Update to new upstream release Percona XtraDB Cluster ${VERSION}-rel${RELEASE}"
    dpkg-buildpackage -S
    #
    rm -fr ${HNAME}-${VERSION}-${RELEASE}

    cd ${WORKDIR} || exit

    mkdir -p $WORKDIR/source_deb
    mkdir -p $CURDIR/source_deb
    cp *.debian.tar.* $WORKDIR/source_deb
    cp *_source.changes $WORKDIR/source_deb
    cp *.dsc $WORKDIR/source_deb
    cp *.orig.tar.gz $WORKDIR/source_deb
    cp *.debian.tar.* $CURDIR/source_deb
    cp *_source.changes $CURDIR/source_deb
    cp *.dsc $CURDIR/source_deb
    cp *.orig.tar.gz $CURDIR/source_deb
}

build_deb(){
    if [ $DEB = 0 ]
    then
        echo "source deb package will not be created"
        return;
    fi
    if [ "x$OS" = "xrpm" ]
    then
        echo "It is not possible to build source deb here"
        exit 1
    fi
    for file in 'dsc' 'orig.tar.gz' 'changes' 'debian.tar*'
    do
        get_deb_sources $file
    done
    cd $WORKDIR || exit
    source ${WORKDIR}/pxc-80.properties
    rm -fv *.deb

    export DEBIAN_VERSION="$(lsb_release -sc)"

    if test -e "/proc/cpuinfo"
    then
        PROCESSORS="$(grep -c ^processor /proc/cpuinfo)"
    else
        PROCESSORS=4
    fi
    export MAKE_JFLAG="${MAKE_JFLAG:--j$PROCESSORS}"
    ARCH=$(uname -m)
    DEBIAN_VERSION="$(lsb_release -sc)"

    COMMON_FLAGS="-Wall -Wp,-D_FORTIFY_SOURCE=2 -Wno-error=unused-result -Wno-error=literal-suffix -Wno-misleading-indentation -Wno-error=deprecated-declarations -Wno-error=nonnull-compare -DPERCONA_INNODB_VERSION=$MYSQL_RELEASE "
    export CFLAGS=" $COMMON_FLAGS -static-libgcc ${CFLAGS:-}"
    export CXXFLAGS=" $COMMON_FLAGS -Wno-virtual-move-assign  ${CXXFLAGS:-}"

    DSC=$(basename $(find . -name '*.dsc' | sort | tail -n 1))
    DIRNAME=$(echo ${DSC} | sed -e 's:_:-:g' | awk -F'-' '{print $1"-"$2"-"$3"-"$4"-"$5}' | sed -e s:.dsc::)
    rm -rf $DIRNAME

    #
    echo "ARCH=${ARCH}" >> ${WORKDIR}/pxc-80.properties
    echo "DEBIAN_VERSION=${DEBIAN_VERSION}" >> ${WORKDIR}/pxc-80.properties

    dpkg-source -x ${DSC}

    cd ${DIRNAME} || exit

    mkdir pxb-2.4
    mkdir pxb-8.0
    dpkg-deb -R /var/cache/apt/archives/percona-xtrabackup-24* pxb-2.4
    dpkg-deb -R /var/cache/apt/archives/percona-xtrabackup-80* pxb-8.0
    cd pxb-2.4 || exit
        mv usr/bin ./
        mv usr/lib* ./
        rm -rf usr *.deb DEBIAN
    cd ../pxb-8.0 || exit
        mv usr/bin ./
        mv usr/lib* ./
        rm -rf usr *.deb DEBIAN
    cd ../ || exit

    if [[ "x$DEBIAN_VERSION" == "xbionic" || "x$DEBIAN_VERSION" == "xstretch" || "x$DEBIAN_VERSION" == "xfocal" || "x$DEBIAN_VERSION" == "xbullseye" ]]; then
        sed -i 's/fabi-version=2/fabi-version=2 -Wno-error=deprecated-declarations -Wno-error=nonnull-compare -Wno-error=literal-suffix -Wno-misleading-indentation/' cmake/build_configurations/compiler_options.cmake
        sed -i 's/gnu++11/gnu++11 -Wno-virtual-move-assign/' cmake/build_configurations/compiler_options.cmake
    fi

    #==========
    export DEB_CFLAGS_APPEND="$CFLAGS" DEB_CXXFLAGS_APPEND="$CXXFLAGS"
    export MYSQL_BUILD_CFLAGS="$CFLAGS"
    export MYSQL_BUILD_CXXFLAGS="$CXXFLAGS"

    if [[ "x$DEBIAN_VERSION" == "xfocal" || "x${DEBIAN_VERSION}" == "xbionic" || "x${DEBIAN_VERSION}" == "xbuster" || "x$DEBIAN_VERSION" == "xbullseye" ]]; then
        sed -i "s:iproute:iproute2:g" debian/control
    fi
    sed -i "s:libcurl4-gnutls-dev:libcurl4-openssl-dev:g" debian/control
    chmod 777 debian/rules
    dch -b -m -D "$DEBIAN_VERSION" --force-distribution -v "1:$MYSQL_VERSION-$MYSQL_RELEASE-$DEB_RELEASE.${DEBIAN_VERSION}" 'Update distribution'
    #
    GALERA_REVNO="${GALERA_REVNO}" SCONS_ARGS=' strict_build_flags=0'  MAKE_JFLAG=-j4  dpkg-buildpackage -rfakeroot -uc -us -b
    #
    cd ${WORKSPACE} || exit
    rm -fv *.dsc *.orig.tar.gz *.debian.tar.gz *.changes
    rm -fr ${DIRNAME}


    cd ${WORKDIR} || exit
    mkdir -p $CURDIR/deb
    mkdir -p $WORKDIR/deb
    cp $WORKDIR/*.deb $WORKDIR/deb
    cp $WORKDIR/*.deb $CURDIR/deb
}

build_tarball(){
    if [ $TARBALL = 0 ]
    then
        echo "Binary tarball will not be created"
        return;
    fi
    source ${WORKDIR}/pxc-80.properties
    get_tar "source_tarball"
    cd $WORKDIR || exit
    source ${WORKDIR}/pxc-80.properties
    TARFILE=$(basename $(find . -iname 'percona-xtradb-cluster*.tar.gz' | sort | tail -n1))
    if [ -f /etc/debian_version ]; then
        export OS_RELEASE="$(lsb_release -sc)"
    fi
    #
    if [ -f /etc/redhat-release ]; then
        export OS_RELEASE="centos$(lsb_release -sr | awk -F'.' '{print $1}')"
        RHEL=$(rpm --eval %rhel)
        source /opt/rh/devtoolset-8/enable
    fi
    #

    ARCH=$(uname -m 2>/dev/null||true)
    JVERSION=${JEMALLOC_VERSION:-4.0.0}

    ROOT_FS=$(pwd)

    TARFILE=$(basename $(find . -iname 'Percona-XtraDB-Cluster*.tar.gz' | grep -v 'galera' | sort | tail -n1))
    NAME="Percona-XtraDB-Cluster"
    VERSION=$MYSQL_VERSION
    RELEASE=$WSREP_VERSION

    export PXC_DIRNAME=${NAME}-${VERSION}-${MYSQL_RELEASE}

    tar zxf $TARFILE
    rm -f $TARFILE
    #
    cd ${NAME}-${VERSION}-${MYSQL_RELEASE} || exit

    BUILD_NUMBER=$(date "+%Y%m%d-%H%M%S")
    mkdir -p $BUILD_NUMBER

    BUILD_ROOT="$ROOT_FS/$PXC_DIRNAME/$BUILD_NUMBER"
    mkdir -p ${BUILD_ROOT}
    CURDIR=$(pwd)
    cd ${BUILD_ROOT} || exit
    if [ -f /etc/redhat-release ]; then
        mkdir pxb-2.4
        pushd pxb-2.4
        yumdownloader percona-xtrabackup-24-2.4.22
        rpm2cpio *.rpm | cpio --extract --make-directories --verbose
        mv usr/bin ./
        mv usr/lib* ./
        mv lib64 lib
        mv lib/xtrabackup/* lib/ || true
        rm -rf lib/xtrabackup
        rm -rf usr
        rm -f *.rpm
        popd

        mkdir pxb-8.0
        pushd pxb-8.0
        yumdownloader percona-xtrabackup-80-8.0.23
        rpm2cpio *.rpm | cpio --extract --make-directories --verbose
        mv usr/bin ./
        mv usr/lib64 ./
        mv lib64 lib
        mv lib/xtrabackup/* lib/
        rm -rf lib/xtrabackup
        rm -rf usr
        rm -f *.rpm
        popd
        tar -zcvf  percona-xtrabackup-2.4.tar.gz pxb-2.4
        tar -zcvf  percona-xtrabackup-8.0.tar.gz pxb-8.0
        rm -rf pxb-8.0 pxb-2.4
    else
        mkdir pxb-2.4
        mkdir pxb-8.0
        dpkg-deb -R /var/cache/apt/archives/percona-xtrabackup-24* pxb-2.4
        dpkg-deb -R /var/cache/apt/archives/percona-xtrabackup-80* pxb-8.0
        cd pxb-2.4 || exit
            mv usr/bin ./
            mv usr/lib* ./
            rm -rf usr *.deb DEBIAN
        cd ../pxb-8.0 || exit
            mv usr/bin ./
            mv usr/lib* ./
            rm -rf usr *.deb DEBIAN
        cd ../ || exit
        tar -zcvf  percona-xtrabackup-2.4.tar.gz pxb-2.4
        tar -zcvf  percona-xtrabackup-8.0.tar.gz pxb-8.0
        rm -rf pxb-8.0 pxb-2.4
    fi
    mkdir -p ${BUILD_ROOT}/target/pxc_extra/
    cp *.tar.gz ${BUILD_ROOT}/target/pxc_extra/
    cp *.tar.gz ${BUILD_ROOT}/target
    cd ${CURDIR} || exit
    rm -rf jemalloc
    wget https://github.com/jemalloc/jemalloc/releases/download/$JVERSION/jemalloc-$JVERSION.tar.bz2
    tar xf jemalloc-$JVERSION.tar.bz2
    mv jemalloc-$JVERSION jemalloc

    export SCONS_ARGS=" strict_build_flags=0"
    if [ -f /etc/redhat-release ]; then
        sed -i 's:cmake ../../:/usr/bin/cmake3 ../../:g' ./build-ps/build-binary.sh
    fi
    if [[ ${DEBUG} == 1 ]]; then
        bash -x ./build-ps/build-binary.sh --debug --with-jemalloc=jemalloc/ -t $BIN_RELEASE $BUILD_ROOT
    else
        bash -x ./build-ps/build-binary.sh --with-jemalloc=jemalloc/ -t $BIN_RELEASE $BUILD_ROOT
    fi

    DIRNAME="tarball"
    if [[ ${DEBUG} == 1 ]]; then
        DIRNAME="debug"
    fi
    mkdir -p ${WORKDIR}/../${DIRNAME}
    mkdir -p ${WORKDIR}/${DIRNAME}
    mkdir -p ${CURDIR}/${DIRNAME}
    rm -f $BUILD_NUMBER/percona-xtrabackup* || true
    cp  $BUILD_NUMBER/*.tar.gz ${WORKDIR}/../${DIRNAME}
    cp  $BUILD_NUMBER/*.tar.gz ${WORKDIR}/${DIRNAME}
    cp  $BUILD_NUMBER/*.tar.gz ${CURDIR}/${DIRNAME}

}

#main

CURDIR=$(pwd)
VERSION_FILE=$CURDIR/pxc-80.properties
args=
WORKDIR=
SRPM=0
SDEB=0
RPM=0
DEB=0
SOURCE=0
NO_CLONE=0
TARBALL=0
OS_NAME=
ARCH=
OS=
INSTALL=0
RPM_RELEASE=1
DEB_RELEASE=1
BIN_RELEASE=1
DEBUG=0
REVISION=0
BRANCH="8.0"
MECAB_INSTALL_DIR="${WORKDIR}/mecab-install"
REPO="https://github.com/percona/percona-xtradb-cluster.git"
PRODUCT=Percona-XtraDB-Cluster-8.0
MYSQL_VERSION=8.0.13
MYSQL_RELEASE=3
WSREP_VERSION=31.33
PRODUCT_FULL=Percona-XtraDB-Cluster-8.0.13-31.33
BOOST_PACKAGE_NAME=boost_1_59_0
parse_arguments PICK-ARGS-FROM-ARGV "$@"

check_workdir
get_system
install_deps
get_sources
build_tarball
build_srpm
build_source_deb
build_rpm
build_deb
