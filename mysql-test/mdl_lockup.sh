#!/bin/bash

count=100

mysql1="mysql -utest -ptestpass -h0 -P3304 test"
mysql2="mysql -utest -ptestpass -h0 -P3302 test "
#mysql1="mysql -utest -ptestpass -h0 -P3307 test"
#mysql2="mysql -utest -ptestpass -h0 -P3308 test "


pre_query="CREATE TABLE IF NOT EXISTS test.xxxx (i int, j varchar(10000))engine=innodb"
echo Executing $query ...
$mysql1 -e "$pre_query"

for ((i=0; $i<$count; i=$i+1)) do
  echo "Iteration $i.."
  query="INSERT INTO test.xxxx VALUES($i, REPEAT('x',1000))"
  query2="FLUSH TABLES"
  $mysql1 -e "$query" &
  $mysql2 -e "$query2" &
done
