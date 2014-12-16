--echo Unloading wsrep provider ...

--let $wsrep_cluster_address_orig = `SELECT @@wsrep_cluster_address`
--let $wsrep_provider_orig = `SELECT @@wsrep_provider`

SET GLOBAL wsrep_provider = 'none';
