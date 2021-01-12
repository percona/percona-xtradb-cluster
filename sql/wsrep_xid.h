/* Copyright (C) 2015 Codership Oy <info@codership.com>

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

#ifndef WSREP_XID_H
#define WSREP_XID_H

#include "handler.h" // XID typedef
#include "wsrep_api.h"

void wsrep_xid_init(xid_t*, const wsrep_uuid_t&, wsrep_seqno_t);
int wsrep_is_wsrep_xid(const void* xid);
const wsrep_uuid_t* wsrep_xid_uuid(const XID&);
wsrep_seqno_t wsrep_xid_seqno(const XID&);

//void wsrep_get_SE_checkpoint(XID&); uncomment if needed
void wsrep_get_SE_checkpoint(wsrep_uuid_t&, wsrep_seqno_t&);
//void wsrep_set_SE_checkpoint(XID&); uncomment if needed
void wsrep_set_SE_checkpoint(const wsrep_uuid_t&, wsrep_seqno_t);

void wsrep_sort_xid_array(XID *array, int len);

#endif /* WSREP_UTILS_H */
