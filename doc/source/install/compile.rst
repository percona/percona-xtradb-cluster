.. _compile:

=========================================
Compiling and Installing from Source Code
=========================================

If you want to compile |PXC|, you can find the source code on
`GitHub <https://github.com/percona/percona-xtradb-cluster>`_.
Before you begin, make sure that the following packages are installed:

.. list-table::
   :header-rows: 1
   :stub-columns: 1

   * - 
     - apt
     - yum
   * - Git
     - ``git``
     - ``git``
   * - SCons
     - ``scons``
     - ``scons``
   * - GCC
     - ``gcc``
     - ``gcc``
   * - g++
     - ``g++``
     - ``gcc-c++``
   * - OpenSSL
     - ``openssl``
     - ``openssl``
   * - Check
     - ``check``
     - ``check``
   * - CMake
     - ``cmake``
     - ``cmake``
   * - Bison
     - ``bison``
     - ``bison``
   * - Boost
     - ``libboost-all-dev``
     - ``boost-devel``
   * - Asio
     - ``libasio-dev``
     - ``asio-devel``
   * - Async I/O
     - ``libaio-dev``
     - ``libaio-devel``
   * - ncurses
     - ``libncurses5-dev``
     - ``ncurses-devel``
   * - Readline
     - ``libreadline-dev``
     - ``readline-devel``
   * - PAM
     - ``libpam-dev``
     - ``pam-devel``

You will likely have all or most of the packages already installed. If you are not sure, run one of the following commands to install any missing dependencies:

.. code-block:: bash

   $ sudo apt-get install -y git scons gcc g++ openssl check cmake bison \
   libboost-all-dev libasio-dev libaio-dev libncurses5-dev libreadline-dev \
   libpam-dev

.. code-block:: bash

   $ sudo yum install -y git scons gcc gcc-c++ openssl check cmake bison \
   boost-devel asio-devel libaio-devel ncurses-devel readline-devel pam-devel

To compile |PXC| from source code:

1. Clone the |PXC| repository:

   .. prompt:: bash

      git clone https://github.com/percona/percona-xtradb-cluster.git

   .. note:: You have to clone the latest repository
      or update it to the latest state.
      Old codebase may not be compatible with the build script.

2. Check out the ``5.7-beta`` branch.

3. Clone Percona's fork of Galera into the same directory:

   .. prompt:: bash

      cd percona-xtradb-cluster
      git clone https://github.com/percona/galera percona-xtradb-cluster-galera

   .. note:: The directory for Galera repository
      must be named ``percona-xtradb-cluster-galera``.

4. Check out the ``rel-3.14.2`` branch.

3. Run the build script :file:`./build-ps/build-binary.sh`.
   By default, it will build into the current directory,
   but you can specify another target output directory.
   For example, if you want to build into :file:`./pxc-build`,
   run the following:

   .. prompt:: bash

      mkdir ./pxc-build
      ./build-ps/build-binary.sh ./pxc-build

