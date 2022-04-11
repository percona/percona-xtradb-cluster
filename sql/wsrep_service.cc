/* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
   Copyright (c) 2019, Percona Inc. All Rights Reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "wsrep_service.h"
#include "debug_sync.h"
#include "wsrep_priv.h"

namespace wsp {

Field_value::Field_value() : is_unsigned(false), has_ptr(false) {}

Field_value::Field_value(const Field_value &other)
    : value(other.value),
      v_string_length(other.v_string_length),
      is_unsigned(other.is_unsigned),
      has_ptr(other.has_ptr) {
  if (other.has_ptr) {
    copy_string(other.value.v_string, other.v_string_length);
  }
}

Field_value &Field_value::operator=(const Field_value &other) {
  if (&other != this) {
    this->~Field_value();

    value = other.value;
    v_string_length = other.v_string_length;
    is_unsigned = other.is_unsigned;
    has_ptr = other.has_ptr;

    if (other.has_ptr) {
      copy_string(other.value.v_string, other.v_string_length);
    }
  }

  return *this;
}

Field_value::Field_value(const longlong &num, bool unsign) {
  value.v_long = num;
  is_unsigned = unsign;
  has_ptr = false;
}

Field_value::Field_value(const double num) {
  value.v_double = num;
  has_ptr = false;
}

Field_value::Field_value(const decimal_t &decimal) {
  value.v_decimal = decimal;
  has_ptr = false;
}

Field_value::Field_value(const MYSQL_TIME &time) {
  value.v_time = time;
  has_ptr = false;
}

void Field_value::copy_string(const char *str, size_t length) {
  value.v_string = (char *)malloc(length + 1);
  if (value.v_string) {
    value.v_string[length] = '\0';
    memcpy(value.v_string, str, length);
    v_string_length = length;
    has_ptr = true;
  } else {
    WSREP_ERROR("Error copying from empty string "); /* purecov: inspected */
  }
}

Field_value::Field_value(const char *str, size_t length) {
  copy_string(str, length);
}

Field_value::~Field_value() {
  if (has_ptr && value.v_string) {
    free(value.v_string);
  }
}

/** resultset class **/

void Sql_resultset::clear() {
  while (!result_value.empty()) {
    std::vector<Field_value *> fld_val = result_value.back();
    result_value.pop_back();
    while (!fld_val.empty()) {
      Field_value *fld = fld_val.back();
      fld_val.pop_back();
      delete fld;
    }
  }
  result_value.clear();
  result_meta.clear();

  current_row = 0;
  num_cols = 0;
  num_rows = 0;
  num_metarow = 0;
  m_resultcs = NULL;
  m_server_status = 0;
  m_warn_count = 0;
  m_affected_rows = 0;
  m_last_insert_id = 0;
  m_sql_errno = 0;
  m_killed = false;
}

void Sql_resultset::new_row() {
  result_value.push_back(std::vector<Field_value *>());
}

void Sql_resultset::new_field(Field_value *val) {
  result_value[num_rows].push_back(val);
}

bool Sql_resultset::next() {
  if (current_row < (int)num_rows - 1) {
    current_row++;
    return true;
  }

  return false;
}

const st_command_service_cbs Sql_service_context_base::sql_service_callbacks = {
    &Sql_service_context_base::sql_start_result_metadata,
    &Sql_service_context_base::sql_field_metadata,
    &Sql_service_context_base::sql_end_result_metadata,
    &Sql_service_context_base::sql_start_row,
    &Sql_service_context_base::sql_end_row,
    &Sql_service_context_base::sql_abort_row,
    &Sql_service_context_base::sql_get_client_capabilities,
    &Sql_service_context_base::sql_get_null,
    &Sql_service_context_base::sql_get_integer,
    &Sql_service_context_base::sql_get_longlong,
    &Sql_service_context_base::sql_get_decimal,
    &Sql_service_context_base::sql_get_double,
    &Sql_service_context_base::sql_get_date,
    &Sql_service_context_base::sql_get_time,
    &Sql_service_context_base::sql_get_datetime,
    &Sql_service_context_base::sql_get_string,
    &Sql_service_context_base::sql_handle_ok,
    &Sql_service_context_base::sql_handle_error,
    &Sql_service_context_base::sql_shutdown,
    &Sql_service_context_base::sql_connection_alive,
};

int Sql_service_context::start_result_metadata(
    uint ncols, uint flags MY_ATTRIBUTE((unused)),
    const CHARSET_INFO *resultcs) {
  DBUG_ENTER("Sql_service_context::start_result_metadata");
  DBUG_PRINT("info", ("resultcs->name: %s", resultcs->name));
  if (resultset) {
    resultset->set_cols(ncols);
    resultset->set_charset(resultcs);
  }
  DBUG_RETURN(0);
}

