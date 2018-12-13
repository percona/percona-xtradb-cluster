.. _gcache_record-set_cache_difference:

=========================================
Understanding GCache and Record-Set Cache
=========================================

In |PXC|, there is a concept of GCache and Record-Set cache
(which can also be called transaction write-set cache).
The use of these two caches is often confusing
if you are running long transactions,
because both of them result in the creation of disk-level files.
This manual describes what their main differences are.

Record-Set Cache
================

When you run a long-running transaction on any particular node,
it will try to append a key for each row that it tries to modify
(the key is a unique identifier for the row ``{db,table,pk.columns}``).
This information is cached in out-write-set,
which is then sent to the group for certification.

Keys are cached in HeapStore
(which has ``page-size=64K`` and ``total-size=4MB``).
If the transaction data-size outgrows this limit,
then the storage is switched from Heap to Page
(which has ``page-size=64MB`` and ``total-limit=free-space-on-disk``).

All these limits are non-configurable,
but having a memory-page size greater than 4MB per transaction
can cause things to stall due to memory pressure,
so this limit is reasonable. This is another
limitation to address when Galera supports large transaction.

The same long-running transaction will also generate binlog data
that also appends to out-write-set on commit (``HeapStore->FileStore``).
This data can be significant,
as it is a binlog image of rows inserted/updated/deleted by the transaction.
The :variable:`wsrep_max_ws_size` variable controls the size
of this part of the write-set.
The threshold doesn't consider size allocated for caching-keys and the header.

If ``FileStore`` is used, it creates a file on the disk
(with names like ``xxxx_keys`` and ``xxxx_data``) to store the cache data.
These files are kept until a transaction is committed,
so the lifetime of the transaction is linked.

When the node is done with the transaction and is about to commit,
it will generate the final-write-set using the two files
(if the data size grew enough to use ``FileStore``)
plus ``HEADER``, and will publish it for certification to cluster.

The native node executing the transaction will also act as subscription node,
and will receive its own write-set through the cluster publish mechanism.
This time, the native node will try to cache write-set into its GCache.
How much data GCache retains is controlled by the GCache configuration.

GCache
======

GCache holds the write-set published on the cluster for replication.
The lifetime of write-set in GCache is not transaction-linked.

When a ``JOINER`` node needs an IST,
it will be serviced through this GCache (if possible).

GCache will also create the files to disk.
You can read more about it
`here <http://severalnines.com/blog/understanding-gcache-galera>`_.

At any given point in time, the native node has two copies of the write-set:
one in GCache and another in Record-Set Cache.

For example, lets say you ``INSERT/UPDATE`` 2 million rows
in a table with the following schema.

.. code-block:: mysql

  (int, char(100), char(100) with pk (int, char(100))

It will create write-set key/data files in the background
similar to the following:

.. code-block:: bash

  -rw------- 1 xxx xxx 67108864 Apr 11 12:26 0x00000707_data.000000
  -rw------- 1 xxx xxx 67108864 Apr 11 12:26 0x00000707_data.000001
  -rw------- 1 xxx xxx 67108864 Apr 11 12:26 0x00000707_data.000002
  -rw------- 1 xxx xxx 67108864 Apr 11 12:26 0x00000707_keys.000000

