--source include/safe_to_bootstrap.inc
--source include/xplugin_drop_user.inc
--source include/restart_mysqld.inc
if ($xplugin_user_dropped) {
  --source include/xplugin_create_user.inc
}