int Sql_service_context::field_metadata(struct st_send_field *field,
                                        const CHARSET_INFO *charset
                                            MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::field_metadata");
  DBUG_PRINT("info", ("field->flags: %d", (int)field->flags));
  DBUG_PRINT("info", ("field->type: %d", (int)field->type));

  if (resultset) {
    Field_type ftype = {field->db_name,        field->table_name,
                        field->org_table_name, field->col_name,
                        field->org_col_name,   field->length,
                        field->charsetnr,      field->flags,
                        field->decimals,       field->type};
    resultset->set_metadata(ftype);
  }
  DBUG_RETURN(0);
}

int Sql_service_context::end_result_metadata(
    uint server_status MY_ATTRIBUTE((unused)),
    uint warn_count MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::end_result_metadata");
  DBUG_RETURN(0);
}

int Sql_service_context::start_row() {
  DBUG_ENTER("Sql_service_context::start_row");
  if (resultset) resultset->new_row();
  DBUG_RETURN(0);
}

int Sql_service_context::end_row() {
  DBUG_ENTER("Sql_service_context::end_row");
  if (resultset) resultset->increment_rows();
  DBUG_RETURN(0);
}

void Sql_service_context::abort_row() {
  DBUG_ENTER("Sql_service_context::abort_row");
  DBUG_VOID_RETURN;
}

ulong Sql_service_context::get_client_capabilities() {
  DBUG_ENTER("Sql_service_context::get_client_capabilities");
  DBUG_RETURN(0);
}

int Sql_service_context::get_null() {
  DBUG_ENTER("Sql_service_context::get_null");
  if (resultset) resultset->new_field(NULL);
  DBUG_RETURN(0);
}

int Sql_service_context::get_integer(longlong value) {
  DBUG_ENTER("Sql_service_context::get_integer");
  if (resultset) resultset->new_field(new Field_value(value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_longlong(longlong value, uint is_unsigned) {
  DBUG_ENTER("Sql_service_context::get_longlong");
  if (resultset) resultset->new_field(new Field_value(value, is_unsigned));
  DBUG_RETURN(0);
}

int Sql_service_context::get_decimal(const decimal_t *value) {
  DBUG_ENTER("Sql_service_context::get_decimal");
  if (resultset) resultset->new_field(new Field_value(*value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_double(double value,
                                    uint32 decimals MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::get_double");
  if (resultset) resultset->new_field(new Field_value(value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_date(const MYSQL_TIME *value) {
  DBUG_ENTER("Sql_service_context::get_date");
  if (resultset) resultset->new_field(new Field_value(*value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_time(const MYSQL_TIME *value,
                                  uint decimals MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::get_time");
  if (resultset) resultset->new_field(new Field_value(*value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_datetime(const MYSQL_TIME *value,
                                      uint decimals MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::get_datetime");
  if (resultset) resultset->new_field(new Field_value(*value));
  DBUG_RETURN(0);
}

int Sql_service_context::get_string(const char *const value, size_t length,
                                    const CHARSET_INFO *const valuecs
                                        MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::get_string");
  DBUG_PRINT("info", ("value: %s", value));
  if (resultset) resultset->new_field(new Field_value(value, length));
  DBUG_RETURN(0);
}

void Sql_service_context::handle_ok(uint server_status,
                                    uint statement_warn_count,
                                    ulonglong affected_rows,
                                    ulonglong last_insert_id,
                                    const char *const message) {
  DBUG_ENTER("Sql_service_context::handle_ok");

  if (resultset) {
    resultset->set_server_status(server_status);
    resultset->set_warn_count(statement_warn_count);
    resultset->set_affected_rows(affected_rows);
    resultset->set_last_insert_id(last_insert_id);
    resultset->set_message(message ? message : "");
  }
  DBUG_VOID_RETURN;
}

void Sql_service_context::handle_error(uint sql_errno,
                                       const char *const err_msg,
                                       const char *const sqlstate) {
  DBUG_ENTER("Sql_service_context::handle_error");
  DBUG_PRINT("info", ("sql_errno: %d", (int)sql_errno));
  DBUG_PRINT("info", ("err_msg: %s", err_msg));
  DBUG_PRINT("info", ("sqlstate: %s", sqlstate));

  if (resultset) {
    resultset->set_rows(0);
    resultset->set_sql_errno(sql_errno);
    resultset->set_err_msg(err_msg ? err_msg : "");
    resultset->set_sqlstate(sqlstate ? sqlstate : "");
  }
  DBUG_VOID_RETURN;
}

void Sql_service_context::shutdown(int flag MY_ATTRIBUTE((unused))) {
  DBUG_ENTER("Sql_service_context::shutdown");
  if (resultset) resultset->set_killed();
  DBUG_VOID_RETURN;
}

}  // namespace wsp
