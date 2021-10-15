.. _PXC-5.7.34-31.51:

================================================================================
*Percona XtraDB Cluster* 5.7.34-31.51
================================================================================

:Date: July 19, 2021
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.7/install/index.html>`_

Improvements
================================================================================

* :jirabug:`PXC-3634`: Erroneous documentation on bootstrapping the XtraDB Cluster (Thanks to user Craig Fisher for reporting this issue)
* :jirabug:`PXC-3092`: Log a warning at startup if a keyring is specified, but the cluster traffic encryption is turned off



Bugs Fixed
================================================================================

* :jirabug:`PXC-3679`: SST fails after the update of socat to '1.7.4.0'
* :jirabug:`PXC-3611`: "Encryption can't find master key" after SST when keyring_file is used if the keyring.backup file exists
* :jirabug:`PXC-3608`: Attempting to read a FK may cause a memory access violation and server exit.
* :jirabug:`PXC-2650`: Lack of support for the data-at-rest encryption options are not mentioned in documentation
* :jirabug:`PXC-3464`: Data is not propagated with SET SESSION sql_log_bin = 0
* :jirabug:`PXC-3666`: Session with wsrep_on=0 blocks TOI transactions
* :jirabug:`PXC-3596`: Node stuck in aborting SST
* :jirabug:`PXC-3226`: Results from CHECK TABLE from PXC server can cause the client libraries to crash
* :jirabug:`PXC-3146`: Galera/SST does not look at the default data directory location for SSL certs


