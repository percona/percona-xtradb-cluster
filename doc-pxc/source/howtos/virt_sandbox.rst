=====================================================
 Setting up PXC reference architecture with HAProxy
=====================================================

This tutorial is a step-by-step guide to set up *Percona XtraDB Cluster*, in a virtualized test sandbox. This example uses Amazon EC2 micro instances, but the content here is applicable for any kind of virtualization technology (for example VirtualBox).
You will need 4 virtual machines. 3 for |PXC| and 1 for the client, which will have |HAProxy|. In this how-to CentOS 6 is used as the operating system, the instructions are similar for any Linux distribution.

The client node will have HAProxy installed and it will redirect requests to |PXC| nodes. This approach works well in real-world scenarios too. Running HAProxy on the application servers instead of having them as dedicated entities gives you benefits like no need for an extra network roundtrip, because loadbalancer and scalability of |PXC|'s load balancing layer scales simply with application servers.

We'll use `Percona <http://www.percona.com/doc/percona-xtradb-cluster/5.6/installation/yum_repo.html>`_ and `EPEL <http://fedoraproject.org/wiki/EPEL>`_ repositories for software installation.

After configuring the repositories you'll be able to install software that will be used. First, install |PXC| on the database nodes. ::

  # yum -y install Percona-XtraDB-Cluster-server Percona-XtraDB-Cluster-client percona-xtrabackup

Install |HAProxy| and |sysbench| on the client node. ::

  # yum -y install haproxy sysbench

After installing everything, we'll configure |PXC| first. On the first node, :file:`my.cnf` should look something like this on a relatively weak machine. ::

  [mysqld]
  server_id=1
  binlog_format=ROW
  log_bin=mysql-bin
  wsrep_cluster_address=gcomm://
  wsrep_provider=/usr/lib/libgalera_smm.so
  datadir=/var/lib/mysql

  wsrep_slave_threads=2
  wsrep_cluster_name=pxctest
  wsrep_sst_method=xtrabackup
  wsrep_node_name=ip-10-112-39-98

  log_slave_updates

  innodb_locks_unsafe_for_binlog=1
  innodb_autoinc_lock_mode=2
  innodb_buffer_pool_size=400M
  innodb_log_file_size=64M

You can start your first node now. Make sure that you only start second and third nodes when the first node is up and running (it will serve as a donor for |SST|).

This configuration is for the first node. For the second and third node, you need to change :option:`wsrep_cluster_address` (alternatively, you can use :option:`wsrep_urls` in [mysqld_safe] section), which should point to a node in the cluster which is already up, so it will join the cluster. The :option:`server_id` and :option:`wsrep_node_name` variables have to be different on each host, for :option:`wsrep_node_name`, you can use the output of `hostname` command.

Based on that, for the second node, the differences in the configuration should be the following. :: 

  server_id=2
  wsrep_cluster_address=gcomm://10.116.39.76 # replace this with the IP of your first node
  wsrep_node_name=ip-10-244-33-92

For the third node, the differences look like this. ::

  server_id=3
  wsrep_cluster_address=gcomm://10.116.39.76 # replace this with the IP of your first node
  wsrep_node_name=ip-10-194-10-179

