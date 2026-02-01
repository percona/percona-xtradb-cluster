# INSERT

--echo
SELECT test_table_access_driver("unknown test");

--echo
--echo # Below will fail since tables do not exist.
SELECT test_table_access_driver("INSERT-CUSTOMER-STRESS-1");
SELECT test_table_access_driver("INSERT-CUSTOMER-STRESS-2");
SELECT test_table_access_driver("INSERT-CUSTOMER");

--let $assert_text= 'Tables do not exist so above statements failed'
--let $assert_cond= "[SELECT COUNT(*) FROM information_schema.tables WHERE TABLE_SCHEMA=\"shop\"]" = 0
--source include/assert.inc

CREATE SCHEMA shop;

SELECT test_table_access_driver("INSERT-CUSTOMER");

--let $assert_text= 'Tables do not exist so above statements failed'
--let $assert_cond= "[SELECT COUNT(*) FROM information_schema.tables WHERE TABLE_SCHEMA=\"shop\"]" = 0
--source include/assert.inc

CREATE TABLE shop.customer (x integer);

--let $test_details= Different schema expect failure
--let $statement=
--let $diff_servers= 1 2
--let $databases= shop
--let $table= customer
--let $order_by_column = x
--source ../include/assert_table_are_same_in_cluster.inc

--let $test_details= Different schema expect failure
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER-STRESS-3");
--source ../include/assert_table_are_same_in_cluster.inc

--let $test_details= Different schema expect failure
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER");
--source ../include/assert_table_are_same_in_cluster.inc

DROP TABLE shop.customer;

CREATE TABLE shop.customer(x integer, y integer, z integer);

--let $test_details= Different schema expect failure
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER");
--source ../include/assert_table_are_same_in_cluster.inc

DROP TABLE shop.customer;

CREATE TABLE shop.customer(
  `ID` integer not null PRIMARY KEY,
  `NAME` varchar(50) not null,
  `ADDRESS` varchar(255));

--let $test_details= No rows exist due to no commit.
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER-NO-COMMIT");
--let $diff_servers= 1 2
--let $databases= shop
--let $table= customer
--let $order_by_column = ID
--source ../include/assert_table_are_same_in_cluster.inc

--let $test_details= No rows exist due to rollback.
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER-ROLLBACK");
--source ../include/assert_table_are_same_in_cluster.inc

--let $test_details= Commit.
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER");
--source ../include/assert_table_are_same_in_cluster.inc

# TEST Multiple inserts
--let $test_details= INSERT multiple records.
--let $statement= SELECT test_table_access_driver("INSERT-CUSTOMER-INSERT-MULTIPLE");
--source ../include/assert_table_are_same_in_cluster.inc

# TEST Delete with commit missing
--let $test_details= DELETE without commit fails.
--let $statement= SELECT test_table_access_driver("DELETE-CUSTOMER-NO-COMMIT");
--source ../include/assert_table_are_same_in_cluster.inc

# TEST Delete with rollback
--let $test_details= DELETE with rollback.
--let $statement= SELECT test_table_access_driver("DELETE-CUSTOMER-ROLLBACK");
--source ../include/assert_table_are_same_in_cluster.inc

# TEST Deleting 1 row
--let $test_details= Single row delete.
--let $statement= SELECT test_table_access_driver("DELETE-CUSTOMER-ID-2");
--source ../include/assert_table_are_same_in_cluster.inc

# TEST Deleting all rows
--let $test_details= All rows delete.
--let $statement= SELECT test_table_access_driver("DELETE-ALL-CUSTOMERS");
--source ../include/assert_table_are_same_in_cluster.inc

DROP TABLE shop.customer;
DROP SCHEMA shop;

--let $databases=
--let $diff_servers=
--let $table=
--let $order_by_column=
--let $table1=
--let $order_by_column1=
--let $statement=
--let $test_details=
--let $outside_component_transactions=
