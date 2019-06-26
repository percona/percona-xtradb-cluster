.. rn:: 5.7.26-31.37

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of Percona XtraDB Cluster |release| on
|date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-57/>`_ or from our
:ref:`software repositories <install>`.

|PXC| |release| is now the current release, based on the following:

.. The versions of Galera are probably not correct

* `Percona Server for MySQL 5.7.26-29 <https://www.percona.com/doc/percona-server/5.7/release-notes/Percona-Server-5.7.26-29.html>`_
* Galera Replication library 3.26
* Galera/Codership WSREP API Release 5.7.25


Bugs Fixed
================================================================================

- :pxcbug:`2480`: In some cases, |PXC| could not replicate ``CURRENT_USER()``
  used in the ``ALTER`` statement. ``USER()`` and ``CURRENT_USER()`` are no
  longer allowed in any ``ALTER`` statement since they fail when replicated.
- :pxcbug:`2487`: The case when a DDL or DML action was in progress from one
  client and the provider was updated from another client could result in a race
  condition.
- :pxcbug:`2490`: Percona XtraDB Cluster could crash when
  :variable:`binlog_space_limit` was set to a value other than zero during
  ``wsrep_recover`` mode.
- :pxcbug:`2491`: SST could fail if the donor had encrypted undo logs.
- :pxcbug:`2537`: Nodes could crash after an attempt to set a password using
  ``mysqladmin``
- :pxcbug:`2497`: The user can set the preferred donor by setting the
  :variable:`wsrep_sst_donor` variable. An IP address is not valid as the value of
  this variable. If the user still used an IP address, an error message was
  produced that did not provide sufficient information. The error message has been
  improved to suggest that the user check the value of the
  :variable:`wsrep_sst_donor` for an IP address.

.. 2560 is not public yet

**Other bugs fixed**:
:pxcbug:`2276`,
:pxcbug:`2292`,
:pxcbug:`2476`,
:pxcbug:`2560`
   
.. |release| replace:: 5.7.26-31.37
.. |date| replace:: June 26, 2019
