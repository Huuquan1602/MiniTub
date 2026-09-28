#include "buffer/lru_k_replacer.h"

#include <set>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "common/exception.h"
#include "test_util.h"

namespace minitub {

// Timestamps in the comments (t0, t1, ...) are the replacer's logical clock:
// every RecordAccess() call takes the next value.

// checks: an empty replacer has nothing to evict.
TEST(LRUKReplacerTest, EmptyReplacerEvictsNothing) {
  LRUKReplacer r(7, 2);
  EXPECT_EQ(r.Size(), 0U);
  EXPECT_EQ(r.Evict(), std::nullopt);
}

// checks: a frame seen for the first time is NOT evictable until SetEvictable(true).
TEST(LRUKReplacerTest, NewFrameStartsNonEvictable) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);
  EXPECT_EQ(r.Size(), 0U);
  EXPECT_EQ(r.Evict(), std::nullopt);
  r.SetEvictable(1, true);
  EXPECT_EQ(r.Size(), 1U);
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Size(), 0U);
}

// checks: frames with fewer than k accesses (+inf distance) tie-break by their OLDEST
// recorded access, not their most recent one. Tricky: plain LRU would evict frame 2 here.
TEST(LRUKReplacerTest, InfiniteDistanceTieGoesToOldestAccess) {
  LRUKReplacer r(7, 3);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(1);  // t2  -> frame 1 was used more recently, but its oldest access is t0
  r.SetEvictable(1, true);
  r.SetEvictable(2, true);
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Evict(), 2);
}

// checks: any +inf frame is evicted before any frame with k accesses, even a more recent one.
TEST(LRUKReplacerTest, InfiniteDistanceBeatsFiniteDistance) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0
  r.RecordAccess(1);  // t1  frame 1: 2 accesses -> finite
  r.RecordAccess(2);  // t2  frame 2: 1 access   -> +inf
  r.SetEvictable(1, true);
  r.SetEvictable(2, true);
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), 1);
}

// checks: with k accesses each, frames are evicted by largest backward k-distance
// (earliest k-th most recent access). Plain LRU would pick 3, 2, 1 instead.
TEST(LRUKReplacerTest, EvictsLargestKDistanceFirst) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(3);  // t2
  r.RecordAccess(3);  // t3
  r.RecordAccess(2);  // t4
  r.RecordAccess(1);  // t5
  // 2nd most recent access: frame 1 -> t0, frame 2 -> t1, frame 3 -> t2
  for (frame_id_t f : {1, 2, 3}) {
    r.SetEvictable(f, true);
  }
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), 3);
}

// checks: only the LAST k accesses count. Tricky: frame 1 has the oldest access overall (t0),
// but its 2nd most recent access is t3, later than frame 2's (t1).
TEST(LRUKReplacerTest, OnlyLastKAccessesCount) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(2);  // t2
  r.RecordAccess(1);  // t3
  r.RecordAccess(1);  // t4
  r.SetEvictable(1, true);
  r.SetEvictable(2, true);
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), 1);
}

// checks: a mix of +inf and finite frames: all +inf first (oldest access first),
// then finite ones by k-distance.
TEST(LRUKReplacerTest, MixedInfiniteAndFiniteOrder) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(3);  // t2  frame 3: 1 access (+inf)
  r.RecordAccess(1);  // t3  frame 1: k-th recent = t0
  r.RecordAccess(2);  // t4  frame 2: k-th recent = t1
  r.RecordAccess(4);  // t5  frame 4: 1 access (+inf)
  for (frame_id_t f : {1, 2, 3, 4}) {
    r.SetEvictable(f, true);
  }
  EXPECT_EQ(r.Evict(), 3);
  EXPECT_EQ(r.Evict(), 4);
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), std::nullopt);
}

// checks: with k = 1 the policy is plain LRU (evict the least recently used).
TEST(LRUKReplacerTest, KEqualsOneIsLru) {
  LRUKReplacer r(7, 1);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(3);  // t2
  r.RecordAccess(1);  // t3
  for (frame_id_t f : {1, 2, 3}) {
    r.SetEvictable(f, true);
  }
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), 3);
  EXPECT_EQ(r.Evict(), 1);
}

// checks: SetEvictable toggles candidates and Size(); repeating the same value is not
// double-counted; a non-evictable frame is skipped even if it is the best victim.
TEST(LRUKReplacerTest, EvictableTogglingAndSize) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0 (best victim)
  r.RecordAccess(2);  // t1
  r.RecordAccess(3);  // t2
  for (frame_id_t f : {1, 2, 3}) {
    r.SetEvictable(f, true);
  }
  EXPECT_EQ(r.Size(), 3U);

  r.SetEvictable(1, false);
  EXPECT_EQ(r.Size(), 2U);
  EXPECT_EQ(r.Evict(), 2);  // 1 is skipped
  EXPECT_EQ(r.Size(), 1U);

  r.SetEvictable(1, true);
  r.SetEvictable(1, true);  // same value again: no change
  EXPECT_EQ(r.Size(), 2U);

  r.SetEvictable(3, false);
  r.SetEvictable(3, false);  // same value again: no change
  EXPECT_EQ(r.Size(), 1U);
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Evict(), std::nullopt);  // 3 is still tracked but not evictable
  EXPECT_EQ(r.Size(), 0U);
}

