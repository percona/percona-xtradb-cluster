.. _secure-network:

====================
Securing the Network
====================

By default, anyone with access to your network can connect to any Percona XtraDB Cluster node
either as a client or as another node joining the cluster.
This could potentially let them query your data or get a complete copy of it.

In general, it is a good idea to disable all remote connections to Percona XtraDB Cluster nodes.
If you require clients or nodes from outside of your network to connect,
you can set up a VPN (virtual private network) for this purpose.

Firewall Configuration
======================

A firewall can let you filter Percona XtraDB Cluster traffic
based on the clients and nodes that you trust.

By default, Percona XtraDB Cluster nodes use the following ports:

* 3306 is used for MySQL client connections
  and :term:`SST` (State Snapshot Transfer) via ``mysqldump``.

* 4444 is used for :term:`SST` via 
  :ref:`Percona XtraBackup <xtrabackup_sst>`.

* 4567 is used for write-set replication traffic (over TCP)
  and multicast replication (over TCP and UDP).

* 4568 is used for :term:`IST` (Incremental State Transfer).

Ideally you want to make sure that these ports on each node
are accessed only from trusted IP addresses.
You can implement packet filtering using ``iptables``, ``firewalld``, ``pf``,
or any other firewall of your choice.

Using iptables
--------------

To restrict access to Percona XtraDB Cluster ports using ``iptables``,
you need to append new rules to the ``INPUT`` chain on the filter table.
In the following example, the trusted range of IP addresses is 192.168.0.1/24.
It is assumed that only Percona XtraDB Cluster nodes and clients will connect from these IPs.
To enable packet filtering, run the commands as root on each Percona XtraDB Cluster node.

.. code-block:: bash

   # iptables --append INPUT --in-interface eth0 \
      --protocol tcp --match tcp --dport 3306 \
      --source 192.168.0.1/24 --jump ACCEPT
   # iptables --append INPUT --in-interface eth0 \
      --protocol tcp --match tcp --dport 4444 \
      --source 192.168.0.1/24 --jump ACCEPT
   # iptables --append INPUT --in-interface eth0 \
      --protocol tcp --match tcp --dport 4567 \
      --source 192.168.0.1/24 --jump ACCEPT
   # iptables --append INPUT --in-interface eth0 \
      --protocol tcp --match tcp --dport 4568 \
      --source 192.168.0.1/24 --jump ACCEPT
   # iptables --append INPUT --in-interface eth0 \
      --protocol udp --match udp --dport 4567 \
      --source 192.168.0.1/24 --jump ACCEPT

.. note:: The last one opens port 4567 for multicast replication over UDP.

If the trusted IPs are not in sequence,
you will need to run these commands for each address on each node.
In this case, you can consider to open all ports between trusted hosts.
This is a little bit less secure, but reduces the amount of commands.
For example, if you have three Percona XtraDB Cluster nodes,
you can run the following commands on each one:

.. code-block:: bash

   # iptables --append INPUT --protocol tcp \
       --source 64.57.102.34 --jump ACCEPT
   # iptables --append INPUT --protocol tcp \
       --source 193.166.3.20  --jump ACCEPT
   # iptables --append INPUT --protocol tcp \
       --source 193.125.4.10  --jump ACCEPT

Running the previous commands will allow TCP connections
from the IP addresses of the other Percona XtraDB Cluster nodes.

.. note:: The changes that you make in ``iptables`` are not persistent
   unless you save the packet filtering state::

    # service save iptables

   For distributions that use ``systemd``,
   you need to save the current packet filtering rules
   to the path where ``iptables`` reads from when it starts.
   This path can vary by distribution,
   but it is usually in the ``/etc`` directory.
   For example:

   * ``/etc/sysconfig/iptables``
   * ``/etc/iptables/iptables.rules``

   Use ``iptables-save`` to update the file::

    # iptables-save > /etc/sysconfig/iptables

