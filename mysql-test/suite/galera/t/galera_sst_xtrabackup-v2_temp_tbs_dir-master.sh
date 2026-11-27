#!/bin/bash
# node_2 keeps its session temporary tablespaces outside the datadir.
# InnoDB validates innodb_temp_tablespaces_dir at startup and refuses to start if
# it does not exist, so the directory has to be created before the server starts.
# It must NOT be emptied here: the test plants a leftover temp_*.ibt itself, so
# this script only ensures the directory exists.
mkdir -p "$MYSQL_TMP_DIR/pxb_n2_temp_tbs"
