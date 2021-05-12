--echo Unloading wsrep provider ...

<<<<<<< HEAD
--let $wsrep_cluster_address_orig = `select @@wsrep_cluster_address`
SET GLOBAL wsrep_cluster_address='';
||||||| merged common ancestors
--let $wsrep_cluster_address_orig = `SELECT @@wsrep_cluster_address`
--let $wsrep_provider_orig = `SELECT @@wsrep_provider`
--let $wsrep_provider_options_orig = `SELECT @@wsrep_provider_options`

SET GLOBAL wsrep_provider = 'none';
=======
--let $wsrep_cluster_address_orig = `SELECT @@wsrep_cluster_address`
--let $wsrep_provider_options_orig = `SELECT @@wsrep_provider_options`

SET GLOBAL wsrep_cluster_address = '';
>>>>>>> wsrep_5.7.33-25.25
