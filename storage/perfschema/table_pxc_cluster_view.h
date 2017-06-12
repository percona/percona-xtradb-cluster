/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

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

#ifndef TABLE_PXC_VIEW_H
#define TABLE_PXC_VIEW_H

/**
  @file storage/perfschema/table_pxc_cluster_view.h
  Table PXC_VIEW (declarations).
*/

#include "pfs_column_types.h"
#include "pfs_engine_table.h"
#include "pfs_instr_class.h"
#include "pfs_instr.h"
#include "pfs_user.h"
#include "table_helper.h"
#include "pfs_variable.h"
#include "pfs_buffer_container.h"
#include "../wsrep/wsrep_api.h"

/**
  @addtogroup Performance_schema_tables
  @{
*/

/**
  Store and retrieve table state information for queries that reinstantiate
  the table object.
*/
class table_pxc_cluster_view : public PFS_engine_table
{
private:
  void make_row(uint index);
  /** Table share lock. */
  static THR_LOCK m_table_lock;
  /** Fields definition. */
  static TABLE_FIELD_DEF m_field_def;
  /** True if the current row exists. */
  bool m_row_exists;
  /** Current row */
  wsrep_node_info_t m_row;
  /** Current position. */
  PFS_simple_index m_pos;
  /** Next position. */
  PFS_simple_index m_next_pos;
  /** Cache fetched entries describing pxc view.
  Currently set to hold only 64 entries that is good enough
  for now since no-one operate PXC with those many number of nodes.*/
  wsrep_node_info_t m_entries[64];

protected:
  /**
    Read the current row values.
    @param table            Table handle
    @param buf              row buffer
    @param fields           Table fields
    @param read_all         true if all columns are read.
  */

  virtual int read_row_values(TABLE *table,
                              unsigned char *buf,
                              Field **fields,
                              bool read_all);

  table_pxc_cluster_view();

public:
  ~table_pxc_cluster_view();

  /** Table share. */
  static PFS_engine_table_share m_share;
  static PFS_engine_table* create();
  static ha_rows get_row_count();
  virtual int rnd_init(bool scan);
  virtual int rnd_next();
  virtual int rnd_pos(const void *pos);
  virtual int rnd_end();
  virtual void reset_position(void);
};

/** @} */
#endif
