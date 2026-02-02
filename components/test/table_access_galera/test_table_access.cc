/* Copyright (c) 2020, 2025, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is designed to work with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have either included with
   the program or referenced in the documentation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* Copyright (c) 2025 Percona LLC and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <mysql/components/component_implementation.h>
#include <mysql/components/service_implementation.h>
#include <mysql/components/services/mysql_current_thread_reader.h>
#include <mysql/components/services/table_access_service.h>
#include <mysql/components/services/udf_metadata.h>
#include <mysql/components/services/udf_registration.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <thread>

REQUIRES_SERVICE_PLACEHOLDER_AS(mysql_current_thread_reader, current_thd_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(udf_registration, udf_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(mysql_udf_metadata, udf_metadata_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(mysql_charset, charset_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(mysql_string_factory, string_factory_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(mysql_string_charset_converter,
                                string_converter_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(table_access_factory_v1, ta_factory_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(table_access_v1, ta_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(table_access_index_v1, ta_index_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(table_access_scan_v1, ta_scan_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(table_access_update_v1, ta_update_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(field_access_nullability_v1, fa_null_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(field_integer_access_v1, fa_integer_srv);
REQUIRES_SERVICE_PLACEHOLDER_AS(field_varchar_access_v1, fa_varchar_srv);

/*
  commit_action:
  0 = none
  1 = commit
  2 = rollback
*/
const char *common_insert_customer(char * /* out */, size_t num_tables,
                                   TA_lock_type lock_type, size_t ticket_fuzz,
                                   int commit_action) {
  const int ID_COL = 0;
  const int NAME_COL = 1;
  const int ADDRESS_COL = 2;
  static const TA_table_field_def columns[] = {
      {ID_COL, "ID", 2, TA_TYPE_INTEGER, false, 0},
      {NAME_COL, "NAME", 4, TA_TYPE_VARCHAR, false, 50},
      {ADDRESS_COL, "ADDRESS", 7, TA_TYPE_VARCHAR, true, 255}};

  const char *result;
  Table_access access = nullptr;
  TA_table table;
  long long id_value;
  my_h_string name_value = nullptr;
  size_t ticket;
  int rc;

  CHARSET_INFO_h utf8mb4_h = charset_srv->get_utf8mb4();
  MYSQL_THD thd;
  current_thd_srv->get(&thd);

  string_factory_srv->create(&name_value);

  access = ta_factory_srv->create(thd, num_tables);
  if (access == nullptr) {
    result = "create() failed";
    goto cleanup;
  }

  ticket = ta_srv->add(access, "shop", 4, "customer", 8, lock_type);

  rc = ta_srv->begin(access);
  if (rc != 0) {
    result = "begin() failed";
    goto cleanup;
  }

  table = ta_srv->get(access, ticket + ticket_fuzz);
  if (table == nullptr) {
    result = "get() failed";
    goto cleanup;
  }

  rc = ta_srv->check(access, table, columns, 3);
  if (rc != 0) {
    result = "check() failed";
    goto cleanup;
  }

  id_value = 1;

  if (fa_integer_srv->set(access, table, ID_COL, id_value)) {
    result = "set(id) failed";
    goto cleanup;
  }

  string_converter_srv->convert_from_buffer(name_value, "John Doe", 8,
                                            utf8mb4_h);

  if (fa_varchar_srv->set(access, table, NAME_COL, name_value)) {
    result = "set(name) failed";
    goto cleanup;
  }

  fa_null_srv->set(access, table, ADDRESS_COL);

  rc = ta_update_srv->insert(access, table);
  if (rc != 0) {
    result = "insert() failed";
    goto cleanup;
  }

  if (commit_action == 1) {
    if (ta_srv->commit(access)) {
      result = "commit() failed";
      goto cleanup;
    }
  } else if (commit_action == 2) {
    if (ta_srv->rollback(access)) {
      result = "rollback() failed";
      goto cleanup;
    }
  } else {
    result = "OK, but forgot to commit";
    goto cleanup;
  }

  result = "OK";

cleanup:
  if (name_value != nullptr) {
    string_factory_srv->destroy(name_value);
  }
  if (access != nullptr) {
    ta_factory_srv->destroy(access);
  }

  return result;
}

