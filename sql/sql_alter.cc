/* Copyright (c) 2010, 2022, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "sql/sql_alter.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "m_ctype.h"
#include "m_string.h"
#include "my_dbug.h"
#include "my_inttypes.h"
#include "my_macros.h"
#include "my_sys.h"
#include "mysql/plugin.h"
#include "mysqld_error.h"
#include "sql/auth/auth_acls.h"
#include "sql/auth/auth_common.h"  // check_access
#include "sql/create_field.h"
#include "sql/dd/types/trigger.h"  // dd::Trigger
#include "sql/derror.h"            // ER_THD
#include "sql/error_handler.h"     // Strict_error_handler
// mysql_exchange_partition
#include "sql/log.h"
#include "sql/mysqld.h"              // lower_case_table_names
#include "sql/parse_tree_helpers.h"  // is_identifier
#include "sql/sql_class.h"           // THD
#include "sql/sql_error.h"
#include "sql/sql_lex.h"
#include "sql/sql_servers.h"
#include "sql/sql_table.h"  // mysql_alter_table,
#include "sql/table.h"
#include "template_utils.h"  // delete_container_pointers

#ifdef WITH_WSREP
#include "mysql/components/services/log_builtins.h"
#include "sql/dd/cache/dictionary_client.h"  // dd::cache::Dictionary_client
#include "sql/dd/dd_table.h"                 // dd::table_storage_engine
#include "sql/dd/types/table.h"              // dd::Table
#include "sql/sql_parse.h"                   // WSREP_TO_ISOLATION_BEGIN_ALTER
#include "sql_parse.h"
#include "wsrep_mysqld.h"
#endif /* WITH_WSREP */

bool has_external_data_or_index_dir(partition_info &pi);

Alter_info::Alter_info(const Alter_info &rhs, MEM_ROOT *mem_root)
    : drop_list(mem_root, rhs.drop_list.begin(), rhs.drop_list.end()),
      alter_list(mem_root, rhs.alter_list.begin(), rhs.alter_list.end()),
      key_list(mem_root, rhs.key_list.begin(), rhs.key_list.end()),
      alter_rename_key_list(mem_root, rhs.alter_rename_key_list.begin(),
                            rhs.alter_rename_key_list.end()),
      alter_index_visibility_list(mem_root,
                                  rhs.alter_index_visibility_list.begin(),
                                  rhs.alter_index_visibility_list.end()),
      alter_constraint_enforcement_list(
          mem_root, rhs.alter_constraint_enforcement_list.begin(),
          rhs.alter_constraint_enforcement_list.end()),
      check_constraint_spec_list(mem_root,
                                 rhs.check_constraint_spec_list.begin(),
                                 rhs.check_constraint_spec_list.end()),
      create_list(rhs.create_list, mem_root),
      delayed_key_list(mem_root, rhs.delayed_key_list.cbegin(),
                       rhs.delayed_key_list.cend()),
      delayed_key_info(rhs.delayed_key_info),
      delayed_key_count(rhs.delayed_key_count),
      flags(rhs.flags),
      keys_onoff(rhs.keys_onoff),
      partition_names(rhs.partition_names, mem_root),
      num_parts(rhs.num_parts),
      requested_algorithm(rhs.requested_algorithm),
      requested_lock(rhs.requested_lock),
      with_validation(rhs.with_validation),
      new_db_name(rhs.new_db_name),
      new_table_name(rhs.new_table_name) {
  /*
    Make deep copies of used objects.
    This is not a fully deep copy - clone() implementations
    of Create_list do not copy string constants. At the same length the only
    reason we make a copy currently is that ALTER/CREATE TABLE
    code changes input Alter_info definitions, but string
    constants never change.
  */
  List_iterator<Create_field> it(create_list);
  Create_field *el;
  while ((el = it++)) it.replace(el->clone(mem_root));

  /* partition_names are not deeply copied currently */
}

