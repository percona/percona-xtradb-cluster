/* Copyright (c) 2024 Percona LLC and/or its affiliates. All rights reserved.

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef WSREP_ASYNC_MONITOR_H
#define WSREP_ASYNC_MONITOR_H

#ifdef WITH_WSREP
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <set>

class Wsrep_async_monitor {
 public:
  Wsrep_async_monitor(unsigned long count) : m_workers_count(count) {}

  using seqno_t = unsigned long long;

  // Method for main thread to add scheduled seqnos
  void schedule(seqno_t seqno);

  // Method for both DDL and DML to enter the monitor
  void enter(seqno_t seqno);

  // Method to be called after DDL/DML processing is complete
  void leave(seqno_t seqno);

  // Method to skip a transaction that will not call enter() and leave()
  void skip(seqno_t seqno);

  // Method to return if the monitor is empty, used by the unittests
  bool is_empty();

 private:
  unsigned long m_workers_count;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::set<seqno_t> skipped_seqnos;      // Tracks skipped sequence numbers
  std::queue<seqno_t> scheduled_seqnos;  // Queue to track scheduled seqnos
};

#endif /* WITH_WSREP */
#endif /* WSREP_ASYNC_MONITOR_H */
