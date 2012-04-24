===========================
How to Execute Kewpie Tests
===========================

To use kewpie for testing it's recommended to use `this MP <https://code.launchpad.net/~patrick-crews/percona-xtradb-cluster/qp-integrate/+merge/93648>`_. As it removes dbqp and integrates kewpie (and cuts size down to 25MB from 400+).
To execute tests: 

.. code-block:: bash

   cd kewpie ; ./kewpie.py [--force ] [--libeatmydata] [--wsrep-provider-path=...]
                  
The defaults are to run the cluster_basic and cluster_randgen suites against a 3 node cluster. Cluster_basic is used for small atomic tests like ADD/DROP single/multiple columns on a table and ensuring the change is replicated. cluster_randgen is used for high stress transactional loads.  There are single and multi-threaded variants.  The load is a mix of INSERT/UPDATE/DELETE/SELECT statements.  This includes both regular transactions, single queries, ROLLBACK's and SAVEPOINTs, and a mix of good and bad SQL statements.
               
To view all options, one may look at "./kewpie.py --help".  Basic documentation is also available as sphinx docs in kewpie/docs folder. Here are the some of the most used options:

.. option:: --force 

   Run all tests despite failures (default is to stop test execution on first failure)

.. option:: --libeatmydata 

   Use libeatmydata if installed. This can greatly speed up testing in many cases.  Can be used in conjunction with:

.. option:: --libeatmydata-path to specify where the library is located.

.. option:: --wsrep-provider-path  
   
   By default, we expect / look for it in /usr/lib/galera/libgalera_smm.so (where it ends up via 'make install'...at least on Ubuntu).  If one has an alternate library/location, specify it with this option.

Any additional suites may be run this way: 

.. code-block:: bash

   ./kewpie.py [options] --suite=any/suitedir/from/kewpie/percona_tests
   ./kewpie.py --suite=crashme
