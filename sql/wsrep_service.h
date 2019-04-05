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


#ifndef WSREP_SERVICE_H
#define WSREP_SERVICE_H

#include <mysql/plugin.h>
#include <string>
#include <vector>
#include "mysql/components/services/log_builtins.h"

namespace wsp {

typedef struct
{
  std::string db_name;
  std::string table_name;
  std::string org_table_name;
  std::string col_name;
  std::string org_col_name;
  unsigned long length;
  unsigned int charsetnr;
  unsigned int flags;
  unsigned int decimals;
  enum_field_types type;
} Field_type;

struct Field_value
{
  Field_value();
  Field_value(const Field_value& other);
  Field_value(const longlong &num, bool unsign = false);
  Field_value(const decimal_t &decimal);
  Field_value(const double num);
  Field_value(const MYSQL_TIME &time);
  Field_value(const char *str, size_t length);
  Field_value& operator=(const Field_value& other);
  ~Field_value();
  union
  {
    longlong v_long;
    double v_double;
    decimal_t v_decimal;
    MYSQL_TIME v_time;
    char *v_string;
  } value;
  size_t v_string_length;
  bool is_unsigned;
  bool has_ptr;

private:
  void copy_string(const char *str, size_t length);
};


class Sql_resultset
{
public:
  Sql_resultset() :
    current_row(0),
    num_cols(0),
    num_rows(0),
    num_metarow(0),
    m_resultcs(NULL),
    m_server_status(0),
    m_warn_count(0),
    m_affected_rows(0),
    m_last_insert_id(0),
    m_sql_errno(0),
    m_killed(false)
  {}

  ~Sql_resultset()
  {
    clear();
  }


  /* new row started for resultset */
  void new_row();

  /*
    add new field to existing row

    @param val Field_value to be stored in resulset
  */
  void new_field(Field_value *val);

  /* truncate and free resultset rows and field values */
  void clear();

  /* move row index to next row */
  bool next();

  /*
    move row index to particular row

    @param row  row position to set
  */
  void absolute(int row) { current_row= row; }

  /* move row index to first row */
  void first() { current_row= 0; }

  /* move row index to last row */
  void last() { current_row= num_rows > 0 ? num_rows - 1 : 0; }

  /* increment number of rows in resulset */
  void increment_rows() { ++num_rows; }


  /** Set Methods **/

  /*
    set number of rows in resulset

    @param rows  number of rows in resultset
  */
  void set_rows(uint rows) { num_rows= rows; }

  /*
    set number of cols in resulset

    @param rows  number of cols in resultset
  */
  void set_cols(uint cols) { num_cols= cols; }

  /**
    set resultset charset info

    @param result_cs   charset of resulset
  */
  void set_charset(const CHARSET_INFO *result_cs)
  {
    m_resultcs= result_cs;
  }

  /**
    set server status. check mysql_com for more details

    @param server_status   server status
  */
  void set_server_status(uint server_status)
  {
    m_server_status= server_status;
  }

  /**
    set count of warning issued during command execution

    @param warn_count  number of warning
  */
  void set_warn_count(uint warn_count)
  {
    m_warn_count= warn_count;
  }

  /**
    set rows affected due to last command execution

    @param affected_rows  number of rows affected due to last operation
  */
  void set_affected_rows(ulonglong affected_rows)
  {
    m_affected_rows= affected_rows;
  }

  /**
    set value of the AUTOINCREMENT column for the last INSERT

    @param last_insert_id   last inserted value in AUTOINCREMENT column
  */
  void set_last_insert_id(ulonglong last_insert_id)
  {
    m_last_insert_id= last_insert_id;
  }

  /**
    set client message

    @param msg  client message
  */
  void set_message(std::string msg)
  {
   m_message= msg;
  }

  /**
    set sql error number saved during error in
    last command execution

    @param sql_errno  sql error number
  */
  void set_sql_errno(uint sql_errno)
  {
    m_sql_errno= sql_errno;
  }

  /**
    set sql error message saved during error in
    last command execution

    @param msg  sql error message
  */
  void set_err_msg(std::string msg)
  {
    m_err_msg= msg;
  }

  /**
    set sql error state saved during error in
    last command execution

    @param state  sql error state
  */
  void set_sqlstate(std::string state)
  {
    m_sqlstate= state;
  }

  /* Session was shutdown while command was running */
  void set_killed()
  {
    m_killed= true; /* purecov: inspected */
  }


