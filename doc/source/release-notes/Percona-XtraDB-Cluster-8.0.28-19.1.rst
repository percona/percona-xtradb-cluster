.. _PXC-8.0.28-19.1:

================================================================================
*Percona XtraDB Cluster* 8.0.28-19.1 (2022-07-19)
================================================================================

.. include:: ../_res/pxc-blurb.txt

.. contents::
   :local:

.. include:: ../_res/8.0.28-highlights.txt

Bugs Fixed
=======================

* :jirabug:`PXC-3923`: When the ``read_only`` or ``super_read_only`` option was set, the ``ANALYZE TABLE`` command removed the node from the cluster.
* :jirabug:`PXC-3388`: Percona XtraDB Cluster stuck in a DESYNCED state after joiner was killed.
* :jirabug:`PXC-3609`: The binary log status variables were updated when the binary log was disabled. Now the status variables are not registered when the binary log is disabled. (Thanks to Stofa Kenida for reporting this issue.)
* :jirabug:`PXC-3848`: The cluster node exited when the ``CURRENT_USER()`` function was used. (Thanks to Steffen Böhme for reporting this issue.)
* :jirabug:`PXC-3872`: A user without `system_user` privilege was able to drop system users. (Thanks to user jackc for reporting this issue.)
* :jirabug:`PXC-3918`: Galera Arbitrator (garbd) could not connect if the Percona XtraDB Cluster server used encrypted connections. The issue persisted even when the proper certificates were specified.
* :jirabug:`PXC-3924`: Using ``TRUNCATE TABLE X`` and ``INSERT INTO X`` options when the foreign keys were disabled and violated caused the ``HA_ERR_FOUND_DUPP_KEY`` error on a slave node. (Thanks to Daniel Bartoníček for reporting this issue.) 
* :jirabug:`PXC-3062`: The ``wsrep_incoming_addresses`` status variable did not contain the garbd IP address.


.. include:: ../_res/useful-links.txt
