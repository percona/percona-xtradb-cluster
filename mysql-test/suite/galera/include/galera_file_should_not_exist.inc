#
# Returns success if the file does NOT exist
# Errors if the file does exist
# (this is the opposite of the file_exists command)
#
# This is useful for times when we need to ensure that a file
# has been deleted.
#
# Input parameter: $galera_filename
#
let MYSQLD_GALERA_FILENAME= $galera_filename;
perl;
	use strict;
	my $galera_file= $ENV{'MYSQLD_GALERA_FILENAME'} or die("MYSQLD_GALERA_FILENAME not set");
	die("Error!  Found a file that should not exist : ${galera_file}\n") if -f $galera_file;
	exit(0);
EOF

