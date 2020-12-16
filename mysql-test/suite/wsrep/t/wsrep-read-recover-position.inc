#
# Parse recover position uuid and seqno from an error log
# that was produced by running mysqld with option --wsrep_recover
#
# Arguments:
#
# ENV{'wsrep_recover_log'} - error log of the recovery step
#
# Return:
#
# wsrep_recover_start_position_uuid - UUID corresponding to recovered position
# wsrep_recover_start_position_seqno - seqno corresponding to recovered position
#

--perl
	use strict;
	my $wsrep_start_position = `grep ' Recovered position:' $ENV{'wsrep_recover_log'} | sed 's/.*Recovered\ position://' | sed 's/^[ \t]*//'`;
	chomp($wsrep_start_position);
        die if $wsrep_start_position eq '';
	open(FILE, ">", "$ENV{MYSQL_TMP_DIR}/galera_wsrep_start_position.inc") or die;
	my ($uuid, $seqno) = split /:/, $wsrep_start_position;
	print FILE "--let \$wsrep_recover_start_position_uuid = $uuid\n";
	print FILE "--let \$wsrep_recover_start_position_seqno = $seqno\n";
	close FILE;
EOF

--source $MYSQL_TMP_DIR/galera_wsrep_start_position.inc

if ($wsrep_recover_start_position_uuid == '') {
   --die "Could not obtain start_position_uuid."
}

if ($wsrep_recover_start_position_seqno == '') {
   --die "Could not obtain start_position_seqno."
}

--remove_file $MYSQL_TMP_DIR/galera_wsrep_start_position.inc
