.. _PXC-5.7.37-31.57:

================================================================================
*Percona XtraDB Cluster* 5.7.37-31.57 (2022-05-18)
================================================================================

.. include:: ../_res/pxc-blurb.txt

.. include:: ../_res/5.7.37-31.57-rh.txt

Bugs Fixed
====================================================

* :jirabug:`PXC-3388`: When the joiner failed and the donor did not abort the SST. The cluster remained in donor/desynced state.
* :jirabug:`PXC-3609`: The binary log status was updated when the binary log was disabled.
* :jirabug:`PXC-3796`: The Garbd IP was not visible in the ``wsrep_incoming_addresses`` status variable.
* :jirabug:`PXC-3848`: Issuing an ``ALTER USER CURRENT_USER()`` command crashed the connected cluster node.
* :jirabug:`PXC-3914`: Upgraded the version of socat used in the Docker image.

.. include:: ../_res/5.7-useful-links.txt