/* Copyright (C) 2013-2018 Codership Oy <info@codership.com>

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

#include "log.h"
#include "mysqld.h"
#include "query_options.h"
#include "rpl_gtid.h"
#include "wsrep_api.h"

#include <my_config.h>
#include "wsrep/gtid.hpp"
#include <string>

/* system variables */
extern const char *wsrep_sst_method;
extern const char *wsrep_sst_receive_address;
extern const char *wsrep_sst_donor;
extern bool wsrep_sst_donor_rejects_queries;

/*! Synchronizes applier thread start with init thread */
extern void wsrep_sst_grab();
/*! Init thread waits for SST completion */
extern bool wsrep_sst_wait();
/*! Signals wsrep that initialization is complete, writesets can be applied */
extern void wsrep_sst_continue();
/*! Cancel the SST script if it is running */
extern void wsrep_sst_cancel(bool call_wsrep_cb);

extern void wsrep_SE_init_grab();         /*! grab init critical section */
extern void wsrep_SE_init_wait(THD *thd); /*! wait for SE init to complete */
extern void wsrep_SE_init_done();         /*! signal that SE init is complte */
extern void wsrep_SE_initialized();       /*! mark SE initialization complete */
extern bool wsrep_is_SE_initialized();

/**
   Return a string containing the state transfer request string.
   Note that the string may contain a '\0' in the middle.
*/
std::string wsrep_sst_prepare();

/**
   Donate a SST.

  @param request SST request string received from the joiner. Note that
                 the string may contain a '\0' in the middle.
  @param gtid    Current position of the donor
  @param bypass  If true, full SST is not needed. Joiner needs to be
                 notified that it can continue starting from gtid.
 */
int wsrep_sst_donate(const std::string &request, const wsrep::gtid &gtid,
                     bool bypass);

extern int wsrep_remove_sst_user(bool initialize_thread);

#endif /* WSREP_SST_H */
