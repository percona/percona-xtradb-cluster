.. _nbo:
 
================================================================================
Non-Blocking Operations (NBO) method for Online Scheme Upgrades (OSU)
================================================================================

.. note::

  The Non-Blocking Operation feature is considered **Tech Preview**. 

An :ref:`online-schema-upgrade` can be a daily issue in an environment with accelerated development and deployment. The task becomes more difficult as the data grows. An ``ALTER TABLE`` statement is a multi-step operation and must run until it is complete. Aborting the statement may be more expensive than letting it complete. 

The Non-Blocking Operations (NBO) method is similar to the ``TOI`` method (see :ref:`online-schema-upgrade` for more information on the available types of online schema upgrades). Every replica processes the DDL statement at the same point in the cluster transaction stream, and other transactions cannot commit during the operation.

In the NBO method, the supported DDL statement acquires a metadata lock on the table or schema at a late stage of the operation. This method provides a more efficient locking strategy and avoids the ``TOI`` issue of long-running DDL statements blocking cluster updates.

Attempting a State Snapshot Transfer (SST) fails during the NBO operation.

To dynamically set the ``NBO`` mode in the client, run the following statement:

.. sourcecode:: mysql

    SET SESSION wsrep_OSU_method='NBO';

.. _supported-ddl:

Supported DDL Statements
-------------------------

The `NBO` method supports the following DDL statements:

* ``ALTER TABLE``

* ``CREATE INDEX``

* ``DROP INDEX``

Limitations
-----------------

The ``NBO`` method does not support running two DDL statements with conflicting locks on the same table. For example, you cannot run two ``ALTER TABLE`` statements for an ``employees`` table.

See the :ref:`PXC-8.0.25-15.1` Release notes for the latest information.
