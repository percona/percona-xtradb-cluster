/* Copyright (c) 2023 Percona LLC and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "sql/wsrep_thd_context.h"  // Wsrep_thd_context

std::vector<wsrep_table_t> &Wsrep_thd_context::get_fk_parent_tables() {
  return fk_parent_tables;
}

std::vector<wsrep_table_t> &Wsrep_thd_context::get_fk_child_tables() {
  return fk_child_tables;
}

void Wsrep_thd_context::clear() {
  fk_parent_tables.clear();
  fk_child_tables.clear();
}