  /** Get Methods **/

  /*
    get number of rows in resulset

    @return number of rows in resultset
  */
  uint get_rows() { return num_rows; }

  /*
    get number of cols in resulset

    @return number of cols in resultset
  */
  uint get_cols() { return num_cols; }

  /**
    get resultset charset info

    @return charset info
  */
  const CHARSET_INFO * get_charset() { return m_resultcs; }

  /**
    get server status. check mysql_com for more details

    @return server status
  */
  uint get_server_status() { return m_server_status; }

  /**
    get count of warning issued during command execution

    @return warn_count
  */
  uint get_warn_count() { return m_warn_count; }

  /**
    return rows affected dure to last command execution

    @return affected_row
  */
  ulonglong get_affected_rows() { return m_affected_rows; }

  /**
    get value of the AUTOINCREMENT column for the last INSERT

    @return the sql error number
  */
  ulonglong get_last_insert_id() { return m_last_insert_id; }

  /**
    get client message

    @return message
  */
  std::string get_message() { return m_message; }


  /** Getting error info **/
  /**
    get sql error number saved during error in last command execution

    @return the sql error number
      @retval 0      OK
      @retval !=0    SQL Error Number
  */
  uint sql_errno() { return m_sql_errno; }


  /**
    get sql error message saved during error in last command execution

    @return the sql error message
  */
  std::string err_msg() { return m_err_msg; /* purecov: inspected */ }

  /**
    get sql error state saved during error in last command execution

    @return the sql error state
  */
  std::string sqlstate() { return m_sqlstate; }


  /* get long field type column */
  longlong getLong(uint columnIndex)
  {
    return result_value[current_row][columnIndex]->value.v_long;
  }

  /* get decimal field type column */
  decimal_t getDecimal(uint columnIndex)
  {
    return result_value[current_row][columnIndex]->value.v_decimal;
  }

  /* get double field type column */
  double getDouble(uint columnIndex)
  {
    return result_value[current_row][columnIndex]->value.v_double;
  }

  /* get time field type column */
  MYSQL_TIME getTime(uint columnIndex)
  {
    return result_value[current_row][columnIndex]->value.v_time;
  }

  /* get string field type column */
  char *getString(uint columnIndex)
  {
    if (result_value[current_row][columnIndex] != NULL)
      return result_value[current_row][columnIndex]->value.v_string;
    return const_cast<char*>("");
  }

  /* resultset metadata functions */

  /* set metadata info */
  void set_metadata(Field_type ftype)
  {
    result_meta.push_back(ftype);
    ++num_metarow;
  }

  /* get database */
  std::string get_database(uint rowIndex= 0)
  {
    return result_meta[rowIndex].db_name;
  }

  /* get table alias */
  std::string get_table(uint rowIndex= 0)
  {
    return result_meta[rowIndex].table_name;
  }

  /* get original table */
  std::string get_org_table(uint rowIndex= 0)
  {
    return result_meta[rowIndex].org_table_name;
  }

  /* get column name alias */
  std::string get_column_name(uint rowIndex= 0)
  {
    return result_meta[rowIndex].col_name;
  }

  /* get original column name */
  std::string get_org_column_name(uint rowIndex= 0)
  {
    return result_meta[rowIndex].org_col_name;
  }

  /* get field width */
  unsigned long get_length(uint rowIndex= 0)
  {
    return result_meta[rowIndex].length;
  }

  /* get field charsetnr */
  unsigned int get_charsetnr(uint rowIndex= 0)
  {
    return result_meta[rowIndex].charsetnr;
  }

  /*
    get field flag.
    Check
      https://dev.mysql.com/doc/refman/5.7/en/c-api-data-structures.html
    for all flags
  */
  unsigned int get_flags(uint rowIndex= 0)
  {
    return result_meta[rowIndex].flags;
  }

  /* get the number of decimals for numeric fields */
  unsigned int get_decimals(uint rowIndex= 0)
  {
    return result_meta[rowIndex].decimals;
  }

  /* get field type. Check enum enum_field_types for whole list */
  enum_field_types get_field_type(uint rowIndex= 0)
  {
    return result_meta[rowIndex].type;
  }

  /*
    get status whether session was shutdown while command was running

    @return session shutdown status
      @retval true   session was stopped
      @retval false  session was not stopped
  */
  bool get_killed_status()
  {
    return m_killed;
  }

private:
  /* resultset store */
  std::vector< std::vector< Field_value* > > result_value;
  /* metadata store */
  std::vector< Field_type > result_meta;

