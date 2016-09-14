.. rn:: 5.6.30-25.16.2

=====================================
Percona XtraDB Cluster 5.6.30-25.16.2
=====================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.30-25.16.2 on September 15, 2016.
Binaries are available from the
`downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.30-25.16.2/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.30-25.16.2 is now the current release,
based on the following:

* `Percona Server 5.6.30-76.3
  <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.30-76.3.html>`_

* Galera Replication library 3.16

* wsrep API version 25

This release is providing a fix for `CVE-2016-6662
<https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2016-6662>`_.

Bug Fixed
=========

 Due to security reasons ``ld_preload`` libraries can now only be loaded from
 the system directories (``/usr/lib64``, ``/usr/lib``) and the *MySQL*
 installation base directory.