/**
  Checks if there are any columns with COLUMN_FORMAT COMRPESSED
  attribute among field definitions in create_list.

  @retval false there are no compressed columns
  @retval true there is at least one compressed column
*/
bool Alter_info::has_compressed_columns() const {
  List_iterator<Create_field> it(const_cast<List<Create_field> &>(create_list));
  const Create_field *sql_field;
  while ((sql_field = it++))
    if (sql_field->column_format() == COLUMN_FORMAT_TYPE_COMPRESSED)
      return true;
  return false;
}

Alter_table_ctx::Alter_table_ctx()
    : datetime_field(nullptr),
      error_if_not_empty(false),
      tables_opened(0),
      db(nullptr),
      table_name(nullptr),
      alias(nullptr),
      new_db(nullptr),
      new_name(nullptr),
      new_alias(nullptr),
      fk_info(nullptr),
      fk_count(0),
      fk_max_generated_name_number(0)
#ifndef NDEBUG
      ,
      tmp_table(false)
#endif
{
}

Alter_table_ctx::Alter_table_ctx(THD *thd, TABLE_LIST *table_list,
                                 uint tables_opened_arg, const char *new_db_arg,
                                 const char *new_name_arg)
    : datetime_field(nullptr),
      error_if_not_empty(false),
      tables_opened(tables_opened_arg),
      new_db(new_db_arg),
      new_name(new_name_arg),
      fk_info(nullptr),
      fk_count(0),
      fk_max_generated_name_number(0)
#ifndef NDEBUG
      ,
      tmp_table(false)
#endif
{
  /*
    Assign members db, table_name, new_db and new_name
    to simplify further comparisons: we want to see if it's a RENAME
    later just by comparing the pointers, avoiding the need for strcmp.
  */
  db = table_list->db;
  table_name = table_list->table_name;
  alias = (lower_case_table_names == 2) ? table_list->alias : table_name;

  if (!new_db || !my_strcasecmp(table_alias_charset, new_db, db)) new_db = db;

  if (new_name) {
    DBUG_PRINT("info", ("new_db.new_name: '%s'.'%s'", new_db, new_name));

    if (lower_case_table_names ==
        1)  // Convert new_name/new_alias to lower case
    {
      my_casedn_str(files_charset_info, const_cast<char *>(new_name));
      new_alias = new_name;
    } else if (lower_case_table_names == 2)  // Convert new_name to lower case
    {
      my_stpcpy(new_alias_buff, new_name);
      new_alias = (const char *)new_alias_buff;
      my_casedn_str(files_charset_info, const_cast<char *>(new_name));
    } else
      new_alias = new_name;  // LCTN=0 => case sensitive + case preserving

    if (!is_database_changed() &&
        !my_strcasecmp(table_alias_charset, new_name, table_name)) {
      /*
        Source and destination table names are equal:
        make is_table_renamed() more efficient.
      */
      new_alias = table_name;
      new_name = table_name;
    }
  } else {
    new_alias = alias;
    new_name = table_name;
  }

  snprintf(tmp_name, sizeof(tmp_name), "%s-%lx_%x", tmp_file_prefix,
           current_pid, thd->thread_id());
  /* Safety fix for InnoDB */
  if (lower_case_table_names) my_casedn_str(files_charset_info, tmp_name);

  if (table_list->table->s->tmp_table == NO_TMP_TABLE) {
    build_table_filename(path, sizeof(path) - 1, db, table_name, "", 0);

    build_table_filename(new_path, sizeof(new_path) - 1, new_db, new_name, "",
                         0);

    build_table_filename(new_filename, sizeof(new_filename) - 1, new_db,
                         new_name, reg_ext, 0);

    build_table_filename(tmp_path, sizeof(tmp_path) - 1, new_db, tmp_name, "",
                         FN_IS_TMP);
  } else {
    /*
      We are not filling path, new_path and new_filename members if
      we are altering temporary table as these members are not used in
      this case. This fact is enforced with assert.
    */
    build_tmptable_filename(thd, tmp_path, sizeof(tmp_path));
#ifndef NDEBUG
    tmp_table = true;
#endif
  }

  /* Initialize MDL requests on new table name and database if necessary. */
  if (table_list->table->s->tmp_table == NO_TMP_TABLE) {
    if (is_table_renamed()) {
      MDL_REQUEST_INIT(&target_mdl_request, MDL_key::TABLE, new_db, new_name,
                       MDL_EXCLUSIVE, MDL_TRANSACTION);

      if (is_database_changed()) {
        MDL_REQUEST_INIT(&target_db_mdl_request, MDL_key::SCHEMA, new_db, "",
                         MDL_INTENTION_EXCLUSIVE, MDL_TRANSACTION);
      }
    }
  }
}