  int current_row; /* current row position */
  uint num_cols; /* number of columns in resultset/metadata */
  uint num_rows; /* number of rows in resultset */
  uint num_metarow; /* number of rows in metadata */

  const CHARSET_INFO *m_resultcs; /* result charset */

  uint m_server_status; /* server status */
  uint m_warn_count;    /* warning count */

  /* rows affected mostly useful for command like update */
  ulonglong m_affected_rows;
  ulonglong m_last_insert_id; /* last auto-increment column value */
  std::string m_message; /* client message */

  uint m_sql_errno;  /* sql error number */
  std::string m_err_msg; /* sql error message */
  std::string m_sqlstate; /* sql error state */

  bool m_killed; /* session killed status */
};


class Sql_service_context_base
{
public:

  /** The sql service callbacks that will call the below virtual methods*/
  static const st_command_service_cbs sql_service_callbacks;

  /**
    Sql_service_context_base constructor
    resets all variables
  */
  Sql_service_context_base() {}

  virtual ~Sql_service_context_base() {}

  /** Getting metadata **/
  /**
    Indicates start of metadata for the result set

    @param num_cols Number of fields being sent
    @param flags    Flags to alter the metadata sending
    @param resultcs Charset of the result set

    @return
      @retval 1  Error
      @retval 0  OK
  */
  virtual int start_result_metadata(uint num_cols, uint flags,
                                    const CHARSET_INFO *resultcs) = 0;

  /**
    Field metadata is provided via this callback

    @param field   Field's metadata (see field.h)
    @param charset Field's charset

    @return
      @retval 1  Error
      @retval 0  OK
  */
  virtual int field_metadata(struct st_send_field *field,
                             const CHARSET_INFO *charset) = 0;

  /**
    Indicates end of metadata for the result set

    @param server_status   Status of server (see mysql_com.h SERVER_STATUS_*)
    @param warn_count      Number of warnings thrown during execution

    @return
      @retval 1  Error
      @retval 0  OK
  */
  virtual int end_result_metadata(uint server_status,
                                  uint warn_count) = 0;

  /**
    Indicates the beginning of a new row in the result set/metadata

    @return
      @retval 1  Error
      @retval 0  OK
  */
  virtual int start_row() = 0;

  /**
    Indicates end of the row in the result set/metadata

    @return
      @retval 1  Error
      @retval 0  OK
  */
  virtual int end_row() = 0;

  /**
    An error occured during execution

    @details This callback indicates that an error occureded during command
    execution and the partial row should be dropped. Server will raise error
    and return.
  */
  virtual void abort_row() = 0;


  /**
    Return client's capabilities (see mysql_com.h, CLIENT_*)

    @return Bitmap of client's capabilities
  */
  virtual ulong get_client_capabilities() = 0;

  /** Getting data **/
  /**
    Receive NULL value from server

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_null() = 0;


  /**
    Get TINY/SHORT/LONG value from server

    @param value         Value received

    @note In order to know which type exactly was received, the plugin must
    track the metadata that was sent just prior to the result set.

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_integer(longlong value) = 0;

  /**
    Get LONGLONG value from server

    @param value         Value received
    @param is_unsigned   TRUE <=> value is unsigned

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_longlong(longlong value, uint is_unsigned) = 0;

  /**
    Receive DECIMAL value from server

    @param value Value received

    @return status
      @retval 1  Error
      @retval 0  OK
   */
  virtual int get_decimal(const decimal_t * value) = 0;

  /**

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_double(double value, uint32 decimals) = 0;

  /**
    Get DATE value from server

    @param value    Value received

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_date(const MYSQL_TIME * value) = 0;

  /**
    Get TIME value from server

    @param value    Value received
    @param decimals Number of decimals

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_time(const MYSQL_TIME * value, uint decimals) = 0;

  /**
    Get DATETIME value from server

    @param value    Value received
    @param decimals Number of decimals

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_datetime(const MYSQL_TIME * value,
                           uint decimals) = 0;

  /**
    Get STRING value from server

    @param value   Value received
    @param length  Value's length
    @param valuecs Value's charset

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  virtual int get_string(const char * const value,
                         size_t length, const CHARSET_INFO * const valuecs) = 0;


  /** Getting execution status **/
  /**
    Command ended with success

    @param server_status        Status of server (see mysql_com.h,
                                SERVER_STATUS_*)
    @param statement_warn_count Number of warnings thrown during execution
    @param affected_rows        Number of rows affected by the command
    @param last_insert_id       Last insert id being assigned during execution
    @param message              A message from server
  */
  virtual void handle_ok(uint server_status, uint statement_warn_count,
                         ulonglong affected_rows, ulonglong last_insert_id,
                         const char * const message) = 0;