/*
  commit_action:
  0 = none
  1 = commit
  2 = rollback
*/
const char *common_insert_customer_insert_multiple(char * /* out */,
                                                   size_t num_tables,
                                                   TA_lock_type lock_type,
                                                   size_t ticket_fuzz,
                                                   int commit_action) {
  const int ID_COL = 0;
  const int NAME_COL = 1;
  const int ADDRESS_COL = 2;
  static const TA_table_field_def columns[] = {
      {ID_COL, "ID", 2, TA_TYPE_INTEGER, false, 0},
      {NAME_COL, "NAME", 4, TA_TYPE_VARCHAR, false, 50},
      {ADDRESS_COL, "ADDRESS", 7, TA_TYPE_VARCHAR, true, 255}};

  const char *result;
  Table_access access = nullptr;
  TA_table table;
  long long id_value;
  my_h_string name_value = nullptr;
  size_t ticket;
  int rc;

  CHARSET_INFO_h utf8mb4_h = charset_srv->get_utf8mb4();
  MYSQL_THD thd;
  current_thd_srv->get(&thd);

  string_factory_srv->create(&name_value);

  access = ta_factory_srv->create(thd, num_tables);
  if (access == nullptr) {
    result = "create() failed";
    goto cleanup;
  }

  ticket = ta_srv->add(access, "shop", 4, "customer", 8, lock_type);

  rc = ta_srv->begin(access);
  if (rc != 0) {
    result = "begin() failed";
    goto cleanup;
  }

  table = ta_srv->get(access, ticket + ticket_fuzz);
  if (table == nullptr) {
    result = "get() failed";
    goto cleanup;
  }

  rc = ta_srv->check(access, table, columns, 3);
  if (rc != 0) {
    result = "check() failed";
    goto cleanup;
  }

  for (int i = 2; i <= 6; ++i) {
    id_value = i;
    if (fa_integer_srv->set(access, table, ID_COL, id_value)) {
      result = "set(id) failed";
      goto cleanup;
    }

    char name_buffer[20];
    snprintf(name_buffer, sizeof(name_buffer), "Unknown Person - %d", i);
    string_converter_srv->convert_from_buffer(name_value, name_buffer,
                                              strlen(name_buffer), utf8mb4_h);

    if (fa_varchar_srv->set(access, table, NAME_COL, name_value)) {
      result = "set(name) failed";
      goto cleanup;
    }

    fa_null_srv->set(access, table, ADDRESS_COL);

    rc = ta_update_srv->insert(access, table);
    if (rc != 0) {
      result = "insert() failed";
      goto cleanup;
    }
  }

  if (commit_action == 1) {
    if (ta_srv->commit(access)) {
      result = "commit() failed";
      goto cleanup;
    }
  } else if (commit_action == 2) {
    if (ta_srv->rollback(access)) {
      result = "rollback() failed";
      goto cleanup;
    }
  } else {
    result = "OK, but forgot to commit";
    goto cleanup;
  }

  result = "OK";

cleanup:
  if (name_value != nullptr) {
    string_factory_srv->destroy(name_value);
  }
  if (access != nullptr) {
    ta_factory_srv->destroy(access);
  }

  return result;
}