Alter_table_ctx::~Alter_table_ctx() = default;

bool Sql_cmd_alter_table::execute(THD *thd) {
  /* Verify that none one of the DISCARD and IMPORT flags are set. */
  assert(!thd_tablespace_op(thd));
  DBUG_EXECUTE_IF("delay_alter_table_by_one_second", { my_sleep(1000000); });

  LEX *lex = thd->lex;
  /* first Query_block (have special meaning for many of non-SELECTcommands) */
  Query_block *query_block = lex->query_block;
  /* first table of first Query_block */
  TABLE_LIST *first_table = query_block->get_table_list();
  /*
    Code in mysql_alter_table() may modify its HA_CREATE_INFO argument,
    so we have to use a copy of this structure to make execution
    prepared statement- safe. A shallow copy is enough as no memory
    referenced from this structure will be modified.
    @todo move these into constructor...
  */
  HA_CREATE_INFO create_info(*lex->create_info);
  Alter_info alter_info(*m_alter_info, thd->mem_root);
  ulong priv = 0;
  ulong priv_needed = ALTER_ACL;
  bool result;

  DBUG_TRACE;

  if (thd->is_fatal_error()) /* out of memory creating a copy of alter_info */
    return true;

  {
    partition_info *part_info = thd->lex->part_info;
    if (part_info != nullptr && has_external_data_or_index_dir(*part_info) &&
        check_access(thd, FILE_ACL, any_db, nullptr, nullptr, false, false))

      return true;
  }
  /*
    We also require DROP priv for ALTER TABLE ... DROP PARTITION, as well
    as for RENAME TO, as being done by SQLCOM_RENAME_TABLE
  */
  if (alter_info.flags &
      (Alter_info::ALTER_DROP_PARTITION | Alter_info::ALTER_RENAME))
    priv_needed |= DROP_ACL;

  /* Must be set in the parser */
  assert(alter_info.new_db_name.str);
  assert(!(alter_info.flags & Alter_info::ALTER_EXCHANGE_PARTITION));
  assert(!(alter_info.flags & Alter_info::ALTER_ADMIN_PARTITION));
  if (check_access(thd, priv_needed, first_table->db,
                   &first_table->grant.privilege,
                   &first_table->grant.m_internal, false, false) ||
      check_access(thd, INSERT_ACL | CREATE_ACL, alter_info.new_db_name.str,
                   &priv,
                   nullptr, /* Don't use first_tab->grant with sel_lex->db */
                   false, false))
    return true; /* purecov: inspected */

  /* If it is a merge table, check privileges for merge children. */
  if (create_info.merge_list.first) {
    /*
      The user must have (SELECT_ACL | UPDATE_ACL | DELETE_ACL) on the
      underlying base tables, even if there are temporary tables with the same
      names.

      From user's point of view, it might look as if the user must have these
      privileges on temporary tables to create a merge table over them. This is
      one of two cases when a set of privileges is required for operations on
      temporary tables (see also CREATE TABLE).

      The reason for this behavior stems from the following facts:

        - For merge tables, the underlying table privileges are checked only
          at CREATE TABLE / ALTER TABLE time.

          In other words, once a merge table is created, the privileges of
          the underlying tables can be revoked, but the user will still have
          access to the merge table (provided that the user has privileges on
          the merge table itself).

        - Temporary tables shadow base tables.

          I.e. there might be temporary and base tables with the same name, and
          the temporary table takes the precedence in all operations.

        - For temporary MERGE tables we do not track if their child tables are
          base or temporary. As result we can't guarantee that privilege check
          which was done in presence of temporary child will stay relevant later
          as this temporary table might be removed.

      If SELECT_ACL | UPDATE_ACL | DELETE_ACL privileges were not checked for
      the underlying *base* tables, it would create a security breach as in
      Bug#12771903.
    */

    if (check_table_access(thd, SELECT_ACL | UPDATE_ACL | DELETE_ACL,
                           create_info.merge_list.first, false, UINT_MAX,
                           false))
      return true;
  }

  if (check_grant(thd, priv_needed, first_table, false, UINT_MAX, false))
    return true; /* purecov: inspected */

  if (alter_info.new_table_name.str &&
      !test_all_bits(priv, INSERT_ACL | CREATE_ACL)) {
    // Rename of table
    assert(alter_info.flags & Alter_info::ALTER_RENAME);
    TABLE_LIST tmp_table;
    tmp_table.table_name = alter_info.new_table_name.str;
    tmp_table.db = alter_info.new_db_name.str;
    tmp_table.grant.privilege = priv;
    if (check_grant(thd, INSERT_ACL | CREATE_ACL, &tmp_table, false, UINT_MAX,
                    false))
      return true; /* purecov: inspected */
  }

#ifdef WITH_WSREP
  /* Check if foreign keys are accessible.
  1. Transaction is replicated first, then is executed locally.
  2. Replicated node applies write sets in context
  of root user.
  Above two conditions may cause that even if we have no access to FKs,
  transactions will replicate with success, but fail locally.

  Here we check only for FK access, not if the engine supports FKs.
  That's enough, because right now we need only to know if transaction
  should be rolled back because of lack of access and not replicated,
  or we can replicate it and let replicated node do the rest of the job. */
  if (alter_info.flags & Alter_info::ADD_FOREIGN_KEY) {
    if (check_fk_parent_table_access(thd, &create_info, &alter_info, false)) {
      return true;
    }
  }

  TABLE *find_temporary_table(THD * thd, const TABLE_LIST *tl);
  if (WSREP(thd) && WSREP_CLIENT(thd) &&
      (!thd->is_current_stmt_binlog_format_row() ||
       !find_temporary_table(thd, first_table))) {
    wsrep::key_array keys;
    // append tables referenced by this table
    // append tables that are referencing this table
    if (wsrep_append_fk_parent_table(thd, first_table, &keys) ||
        wsrep_append_child_tables(thd, first_table, &keys)) {
      WSREP_DEBUG("TOI replication for ALTER failed");
      return true;
    }

    WSREP_TO_ISOLATION_BEGIN_ALTER(first_table->db, first_table->table_name,
                                   first_table, &alter_info, &keys) {
      WSREP_DEBUG("TOI replication for ALTER failed");
      return true;
    }
  }
#endif /* WITH_WSREP */

  /* Don't yet allow changing of symlinks with ALTER TABLE */
  if (create_info.data_file_name)
    push_warning_printf(thd, Sql_condition::SL_WARNING, WARN_OPTION_IGNORED,
                        ER_THD(thd, WARN_OPTION_IGNORED), "DATA DIRECTORY");
  if (create_info.index_file_name)
    push_warning_printf(thd, Sql_condition::SL_WARNING, WARN_OPTION_IGNORED,
                        ER_THD(thd, WARN_OPTION_IGNORED), "INDEX DIRECTORY");
  create_info.data_file_name = create_info.index_file_name = nullptr;

  thd->set_slow_log_for_admin_command();

#ifdef WITH_WSREP
  /* PXC doesn't recommend/allow ALTER operation on table created using
  non-transactional storage engine (like MyISAM, HEAP/MEMORY, etc....)
  except ALTER operation to change storage engine to transactional storage
  engine. (of-course if that is feasible with other constraints)
  Application only for non-temporary (persistent) tables.
  Temporarily tables are generally created by some internal workload
  so we continue to allow them for now as avoid breaking the application.
  Note: Temporary table are never replicated. */

  if (WSREP_ON && !is_temporary_table(first_table)) {
    enum legacy_db_type existing_db_type, new_db_type;

    TABLE_LIST *table = first_table;

    // mdl_lock scope begin
    {
      // Acquire lock on the table as it is needed to get the instance from DD
      MDL_request mdl_request;
      MDL_REQUEST_INIT(&mdl_request, MDL_key::TABLE, table->db,
                       table->table_name, MDL_SHARED, MDL_EXPLICIT);
      wsrep_scope_guard mdl_lock(
          [thd, &mdl_request]() {
            thd->mdl_context.acquire_lock(&mdl_request,
                                          thd->variables.lock_wait_timeout);
          },
          [thd, &mdl_request]() {
            thd->mdl_context.release_lock(mdl_request.ticket);
          });

      const char *schema_name = table->db;
      const char *table_name = table->table_name;
      dd::cache::Dictionary_client::Auto_releaser releaser(thd->dd_client());
      const dd::Table *table_ref = NULL;

      if (thd->dd_client()->acquire(schema_name, table_name, &table_ref)) {
        return true;
      }

      if (table_ref == nullptr ||
          table_ref->hidden() == dd::Abstract_table::HT_HIDDEN_SE) {
        my_error(ER_NO_SUCH_TABLE, MYF(0), schema_name, table_name);
        return true;
      }

      handlerton *hton = nullptr;
      if (dd::table_storage_engine(thd, table_ref, &hton)) {
        return true;
      }

      existing_db_type = hton->db_type;
    }  // mdl_lock scope end

    bool is_system_db =
        (first_table && ((strcmp(first_table->db, "mysql") == 0) ||
                         (strcmp(first_table->db, "information_schema") == 0)));

    if (!is_temporary_table(first_table) && !is_system_db) {
      bool safe_ops = false;

      /* If create_info.db_type = NULL that means ALTER operation
      is modifying existing table. */
      new_db_type =
          ((create_info.db_type != NULL) ? create_info.db_type->db_type
                                         : existing_db_type);

      /* Existing table is created with non-transactional storage engine
      and so switching it to transactional storage engine is allowed.
      Currently transactional storage engine supported is InnoDB/XtraDB */
      if ((existing_db_type != DB_TYPE_INNODB) &&
          (alter_info.flags & Alter_info::ALTER_OPTIONS) &&
          (create_info.used_fields & HA_CREATE_USED_ENGINE) &&
          (new_db_type == DB_TYPE_INNODB || existing_db_type == new_db_type))
        safe_ops = true;

      /* Existing table is created with transactional storage engine
      and switching it to non-transactional storage engine is blocked
      other operations are allowed. */
      if ((existing_db_type == DB_TYPE_INNODB) &&
          (alter_info.flags & Alter_info::ALTER_OPTIONS) &&
          (create_info.used_fields & HA_CREATE_USED_ENGINE) &&
          (new_db_type != DB_TYPE_INNODB))
        safe_ops = false;
      else if (existing_db_type == DB_TYPE_INNODB ||
               existing_db_type == DB_TYPE_PERFORMANCE_SCHEMA)
        safe_ops = true;

      if (!safe_ops && existing_db_type != DB_TYPE_INNODB &&
          existing_db_type != DB_TYPE_UNKNOWN) {
        bool block = false;

        switch (pxc_strict_mode) {
          case PXC_STRICT_MODE_DISABLED:
            break;
          case PXC_STRICT_MODE_PERMISSIVE:
            WSREP_WARN(
                "Percona-XtraDB-Cluster doesn't recommend use of"
                " ALTER command on a table (%s.%s) that resides in"
                " non-transactional storage engine"
                " (except switching to transactional engine)"
                " with pxc_strict_mode = PERMISSIVE",
                first_table->db, first_table->table_name);
            push_warning_printf(
                thd, Sql_condition::SL_WARNING, ER_UNKNOWN_ERROR,
                "Percona-XtraDB-Cluster doesn't recommend use of"
                " ALTER command on a table (%s.%s) that resides in"
                " non-transactional storage engine"
                " (except switching to transactional engine)"
                " with pxc_strict_mode = PERMISSIVE",
                first_table->db, first_table->table_name);
            break;
          case PXC_STRICT_MODE_ENFORCING:
          case PXC_STRICT_MODE_MASTER:
          default:
            block = true;
            WSREP_ERROR(
                "Percona-XtraDB-Cluster prohibits use of"
                " ALTER command on a table (%s.%s) that resides in"
                " non-transactional storage engine"
                " (except switching to transactional engine)"
                " with pxc_strict_mode = ENFORCING or MASTER",
                first_table->db, first_table->table_name);
            char message[1024];
            sprintf(message,
                    "Percona-XtraDB-Cluster prohibits use of"
                    " ALTER command on a table (%s.%s) that resides in"
                    " non-transactional storage engine"
                    " (except switching to transactional engine)"
                    " with pxc_strict_mode = ENFORCING or MASTER",
                    first_table->db, first_table->table_name);
            my_message(ER_UNKNOWN_ERROR, message, MYF(0));
            break;
        }

        if (block) return true;
      } else if (!safe_ops && existing_db_type == DB_TYPE_INNODB) {
        bool block = false;

        switch (pxc_strict_mode) {
          case PXC_STRICT_MODE_DISABLED:
            break;
          case PXC_STRICT_MODE_PERMISSIVE:
            WSREP_WARN(
                "Percona-XtraDB-Cluster doesn't recommend changing"
                " storage engine of a table (%s.%s) from"
                " transactional to non-transactional"
                " with pxc_strict_mode = PERMISSIVE",
                first_table->db, first_table->table_name);
            push_warning_printf(
                thd, Sql_condition::SL_WARNING, ER_UNKNOWN_ERROR,
                "Percona-XtraDB-Cluster doesn't recommend changing"
                " storage engine of a table (%s.%s) from"
                " transactional to non-transactional"
                " with pxc_strict_mode = PERMISSIVE",
                first_table->db, first_table->table_name);
            break;
          case PXC_STRICT_MODE_ENFORCING:
          case PXC_STRICT_MODE_MASTER:
          default:
            block = true;
            WSREP_ERROR(
                "Percona-XtraDB-Cluster prohibits changing"
                " storage engine of a table (%s.%s) from"
                " transactional to non-transactional"
                " with pxc_strict_mode = ENFORCING or MASTER",
                first_table->db, first_table->table_name);
            char message[1024];
            sprintf(message,
                    "Percona-XtraDB-Cluster prohibits changing"
                    " storage engine of a table (%s.%s) from"
                    " transactional to non-transactional"
                    " with pxc_strict_mode = ENFORCING or MASTER",
                    first_table->db, first_table->table_name);
            my_message(ER_UNKNOWN_ERROR, message, MYF(0));
            break;
        }

        if (block) return true;
      }
    }
  }

  /* This could be looked upon as too restrictive given it is taking
  a global mutex but anyway being TOI if there is alter tablespace
  operation active in parallel TOI would streamline it. */
  // TODO: the above comment is no longer true with NBO, this could be
  // too restrictive
  if (create_info.tablespace) {
    mysql_mutex_lock(&LOCK_wsrep_alter_tablespace);
  }

  {
    extern TABLE *find_temporary_table(THD * thd, const TABLE_LIST *tl);

    if ((!thd->is_current_stmt_binlog_format_row() ||
         !find_temporary_table(thd, first_table))) {
      if (WSREP(thd) &&
          wsrep_to_isolation_begin(
              thd, ((thd->lex->name.str) ? thd->lex->query_block->db : NULL),
              ((thd->lex->name.str) ? thd->lex->name.str : NULL), first_table,
              NULL, &alter_info)) {
        WSREP_WARN("ALTER TABLE isolation failure");
        if (create_info.tablespace) {
          mysql_mutex_unlock(&LOCK_wsrep_alter_tablespace);
        }
        return true;
      }
    }
  }

  if (create_info.tablespace) {
    mysql_mutex_unlock(&LOCK_wsrep_alter_tablespace);
  }
#endif /* WITH_WSREP */

  /* Push Strict_error_handler for alter table*/
  Strict_error_handler strict_handler;
  if (!thd->lex->is_ignore() && thd->is_strict_mode())
    thd->push_internal_handler(&strict_handler);

  result = mysql_alter_table(thd, alter_info.new_db_name.str,
                             alter_info.new_table_name.str, &create_info,
                             first_table, &alter_info);

  if (!thd->lex->is_ignore() && thd->is_strict_mode())
    thd->pop_internal_handler();

  return result;
}