  /**
    Command ended with ERROR

    @param sql_errno Error code
    @param err_msg   Error message
    @param sqlstate  SQL state corresponding to the error code
  */
  virtual void handle_error(uint sql_errno,
                            const char * const err_msg,
                            const char * const sqlstate) = 0;

  /**
   Session was shutdown while command was running
  */
  virtual void shutdown(int flag) = 0;

private:
  static int sql_start_result_metadata(void *ctx, uint num_cols, uint flags,
                                       const CHARSET_INFO *resultcs)
  {
    return ((Sql_service_context_base *) ctx)->start_result_metadata(num_cols,
                                                                 flags,
                                                                 resultcs);
  }

  static int sql_field_metadata(void *ctx, struct st_send_field *field,
                                const CHARSET_INFO *charset)
  {
    return ((Sql_service_context_base *) ctx)->field_metadata(field,
                                                          charset);
  }

  static int sql_end_result_metadata(void *ctx, uint server_status,
                                     uint warn_count)
  {
    return ((Sql_service_context_base *) ctx)->end_result_metadata(server_status,
                                                               warn_count);
  }

  static int sql_start_row(void *ctx)
  {
    return ((Sql_service_context_base *) ctx)->start_row();
  }

  static int sql_end_row(void *ctx)
  {
    return ((Sql_service_context_base *) ctx)->end_row();
  }

  static void sql_abort_row(void *ctx)
  {
    return ((Sql_service_context_base *) ctx)->abort_row(); /* purecov: inspected */
  }

  static ulong sql_get_client_capabilities(void *ctx)
  {
    return ((Sql_service_context_base *) ctx)->get_client_capabilities();
  }

  static int sql_get_null(void *ctx)
  {
    return ((Sql_service_context_base *) ctx)->get_null();
  }

  static int sql_get_integer(void * ctx, longlong value)
  {
    return ((Sql_service_context_base *) ctx)->get_integer(value);
  }

  static int sql_get_longlong(void * ctx, longlong value, uint is_unsigned)
  {
    return ((Sql_service_context_base *) ctx)->get_longlong(value, is_unsigned);
  }

  static int sql_get_decimal(void * ctx, const decimal_t * value)
  {
    return ((Sql_service_context_base *) ctx)->get_decimal(value);
  }

  static int sql_get_double(void * ctx, double value, uint32 decimals)
  {
    return ((Sql_service_context_base *) ctx)->get_double(value, decimals);
  }

  static int sql_get_date(void * ctx, const MYSQL_TIME * value)
  {
    return ((Sql_service_context_base *) ctx)->get_date(value);
  }

  static int sql_get_time(void * ctx, const MYSQL_TIME * value, uint decimals)
  {
    return ((Sql_service_context_base *) ctx)->get_time(value, decimals);
  }

  static int sql_get_datetime(void * ctx, const MYSQL_TIME * value,
                              uint decimals)
  {
    return ((Sql_service_context_base *) ctx)->get_datetime(value, decimals);
  }

  static int sql_get_string(void * ctx, const char * const value,
                            size_t length, const CHARSET_INFO * const valuecs)
  {
    return ((Sql_service_context_base *) ctx)->get_string(value, length, valuecs);
  }

  static void sql_handle_ok(void * ctx,
                            uint server_status, uint statement_warn_count,
                            ulonglong affected_rows, ulonglong last_insert_id,
                            const char * const message)
  {
    return ((Sql_service_context_base *) ctx)->handle_ok(server_status,
                                                     statement_warn_count,
                                                     affected_rows,
                                                     last_insert_id,
                                                     message);
  }


  static void sql_handle_error(void * ctx, uint sql_errno,
                               const char * const err_msg,
                               const char * const sqlstate)
  {
    return ((Sql_service_context_base *) ctx)->handle_error(sql_errno,
                                                        err_msg,
                                                        sqlstate);
  }


