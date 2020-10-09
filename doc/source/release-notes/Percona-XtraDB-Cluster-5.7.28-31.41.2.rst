.. _PXC-5.7.28-31.41.2:

================================================================================
*Percona XtraDB Cluster* 5.7.28-31.41.2
================================================================================

:Date: April 14, 2020
:Installation: `Installing Percona XtraDB Cluster <https://www.percona.com/doc/percona-xtradb-cluster/5.7/install/index.html>`_

Percona XtraDB Cluster 5.7.28-31.41.2 requires `Percona XtraBackup 2.4.20
<https://www.percona.com/doc/percona-xtrabackup/2.4/release-notes/2.4/2.4.20.html>`_.

This release fixes security vulnerability `CVE-2020-10996 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-10996>`_

.. seealso::

   Percona Database Performance Blog
      `This release fixes security vulnerability
      <https://www.percona.com/blog/2020/04/16/cve-2020-10997-percona-xtrabackup-information-disclosure-of-command-line-arguments/>`_

Bugs Fixed
================================================================================

* :jirabug:`PXC-3117`: Transition key was hardcoded

  .. important::

     SST with an encrypted table will fail if sent to a downlevel node:

     - will work: 5.7.28  to 5.7.28.2
     - will not work: 5.7.28.2 to 5.7.28
    
* :jirabug:`PXB-2142`: Transition key was written to backup / stream


