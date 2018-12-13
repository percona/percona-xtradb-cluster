#
# Gets the node name for the server
# This will return the variable in $galera_actual_node_name
#
-- echo # Extracting server node name
--perl
	use_strict;
		my $galera_actual_node_name = `uname -n`;
		chomp($galera_actual_node_name);

	open(FILE, ">", "$ENV{MYSQL_TMP_DIR}/galera_actual_node_name.inc") or die;
		print FILE "--let \$galera_actual_node_name = $galera_actual_node_name\n";
		close FILE;
EOF
--source $MYSQL_TMP_DIR/galera_actual_node_name.inc
--remove_file $MYSQL_TMP_DIR/galera_actual_node_name.inc
