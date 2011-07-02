/* Copyright 2008 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <mysqld.h>
#include <sql_class.h>
#include <sql_plugin.h>
#include <set_var.h>
#include <sql_acl.h>
//#include <sys_vars.h>
#include "wsrep_priv.h"
#include <my_dir.h>
#include <cstdio>
#include <cstdlib>

// trx history position to start with
static char  start_position[128]   = { 0, };
const  char* wsrep_start_position = start_position;

static char  provider[256]  = { 0, };
const  char* wsrep_provider = provider;

static char  cluster_address[256]  = { 0, };
const  char* wsrep_cluster_address = cluster_address;

int wsrep_init_vars()
{
  wsrep_start_position_default (NULL, OPT_DEFAULT);
  wsrep_provider_default       (NULL, OPT_DEFAULT);
  wsrep_cluster_address_default(NULL, OPT_DEFAULT);

  return 0;
}

bool wsrep_on_update (sys_var *self, THD* thd, enum_var_type var_type)
{
  if (var_type == OPT_GLOBAL) {
    // FIXME: this variable probably should be changed only per session
    thd->variables.wsrep_on = global_system_variables.wsrep_on;
  }
  else {
  }

  if (thd->variables.wsrep_on)
    thd->variables.option_bits |= (OPTION_BIN_LOG);
  else
    thd->variables.option_bits &= ~(OPTION_BIN_LOG);
  return true;
}

static int wsrep_start_position_verify (const char* start_str)
{
  size_t        start_len;
  char*         endptr;
  wsrep_uuid_t  uuid;
  ssize_t       uuid_len;
  wsrep_seqno_t seqno;

  start_len = strlen (start_str);
  if (start_len < 34)
    return 1;

  uuid_len = wsrep_uuid_scan (start_str, start_len, &uuid);
  if (uuid_len < 0 || (start_len - uuid_len) < 2)
    return 1;

  if (start_str[uuid_len] != ':') // separator should follow UUID
    return 1;

  seqno = strtoll (&start_str[uuid_len + 1], &endptr, 10);
  if (*endptr == '\0') return 0; // remaining string was seqno

  return 1;
}

bool wsrep_start_position_check (sys_var *self, THD* thd, set_var* var)
{
  char   buff[FN_REFLEN];
  String str(buff, sizeof(buff), system_charset_info), *res;
  const char*   start_str = NULL;

  if (!(thd->security_ctx->master_access & SUPER_ACL)) {
    my_error(ER_SPECIFIC_ACCESS_DENIED_ERROR, MYF(0), "SUPER");
    return 1;
  }

  if (!(res = var->value->val_str(&str))) goto err;

  start_str = res->c_ptr();

  if (!start_str) goto err;

  if (!wsrep_start_position_verify(start_str)) return 0;

err:

  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name, 
           start_str ? start_str : "NULL");
  return 1;
}

void wsrep_set_local_position (const char* value)
{
  size_t value_len  = strlen (value);
  size_t uuid_len   = wsrep_uuid_scan (value, value_len, &local_uuid);

  local_seqno = strtoll (value + uuid_len + 1, NULL, 10);
  strncpy (start_position, value, sizeof(start_position) - 1);
  sql_print_information ("WSREP: wsrep_start_position var submitted: '%s'",
                         wsrep_start_position);
}

bool wsrep_start_position_update (sys_var *self, THD* thd, enum_var_type type)
{
  // since this value passed wsrep_start_position_check, don't check anything
  // here
  char *latin= "latin1";
  LEX_STRING charset; 
  charset.str= latin;
  charset.length= strlen(latin);

  const uchar* value = self->value_ptr(thd, type, &charset);

  wsrep_set_local_position ((const char*)value);

  if (wsrep) {
    wsrep->sst_received (wsrep, &local_uuid, local_seqno, NULL, 0);
  }

  return 0;
}

void wsrep_start_position_default (THD* thd, enum_var_type var_type)
{
  static const char zero[] = "00000000-0000-0000-0000-000000000000:-1";
  strncpy (start_position, zero, sizeof(zero));
}

void wsrep_start_position_init (const char* val)
{
  if (NULL == val || wsrep_start_position_verify (val))
  {
    sql_print_error ("WSREP: Bad initial value for wsrep_start_position: "
                     "%s", (val ? val : ""));
    return;
  }

  wsrep_set_local_position (val);
}

static int wsrep_provider_verify (const char* provider_str)
{
  MY_STAT   f_stat;
  char path[FN_REFLEN];

  if (!provider_str || strlen(provider_str)== 0)
    return 1;

  if (!strcmp(provider_str, WSREP_NONE))
    return 0;

  if (!unpack_filename(path, provider_str))
    return 1;

  /* check that provider file exists */
  bzero(&f_stat, sizeof(MY_STAT));
  if (!my_stat(path, &f_stat, MYF(0)))
  {
    return 1;
  }
  return 0;
}

