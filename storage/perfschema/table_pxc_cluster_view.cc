/* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file storage/perfschema/table_pxc_cluster_view.cc
  Table PXC_VIEW (implementation).
*/

#include "my_global.h"
#include "table_pxc_cluster_view.h"
#include "my_thread.h"
#include "pfs_instr_class.h"
#include "pfs_column_types.h"
#include "pfs_column_values.h"
#include "pfs_global.h"
#include "pfs_account.h"
#include "pfs_visitor.h"
#include "wsrep_mysqld.h"


THR_LOCK table_pxc_cluster_view::m_table_lock;

static const TABLE_FIELD_TYPE field_types[]=
{
  {
    { C_STRING_WITH_LEN("HOST_NAME") },
    { C_STRING_WITH_LEN("char(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("UUID") },
    { C_STRING_WITH_LEN("char(36)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("STATUS") },
    { C_STRING_WITH_LEN("char(64)") },
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("LOCAL_INDEX")},
    { C_STRING_WITH_LEN("int(11)")},
    { NULL, 0}
  },
  {
    { C_STRING_WITH_LEN("SEGMENT")},
    { C_STRING_WITH_LEN("int(11)")},
    { NULL, 0}
  }
};

TABLE_FIELD_DEF
table_pxc_cluster_view::m_field_def=
{ 5, field_types };

PFS_engine_table_share
table_pxc_cluster_view::m_share=
{
  { C_STRING_WITH_LEN("pxc_cluster_view") },
  &pfs_truncatable_acl,
  table_pxc_cluster_view::create,
  NULL, /* write_row */
  NULL, /* delete all rows */
  table_pxc_cluster_view::get_row_count,
  sizeof(PFS_simple_index),
  &m_table_lock,
  &m_field_def,
  false, /* checked */
  false  /* perpetual */
};

PFS_engine_table*
table_pxc_cluster_view::create(void)
{
  return new table_pxc_cluster_view();
}

table_pxc_cluster_view::table_pxc_cluster_view()
  : PFS_engine_table(&m_share, &m_pos),
    m_row_exists(false), m_pos(0), m_next_pos(0), m_entries()
{
  memset(&m_entries, 0, (sizeof(wsrep_node_info_t) * 64));
}

table_pxc_cluster_view::~table_pxc_cluster_view()
{}

void table_pxc_cluster_view::reset_position(void)
{
  m_pos.m_index= 0;
  m_next_pos.m_index= 0;
}

ha_rows table_pxc_cluster_view::get_row_count(void)
{
  return(wsrep_cluster_size);
}

int table_pxc_cluster_view::rnd_init(bool scan)
{
  wsrep->fetch_pfs_info(wsrep, m_entries, 64);
  return(0);
}

int table_pxc_cluster_view::rnd_next(void)
{
  for (m_pos.set_at(&m_next_pos);
       m_pos.m_index < get_row_count();
       m_pos.next())
  {
    make_row(m_pos.m_index);
    m_next_pos.set_after(&m_pos);
    return 0;
  }

  return HA_ERR_END_OF_FILE;
}

int
table_pxc_cluster_view::rnd_pos(const void *pos)
{
  set_position(pos);
  DBUG_ASSERT(m_pos.m_index < get_row_count());
  make_row(m_pos.m_index);

  return 0;
}

int table_pxc_cluster_view::rnd_end()
{
  memset(&m_entries, 0, (sizeof(wsrep_node_info_t) * 64));
  return 0;
}

void table_pxc_cluster_view::make_row(uint index)
{
  // Set default values.
  m_row_exists = false;
  m_row.local_index = 0;
  m_row.segment = 0;

  m_row = m_entries[index];

  m_row_exists= true;
}

int table_pxc_cluster_view
::read_row_values(TABLE *table,
                  unsigned char *buf,
                  Field **fields,
                  bool read_all)
{
  Field *f;

  if (unlikely(! m_row_exists))
    return HA_ERR_RECORD_DELETED;

  DBUG_ASSERT(table->s->null_bytes == 1);
  buf[0]= 0;

  for (; (f= *fields) ; fields++)
  {
    if (read_all || bitmap_is_set(table->read_set, f->field_index))
    {
      switch(f->field_index)
      {
      case 0: /** host name */
        set_field_char_utf8(f, m_row.host_name, strlen(m_row.host_name));
        break;
      case 1: /** uuid */
        set_field_char_utf8(f, m_row.uuid, WSREP_UUID_STR_LEN);
        break;
      case 2: /** status */
        set_field_char_utf8(f, m_row.status, strlen(m_row.status));
        break;
      case 3: /** local_index */
          set_field_ulong(f, m_row.local_index);
        break;
      case 4: /** segment */
        set_field_ulong(f, m_row.segment);
        break;
      default:
        DBUG_ASSERT(false);
      }
    }
  }
  return 0;
}