bool Sql_cmd_discard_import_tablespace::execute(THD *thd) {
  /* Verify that exactly one of the DISCARD and IMPORT flags are set. */
  assert((m_alter_info->flags & Alter_info::ALTER_DISCARD_TABLESPACE) ^
         (m_alter_info->flags & Alter_info::ALTER_IMPORT_TABLESPACE));

  /*
    Verify that none of the other flags are set, except for
    ALTER_ALL_PARTITION, which may be set or not, and is
    therefore masked away along with the DISCARD/IMPORT flags.
  */
  assert(!(m_alter_info->flags & ~(Alter_info::ALTER_DISCARD_TABLESPACE |
                                   Alter_info::ALTER_IMPORT_TABLESPACE |
                                   Alter_info::ALTER_ALL_PARTITION)));

  /* first Query_block (have special meaning for many of non-SELECTcommands) */
  Query_block *query_block = thd->lex->query_block;
  /* first table of first Query_block */
  TABLE_LIST *table_list = query_block->get_table_list();

#ifdef WITH_WSREP
  /* Disable DISCARD/IMPORT TABLESPACE as this command
  will execute on single node and can introduce data
  in-consistency in cluster. */
  bool block = false;
  switch (pxc_strict_mode) {
    case PXC_STRICT_MODE_DISABLED:
      break;
    case PXC_STRICT_MODE_PERMISSIVE:
      WSREP_WARN(
          "Percona-XtraDB-Cluster doesn't recommend"
          " DISCARD/IMPORT of tablespace as it can introduce"
          " data in-consistency (if not done in tandem on all nodes)"
          " with pxc_strict_mode = PERMISSIVE");
      push_warning_printf(
          thd, Sql_condition::SL_WARNING, ER_UNKNOWN_ERROR,
          "Percona-XtraDB-Cluster doesn't recommend"
          " DISCARD/IMPORT of tablespace as it can introduce"
          " data in-consistency (if not done in tandem on all nodes)"
          " with pxc_strict_mode = PERMISSIVE");
      break;
    case PXC_STRICT_MODE_ENFORCING:
    case PXC_STRICT_MODE_MASTER:
    default:
      block = true;
      WSREP_ERROR(
          "Percona-XtraDB-Cluster doesn't recommend"
          " DISCARD/IMPORT of tablespace as it can introduce"
          " data in-consistency (if not done in tandem on all nodes)"
          " with pxc_strict_mode = ENFORCING or MASTER");
      char message[1024];
      sprintf(message,
              "Percona-XtraDB-Cluster prohibits"
              " DISCARD/IMPORT of tablespace as it can introduce"
              " data in-consistency (if not done in tandem on all nodes)"
              " with pxc_strict_mode = ENFORCING or MASTER");
      my_message(ER_UNKNOWN_ERROR, message, MYF(0));
      break;
  }

  if (block) return true;

  /* Setting this to true will avoid processing this statement through
  wsrep_replicate that is not meant to handle non-replicated atomic DDL. */
  thd->wsrep_non_replicating_atomic_ddl = true;

  if (WSREP(thd)) {
    WSREP_WARN(
        "While running PXC node in cluster mode it will skip"
        " binlogging of non-replicating DDL statement");

    /* Cache the state of sql_bin_log before toggling it. */
    if (thd->variables.option_bits & OPTION_BIN_LOG) {
      thd->variables.wsrep_saved_binlog_state =
          System_variables::wsrep_binlog_state_t::WSREP_BINLOG_ENABLED;
    } else {
      thd->variables.wsrep_saved_binlog_state =
          System_variables::wsrep_binlog_state_t::WSREP_BINLOG_DISABLED;
    }
    thd->variables.wsrep_saved_binlog_internal_off_state =
        thd->variables.option_bits & OPTION_BIN_LOG_INTERNAL_OFF;

    thd->variables.option_bits &= ~OPTION_BIN_LOG;
    thd->variables.option_bits |= OPTION_BIN_LOG_INTERNAL_OFF;
  }
#endif /* WITH_WSREP */

  if (check_access(thd, ALTER_ACL, table_list->db, &table_list->grant.privilege,
                   &table_list->grant.m_internal, false, false))
    return true;

  if (check_grant(thd, ALTER_ACL, table_list, false, UINT_MAX, false))
    return true;

  thd->set_slow_log_for_admin_command();

  /*
    Check if we attempt to alter mysql.slow_log or
    mysql.general_log table and return an error if
    it is the case.
    TODO: this design is obsolete and will be removed.
  */
  enum_log_table_type table_kind =
      query_logger.check_if_log_table(table_list, false);

  if (table_kind != QUERY_LOG_NONE) {
    /* Disable alter of enabled query log tables */
    if (query_logger.is_log_table_enabled(table_kind)) {
      my_error(ER_BAD_LOG_STATEMENT, MYF(0), "ALTER");
      return true;
    }
  }

  /*
    Add current database to the list of accessed databases
    for this statement. Needed for MTS.
  */
  thd->add_to_binlog_accessed_dbs(table_list->db);

  return mysql_discard_or_import_tablespace(thd, table_list);
}

bool Sql_cmd_secondary_load_unload::execute(THD *thd) {
  // One of the SECONDARY_LOAD/SECONDARY_UNLOAD flags must have been set.
  assert(((m_alter_info->flags & Alter_info::ALTER_SECONDARY_LOAD) == 0) !=
         ((m_alter_info->flags & Alter_info::ALTER_SECONDARY_UNLOAD) == 0));

  // No other flags should've been set.
  assert(!(m_alter_info->flags & ~(Alter_info::ALTER_SECONDARY_LOAD |
                                   Alter_info::ALTER_SECONDARY_UNLOAD)));

  TABLE_LIST *table_list = thd->lex->query_block->get_table_list();

  if (check_access(thd, ALTER_ACL, table_list->db, &table_list->grant.privilege,
                   &table_list->grant.m_internal, false, false))
    return true;

  if (check_grant(thd, ALTER_ACL, table_list, false, UINT_MAX, false))
    return true;

  return mysql_secondary_load_or_unload(thd, table_list);
}