bool wsrep_provider_check (sys_var *self, THD* thd, set_var* var)
{
  char   buff[FN_REFLEN];
  String str(buff, sizeof(buff), system_charset_info), *res;
  const char*   provider_str = NULL;

  if (!(thd->security_ctx->master_access & SUPER_ACL)) {
    my_error(ER_SPECIFIC_ACCESS_DENIED_ERROR, MYF(0), "SUPER");
    return 1;
  }

  if (!(res = var->value->val_str(&str))) goto err;

  provider_str = res->c_ptr();

  if (!provider_str) goto err;

  if (!wsrep_provider_verify(provider_str)) return 0;

err:

  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name, 
           provider_str ? provider_str : "NULL");
  return 1;
}

bool wsrep_provider_update (sys_var *self, THD* thd, enum_var_type type)
{
  //const char* value = var->value->str_value.c_ptr();
  char *latin= "latin1";
  LEX_STRING charset; 
  charset.str= latin;
  charset.length= strlen(latin);
  const char* value = (const char*)self->value_ptr(thd, type, &charset);

  if (strcmp(value, provider)) {
    /* provider has changed */

    memset(provider, '\0', sizeof(provider));
    strncpy (provider, value, sizeof(provider) - 1);

    bool wsrep_on_saved= thd->variables.wsrep_on;
    thd->variables.wsrep_on= false;

    wsrep_stop_replication(thd);
    wsrep_start_replication();

    thd->variables.wsrep_on= wsrep_on_saved;
  }

  return 0;
}

void wsrep_provider_default (THD* thd, enum_var_type var_type)
{
  memset(provider, '\0', sizeof(provider));
  strncpy (provider, WSREP_NONE, sizeof(provider) - 1);
}

void wsrep_provider_init (const char* value)
{
  if (NULL == value || wsrep_provider_verify (value))
  {
    sql_print_error ("WSREP: Bad initial value for wsrep_provider: "
                     "%s", (value ? value : ""));
    return;
  }

  memset(provider, '\0', sizeof(provider));
  strncpy (provider, value, sizeof(provider) - 1);
}

static int wsrep_cluster_address_verify (const char* cluster_address_str)
{
  /* allow empty cluster address */
  if (!cluster_address_str               || 
      strlen(cluster_address_str) == 0   ||
      !strcmp(cluster_address_str, "dummy://"))
    return 0;

  /* supported GCSs: gcomm, vsbes */
  if (is_prefix(cluster_address_str, "gcomm") ||
      is_prefix(cluster_address_str, "vsbes"))
    return 0;

  return 1;
}

bool wsrep_cluster_address_check (sys_var *self, THD* thd, set_var* var)
{
  char   buff[FN_REFLEN];
  String str(buff, sizeof(buff), system_charset_info), *res;
  const char*   cluster_address_str = NULL;

  if (!(thd->security_ctx->master_access & SUPER_ACL)) {
    my_error(ER_SPECIFIC_ACCESS_DENIED_ERROR, MYF(0), "SUPER");
    return false;
  }

  if (!(res = var->value->val_str(&str))) goto err;

  cluster_address_str = res->c_ptr();

  if (!cluster_address_str) goto err;

  if (!wsrep_cluster_address_verify(cluster_address_str)) return true;

 err:

  my_error(ER_WRONG_VALUE_FOR_VAR, MYF(0), var->var->name, 
             cluster_address_str ? cluster_address_str : "NULL");
  return false;
}

