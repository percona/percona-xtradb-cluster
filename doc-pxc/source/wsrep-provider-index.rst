.. _wsrep_provider_index:

============================================
 Index of :variable:`wsrep_provider` options
============================================

Following variables can be set and checked in the :variable:`wsrep_provider_options` variable. Value of the variable can be changed in the |MySQL| configuration file, :file:`my.cnf`, or by setting the variable value in the |MySQL| client.

To change the value of the in the :file:`my.cnf` following syntax should be used: :: 

  wsrep_provider_options="variable1=value1;[variable2=value2]"

For example to increase the size of the Galera buffer storage from the default value to 512MB, :file:`my.cnf` option should look like: ::

  wsrep_provider_options="gcache.size=512M"

Dynamic variables can be changed from the |MySQL| client by using the ``SET GLOBAL`` syntax. To change the value of the :variable:`pc.ignore_sb` following command should be used:: 

  mysql> SET GLOBAL wsrep_provider_options="pc.ignore_sb=true";


Index
=====

.. variable:: base_host

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: value of the :variable:`wsrep_node_address`

This variable sets the value of the node's base IP. This is an IP address on which Galera listens for connections from other nodes. Setting this value incorrectly would stop the node from communicating with other nodes.

.. variable:: base_port

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 4567

This variable sets the port on which the Galera listens for connections from other nodes. Setting this value incorrectly would stop the node from communicating with other nodes.

.. variable:: cert.log_conflicts

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: no

.. variable:: evs.causal_keepalive_period

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: value of :variable:`evs.keepalive_period`

This variable is used for development purposes and shouldn't be used by regular users. 

.. variable:: evs.debug_log_mask

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 0x1

This variable is used for EVS (Extended Virtual Synchrony) debugging it can be used only when :variable:`wsrep_debug` is set to ``ON``.

.. variable:: evs.inactive_check_period

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT0.5S

This variable defines how often to check for peer inactivity.

.. variable:: evs.inactive_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT15S

This variable defines the inactivity limit, once this limit is reached the node will be pronounced dead.

.. variable:: evs.info_log_mask

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

This variable is used for controlling the extra EVS info logging.

.. variable:: evs.install_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: PT15S

This variable defines the timeout on waiting for install message acknowledgments.

.. variable:: evs.join_retrans_period

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT1S

This variable defines how often to retransmit EVS join messages when forming cluster membership.

.. variable:: evs.keepalive_period 

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT1S 

This variable defines how often will keepalive beacons will be emmited (in the absence of any other traffic).

.. variable:: evs.max_install_timeouts

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable defines how many membership install rounds to try before giving up (total rounds will be :variable:`evs.max_install_timeouts` + 2).

.. variable:: evs.send_window

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 4

This variable defines the maximum number of data packets in replication at a time. For WAN setups may be set considerably higher, e.g. 512. This variable must be no less than :variable:`evs.user_send_window`.

.. variable:: evs.stats_report_period

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT1M 

This variable defines the control period of EVS statistics reporting.

.. variable:: evs.suspect_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT5S

This variable defines the inactivity period after which the node is “suspected” to be dead. If all remaining nodes agree on that, the node will be dropped out of cluster even before :variable:`evs.inactive_timeout` is reached.

.. variable:: evs.use_aggregate

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: true

When this variable is enabled smaller packets will be aggregated into one.

.. variable:: evs.user_send_window

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 2

This variable defines the maximum number of data packets in replication at a time. For WAN setups may be set considerably higher, e.g. 512.

.. variable:: evs.version

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

.. variable:: evs.view_forget_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: P1D

This variable defines the timeout after which past views will be dropped from history.

.. variable:: gcache.dir

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: :term:`datadir`

This variable can be used to define the location of the :file:`galera.cache` file.

.. variable:: gcache.keep_pages_size

   :cli: Yes
   :conf: Yes
   :scope: Local, Global
   :dyn: No
   :default: 0

This variable is used to specify total size of the page storage pages to keep for caching purposes. If only page storage is enabled, one page is always present.

.. variable:: gcache.mem_size

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0


.. variable:: gcache.name

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: /var/lib/mysql/galera.cache

This variable can be used to specify the name of the Galera cache file.

.. variable:: gcache.page_size

   :cli: No
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 128M

This variable can be used to specify the size of the page files in the page storage.

.. variable:: gcache.size

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 128M

Size of the transaction cache for Galera replication. This defines the size of the :file:`galera.cache` file which is used as source for |IST|. If this value is bigger there are better chances that the re-joining node will get IST instead of |SST|.

.. variable:: gcs.fc_debug

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

This variable specifies after how many writesets the debug statistics about SST flow control will be posted.

.. variable:: gcs.fc_factor

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable is used for replication flow control. Replication will be paused till the value of this variable goes below the value of :variable:`gcs.fc_factor` * :variable:`gcs.fc_limit`.

.. variable:: gcs.fc_limit

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 16

This variable is used for replication flow control. When slave queue exceeds this limit replication will be paused.

.. variable:: gcs.fc_master_slave

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: NO

This variable is used to specify if there is only one master node in the cluster.

.. variable:: gcs.max_packet_size

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 64500

This variable is used to specify the writeset size after which they will be fragmented.

.. variable:: gcs.max_throttle

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0.25

