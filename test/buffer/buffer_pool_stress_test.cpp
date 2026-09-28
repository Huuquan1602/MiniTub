// Concurrency stress test for BufferPoolManager. Must pass under TSan (ctest --preset tsan).

#include <atomic>
#include <cstdint>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "buffer/buffer_pool_manager.h"
#include "storage/disk/disk_manager.h"
#include "test_util.h"

namespace minitub {

using test::GetU64;
using test::kCounterOffset;
using test::kStampOffset;
using test::PutU64;
using test::TempFile;

// checks: 8 threads x 10k random fetch/new/unpin operations on a 16-frame pool:
//  - no frame mix-ups: every fetched page carries its own id as a stamp;
//  - no lost writes: per-page counters (incremented under the page write latch) match;
//  - no spurious "no frame": each thread pins at most one page and 16 > 8, so a free or
//    evictable frame always exists and NewPage/FetchPage must never return nullptr;
//  - no duplicate page ids from concurrent NewPage;
//  - no frame leak: afterwards all 16 frames can be pinned at once;
//  - no data races (TSan).
TEST(BufferPoolStressTest, EightThreadsRandomFetchNewUnpin) {
  constexpr std::size_t kPoolSize = 16;
  constexpr int kThreads = 8;
  constexpr int kOpsPerThread = 10'000;
  constexpr int kInitialPages = 64;
  constexpr int kMaxPages = kInitialPages + kThreads * kOpsPerThread;

  TempFile file;
  DiskManager dm(file.Path());
  BufferPoolManager bpm(kPoolSize, &dm, 2);

  // ready[id]: page `id` exists and is stamped, so other threads may fetch it.
  std::vector<std::atomic<bool>> ready(kMaxPages);
  std::vector<std::atomic<std::uint64_t>> expected_count(kMaxPages);
  std::atomic<int> known_pages{0};  // upper bound on ids handed out so far
  std::atomic<int> failures{0};

  auto create_page = [&]() -> bool {
    Page *p = bpm.NewPage();
    if (p == nullptr) {
      return false;
    }
    page_id_t id = p->GetPageId();
    PutU64(p->GetData(), kStampOffset, static_cast<std::uint64_t>(id));
    PutU64(p->GetData(), kCounterOffset, 0);
    if (!bpm.UnpinPage(id, true) || id < 0 || id >= kMaxPages || ready[id].exchange(true)) {
      return false;  // bad unpin, id out of range, or the same id handed out twice
    }
    int seen = known_pages.load();
    while (seen < id + 1 && !known_pages.compare_exchange_weak(seen, id + 1)) {
    }
    return true;
  };

  for (int i = 0; i < kInitialPages; i++) {
    ASSERT_TRUE(create_page());
  }

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t] {
      std::mt19937 rng(42 + t);
      for (int op = 0; op < kOpsPerThread; op++) {
        int kind = static_cast<int>(rng() % 10);
        if (kind >= 8) {  // 20%: new page
          failures += create_page() ? 0 : 1;
          continue;
        }
        page_id_t id = static_cast<page_id_t>(rng() % known_pages.load());
        if (!ready[id].load()) {
          continue;  // allocated by another thread but not stamped yet
        }
        Page *p = bpm.FetchPage(id);
        if (p == nullptr || p->GetPageId() != id) {
          failures++;
          continue;
        }
        if (kind < 5) {  // 50%: read and check the stamp
          p->RLatch();
          failures += GetU64(p->GetData(), kStampOffset) == static_cast<std::uint64_t>(id) ? 0 : 1;
          p->RUnlatch();
          failures += bpm.UnpinPage(id, false) ? 0 : 1;
        } else {  // 30%: increment the counter
          p->WLatch();
          failures += GetU64(p->GetData(), kStampOffset) == static_cast<std::uint64_t>(id) ? 0 : 1;
          PutU64(p->GetData(), kCounterOffset, GetU64(p->GetData(), kCounterOffset) + 1);
          expected_count[id]++;
          p->WUnlatch();
          failures += bpm.UnpinPage(id, true) ? 0 : 1;
        }
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }
  EXPECT_EQ(failures.load(), 0);

  // Every page: right stamp, no lost increments.
  for (int id = 0; id < known_pages.load(); id++) {
    if (!ready[id].load()) {
      continue;
    }
    Page *p = bpm.FetchPage(id);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(GetU64(p->GetData(), kStampOffset), static_cast<std::uint64_t>(id));
    EXPECT_EQ(GetU64(p->GetData(), kCounterOffset), expected_count[id].load()) << "page " << id;
    ASSERT_TRUE(bpm.UnpinPage(id, false));
  }

  // No frame leak: all frames can be pinned at the same time.
  for (std::size_t i = 0; i < kPoolSize; i++) {
    ASSERT_NE(bpm.NewPage(), nullptr) << "frame leak: only " << i << " frames usable";
  }
}

}  // namespace minitub
