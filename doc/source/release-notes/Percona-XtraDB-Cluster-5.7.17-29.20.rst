.. rn:: 5.7.17-29.20

===================================
Percona XtraDB Cluster 5.7.17-29.20
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.7.17-29.20 on April 13, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster 5.7.17-29.20 is now the current release,
based on the following:

* `Percona Server 5.7.17-13 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.17-13.html>`_

* Galera Replication library 3.20

* wsrep API version 29

All Percona software is open-source and free.
Details of this release can be found in the
`5.7.17-29.20 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.7.17-29.20>`_.

Fixed Bugs
==========

* PXC-692: Improved SST and IST log messages
  for better readability and unification.

* PXC-694: Excluded the ``garbd`` node from flow control calculations.

* PXC-725: Added extra checks to verify that SSL files
  (certificate, certificate authority, and key)
  are compatible before openning connection.

* PXC-727: Improved parallelism for better scaling with multiple threads.

* PXC-754: Added validations for ``DISCARD TABLESPACE``
  and ``IMPORT TABLESPACE`` in :ref:`pxc-strict-mode`
  to prevent data inconsistency.

* PXC-759: Added the :variable:`wsrep_flow_control_status` variable
  to indicate if node is in flow control (paused).

* :jirabug:`PXC-766`: Added the :variable:`wsrep_ist_receive_status` variable
  to show progress during an IST.

* PXC-768: Allowed ``CREATE TABLE ... AS SELECT`` (CTAS) statements
  with temporary tables (``CREATE TEMPORARY TABLE ... AS SELECT``)
  in :ref:`pxc-strict-mode`.

* :jirabug:`PXC-783`: Improved the wsrep stage framework.

* :jirabug:`PXC-795`: Set ``--parallel=4`` as default option
  for ``wsrep_sst_xtrabackup-v2`` to run four threads with XtraBackup.

* :jirabug:`PXC-805`: Inherited upstream fix
  to avoid using deprecated variables,
  such as ``INFORMATION_SCHEMA.SESSION_VARIABLE``.

* Moved wsrep settings into a separate configuration file
  (:file:`/etc/my.cnf.d/wsrep.cnf`).

* Fixed ``mysqladmin shutdown`` to correctly stop the server
  on systems using ``systemd``.

