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

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "sql/wsrep_async_monitor.h"  // Include the header file of your class

using seqno_t = unsigned long long;

class WsrepAsyncMonitorTest : public ::testing::Test {
 protected:
  Wsrep_async_monitor monitor{8};

  void SetUp() override {
    // Any setup can be done here, if needed
  }

  void TearDown() override {
    // Any teardown can be done here, if needed
  }
};

// Helper function to simulate processing a transaction
void processThread(Wsrep_async_monitor &monitor, seqno_t seqno) {
  monitor.enter(seqno);
  // Simulate work by sleeping briefly
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  monitor.leave(seqno);
}

// Test that the monitor can handle single transactions (DML and DDL)
TEST_F(WsrepAsyncMonitorTest, SingleTransaction) {
  monitor.schedule(1);

  std::thread t1(processThread, std::ref(monitor), 1);
  t1.join();

  // Check if the queue is empty after leaving
  ASSERT_TRUE(monitor.is_empty());
}

// Test that monitor can handle sequence mismatch.
TEST_F(WsrepAsyncMonitorTest, LeaveSequenceNumberMismatch) {
  seqno_t seqno1 = 1;
  seqno_t seqno2 = 2;
  monitor.schedule(seqno1);
  monitor.enter(seqno1);

  // Check if the queue is not empty
  ASSERT_TRUE(!monitor.is_empty());

  // Expect an assertion failure or exit due to sequence number mismatch
  ASSERT_DEATH(monitor.leave(seqno2), "Sequence number mismatch in leave()");
}

// Test that multiple transactions are processed in the correct sequence
TEST_F(WsrepAsyncMonitorTest, MultipleQueriesInSequence) {
  monitor.schedule(1);
  monitor.schedule(2);
  monitor.schedule(3);
  monitor.schedule(4);

  std::vector<int> processed_seqnos;
  auto transactionWorker = [&](seqno_t seqno, bool isDDL) {
    monitor.enter(seqno);
    processed_seqnos.push_back(seqno);
    if (isDDL) {
      // Simulate work by sleeping briefly
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    monitor.leave(seqno);
  };

  std::thread t1(transactionWorker, 1, true);   // DML with seqno 1
  std::thread t2(transactionWorker, 2, false);  // DDL with seqno 2
  std::thread t3(transactionWorker, 3, true);   // DML with seqno 3
  std::thread t4(transactionWorker, 4, false);  // DML with seqno 4

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  EXPECT_EQ(processed_seqnos, std::vector<int>({1, 2, 3, 4}));
}

// Test that skipped transactions are not processed but do not block other
// transactions
TEST_F(WsrepAsyncMonitorTest, Skip) {
  monitor.schedule(1);
  monitor.schedule(2);
  monitor.schedule(3);
  monitor.schedule(4);
  monitor.schedule(5);
  monitor.schedule(6);

  std::vector<int> processed_seqnos;
  auto transactionWorker = [&](seqno_t seqno, bool isDDL) {
    monitor.enter(seqno);
    processed_seqnos.push_back(seqno);
    if (isDDL) {
      // Simulate work by sleeping briefly
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    monitor.leave(seqno);
  };
  auto transactionSkip = [&](seqno_t seqno) { monitor.skip(seqno); };

  // Make sure that the logic works for out-of-order enter() calls
  std::thread t1(transactionWorker, 3, false);  // DML with seqno 3
  std::thread t2(transactionSkip, 2);           // Skipped seqno 2

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::thread t3(transactionWorker, 1, false);  // DML with seqno 1

  t1.join();
  t2.join();
  t3.join();

  // Make sure that the logic works for out-of-order enter() calls
  std::thread t4(transactionSkip, 5);          // DML with seqno 3
  std::thread t5(transactionWorker, 6, true);  // Skipped seqno 2

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::thread t6(transactionWorker, 4, false);  // DML with seqno 1

  t4.join();
  t5.join();
  t6.join();
  EXPECT_EQ(processed_seqnos, std::vector<int>({1, 3, 4, 6}));
}

// Test that transactions can be skipped without blocking other transactions
//
// Imagine a scenario where scheduled seqnos is 1,2(skip),3 and threads enter in
// an out of order manner.
//
// - 3 enters the monitor first, since it is not in the front of the queue, it
//   goes to cond_wait
// - 2 enters the monitor, adds 2 to skipped_seqnos
// - 1 enters the monitor, since it is in the front, it acquires the monitor,
//   does its job and leaves the monitor by removing itself from the
//   scheduled_seqnos and signals 3.
// - 3 wakes up, but it will see that 2 in the front of the
//   queue.
//
//   To proceed, it needs to remove all the skipped seqnos from the
//   scheduled_seqnos queue.
TEST_F(WsrepAsyncMonitorTest, SkipThenEnterTransaction) {
  monitor.schedule(1);
  monitor.schedule(2);
  monitor.schedule(3);

  monitor.skip(2);  // Skip seqno 2

  std::vector<int> processed_seqnos;
  std::mutex seqno_mutex;

  auto transactionWorker = [&](unsigned long seqno) {
    monitor.enter(seqno);
    // Dont process seqno 2
    if (seqno != 2) {
      std::lock_guard<std::mutex> lock(seqno_mutex);
      processed_seqnos.push_back(seqno);
    }
    monitor.leave(seqno);
  };

  std::thread t1(transactionWorker, 1);  // seqno 1
  std::thread t2(transactionWorker,
                 2);  // seqno 2 (should skip without blocking)
  std::thread t3(transactionWorker, 3);  // seqno 3

  t1.join();
  t2.join();
  t3.join();

  EXPECT_EQ(processed_seqnos, std::vector<int>({1, 3}));
}

// Test that transactions can be processed concurrently
TEST_F(WsrepAsyncMonitorTest, Concurrents) {
  monitor.schedule(1);
  monitor.schedule(2);
  monitor.schedule(3);
  monitor.schedule(4);

  std::vector<seqno_t> processed_seqnos;
  std::mutex seqno_mutex;

  auto transactionWorker = [&](seqno_t seqno, bool isDDL) {
    monitor.enter(seqno);
    {
      std::lock_guard<std::mutex> lock(seqno_mutex);
      processed_seqnos.push_back(seqno);
      if (isDDL) {
        // Simulate work by sleeping briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
    monitor.leave(seqno);
  };

  std::thread t1(transactionWorker, 4, false);  // DML with seqno 4
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::thread t2(transactionWorker, 3, true);  // DDL with seqno 3
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::thread t3(transactionWorker, 2, false);  // DML with seqno 2
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::thread t4(transactionWorker, 1, true);  // DDL with seqno 1

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  EXPECT_EQ(processed_seqnos, std::vector<seqno_t>({1, 2, 3, 4}));
}
#endif /* WITH_WSREP */