// checks: SetEvictable on a frame the replacer has never seen does nothing.
TEST(LRUKReplacerTest, SetEvictableOnUntrackedFrameIsNoop) {
  LRUKReplacer r(7, 2);
  r.SetEvictable(5, true);
  EXPECT_EQ(r.Size(), 0U);
  EXPECT_EQ(r.Evict(), std::nullopt);
}

// checks: after eviction a frame's history is gone: its next access starts fresh
// (+inf distance, non-evictable). Tricky: keeping the old history would evict frame 2 first.
TEST(LRUKReplacerTest, EvictedFrameStartsWithFreshHistory) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);  // t0
  r.RecordAccess(2);  // t1
  r.RecordAccess(1);  // t2
  r.RecordAccess(2);  // t3  frame 2: k-th recent = t1
  r.SetEvictable(1, true);
  EXPECT_EQ(r.Evict(), 1);

  r.RecordAccess(1);  // t4: fresh history -> +inf (stale history would give k-th recent = t2)
  EXPECT_EQ(r.Size(), 0U) << "a re-accessed frame must start non-evictable";
  r.SetEvictable(1, true);
  r.SetEvictable(2, true);
  EXPECT_EQ(r.Evict(), 1);
  EXPECT_EQ(r.Evict(), 2);
}

// checks: Remove drops an evictable frame so it is never returned by Evict.
TEST(LRUKReplacerTest, RemoveEvictableFrame) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);
  r.RecordAccess(2);
  r.SetEvictable(1, true);
  r.SetEvictable(2, true);
  r.Remove(1);
  EXPECT_EQ(r.Size(), 1U);
  EXPECT_EQ(r.Evict(), 2);
  EXPECT_EQ(r.Evict(), std::nullopt);
}

// checks: Remove on a frame that is not tracked does nothing.
TEST(LRUKReplacerTest, RemoveUntrackedFrameIsNoop) {
  LRUKReplacer r(7, 2);
  r.Remove(4);
  EXPECT_EQ(r.Size(), 0U);
}

// checks: Remove on a tracked but non-evictable (pinned) frame throws Invalid and keeps it.
TEST(LRUKReplacerTest, RemoveNonEvictableFrameThrows) {
  LRUKReplacer r(7, 2);
  r.RecordAccess(1);
  test::ExpectThrowsType([&] { r.Remove(1); }, ExceptionType::Invalid);
  r.SetEvictable(1, true);
  EXPECT_EQ(r.Size(), 1U) << "the failed Remove must not forget the frame";
}

// checks: frame ids outside [0, num_frames) are rejected with OutOfRange.
TEST(LRUKReplacerTest, InvalidFrameIdsThrow) {
  LRUKReplacer r(7, 2);
  test::ExpectThrowsType([&] { r.RecordAccess(7); }, ExceptionType::OutOfRange);
  test::ExpectThrowsType([&] { r.RecordAccess(-1); }, ExceptionType::OutOfRange);
  test::ExpectThrowsType([&] { r.SetEvictable(7, true); }, ExceptionType::OutOfRange);
  test::ExpectThrowsType([&] { r.Remove(7); }, ExceptionType::OutOfRange);
}

// checks: the replacer is thread-safe: 8 threads recording accesses and toggling
// evictability concurrently leave a consistent state (run under TSan).
TEST(LRUKReplacerTest, ConcurrentUseIsSafe) {
  constexpr int kThreads = 8;
  constexpr int kFramesPerThread = 100;
  LRUKReplacer r(kThreads * kFramesPerThread, 2);

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t] {
      for (int i = 0; i < kFramesPerThread; i++) {
        frame_id_t f = t * kFramesPerThread + i;
        r.RecordAccess(f);
        r.RecordAccess(f);
        r.RecordAccess(f);
        r.SetEvictable(f, true);
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }
  EXPECT_EQ(r.Size(), static_cast<std::size_t>(kThreads * kFramesPerThread));

  std::set<frame_id_t> evicted;
  for (int i = 0; i < kThreads * kFramesPerThread; i++) {
    auto f = r.Evict();
    ASSERT_TRUE(f.has_value());
    evicted.insert(*f);
  }
  EXPECT_EQ(evicted.size(), static_cast<std::size_t>(kThreads * kFramesPerThread)) << "a frame was evicted twice";
  EXPECT_EQ(r.Evict(), std::nullopt);
}

}  // namespace minitub