This variable specifies how much the replication can be throttled during the state transfer in order to avoid running out of memory. Value can be set to ``0.0`` if stopping replication is acceptable in order to finish state transfer.
 
.. variable:: gcs.recv_q_hard_limit

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 9223372036854775807

This variable specifies the maximum allowed size of the receive queue. This should normally be half of (RAM + swap). If this limit is exceeded, Galera will abort the server.

.. variable:: gcs.recv_q_soft_limit

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0.25

This variable specifies the fraction of the :variable:`gcs.recv_q_hard_limit` after which replication rate will be throttled.

.. variable:: gcs.sync_donor

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: NO

This variable controls if the rest of the cluster should be in sync with the donor node. When this variable is set to ``Yes`` whole cluster will be blocked if the donor node is blocked with SST.

.. variable:: gmcast.listen_addr

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: tcp://0.0.0.0:4567

This variable defines the address on which node listens to connections from other nodes in the cluster.

.. variable:: gmcast.mcast_addr

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: None

This variable should be set up if UDP multicast should be used for replication.

.. variable:: gmcast.mcast_ttl

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 1

This variable can be used to define TTL for multicast packets.

.. variable:: gmcast.peer_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT3S

This variable specifies the connection timeout to initiate message relaying.

.. variable:: gmcast.segment

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

This variable specifies the group segment this member should be a part of. Same segment members are treated as equally physically close.

.. variable:: gmcast.time_wait

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT5S

This variable specifies the time to wait until allowing peer declared outside of stable view to reconnect.

.. variable:: gmcast.version

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

.. variable:: ist.recv_addr

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: value of :variable:`wsrep_node_address`

This variable specifies the address on which nodes listens for Incremental State Transfer (|IST|).

.. variable:: pc.checksum

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: true

This variable controls will the replicated messages will be checksummed or not.

.. variable::  pc.ignore_quorum

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: false

When this variable is set to ``TRUE`` node will completely ignore the quorum calculations. This should be used with extreme caution even in master-slave setups, because slaves won't automatically reconnect to master in this case.

.. variable::  pc.ignore_sb

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: false

When this variable us set ti ``TRUE`` node will process updates even in the case of split brain. This should be used with extreme caution in multi-master setup, but should simplify things in master-slave cluster (especially if only 2 nodes are used).

.. variable::  pc.linger 

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: PT20S

This variable specifies the period which PC protocol waits for EVS termination.

.. variable::  pc.npvo 

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: false

When this variable is set to ``TRUE`` more recent primary component overrides older ones in case of conflicting prims.

.. variable::  pc.recovery

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: true

When this variable is set to ``true`` the node stores the Primary Component state to disk. The Primary Component can then recover automatically when all nodes that were part of the last saved state re-establish communications with each other. This feature allows automatic recovery from full cluster crashes, such as in the case of a data center power outage and graceful full cluster restarts without the need for explicitly bootstrapping a new Primary Component.

.. variable::  pc.version

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

.. variable::  pc.weight 

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: 1

This variable specifies the node weight that's going to be used for Weighted Quorum calculations.

.. variable::  protonet.backend

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: asio

This variable is used to define which transport backend should be used. Currently only ``ASIO`` is supported.

.. variable::  protonet.version

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 0

.. variable::  repl.causal_read_timeout

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: PT30S

This variable specifies the causal read timeout.

.. variable::  repl.commit_order

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 3

This variable is used to specify Out-Of-Order committing (which is used to improve parallel applying performance). Allowed values are:

 * ``0`` – BYPASS: all commit order monitoring is turned off (useful for measuring performance penalty)
 * ``1`` – OOOC: allow out of order committing for all transactions
 * ``2`` – LOCAL_OOOC: allow out of order committing only for local transactions
 * ``3`` – NO_OOOC: no out of order committing is allowed (strict total order committing)

.. variable::  repl.key_format 
 
   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: Yes
   :default: FLAT8

This variable is used to specify the replication key format. Allowed values are:

 * ``FLAT8`` shorter - higher probability of key match false positives
 * ``FLAT16`` longer - lower probability of false positives. 
 * ``FLAT8A`` - same as ``FLAT8`` but with annotations for debug purposes.
 * ``FLAT16A`` - same as ``FLAT16`` but with annotations for debug purposes.

.. variable::  repl.proto_max 

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 5

This variable is used to specify the highest communication protocol version to accept in the cluster. This variable is used only for debugging.

.. variable::  socket.checksum

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: 2

This variable is used to choose the checksum algorithm for network packets. Available options are:

 * ``0`` - disable checksum
 * ``1`` - plain ``CRC32`` (used in Galera 2.x)
 * ``2`` - hardware accelerated ``CRC32-C``

.. variable::  socket.ssl

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: No

This variable is used to specify if the SSL encryption should be used.


.. variable::  socket.ssl_cert

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No

This variable is used to specify the path (absolute or relative to working directory) to an SSL certificate (in PEM format).

.. variable:: socket.ssl_key

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No


This variable is used to specify the path (absolute or relative to working directory) to an SSL private key for the certificate (in PEM format).

.. variable:: socket.ssl_compression

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: Yes

This variable is used to specify if the SSL compression is to be used.

.. variable:: socket.ssl_cipher	

   :cli: Yes
   :conf: Yes
   :scope: Global
   :dyn: No
   :default: AES128-SHA 

This variable is used to specify what cypher will be used for encryption.


