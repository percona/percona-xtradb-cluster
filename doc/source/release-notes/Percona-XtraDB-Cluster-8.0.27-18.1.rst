.. _PXC-8.0.27-18.1:

================================================================================
*Percona XtraDB Cluster* 8.0.27-18.1
================================================================================

.. include:: ../_res/date.txt

.. include:: ../_res/pxc-blurb.txt

.. contents::
   :local:

.. include:: ../_res/8.0.27-highlights.txt


Bugs Fixed
=======================

* :jirabug:`PXC-3831`: Allowed certified high-priority transactions to proceed without lock conflicts.
* :jirabug:`PXC-3766`: Stopped every XtraBackup-based SST operation from executing the version-check procedure.
* :jirabug:`PXC-3704`: Based the maximum writeset size on ``repl.max_ws_size`` when both ``repl.max_ws_size`` and ``wsrep_max_ws_size`` values are passed during startup.

.. include:: ../_res/useful-links.txt
