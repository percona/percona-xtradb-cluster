--let $wsrep_cluster_address_orig = `select @@wsrep_cluster_address`
SET GLOBAL wsrep_cluster_address='';
