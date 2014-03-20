.. _faq:

============================
 Frequently Asked Questions
============================

Q: How do you solve locking issues like auto increment?
========================================================
A: For auto-increment particular, Cluster changes auto_increment_offset
for each new node.
In the single node workload, locking handled by usual way how |InnoDB| handles locks. 
In case of write load on several nodes, Cluster uses `optimistic locking <http://en.wikipedia.org/wiki/Optimistic_concurrency_control>`_ and application may receive lock error in the response on COMMIT query.

Q: What if one of the nodes crashes and innodb recovery roll back some transactions? 
=====================================================================================
A: When the node crashes, after the restart it will copy whole dataset from another node
(if there were changes to data since crash). 

Q: How can I check the Galera node health?
==========================================
A:  Your check should be simply: 

.. code-block:: mysql

   SELECT 1 FROM dual;

3 different results are possible:
   * You get the row with id=1 (node is healthy)
   * Unknown error (node is online but Galera is not connected/synced with the cluster)
   * Connection error (node is not online)

You can also check the node health with the ``clustercheck`` script. You need to set up ``clustercheck`` user:

.. code-block:: mysql

   GRANT USAGE ON *.* TO 'clustercheck'@'localhost' IDENTIFIED BY PASSWORD '*2470C0C06DEE42FD1618BB99005ADCA2EC9D1E19';

You can then check the node health by running the ``clustercheck`` script: 

.. code-block:: bash

   /usr/bin/clustercheck clustercheck password 0

If the node is running correctly you should get the following status: :: 

  HTTP/1.1 200 OK
  Content-Type: text/plain
  Connection: close
  Content-Length: 40

  Percona XtraDB Cluster Node is synced.

In case node isn't sync or if it is off-line status will look like: :: 

  HTTP/1.1 503 Service Unavailable
  Content-Type: text/plain
  Connection: close
  Content-Length: 44

  Percona XtraDB Cluster Node is not synced. 

.. note::

   clustercheck syntax: 

   <user> <pass> <available_when_donor=0|1> <log_file> <available_when_readonly=0|1> <defaults_extra_file>"
   Recommended: server_args = user pass 1 /var/log/log-file 0 /etc/my.cnf.local"
   Compatibility: server_args = user pass 1 /var/log/log-file 1 /etc/my.cnf.local"

Q: How does XtraDB Cluster handle big transaction?
==================================================
A: XtraDB Cluster populates write set in memory before replication and this sets one limit for how large transactions make sense. There are wsrep variables for max row count and max size of of write set to make sure that server is not running out of memory.

Q: Is there a chance to have different table structure on the nodes? 
=====================================================================
What I mean is like having 4 nodes, 4 tables like sessions_a, sessions_b, sessions_c and sessions_d and have each only on one of the nodes? 

A: Not at the moment for InnoDB tables. But it will work for MEMORY tables.

Q: What if a node fail and/or what if there is a network issue between them? 
=============================================================================
A: Then Quorum mechanism in XtraDB Cluster will decide what nodes can accept traffic
and will shutdown nodes that not belong to quorum. Later when the failure is fixed,
the nodes will need to copy data from working cluster.

Q: How would it handle split brain? 
====================================
A: It would not handle it. The |split brain| is hard stop, XtraDB Cluster can't resolve it.
That's why the minimal recommendation is to have 3 nodes. 
However there is possibility to allow a node to handle the traffic, option is: ::
  
  wsrep_provider_options="pc.ignore_sb = yes"

Q: Is it possible to set up cluster without state transfer
==========================================================
A: It is possible in two ways:

1. By default Galera reads starting position from a text file <datadir>/grastate.dat. Just make this file identical on all nodes, and there will be no state transfer upon start.

2. With :variable:`wsrep_start_position` variable - start the nodes with the same *UUID:seqno* value and there you are.

Q: I have a two nodes setup. When node1 fails, node2 does not accept commands, why?
====================================================================================
A: This is expected behavior, to prevent |split brain|. See previous question.

Q: What tcp ports are used by Percona XtraDB Cluster?
======================================================
A: You may need to open up to 4 ports if you are using firewall.

1. Regular MySQL port, default 3306.

2. Port for group communication, default 4567. It can be changed by the option: ::

     wsrep_provider_options ="gmcast.listen_addr=tcp://0.0.0.0:4010; "

3. Port for State Transfer, default 4444. It can be changed by the option: ::

     wsrep_sst_receive_address=10.11.12.205:5555

4. Port for Incremental State Transfer, default port for group communication + 1 (4568). It can be changed by the option: ::

     wsrep_provider_options = "ist.recv_addr=10.11.12.206:7777; "

Q: Is there "async" mode for Cluster or only "sync" commits are supported? 
===========================================================================
A: There is no "async" mode, all commits are synchronous on all nodes.
Or, to be fully correct, the commits are "virtually" synchronous. Which
means that transaction should pass "certification" on nodes, not physical commit.
"Certification" means a guarantee that transaction does not have conflicts with 
another transactions on corresponding node.

Q: Does it work with regular MySQL replication?
================================================
A: Yes. On the node  you are going to use as master, you should enable log-bin and log-slave-update options.

Q: Init script (/etc/init.d/mysql) does not start
=================================================
A: Try to disable SELinux. Quick way is: ::
  
  echo 0 > /selinux/enforce

Q: I'm getting "nc: invalid option -- 'd'" in the sst.err log file
==================================================================
A: This is Debian/Ubuntu specific error, Percona-XtraDB-Cluster uses netcat-openbsd package. This dependency has been fixed in recent releases. Future releases of PXC will be compatible with any netcat (bug :bug:`959970`).


