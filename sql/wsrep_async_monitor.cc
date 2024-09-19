#ifdef WITH_WSREP
#include "sql/wsrep_async_monitor.h"
#include <algorithm>
#include <cassert>

void Wsrep_async_monitor::enter(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Wait for its turn before entering
  m_cond.wait(lock, [this, seqno]() { return seqno == m_last_left + 1; });
  m_last_entered = (seqno > m_last_entered) ? seqno : m_last_entered;
  fprintf(stderr, "Entered the monitor with seqno: %llu\n", seqno);
}

void Wsrep_async_monitor::leave(seqno_t seqno) {
  // Wait for its turn before leaving
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cond.wait(lock, [this, seqno]() { return seqno == m_last_left + 1; });
  m_last_left = seqno;

  fprintf(stderr, "Left the monitor seqno: %llu\n", seqno);
  // Notify all waiting threads
  m_cond.notify_all();
}

void Wsrep_async_monitor::reset(seqno_t seqno) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // We can reset only if the last entered and last left
  // are same, meaning that there is no one who is inside
  // the monitor
  assert(m_last_entered == m_last_left);
  m_last_entered = seqno;
  m_last_left = seqno;
  m_cond.notify_all();
}

#endif /* WITH_WSREP */
