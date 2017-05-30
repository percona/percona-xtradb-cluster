.. rn:: 5.7.18-29.20

===================================
Percona XtraDB Cluster 5.7.18-29.20
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.7.18-29.20 on May 31, 2017.
Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_
or from our :ref:`software repositories <install>`.

Percona XtraDB Cluster 5.7.18-29.20 is now the current release,
based on the following:

* `Percona Server 5.7.18-15 <http://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.18-15.html>`_

* Galera Replication library 3.20

* wsrep API version 29

All Percona software is open-source and free.

Fixed Bugs
==========

* :jirabug:`PXC-749`: Fixed memory leak when running ``INSERT``
  on a table without primary key defined
  and :variable:`wsrep_certify_nonPK` disabled (set to ``0``).

  .. note:: It is recommended to have primary keys defined on all tables
     for correct write set replication.

* :jirabug:`PXC-812`: Fixed SST script to leave the DONOR keyring
  when JOINER clears the datadir.

* :jirabug:`PXC-813`: Fixed SST script to use UTC time format.

* :jirabug:`PXC-816`: Fixed hook for caching GTID events
  in asynchronous replication.
  For more information, see :bug:`1681831`.

* :jirabug:`PXC-819`: Added five new status variables
  to expose required values from :variable:`wsrep_ist_receive_status`
  and :variable:`wsrep_flow_control_interval` as numbers,
  rather than strings that need to be parsed:

  * ``wsrep_flow_control_interval_low``
  * ``wsrep_flow_control_interval_high``
  * ``wsrep_ist_receive_seqno_start``
  * ``wsrep_ist_receive_seqno_current``
  * ``wsrep_ist_receive_seqno_end``

* :jirabug:`PXC-820`: Enabled querying of :variable:`pxc_maint_mode`
  by another client during the transition period.

* :jirabug:`PXC-823`: Fixed SST flow to gracefully shut down JOINER node
  if SST fails because DONOR leaves the cluster due to network failure.
  This ensures that the DONOR is then able to recover to synced state
  when network connectivity is restored
  For more information, see :bug:`1684810`.

* :jirabug:`PXC-824`: Fixed graceful shutdown of |PXC| node
  to wait until applier thread finishes.

