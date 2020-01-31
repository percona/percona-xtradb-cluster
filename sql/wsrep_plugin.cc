/* Copyright 2016 Codership Oy <http://www.codership.com>

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

#include "mysql/components/services/log_builtins.h"
#include "wsrep_trans_observer.h"
#include "wsrep_mysqld.h"

#include <mysql/plugin.h>

static int wsrep_plugin_init(void *p __attribute__((unused)))
{
  WSREP_DEBUG("wsrep_plugin_init()");
  return 0;
}

static int wsrep_plugin_deinit(void *p __attribute__((unused)))
{
  WSREP_DEBUG("wsrep_plugin_deinit()");
  return 0;
}

struct st_mysql_storage_engine wsrep_storage_engine = {
    MYSQL_HANDLERTON_INTERFACE_VERSION
};

mysql_declare_plugin(wsrep){
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &wsrep_storage_engine,
    "wsrep",
    "Codership Oy",
    "A pseudo storage engine to represent transactions in multi-master "
    "synchornous replication",
    PLUGIN_LICENSE_GPL,
    wsrep_plugin_init, /* Plugin Init */
    NULL,            /* Plugin Check uninstall */
    wsrep_plugin_deinit, /* Plugin Deinit */
    0x0100 /* 1.0 */,
    NULL, /* status variables                */
    NULL, /* system variables                */
    NULL, /* config options                  */
    0,    /* flags                           */
} mysql_declare_plugin_end;

