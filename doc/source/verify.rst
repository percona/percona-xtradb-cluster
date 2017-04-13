.. _verify:

=====================
Verifying Replication
=====================

Use the following procedure to verify replication
by creating a new database on the second node,
creating a table for that database on the third node,
and adding some records to the table on the first node.

1. Create a new database on the second node:

   .. code-block:: sql

      mysql@pxc2> CREATE DATABASE percona;
      Query OK, 1 row affected (0.01 sec)

#. Create a table on the third node:

   .. code-block:: sql

      mysql@pxc3> USE percona;
      Database changed

      mysql@pxc3> CREATE TABLE example (node_id INT PRIMARY KEY, node_name VARCHAR(30));
      Query OK, 0 rows affected (0.05 sec)

#. Insert records on the first node:

   .. code-block:: sql

      mysql@pxc1> INSERT INTO percona.example VALUES (1, 'percona1');
      Query OK, 1 row affected (0.02 sec)

#. Retrieve rows from that table on the second node:

   .. code-block:: sql

      mysql@pxc2> SELECT * FROM percona.example;
      +---------+-----------+
      | node_id | node_name |
      +---------+-----------+
      |       1 | percona1  |
      +---------+-----------+
      1 row in set (0.00 sec)

Next Steps
==========

* Consider installing ProxySQL_ on client nodes
  for efficient workload management
  across the cluster without any changes
  to the applications that generate queries.
  This is the recommended high-availability solution for |PXC|.

  For more information, see :ref:`load_balancing_with_proxysql`.

* |PMM|_ is the best choice for managing and monitoring |PXC| performance.
  It provides visibility for the cluster
  and enables efficient troubleshooting.

