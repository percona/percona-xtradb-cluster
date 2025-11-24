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

#ifndef WSREP_THD_CONTEXT_H
#define WSREP_THD_CONTEXT_H

#include <map>
#include <string>

/*
  This class encapsulates the wsrep context associated with the THD
  object.
 */
class Wsrep_thd_context {
 private:
  /*
    We cache keys related to parent/child tables for some TOIs.
    We need this cache, because of what is said in comment of
    wsrep::key::append_key_part():
    "The caller is supposed to take
     care that the pointer remains valid over the lifetime
     if the key object"
  */
  std::multimap<std::string, std::string> fk_parent_tables;
  std::multimap<std::string, std::string> fk_child_tables;

  Wsrep_thd_context(const Wsrep_thd_context &) = delete;  // copy constructor
  Wsrep_thd_context operator=(Wsrep_thd_context &&) =
      delete;  // move assignment
  Wsrep_thd_context operator=(const Wsrep_thd_context &) =
      delete;  // copy assignment

 public:
  Wsrep_thd_context() {}
  void clear();

  std::multimap<std::string, std::string> &get_fk_parent_tables();
  std::multimap<std::string, std::string> &get_fk_child_tables();
};

#endif /* WSREP_THD_CONTEXT_H */