/*
  commit_action:
  0 = none
  1 = commit
  2 = rollback
*/
const char *common_delete_customer(
    char * /* out */, size_t num_tables, TA_lock_type lock_type,
    size_t ticket_fuzz, int commit_action,
    int primary_key_value /*0 for delete all rows*/) {
  const int ID_COL = 0;
  const int NAME_COL = 1;
  const int ADDRESS_COL = 2;
  static const char *pk_customer_name = "PRIMARY";
  static size_t pk_customer_name_length = 7;
  static const TA_index_field_def pk_customer_cols[] = {{"ID", 2, false}};
  static const size_t pk_customer_numcol = 1;

  static const TA_table_field_def columns[] = {
      {ID_COL, "ID", 2, TA_TYPE_INTEGER, false, 0},
      {NAME_COL, "NAME", 4, TA_TYPE_VARCHAR, false, 50},
      {ADDRESS_COL, "ADDRESS", 7, TA_TYPE_VARCHAR, true, 255}};

  const char *result;
  Table_access access = nullptr;
  TA_table table = nullptr;
  TA_key customer_pk = nullptr;
  size_t ticket;
  int rc;

  MYSQL_THD thd;
  current_thd_srv->get(&thd);

  access = ta_factory_srv->create(thd, num_tables);
  if (access == nullptr) {
    result = "create() failed";
    goto cleanup;
  }

  ticket = ta_srv->add(access, "shop", 4, "customer", 8, lock_type);

  rc = ta_srv->begin(access);
  if (rc != 0) {
    result = "begin() failed";
    goto cleanup;
  }

  table = ta_srv->get(access, ticket + ticket_fuzz);
  if (table == nullptr) {
    result = "get() failed";
    goto cleanup;
  }

  rc = ta_srv->check(access, table, columns, 3);
  if (rc != 0) {
    result = "check() failed";
    goto cleanup;
  }

  if (primary_key_value == 0) {  // Delete all rows
    if (ta_scan_srv->init(access, table)) {
      result = "rnd_init() failed";
      goto cleanup;
    }
    while (ta_scan_srv->next(access, table) == 0) {
      rc = ta_update_srv->delete_row(access, table);
      if (rc != 0) {
        result = "delete_row() failed during scan";
        // Attempt to end scan even if delete fails
        ta_scan_srv->end(access, table);
        goto cleanup;
      }
    }
    if (ta_scan_srv->end(access, table)) {
      result = "end() failed";
      goto cleanup;
    }
  } else {  // Delete specific row by ID
    // Initialize the index for the customer table using its primary key (ID)
    // If table does not have index column init will fail and inidividual row
    // cannot be deleted.
    if (ta_index_srv->init(access, table, pk_customer_name,
                           pk_customer_name_length, pk_customer_cols,
                           pk_customer_numcol, &customer_pk)) {
      result = "Index server init() failed for customer PK";
      goto cleanup;
    }

    // Set the value for the ID column to search for
    if (fa_integer_srv->set(access, table, ID_COL, primary_key_value)) {
      result = "set(customer::id) failed for index search";
      goto cleanup;
    }

    // Read the row using the index
    rc = ta_index_srv->read_map(access, table, 1, customer_pk);
    if (rc != 0) {
      result = "No such order";
      goto cleanup;
    }
    // If row found, delete it
    rc = ta_update_srv->delete_row(access, table);
    if (rc != 0) {
      result = "delete_row() failed for specific ID";
      goto cleanup;
    }
  }

  if (commit_action == 1) {
    if (ta_srv->commit(access)) {
      result = "commit() failed";
      goto cleanup;
    }
  } else if (commit_action == 2) {
    if (ta_srv->rollback(access)) {
      result = "rollback() failed";
      goto cleanup;
    }
  } else {
    result = "OK, but forgot to commit";
    goto cleanup;
  }

  result = "OK";

cleanup:
  if (customer_pk != nullptr) ta_index_srv->end(access, table, customer_pk);
  if (access != nullptr) {
    ta_factory_srv->destroy(access);
  }

  return result;
}

const char *test_insert_customer(char *out) {
  // Nominal path
  return common_insert_customer(out, 1, TA_WRITE, 0, 1);
}

const char *test_insert_customer_insert_multiple(char *out) {
  // Nominal path
  return common_insert_customer_insert_multiple(out, 1, TA_WRITE, 0, 1);
}

const char *test_insert_customer_1(char *out) {
  return common_insert_customer(out, 0, TA_READ, 0, 1);
}

const char *test_insert_customer_2(char *out) {
  return common_insert_customer(out, 5, TA_READ, 99, 1);
}

