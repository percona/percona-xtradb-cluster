/* Copyright (C) 2013 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef WSREP_SST_H
#define WSREP_SST_H

// #include <mysql.h> // my_bool

#include "mysqld.h"
#include "log.h"
typedef struct st_mysql_show_var SHOW_VAR;
#include "query_options.h"
#include "rpl_gtid.h"
#include "wsrep_api.h"

/* system variables */
extern const char* wsrep_sst_method;
extern const char* wsrep_sst_receive_address;
extern const char* wsrep_sst_donor;
extern       char* wsrep_sst_auth;
extern    my_bool  wsrep_sst_donor_rejects_queries;
extern const char *wsrep_sst_allowed_methods;

/*! Synchronizes applier thread start with init thread */
extern void wsrep_sst_grab();
/*! Init thread waits for SST completion */
extern bool wsrep_sst_wait();
/*! Signals wsrep that initialization is complete, writesets can be applied */
extern void wsrep_sst_continue();
/*! Cancel the SST script if it is running */
extern void wsrep_sst_cancel(bool call_wsrep_cb);

extern void wsrep_SE_init_grab();          /*! grab init critical section */
extern void wsrep_SE_init_wait(THD* thd);  /*! wait for SE init to complete */
extern void wsrep_SE_init_done();          /*! signal that SE init is complte */
extern void wsrep_SE_initialized();        /*! mark SE initialization complete */
extern bool wsrep_is_SE_initialized();

#endif /* WSREP_SST_H */
