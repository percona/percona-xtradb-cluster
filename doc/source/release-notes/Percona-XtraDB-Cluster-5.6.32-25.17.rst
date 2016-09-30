.. rn:: 5.6.32-25.17

===================================
Percona XtraDB Cluster 5.6.32-25.17
===================================

Percona is glad to announce the release of
|Percona XtraDB Cluster| 5.6.32-25.17 on June 10, 2016.
Binaries are available from the
`downloads section <http://www.percona.com/downloads/Percona-XtraDB-Cluster-56/release-5.6.30-25.16/>`_
or from our :ref:`software repositories <installation>`.

Percona XtraDB Cluster 5.6.32-25.17 is now the current release,
based on the following:

* `Percona Server 5.6.32-78.1 <http://www.percona.com/doc/percona-server/5.6/release-notes/Percona-Server-5.6.32-78.1.html>`_

* Galera Replication library 3.17

* wsrep API version 25

All Percona software is open-source and free.
Details of this release can be found in the
`5.6.32-25.17 milestone on Launchpad
<https://launchpad.net/percona-xtradb-cluster/+milestone/5.6.30-25.16>`_.

For more information about relevant Codership releases, see `this announcement
<http://galeracluster.com/2016/08/announcing-galera-cluster-5-5-50-and-5-6-31-with-galera-3-17/>`_.

Fixed Bugs
==========

* Fixed ``DONOR`` node getting stuck in ``Joined`` state after successful SST.
  Bugs fixed :bug:`1608680` and :bug:`1611728`.

* Removed protection against repeated calls of ``wsrep->pause()`` on the same
  node to allow parallel RSU operation.

* Fixed error when running ``SHOW STATUS`` during group state update.

* Setting the :variable:`wsrep_auto_increment_control` to ``OFF`` didn't
  restore the original user set value.

* Corrected the return code of ``sst_flush_tables()`` function to return a
  non-negative error code and thus pass assertion.

* Using |Percona XtraBackup| as the SST method now requires |Percona
  XtraBackup| 2.3.5 or later.

* Fixed memory leak and stale pointer due to stats not freeing when toggling
  the :variable:`wsrep_provider` variable.

* Fixed failure of ``ROLLBACK`` to register ``wsrep_handler``.

* Improved rollback process to ensure that when a transaction is rolled back,
  any statements open by the transaction are also rolled back.

* Fixed failure of symmetric encryption during SST.

* Performing an SST would display the encryption key in the logs. It's
  recommended to use use :option:`encrypt-key-file` instead of
  :option:`encrypt-key` option.

* Node transitioning to non-primary state with active ``ALTER TABLE`` could
  result in a crash.

* Fixed setting of ``seqno`` in :file:`grastate.dat` to ``-1`` on clean
  shutdown.

* Fixed failure of asynchronous TOI actions (like ``DROP``) for non-primary
  nodes.

* Removed the unused :variable:`sst_special_dirs` variable.

* Added support of |defaults-group-suffix|_ for SST scripts.

* Other low-level fixes and improvements for better stability.

.. |defaults-group-suffix| replace:: ``defaults-group-suffix``
.. _defaults-group-suffix: http://dev.mysql.com/doc/refman/5.7/en/option-file-options.html#option_general_defaults-group-suffix