const char *test_insert_customer_3(char *out) {
  return common_insert_customer(out, 1, TA_WRITE, 99, 1);
}

const char *test_insert_customer_4(char *out) {
  return common_insert_customer(out, 1, TA_WRITE, 0, 0);
}

const char *test_insert_customer_5(char *out) {
  return common_insert_customer(out, 1, TA_WRITE, 0, 2);
}

const char *test_delete_customer(char *out) {
  // Nominal path
  return common_delete_customer(out, 1, TA_WRITE, 0, 1, 0);
}

const char *test_delete_customer_4(char *out) {
  // Nominal path
  return common_delete_customer(out, 1, TA_WRITE, 0, 0, 0);
}

const char *test_delete_customer_5(char *out) {
  // Nominal path
  return common_delete_customer(out, 1, TA_WRITE, 0, 2, 0);
}

const char *test_delete_all_customers(char *out) {
  // Delete all rows (primary_key_value = 0)
  return common_delete_customer(out, 1, TA_WRITE, 0, 1, 0);
}

const char *test_delete_customer_id_2(char *out) {
  // Delete row with ID = 2
  return common_delete_customer(out, 1, TA_WRITE, 0, 1, 2);
}

typedef const char *(*test_driver_fn)(char *output_message_buffer);

struct test_driver_t {
  const char *m_name;
  test_driver_fn m_driver;
};

static test_driver_t driver[] = {
    {"INSERT-CUSTOMER", test_insert_customer},
    {"INSERT-CUSTOMER-STRESS-1", test_insert_customer_1},
    {"INSERT-CUSTOMER-STRESS-2", test_insert_customer_2},
    {"INSERT-CUSTOMER-STRESS-3", test_insert_customer_3},
    {"INSERT-CUSTOMER-NO-COMMIT", test_insert_customer_4},
    {"INSERT-CUSTOMER-ROLLBACK", test_insert_customer_5},
    {"INSERT-CUSTOMER-INSERT-MULTIPLE", test_insert_customer_insert_multiple},
    {"DELETE-CUSTOMER", test_delete_customer},
    {"DELETE-CUSTOMER-NO-COMMIT", test_delete_customer_4},
    {"DELETE-CUSTOMER-ROLLBACK", test_delete_customer_5},
    {"DELETE-ALL-CUSTOMERS", test_delete_all_customers},
    {"DELETE-CUSTOMER-ID-2", test_delete_customer_id_2},
    {nullptr, nullptr},
};

static const char *const udf_name = "test_table_access_driver";
static const size_t udf_result_size = 80;

static bool udf_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
  initid->maybe_null = true;
  initid->max_length = udf_result_size;

  if (args->arg_count != 1) {
    sprintf(message, "%s() requires 1 argument", udf_name);
    return true;
  }

  args->arg_type[0] = STRING_RESULT;

  const char *attr_name = "charset";
  const char *attr_value = "utf8mb4";
  char *attr_value_2 = const_cast<char *>(attr_value);
  if (udf_metadata_srv->result_set(initid, attr_name, attr_value_2)) {
    return true;
  }

  return false;
}

static void udf_deinit(UDF_INIT *) {}

static char *test_table_access_driver(UDF_INIT *, UDF_ARGS *args, char *result,
                                      unsigned long *length,
                                      unsigned char *is_null,
                                      unsigned char *error) {
  const char *p1 = args->args[0];
  size_t len_p1 = args->lengths[0];

  test_driver_t *entry;
  char output_message[255];

  for (entry = &driver[0]; entry->m_name != nullptr; entry++) {
    if (strlen(entry->m_name) == len_p1) {
      if (strncmp(entry->m_name, p1, len_p1) == 0) {
        const char *fn_result = (*entry->m_driver)(output_message);
        if (fn_result != nullptr) {
          size_t len = strlen(fn_result);
          len = std::min(len, udf_result_size);
          memcpy(result, fn_result, len);
          *length = len;
          *is_null = 0;
          *error = 0;
        } else {
          *is_null = 1;
          *error = 0;
        }
        return result;
      }
    }
  }

  *error = 1;
  return nullptr;
}

