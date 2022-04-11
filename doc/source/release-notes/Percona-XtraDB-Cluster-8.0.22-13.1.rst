.. _PXC-8.0.22-13.1:

================================================================================
*Percona XtraDB Cluster* 8.0.22-13.1
================================================================================

:Date: March 22, 2021
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/8.0/install/index.html>`_

Percona XtraDB Cluster 8.0.22-13.1 includes all of the features and bug fixes available in Percona Server for MySQL. See the corresponding `release notes for Percona Server for MySQL 8.0.22-13 <https://www.percona.com/doc/percona-server/LATEST/release-notes/Percona-Server-8.0.22-13.html>`__ for more details on these changes.

This release fixes security vulnerability `CVE-2021-27928 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-27928>`_, a similar issue to `CVE-2020-15180 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-15180>`_

Improvements
================================================================================

* :jirabug:`PXC-3575`: Implement package changes for SELinux and AppArmor
* :jirabug:`PXC-3115`: Create Default SELinux and AppArmor policy



Bugs Fixed
================================================================================

* :jirabug:`PXC-3536`: Modify processing to not allow threads/queries to be killed if the thread is in TOI
* :jirabug:`PXC-3565`: Correct Performance of SELECT in PXC
* :jirabug:`PXC-3502`: Correct condition in thd_binlog_format() function for List Index process (Thanks to user Paweł Bromboszcz for reporting this issue)
* :jirabug:`PXC-3501`: Modify wsrep_row_upd_check_foreign_constraints() to include foreign key dependencies in the writesets for DELETE query (Thanks to user Steven Gales for reporting this issue)
* :jirabug:`PXC-2913`: Correct MDL locks assertion when wsrep provider is unloaded
* :jirabug:`PXC-3475`: Adjust mysqld_safe script to parse 8.0 log style properly



Known Issues (unfixed problems that you should be aware of)
================================================================================

* :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tries to join an SSL-enabled cluster
* :jirabug:`PXC-3092`: Log a warning at startup if a keyring is specified, but cluster traffic encryption is turned off
* :jirabug:`PXC-3093`: Completed SST Transfer incorrectly logged by garbd (Timing is not correct)


