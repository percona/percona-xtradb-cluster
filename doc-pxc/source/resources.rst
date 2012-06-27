=========
Resources
=========

In general there are 4 resources that need to be different when you want to run several MySQL/Galera nodes on one host:

1) data directory
2) mysql client port and/or address
3) galera replication listen port and/or address
4) receive address for state snapshot transfer

and later incremental state transfer receive address will be added to the bunch. (I know, it is kinda a lot, but we don't see how it can be meaningfully reduced yet).

The first two are the usual mysql stuff.

You figured out the third. It is also possible to pass it via: ::

   wsrep_provider_options="gmcast.listen_addr=tcp://127.0.0.1:5678"

as most other Galera options. This may save you some extra typing.

The fourth one is :option:`wsrep_sst_receive_address`. This is the address at which the node will be listening for and receiving the state. Note that in galera cluster joining nodes are waiting for connections from donors. It goes contrary to tradition and seems to confuse people time and again, but there are good reasons it was made like that.

If you use mysqldump SST it should be the same as this mysql client connection address plus you need to set :option:`wsrep_sst_auth` variable to hold user:password pair. The user should be privileged enough to read system tables from donor and create system tables on this node. For simplicity that could be just the root user. Note that it also means that you need to properly set up the privileges on the new node before attempting to join the cluster. If you use |xtrabackup| as SST method, it will use /usr/bin/wsrep_sst_xtrabackup provided in Percona-XtraDB-Cluster-server package. And this script also needs user password if you have a password for root@localhost.

If you use rsync SST, :option:`wsrep_sst_auth` is not necessary unless your SST script makes use of it. :option:`wsrep_sst_address` can be anything local (it may even be the same on all nodes provided you'll be starting them one at a time).

