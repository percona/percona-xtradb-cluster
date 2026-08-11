--disable_query_log
--disable_warnings
# Make sure no telemetry component is already installed.
<<<<<<< HEAD
let $have_component = `SELECT count(*)>=1 AS installed FROM performance_schema.global_variables WHERE variable_name like "%telemetry%" and variable_name not like "percona_telemetry%";`;
||||||| d76e81f492e
let $have_component = `SELECT count(*)>=1 AS installed FROM performance_schema.global_variables WHERE variable_name like "%telemetry%";`;
=======
# The percona_telemetry component and the percona_telemetry_disable server
# variable also match "%telemetry%", but percona_telemetry only collects usage
# data and does not conflict with the server telemetry traces/logs components,
# so exclude it. Without this the whole family of telemetry tests always skips
# on Percona Server.
let $have_component = `SELECT count(*)>=1 AS installed FROM performance_schema.global_variables WHERE variable_name like "%telemetry%" AND variable_name NOT LIKE "percona_telemetry%";`;
>>>>>>> ps/release-8.4.11-11
if ($have_component)
{
  skip Requires no telemetry component already installed;
}
--enable_warnings
--enable_query_log
