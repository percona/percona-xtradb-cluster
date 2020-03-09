================================================================================
|Percona XtraDB Cluster| |release| Release Notes
================================================================================

:Date: |date|
:Installation: :ref:`install`

Binaries are available from the `downloads
<https://www.percona.com/downloads/Percona-XtraDB-Cluster-80/>`_ section or from
our :ref:`software repositories <install>`.

This is a release candadate. We do not recommend that you use it in your
production environment.

Known Issues
================================================================================

- :jirabug:`PXC-3039`: No useful error messages if an SSL-disabled node tries to join SSL-enabled cluster
- :jirabug:`PXC-3028`: Memory leaks inside wsrep_st
- :jirabug:`PXC-3027`: Global wsrep memory leaks
- :jirabug:`PXC-3020`: Cluster goes down if a node is restarted during load
- :jirabug:`PXC-3016`: 8.0 set persist binlog_format='STATEMENT' should not succeed
- :jirabug:`PXC-2999`: DML commands are not replicating to PXC-8.0 nodes through MySQL shell
- :jirabug:`PXC-2994`: 8.0 clone plugin doesn't work for cloning remote data
- :jirabug:`PXC-3047`: hung if cannot connect to members PXC 8.0

.. |date| replace:: March 10, 2020
.. |release| replace:: 8.0.18-4.1
