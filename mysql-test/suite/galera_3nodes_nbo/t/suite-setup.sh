datadir=$1
name=$2
cnf_file_name=component_keyring_file.cnf

# create local keyring config in datadir
echo "{\"path\": \"${MYSQL_TMP_DIR}/${name}/component_keyring_file\", \"read_only\": false }" > ${datadir}/$cnf_file_name

# create local manifest file in datadir
echo "{ \"components\": \"file://component_keyring_file\" }" > $datadir/mysqld.my