  static void sql_shutdown(void *ctx, int flag)
  {
    return ((Sql_service_context_base *) ctx)->shutdown(flag);
  }
};


class Sql_service_context : public Sql_service_context_base
{
public:
  Sql_service_context(wsp::Sql_resultset *rset)
    :resultset(rset)
  {
    if (rset != NULL)
      resultset->clear();
  }

  ~Sql_service_context() {}


  /** Getting metadata **/
  /**
    Indicates start of metadata for the result set

    @param num_cols Number of fields being sent
    @param flags    Flags to alter the metadata sending
    @param resultcs Charset of the result set

    @return
      @retval 1  Error
      @retval 0  OK
  */
  int start_result_metadata(uint num_cols, uint flags,
                            const CHARSET_INFO *resultcs);

  /**
    Field metadata is provided via this callback

    @param field   Field's metadata (see field.h)
    @param charset Field's charset

    @return
      @retval 1  Error
      @retval 0  OK
  */
  int field_metadata(struct st_send_field *field,
                     const CHARSET_INFO *charset);

  /**
    Indicates end of metadata for the result set

    @param server_status   Status of server (see mysql_com.h SERVER_STATUS_*)
    @param warn_count      Number of warnings thrown during execution

    @return
      @retval 1  Error
      @retval 0  OK
  */
  int end_result_metadata(uint server_status,
                          uint warn_count);

  /**
    Indicates the beginning of a new row in the result set/metadata

    @return
      @retval 1  Error
      @retval 0  OK
  */
  int start_row();

  /**
    Indicates end of the row in the result set/metadata

    @return
      @retval 1  Error
      @retval 0  OK
  */
  int end_row();

  /**
    An error occured during execution

    @details This callback indicates that an error occureded during command
    execution and the partial row should be dropped. Server will raise error
    and return.
  */
  void abort_row();

  /**
    Return client's capabilities (see mysql_com.h, CLIENT_*)

    @return Bitmap of client's capabilities
  */
  ulong get_client_capabilities();

  /** Getting data **/
  /**
    Receive NULL value from server

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_null();

  /**
    Get TINY/SHORT/LONG value from server

    @param value         Value received

    @note In order to know which type exactly was received, the plugin must
    track the metadata that was sent just prior to the result set.

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_integer(longlong value);

  /**
    Get LONGLONG value from server

    @param value         Value received
    @param is_unsigned   TRUE <=> value is unsigned

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_longlong(longlong value, uint is_unsigned);

  /**
    Receive DECIMAL value from server

    @param value Value received

    @return status
      @retval 1  Error
      @retval 0  OK
   */
  int get_decimal(const decimal_t * value);

  /**
    Receive DOUBLE value from server

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_double(double value, uint32 decimals);

  /**
    Get DATE value from server

    @param value    Value received

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_date(const MYSQL_TIME * value);

  /**
    Get TIME value from server

    @param value    Value received
    @param decimals Number of decimals

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_time(const MYSQL_TIME * value, uint decimals);

  /**
    Get DATETIME value from server

    @param value    Value received
    @param decimals Number of decimals

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_datetime(const MYSQL_TIME * value,
                   uint decimals);

  /**
    Get STRING value from server

    @param value   Value received
    @param length  Value's length
    @param valuecs Value's charset

    @return status
      @retval 1  Error
      @retval 0  OK
  */
  int get_string(const char * const value,
                 size_t length, const CHARSET_INFO * const valuecs);

  /** Getting execution status **/
  /**
    Command ended with success

    @param server_status        Status of server (see mysql_com.h,
                                SERVER_STATUS_*)
    @param statement_warn_count Number of warnings thrown during execution
    @param affected_rows        Number of rows affected by the command
    @param last_insert_id       Last insert id being assigned during execution
    @param message              A message from server
  */
  void handle_ok(uint server_status, uint statement_warn_count,
                 ulonglong affected_rows, ulonglong last_insert_id,
                 const char * const message);

  /**
    Command ended with ERROR

    @param sql_errno Error code
    @param err_msg   Error message
    @param sqlstate  SQL state correspongin to the error code
  */
  void handle_error(uint sql_errno,
                    const char * const err_msg,
                    const char * const sqlstate);

  /**
    Session was shutdown while command was running
  */
  void shutdown(int flag);

private:
  /* executed command result store */
  wsp::Sql_resultset *resultset;
};



} // namespace wsp

#endif //WSREP_SERVICE_H