#define CONST_STR_AND_LEN(x) x, sizeof(x) - 1

/**
 @param [out] status: true for failure, false otherwise
 */
static void thd_function(bool *ret) {
  TA_table tb = nullptr, write_tb = nullptr;
  Table_access ta = nullptr;
  size_t ticket = 0, ticket_write = 0;
  bool txn_started = false;
  *ret = true;

  ta = ta_factory_srv->create(nullptr, 2);

  if (!ta) goto cleanup;
  ticket = ta_srv->add(ta, CONST_STR_AND_LEN("mysql"), CONST_STR_AND_LEN("db"),
                       TA_READ);
  ticket_write = ta_srv->add(ta, CONST_STR_AND_LEN("mysql"),
                             CONST_STR_AND_LEN("user"), TA_WRITE);

  if (ta_srv->begin(ta)) goto cleanup;
  txn_started = true;
  tb = ta_srv->get(ta, ticket);
  if (!tb) goto cleanup;
  write_tb = ta_srv->get(ta, ticket_write);
  if (!write_tb) goto cleanup;

  *ret = false;
cleanup:
  if (txn_started) ta_srv->rollback(ta);
  if (ta) ta_factory_srv->destroy(ta);
}

static bool test_native_thread() {
  bool retval = true;
  std::thread thd(thd_function, &retval);
  thd.join();
  return retval;
}

mysql_service_status_t test_table_access_init() {
  if (udf_srv->udf_register(udf_name, Item_result::STRING_RESULT,
                            (Udf_func_any)test_table_access_driver, udf_init,
                            udf_deinit)) {
    return 1;
  }

  if (test_native_thread()) return 1;

  return 0;
}

mysql_service_status_t test_table_access_deinit() {
  int was_present = 0;

  if (udf_srv->udf_unregister(udf_name, &was_present)) {
    return 1;
  }

  return 0;
}

BEGIN_COMPONENT_PROVIDES(test_table_access)
END_COMPONENT_PROVIDES();

BEGIN_COMPONENT_REQUIRES(test_table_access)
REQUIRES_SERVICE_AS(mysql_current_thread_reader, current_thd_srv),
    REQUIRES_SERVICE_AS(udf_registration, udf_srv),
    REQUIRES_SERVICE_AS(mysql_udf_metadata, udf_metadata_srv),
    REQUIRES_SERVICE_AS(mysql_charset, charset_srv),
    REQUIRES_SERVICE_AS(mysql_string_factory, string_factory_srv),
    REQUIRES_SERVICE_AS(mysql_string_charset_converter, string_converter_srv),
    REQUIRES_SERVICE_AS(table_access_factory_v1, ta_factory_srv),
    REQUIRES_SERVICE_AS(table_access_v1, ta_srv),
    REQUIRES_SERVICE_AS(table_access_index_v1, ta_index_srv),
    REQUIRES_SERVICE_AS(table_access_scan_v1, ta_scan_srv),
    REQUIRES_SERVICE_AS(table_access_update_v1, ta_update_srv),
    REQUIRES_SERVICE_AS(field_access_nullability_v1, fa_null_srv),
    REQUIRES_SERVICE_AS(field_integer_access_v1, fa_integer_srv),
    REQUIRES_SERVICE_AS(field_varchar_access_v1, fa_varchar_srv),
    END_COMPONENT_REQUIRES();

BEGIN_COMPONENT_METADATA(test_table_access)
METADATA("mysql.author", "Oracle Corporation"),
    METADATA("mysql.license", "GPL"), METADATA("test_table_access", "1"),
    END_COMPONENT_METADATA();

DECLARE_COMPONENT(test_table_access, "mysql:test_table_access")
test_table_access_init, test_table_access_deinit END_DECLARE_COMPONENT();

DECLARE_LIBRARY_COMPONENTS &COMPONENT_REF(test_table_access)
    END_DECLARE_LIBRARY_COMPONENTS
