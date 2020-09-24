# galera_cluster.inc
# ==================
#
# Description
# -----------
# Configure galera cluster with 3 nodes.
#

--let $galera_cluster_size = 3
--source include/galera_init.inc