bool wsrep_cluster_address_update (sys_var *self, THD* thd, enum_var_type type)
{
  char *latin= "latin1";
  LEX_STRING charset; 
  charset.str= latin;
  charset.length= strlen(latin);
  const uchar* value = self->value_ptr(thd, type, &charset);

  memset(cluster_address, '\0', sizeof(cluster_address));
  strncpy(cluster_address, (char *)value, sizeof(cluster_address) - 1);

  bool wsrep_on_saved= thd->variables.wsrep_on;
  thd->variables.wsrep_on= false;

  wsrep_stop_replication(thd);
  wsrep_start_replication();

  thd->variables.wsrep_on= wsrep_on_saved;

  return 0;
}

void wsrep_cluster_address_default (THD* thd, enum_var_type var_type)
{
  memset(cluster_address, '\0', sizeof(cluster_address));
}

void wsrep_cluster_address_init (const char* value)
{
  if (NULL == value || wsrep_cluster_address_verify (value))
  {
    sql_print_error ("WSREP: Bad initial value for wsrep_cluster_address: "
                     "%s", (value ? value : ""));
    return;
  }

  memset(cluster_address, '\0', sizeof(cluster_address));
  strncpy (cluster_address, value, sizeof(cluster_address) - 1);
}


/*
 * Status variables stuff below
 */
static inline void
wsrep_assign_to_mysql (SHOW_VAR* mysql, wsrep_stats_var* wsrep)
{
  mysql->name = wsrep->name;
  switch (wsrep->type) {
  case WSREP_VAR_INT64:
    mysql->value = (char*) &wsrep->value._int64;
    mysql->type  = SHOW_LONGLONG;
    break;
  case WSREP_VAR_STRING:
    mysql->value = (char*) &wsrep->value._string;
    mysql->type  = SHOW_CHAR_PTR;
    break;
  case WSREP_VAR_DOUBLE:
    mysql->value = (char*) &wsrep->value._double;
    mysql->type  = SHOW_DOUBLE;
    break;
  }
}

static wsrep_stats_var*  wsrep_status_vars = NULL;

#if DYNAMIC
// somehow this mysql status thing works only with statically allocated arrays.
static SHOW_VAR*          mysql_status_vars = NULL;
static int                mysql_status_len  = -1;
#else
static SHOW_VAR           mysql_status_vars[100 + 1];
static const int          mysql_status_len  = 100;
#endif

static void export_wsrep_status_to_mysql()
{
  int wsrep_status_len, i;

  if (wsrep_status_vars) wsrep->stats_free (wsrep, wsrep_status_vars);

  wsrep_status_vars = wsrep->stats_get (wsrep);

  if (!wsrep_status_vars) {
    return;
  }

  for (wsrep_status_len = 0;
       wsrep_status_vars[wsrep_status_len].name != NULL;
       wsrep_status_len++);

#if DYNAMIC
  if (wsrep_status_len != mysql_status_len) {
    void* tmp = realloc (mysql_status_vars,
                         (wsrep_status_len + 1) * sizeof(SHOW_VAR));
    if (!tmp) {
      
      sql_print_error ("Out of memory for wsrep status variables."
                       "Number of variables: %d", wsrep_status_len);
      return;
    }
    
    mysql_status_len  = wsrep_status_len;
    mysql_status_vars = (SHOW_VAR*)tmp;
  }
  /* @TODO: fix this: */
#lese
  if (mysql_status_len < wsrep_status_len) wsrep_status_len= mysql_status_len;
#endif

  for (i = 0; i < wsrep_status_len; i++)
    wsrep_assign_to_mysql (mysql_status_vars + i, wsrep_status_vars + i);

  mysql_status_vars[wsrep_status_len].name  = NullS;
  mysql_status_vars[wsrep_status_len].value = NullS;
  mysql_status_vars[wsrep_status_len].type  = SHOW_LONG;
}

int wsrep_show_status (THD *thd, SHOW_VAR *var, char *buff)
{
  export_wsrep_status_to_mysql();
  var->type= SHOW_ARRAY;
  var->value= (char *) &mysql_status_vars;
  return 0;
}
