#ifndef WSREP_ASYNC_MONITOR_H
#define WSREP_ASYNC_MONITOR_H

#ifdef WITH_WSREP
#include <iostream>
#include <condition_variable>
#include <mutex>

class Wsrep_async_monitor {
 public:
  using seqno_t = unsigned long long;

  Wsrep_async_monitor() : m_last_entered(0), m_last_left(0) {}

  ~Wsrep_async_monitor() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_last_entered = 0;
  m_last_left = 0;
  m_cond.notify_all();
}

  void enter(seqno_t seqno);
  void leave(seqno_t seqno);
  void reset(seqno_t seqno);

  seqno_t last_entered() const { return m_last_entered; }
  seqno_t last_left() const { return m_last_left; }

 private:
  std::mutex m_mutex;
  std::condition_variable m_cond;

  // TODO: Evaluate if we really need m_last_entered
  seqno_t m_last_entered;
  seqno_t m_last_left;
};

#endif /* WITH_WSREP */
#endif /* WSREP_ASYNC_MONITOR_H */
