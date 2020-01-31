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

#include "storage/perfschema/table_pxc_cluster_view.h"
#include "storage/perfschema/pfs_instr.h"
#include "storage/perfschema/pfs_instr_class.h"

#include "sql/table.h"
#include "sql/field.h"
#include "sql/plugin_table.h"

#include "sql/wsrep_mysqld.h"
#include "sql/wsrep_server_state.h"

THR_LOCK table_pxc_cluster_view::m_table_lock;

Plugin_table table_pxc_cluster_view::m_table_def(
    /* Schema name */
    "performance_schema",
    /* Name */
    "pxc_cluster_view",
    /* Definition */
    " HOST_NAME CHAR(64) not null,\n"
    " UUID CHAR(36) not null,\n"
    " STATUS CHAR(64) not null,\n"
    " LOCAL_INDEX INT UNSIGNED not null,\n"
    " SEGMENT INT UNSIGNED not null\n",
    /* Options */
    " ENGINE=PERFORMANCE_SCHEMA",
    /* Tablespace */
    nullptr);

PFS_engine_table_share table_pxc_cluster_view::m_share = {
    &pfs_readonly_acl,
    table_pxc_cluster_view::create,
    NULL, /* write_row */
    NULL, /* delete_all_rows */
    table_pxc_cluster_view::get_row_count,
    sizeof(PFS_simple_index),
    &m_table_lock,
    &m_table_def,
    true, /* perpetual */
    PFS_engine_table_proxy(),
    {0},
    false /* m_in_purgatory */
};

PFS_engine_table *table_pxc_cluster_view::create(PFS_engine_table_share *) {
  return new table_pxc_cluster_view();
}

table_pxc_cluster_view::table_pxc_cluster_view()
    : PFS_engine_table(&m_share, &m_pos),
      m_row_exists(false),
      m_pos(0),
      m_next_pos(0),
      m_entries() {
  memset(&m_entries, 0, (sizeof(wsrep_node_info_t) * 64));
}

table_pxc_cluster_view::~table_pxc_cluster_view() {}

void table_pxc_cluster_view::reset_position(void) {
  m_pos.m_index = 0;
  m_next_pos.m_index = 0;
}

ha_rows table_pxc_cluster_view::get_row_count(void) {
  return (wsrep_cluster_size);
}

int table_pxc_cluster_view::rnd_init(bool) {
  Wsrep_server_state::instance().get_provider().fetch_pfs_info(m_entries, 64);
  return (0);
}

int table_pxc_cluster_view::rnd_next(void) {
  for (m_pos.set_at(&m_next_pos); m_pos.m_index < get_row_count();
       m_pos.next()) {
    make_row(m_pos.m_index);
    m_next_pos.set_after(&m_pos);
    return 0;
  }

  return HA_ERR_END_OF_FILE;
}

int table_pxc_cluster_view::rnd_pos(const void *pos) {
  set_position(pos);
  DBUG_ASSERT(m_pos.m_index < get_row_count());
  make_row(m_pos.m_index);

  return 0;
}

int table_pxc_cluster_view::rnd_end() {
  memset(&m_entries, 0, (sizeof(wsrep_node_info_t) * 64));
  return 0;
}

void table_pxc_cluster_view::make_row(uint index) {
  // Set default values.
  m_row_exists = false;
  m_row.local_index = 0;
  m_row.segment = 0;

  m_row = m_entries[index];

  m_row_exists = true;
}

int table_pxc_cluster_view ::read_row_values(TABLE *table, unsigned char *buf,
                                             Field **fields, bool read_all) {
  Field *f;

  if (unlikely(!m_row_exists)) return HA_ERR_RECORD_DELETED;

  DBUG_ASSERT(table->s->null_bytes == 1);
  buf[0] = 0;

  for (; (f = *fields); fields++) {
    if (read_all || bitmap_is_set(table->read_set, f->field_index)) {
      switch (f->field_index) {
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

