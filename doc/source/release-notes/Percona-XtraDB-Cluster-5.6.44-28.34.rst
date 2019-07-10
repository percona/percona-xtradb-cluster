.. rn:: 5.6.44-28.34

================================================================================
|PXC| |release|
================================================================================

Percona is glad to announce the release of |PXC| |release|
on |date|.  Binaries are available from the `downloads section
<http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/>`_ or from our
:ref:`software repositories <installation>`.

|PXC| |release| is now the current release, based on the following:

- `Percona Server 5.6.44
  <https://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.44-86.0.html>`_
- Codership WSREP API release 5.6.43
- Codership Galera library 3.26

All Percona software is open-source and free.

Bugs Fixed
================================================================================

- :jirabug:`2480`: In some cases, |PXC| could not replicate
  ``CURRENT_USER()`` used in the ``ALTER`` statement. ``USER()`` and
  ``CURRENT_USER()`` are no longer allowed in any ``ALTER`` statement
  since they fail when replicated.
- :jirabug:`2487`: The case when a DDL or DML action was in progress
  from one client and the provider was updated from another client could
  result in a race condition.
- :jirabug:`2490`: |PXC| could crash when ``binlog_space_limit`` was
  set to a value other than zero during ``wsrep_recover`` mode.
- :jirabug:`2497`: The user can set the preferred donor by setting the
  :variable:`wsrep_sst_donor` variable. An IP address is not valid as
  the value of this variable. If the user still used an IP address, an
  error message was produced that did not provide sufficient
  information. The error message has been improved to suggest that the
  user check the value of the :variable:`wsrep_sst_donor` for an IP
  address.

.. |release| replace:: 5.6.44-28.34
.. |date| replace:: June 19, 2019