For |SST| we use |xtrabackup|. This means at startup time, the new node will connect to an existing node in the cluster and it takes a backup of that node with xtrabackup and copies it to the new node with `netcat`. After a successful |SST|, you should see this in the error log. ::

  120619 13:20:17 [Note] WSREP: State transfer required:
        Group state: 77c9da88-b965-11e1-0800-ea53b7b12451:97
        Local state: 00000000-0000-0000-0000-000000000000:-1
  120619 13:20:17 [Note] WSREP: New cluster view: global state: 77c9da88-b965-11e1-0800-ea53b7b12451:97, view# 18: Primary, number of nodes: 3, my index: 0, protocol version 2
  120619 13:20:17 [Warning] WSREP: Gap in state sequence. Need state transfer.
  120619 13:20:19 [Note] WSREP: Running: 'wsrep_sst_xtrabackup 'joiner' '10.195.206.117' '' '/var/lib/mysql/' '/etc/my.cnf' '20758' 2>sst.err'
  120619 13:20:19 [Note] WSREP: Prepared |SST| request: xtrabackup|10.195.206.117:4444/xtrabackup_sst
  120619 13:20:19 [Note] WSREP: wsrep_notify_cmd is not defined, skipping notification.
  120619 13:20:19 [Note] WSREP: Assign initial position for certification: 97, protocol version: 2
  120619 13:20:19 [Warning] WSREP: Failed to prepare for incremental state transfer: Local state UUID (00000000-0000-0000-0000-000000000000) does not match group state UUID (77c9da88-b965-11e1-0800-ea53b7b12451): 1 (Operation not permitted)
         at galera/src/replicator_str.cpp:prepare_for_IST():439. IST will be unavailable.
  120619 13:20:19 [Note] WSREP: Node 0 (ip-10-244-33-92) requested state transfer from '*any*'. Selected 1 (ip-10-112-39-98)(SYNCED) as donor.
  120619 13:20:19 [Note] WSREP: Shifting PRIMARY -> JOINER (TO: 102)
  120619 13:20:19 [Note] WSREP: Requesting state transfer: success, donor: 1
  120619 13:20:59 [Note] WSREP: 1 (ip-10-112-39-98): State transfer to 0 (ip-10-244-33-92) complete.
  120619 13:20:59 [Note] WSREP: Member 1 (ip-10-112-39-98) synced with group.
  120619 13:21:17 [Note] WSREP: |SST| complete, seqno: 105
  120619 13:21:17 [Note] Plugin 'FEDERATED' is disabled.
  120619 13:21:17 InnoDB: The InnoDB memory heap is disabled
  120619 13:21:17 InnoDB: Mutexes and rw_locks use GCC atomic builtins
  120619 13:21:17 InnoDB: Compressed tables use zlib 1.2.3
  120619 13:21:17 InnoDB: Using Linux native AIO
  120619 13:21:17 InnoDB: Initializing buffer pool, size = 400.0M
  120619 13:21:17 InnoDB: Completed initialization of buffer pool
  120619 13:21:18 InnoDB: highest supported file format is Barracuda.
  120619 13:21:18  InnoDB: Waiting for the background threads to start
  120619 13:21:19 Percona XtraDB (http://www.percona.com) 1.1.8-rel25.3 started; log sequence number 246661644
  120619 13:21:19 [Note] Recovering after a crash using mysql-bin
  120619 13:21:19 [Note] Starting crash recovery...
  120619 13:21:19 [Note] Crash recovery finished.
  120619 13:21:19 [Note] Server hostname (bind-address): '(null)'; port: 3306
  120619 13:21:19 [Note]   - '(null)' resolves to '0.0.0.0';
  120619 13:21:19 [Note]   - '(null)' resolves to '::';
  120619 13:21:19 [Note] Server socket created on IP: '0.0.0.0'.
  120619 13:21:19 [Note] Event Scheduler: Loaded 0 events
  120619 13:21:19 [Note] WSREP: Signalling provider to continue.
  120619 13:21:19 [Note] WSREP: Received |SST|: 77c9da88-b965-11e1-0800-ea53b7b12451:105
  120619 13:21:19 [Note] WSREP: |SST| received: 77c9da88-b965-11e1-0800-ea53b7b12451:105
  120619 13:21:19 [Note] WSREP: 0 (ip-10-244-33-92): State transfer from 1 (ip-10-112-39-98) complete.
  120619 13:21:19 [Note] WSREP: Shifting JOINER -> JOINED (TO: 105)
  120619 13:21:19 [Note] /usr/sbin/mysqld: ready for connections.
  Version: '5.5.24-log'  socket: '/var/lib/mysql/mysql.sock'  port: 3306  Percona XtraDB Cluster (GPL), wsrep_23.6.r340
  120619 13:21:19 [Note] WSREP: Member 0 (ip-10-244-33-92) synced with group.
  120619 13:21:19 [Note] WSREP: Shifting JOINED -> SYNCED (TO: 105)
  120619 13:21:20 [Note] WSREP: Synchronized with group, ready for connections

For debugging information about the |SST|, you can check the sst.err file and the error log too.

After the SST's is done, you should check if you have a 3 node cluster.

.. code-block:: mysql

 mysql> show global status like 'wsrep_cluster_size';
 +--------------------+-------+
 | Variable_name      | Value |
 +--------------------+-------+
 | wsrep_cluster_size | 3     |
 +--------------------+-------+
 1 row in set (0.00 sec)

When all nodes are started, you can set up HAProxy on the client. The point of this is that the application will be able to connect to localhost as |MySQL| server, so although we are using |PXC|, the application will see this as a single MySQL server running on localhost.

In order to achieve this, you'll need to configure HAProxy on the client node. There are 2 possible configurations here.
First is configuring round robin, which means you will connect and write to all cluster nodes. This can be done, but because of optimistic locking at commit time, rollbacks can happen if you have conflicting writes. In the second configuration, you will configure HAProxy in a way that it writes only to one node, so the application doesn't have to be prepared about unexpected rollbacks. The first configuration is a good choice in most cases, not handling rollbacks is not healthy in a well behaving application anyway.

HAProxy can be configured in the /etc/haproxy/haproxy.cfg and it should look like this. ::

  global
  log 127.0.0.1 local0
  log 127.0.0.1 local1 notice
  maxconn 4096
  chroot /usr/share/haproxy
  user haproxy
  group haproxy
  daemon

  defaults
  log global
  mode http
  option tcplog
  option dontlognull
  retries 3
  option redispatch
  maxconn 2000
  contimeout 5000
  clitimeout 50000
  srvtimeout 50000

  frontend pxc-front
  bind *:3307
  mode tcp
  default_backend pxc-back

  frontend stats-front
  bind *:80
  mode http
  default_backend stats-back

  frontend pxc-onenode-front
  bind *:3306
  mode tcp
  default_backend pxc-onenode-back

  backend pxc-back
  mode tcp
  balance leastconn
  option httpchk
  server c1 10.116.39.76:3306 check port 9200 inter 12000 rise 3 fall 3
  server c2 10.195.206.117:3306 check port 9200 inter 12000 rise 3 fall 3
  server c3 10.202.23.92:3306 check port 9200 inter 12000 rise 3 fall 3

  backend stats-back
  mode http
  balance roundrobin
  stats uri /haproxy/stats
  stats auth pxcstats:secret

  backend pxc-onenode-back
  mode tcp
  balance leastconn
  option httpchk
  server c1 10.116.39.76:3306 check port 9200 inter 12000 rise 3 fall 3
  server c2 10.195.206.117:3306 check port 9200 inter 12000 rise 3 fall 3 backup
  server c3 10.202.23.92:3306 check port 9200 inter 12000 rise 3 fall 3 backup

In this configuration, three frontend-backend pairs are defined. The stats pair is for |HAProxy| statistics page, and the others are for |PXC|. |MySQL| will be listening on ports 3306 and 3307. If you connect to port 3306, you'll connect to `pxc-onenode`, and you'll be only using one node at a time (to avoid rollbacks because of optimistic locking). If that node goes off-line, you'll start using an other one.
However if you connect to port 3307, you'll be using all three nodes for reads and writes too. In this case  the `leastconn` load balancing method is used instead of round robin, which means you always connect to the backend with the least connections established.
The statistics page is accessible on the client node with a browser pointed to `/haproxy/stats`, the stats auth parameter in the configuration has the credentials for that in plain text. You can also use this for monitoring purposes (the CSV version is good for trending and alerting).

Here |MySQL| is checked via HTTP checks. |MySQL| won't serve these requests. As part of |PXC| packages, we distribute the clustercheck utility which has to be set up. After that, HAProxy will be able to use check |MySQL| via HTTP. The clustercheck script is a simple shell script, which accepts HTTP requests, and checks MySQL on incoming request. If the |PXC| node is ok, it will emit a response with HTTP code 200 OK, otherwise, it emits 503. The script examines :option:`wsrep_local_state` variable.

To set it up, create the clustercheck user. 

.. code-block:: mysql

  mysql> grant process on *.* to 'clustercheckuser'@'localhost' identified by 'clustercheckpassword!';
  Query OK, 0 rows affected (0.00 sec)

  mysql> flush privileges;
  Query OK, 0 rows affected (0.00 sec)

If you want to use a different username or password, you have to modify them in the script too.
Let's test. :: 

  # clustercheck
  HTTP/1.1 200 OK

  Content-Type: Content-Type: text/plain

Node is running.

You can use `xinetd` to daemonize the script. If `xinetd` is not installed yet, you can install it with yum. :: 

  # yum -y install xinetd

The service itself should be configured in :file:`/etc/xinetd.d/mysqlchk`. :: 

  # default: on
  # description: mysqlchk
  service mysqlchk
  {
  # this is a config for xinetd, place it in /etc/xinetd.d/
    disable = no
    flags = REUSE
    socket_type = stream
    port = 9200
    wait = no
    user = nobody
    server = /usr/bin/clustercheck
    log_on_failure += USERID
    only_from = 0.0.0.0/0
    # recommended to put the IPs that need
    # to connect exclusively (security purposes)
    per_source = UNLIMITED
  }

Also, you should add the new service to :file:`/etc/services`. ::

  mysqlchk 9200/tcp # mysqlchk

Clustercheck will now listen on port 9200 after xinetd restart, and |HAProxy| is ready to check |MySQL| via HTTP. ::

  # service xinetd restart

If you did everything right so far, the statistics page of |HAProxy| should look like this.

.. image:: ../_static/pxc_haproxy_status_example.png


Testing the cluster with sysbench
=================================

You can test the cluster using the `sysbench <https://launchpad.net/sysbench/>`_ (this example uses one from the EPEL repository). First, you need to create a database and a user for it.

.. code-block:: mysql

  mysql> create database sbtest;
  Query OK, 1 row affected (0.01 sec)

  mysql> grant all on sbtest.* to 'sbtest'@'%' identified by 'sbpass';
  Query OK, 0 rows affected (0.00 sec)

  mysql> flush privileges;
  Query OK, 0 rows affected (0.00 sec)

Populate the table with data for the benchmark. ::

  # sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3307 --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 prepare

You can now run the benchmark against the 3307 port. ::

  # sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3307 --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 --num-threads=8 run

.. image:: ../_static/pxc_haproxy_lb_leastconn.png

This is the status of `pxc-back backend` while the |sysbench| above is running. If you look at Cur column under Session, you can see, that c1 has 2 threads connected, c2 and c3 has 3.

If you run the same benchmark, but against the 3306 backend, |HAProxy| stats will show us that the all the threads are going to hit the c1 server. ::

  # sysbench --test=oltp --db-driver=mysql --mysql-engine-trx=yes --mysql-table-engine=innodb --mysql-host=127.0.0.1 --mysql-port=3306 --mysql-user=sbtest --mysql-password=sbpass --oltp-table-size=10000 --num-threads=8 run

.. image:: ../_static/pxc_haproxy_lb_active_backup.png

This is the status of `pxc-onenode-back` while |sysbench| above is running. Here only c1 has 8 connected threads, c2 and c3 are acting as backup nodes.

If you are using |HAProxy| for |MySQL| you can break the privilege system’s host part, because |MySQL| will think that the connections are always coming from the load balancer. You can work this around using T-Proxy patches and some `iptables` magic for the backwards connections. However in the setup described in this how-to this is not an issue, since each application server has it's own |HAProxy| instance, each application server connects to 127.0.0.1, so MySQL will see that connections are coming from the application servers. Just like in the normal case.
