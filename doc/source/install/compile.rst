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
   * - socat
     - ``socat``
     - ``socat``
   * - curl
     - ``libcurl-dev``
     - ``libcurl-devel``

You will likely have all or most of the packages already installed. If you are
not sure, run one of the following commands to install any missing
dependencies:

* For Debian or Ubuntu:

.. code-block:: bash

   $ sudo apt-get install -y git scons gcc g++ openssl check cmake bison \
   libboost-all-dev libasio-dev libaio-dev libncurses5-dev libreadline-dev \
   libpam-dev socat libcurl-dev
   
   .. libcurl-dev (not found on debian; used libcurl4-gnutls-dev
   .. zlib-dev is missing here. on debian worked with zlib1g-dev

* For Red Hat Enterprise Linux or CentOS:

.. code-block:: bash

   $ sudo yum install -y git scons gcc gcc-c++ openssl check cmake bison \
   boost-devel asio-devel libaio-devel ncurses-devel readline-devel pam-devel \
   socat libcurl-devel

To compile |PXC| from source code:

1. Clone the |PXC| repository:

   .. code-block:: bash

      $ git clone https://github.com/percona/percona-xtradb-cluster.git

   .. important::

      Clone the latest repository or update it to the latest state.
      Old codebase may not be compatible with the build script.

#. Check out the ``5.7`` branch:

   .. code-block:: bash

      $ cd percona-xtradb-cluster-galera
      $ git checkout 5.7

#. Initialize the submodule:

   .. code-block:: bash

      $ git submodule init wsrep/src && git submodule update wsrep/src
      $ git submodule init percona-xtradb-cluster-galera && git submodule update percona-xtradb-cluster-galera
      $ cd  percona-xtradb-cluster-galera
      $ git submodule init wsrep/src && git submodule update wsrep/src
      & git submodule init && git submodule update
      $ cd ..

#. Run the build script :file:`./build-ps/build-binary.sh`.
   By default, it attempts building into the current directory. Specify
   the target output directory, such as :file:`./pxc-build`:

   .. code-block:: bash

      $ mkdir ./pxc-build
      $ ./build-ps/build-binary.sh ./pxc-build

When the compilation completes, |pxc-build| contains a tarball, such as |tarball|, that you can deploy on your system.

.. note::

   The exact version and release numbers may differ.


.. |pxc-build| replace:: :file:`pxc-build`
.. |tarball| replace:: :file:`Percona-XtraDB-Cluster-5.7.25-rel28-31.35.1.Linux.x86_64.tar.gz`
   
