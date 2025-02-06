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

#ifdef WITH_WSREP
#include "sql/wsrep_async_monitor.h"
#include <cassert>
#include "sql/mysqld.h"

// Method for main thread to add scheduled seqnos
void Wsrep_async_monitor::schedule(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);
  scheduled_seqnos.push(seqno);
}

// Method for both DDL and DML to enter the monitor
void Wsrep_async_monitor::enter(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Don't wait if it is a skipped seqno
  if (skipped_seqnos.count(seqno) > 0) return;

  // Wait until this transaction is at the head of the scheduled queue
  m_cond.wait(lock, [this, seqno] {
    // Here we need to remove skipped transactions

    // Imagine a scenario where scheduled seqnos is 1,2(skip),3 and threads
    // enter in an out of order manner.
    //
    // - 3 enters the monitor first, since it is not in the front of the queue,
    // it
    //   goes to cond_wait
    // - 2 enters the monitor, adds 2 to skipped_seqnos
    // - 1 enters the monitor, since it is in the front, it acquires the
    // monitor,
    //   does its job and leaves the monitor by removing itself from the
    //   scheduled_seqnos and signals 3.
    // - 3 wakes up, but it will see that 2 in the front of the
    //   queue.
    //
    //   To proceed, it needs to remove all the skipped seqnos from the
    //   scheduled_seqnos queue.
    while (!scheduled_seqnos.empty() &&
           skipped_seqnos.count(scheduled_seqnos.front()) > 0) {
      scheduled_seqnos.pop();
    }
    return !scheduled_seqnos.empty() && scheduled_seqnos.front() == seqno;
  });
}

// Method to be called after DDL/DML processing is complete
void Wsrep_async_monitor::leave(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Don't wait if it is a skipped seqno
  if (skipped_seqnos.count(seqno) > 0) return;

  // Check if the sequence number matches the front of the queue.
  // In a correctly functioning monitor this should always be true
  // as each transaction should exit in the order it was scheduled
  // and processed.
  if (!scheduled_seqnos.empty() && scheduled_seqnos.front() == seqno) {
    // Remove the seqno from the scheduled queue now that it has completed
    scheduled_seqnos.pop();
  } else {
    // std::cout << "Error: Mismatch in sequence numbers. Expected "
    //           << (scheduled_seqnos.empty()
    //                   ? "none"
    //                   : std::to_string(scheduled_seqnos.front()))
    //           << " but got " << seqno << "." << std::endl;
    assert(false && "Sequence number mismatch in leave()");
#ifndef EXTRA_CODE_FOR_UNIT_TESTING
    unireg_abort(1);
#endif
  }

  // Remove seqnos from skipped_seqnos.
  // Note: Here we remove all seqnos less than `seqno - m_workers_count` to
  // ensure that at most the skipped_seqnos has `m_workers_count` items.
  //
  // Example: when there are 4 workers and if seqno 15 enters, this will remove
  // all entries less than 11
  seqno_t remove_upto =
      (seqno > m_workers_count) ? (seqno - m_workers_count) : 0;
  for (auto it = skipped_seqnos.begin();
       it != skipped_seqnos.end() && *it <= remove_upto;
       /* No increment here*/) {
    it = skipped_seqnos.erase(it);
  }

  // Notify waiting threads in case the next scheduled sequence can enter
  m_cond.notify_all();
}

// Method to skip a transaction that will not call enter() and leave()
void Wsrep_async_monitor::skip(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Check if the seqno is already marked as skipped
  if (skipped_seqnos.count(seqno) > 0) {
    return;  // Already skipped, so do nothing
  }

  // Mark the seqno as skipped
  skipped_seqnos.insert(seqno);

  // Remove it from the scheduled queue if it is at the front
  if (!scheduled_seqnos.empty() && scheduled_seqnos.front() == seqno) {
    scheduled_seqnos.pop();
  }

  // Remove seqnos from skipped_seqnos.
  // Note: Here we remove all seqnos less than `seqno - m_workers_count` to
  // ensure that at most the skipped_seqnos has `m_workers_count` items.
  //
  // Example: when there are 4 workers and if seqno 15 enters, this will remove
  // all entries less than 11
  seqno_t remove_upto =
      (seqno > m_workers_count) ? (seqno - m_workers_count) : 0;
  for (auto it = skipped_seqnos.begin();
       it != skipped_seqnos.end() && *it <= remove_upto;
       /* No increment here*/) {
    it = skipped_seqnos.erase(it);
  }

  // Notify in case other transactions are waiting to enter
  m_cond.notify_all();
}

// Method to return if the monitor is empty, used by the unittests
bool Wsrep_async_monitor::is_empty() {
  std::unique_lock<std::mutex> lock(m_mutex);
  return scheduled_seqnos.empty();
}
#endif /* WITH_WSREP */
