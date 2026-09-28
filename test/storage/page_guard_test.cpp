#include "storage/page/page_guard.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <utility>
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
using namespace std::chrono_literals;

class PageGuardTest : public ::testing::Test {
 protected:
  void SetUp() override { dm_ = std::make_unique<DiskManager>(file_.Path()); }

  auto MakeBpm(std::size_t pool_size) -> std::unique_ptr<BufferPoolManager> {
    return std::make_unique<BufferPoolManager>(pool_size, dm_.get(), 2);
  }

  /** Creates `n` pages (0 .. n-1) and unpins them; returns their frames (valid while resident). */
  auto CreatePages(BufferPoolManager &bpm, int n) -> std::vector<Page *> {
    std::vector<Page *> pages;
    for (int i = 0; i < n; i++) {
      Page *p = bpm.NewPage();
      EXPECT_NE(p, nullptr);
      pages.push_back(p);
      bpm.UnpinPage(i, false);
    }
    return pages;
  }

  TempFile file_;
  std::unique_ptr<DiskManager> dm_;
};

// checks: a default-constructed guard is empty and Drop() on it is harmless.
TEST_F(PageGuardTest, DefaultGuardsAreEmpty) {
  BasicPageGuard basic;
  ReadPageGuard read;
  WritePageGuard write;
  EXPECT_FALSE(basic.IsValid());
  EXPECT_FALSE(read.IsValid());
  EXPECT_FALSE(write.IsValid());
  basic.Drop();
  read.Drop();
  write.Drop();
}

// checks: a guard unpins its page when it goes out of scope.
TEST_F(PageGuardTest, GuardUnpinsOnDestruction) {
  auto bpm = MakeBpm(1);
  {
    BasicPageGuard g = bpm->NewPageGuarded();
    ASSERT_TRUE(g.IsValid());
    EXPECT_EQ(g.PageId(), 0);
    EXPECT_EQ(bpm->NewPage(), nullptr) << "the guard holds the only frame";
  }
  EXPECT_NE(bpm->NewPage(), nullptr) << "the guard did not unpin on destruction";
}

// checks: move construction transfers the single pin; the moved-from guard is empty and
// neither guard unpins twice.
TEST_F(PageGuardTest, MoveConstructionTransfersPin) {
  auto bpm = MakeBpm(2);
  Page *p = CreatePages(*bpm, 1)[0];

  BasicPageGuard g1 = bpm->FetchPageBasic(0);
  EXPECT_EQ(p->GetPinCount(), 1);
  BasicPageGuard g2(std::move(g1));
  EXPECT_FALSE(g1.IsValid());  // NOLINT(bugprone-use-after-move): testing the moved-from state
  EXPECT_TRUE(g2.IsValid());
  EXPECT_EQ(p->GetPinCount(), 1) << "moving must not add or drop a pin";

  g1.Drop();  // NOLINT(bugprone-use-after-move)
  EXPECT_EQ(p->GetPinCount(), 1) << "the moved-from guard must not unpin";
  g2.Drop();
  EXPECT_EQ(p->GetPinCount(), 0);
  g2.Drop();  // second Drop is a no-op
  EXPECT_EQ(p->GetPinCount(), 0);
}

// checks: move assignment first releases the page the target held, then takes the new one.
TEST_F(PageGuardTest, MoveAssignmentReleasesOldPage) {
  auto bpm = MakeBpm(3);
  auto pages = CreatePages(*bpm, 2);

  BasicPageGuard ga = bpm->FetchPageBasic(0);
  BasicPageGuard gb = bpm->FetchPageBasic(1);
  ga = std::move(gb);
  EXPECT_EQ(pages[0]->GetPinCount(), 0) << "page 0 was leaked by move assignment";
  EXPECT_EQ(pages[1]->GetPinCount(), 1);
  EXPECT_EQ(ga.PageId(), 1);
  EXPECT_FALSE(gb.IsValid());  // NOLINT(bugprone-use-after-move)
}

// checks: read and write guards move correctly too (pin and latch travel with the guard).
TEST_F(PageGuardTest, ReadAndWriteGuardsMove) {
  auto bpm = MakeBpm(2);
  Page *p = CreatePages(*bpm, 1)[0];
  {
    ReadPageGuard r1 = bpm->FetchPageRead(0);
    ReadPageGuard r2 = std::move(r1);
    EXPECT_TRUE(r2.IsValid());
    EXPECT_EQ(p->GetPinCount(), 1);
  }
  {
    WritePageGuard w1 = bpm->FetchPageWrite(0);  // would deadlock if the read latch leaked
    WritePageGuard w2 = std::move(w1);
    EXPECT_TRUE(w2.IsValid());
    EXPECT_EQ(p->GetPinCount(), 1);
  }
  EXPECT_EQ(p->GetPinCount(), 0);
}

// checks: a fetch that finds no frame returns an empty guard instead of crashing.
TEST_F(PageGuardTest, FailedFetchGivesEmptyGuard) {
  auto bpm = MakeBpm(1);
  ASSERT_NE(bpm->NewPage(), nullptr);
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  ASSERT_NE(bpm->NewPage(), nullptr);  // page 1 now pins the only frame
  EXPECT_FALSE(bpm->FetchPageRead(0).IsValid());
  EXPECT_FALSE(bpm->FetchPageWrite(0).IsValid());
  EXPECT_FALSE(bpm->FetchPageBasic(0).IsValid());
}

// checks: only GetDataMut() marks a page dirty; read-only access leaves it clean.
TEST_F(PageGuardTest, DirtyFlagFollowsMutableAccess) {
  auto bpm = MakeBpm(3);
  auto pages = CreatePages(*bpm, 3);
  {
    ReadPageGuard r = bpm->FetchPageRead(0);
    (void)r.GetData();
  }
  EXPECT_FALSE(pages[0]->IsDirty()) << "a read guard must not dirty the page";
  {
    BasicPageGuard b = bpm->FetchPageBasic(1);
    (void)b.GetData();
  }
  EXPECT_FALSE(pages[1]->IsDirty()) << "GetData() alone must not dirty the page";
  {
    BasicPageGuard b = bpm->FetchPageBasic(1);
    PutU64(b.GetDataMut(), kStampOffset, 11);
  }
  EXPECT_TRUE(pages[1]->IsDirty());
  {
    WritePageGuard w = bpm->FetchPageWrite(2);
    PutU64(w.GetDataMut(), kStampOffset, 22);
  }
  EXPECT_TRUE(pages[2]->IsDirty());
}

// checks: data written through a write guard survives eviction (the dirty flag reached the pool).
TEST_F(PageGuardTest, WriteGuardDataSurvivesEviction) {
  auto bpm = MakeBpm(1);
  CreatePages(*bpm, 1);
  {
    WritePageGuard w = bpm->FetchPageWrite(0);
    PutU64(w.GetDataMut(), kStampOffset, 0xBEEF);
  }
  {
    BasicPageGuard other = bpm->NewPageGuarded();
  }  // evicts page 0
  ReadPageGuard r = bpm->FetchPageRead(0);
  ASSERT_TRUE(r.IsValid());
  EXPECT_EQ(GetU64(r.GetData(), kStampOffset), 0xBEEFU);
}

// checks: read guards share the latch: two readers of one page can coexist
// (an exclusive latch here would deadlock and hit the test timeout).
TEST_F(PageGuardTest, ReadGuardsShareTheLatch) {
  auto bpm = MakeBpm(2);
  Page *p = CreatePages(*bpm, 1)[0];
  ReadPageGuard r1 = bpm->FetchPageRead(0);
  ReadPageGuard r2 = bpm->FetchPageRead(0);
  EXPECT_TRUE(r1.IsValid());
  EXPECT_TRUE(r2.IsValid());
  EXPECT_EQ(p->GetPinCount(), 2);
}

// checks: a write guard excludes readers until it is dropped, then the reader proceeds.
TEST_F(PageGuardTest, WriteGuardBlocksReadersUntilDropped) {
  auto bpm = MakeBpm(2);
  CreatePages(*bpm, 1);
  WritePageGuard w = bpm->FetchPageWrite(0);

  std::atomic<bool> reader_in{false};
  std::thread reader([&] {
    ReadPageGuard r = bpm->FetchPageRead(0);
    reader_in = true;
  });
  std::this_thread::sleep_for(100ms);
  EXPECT_FALSE(reader_in.load()) << "reader got in while the write latch was held";
  w.Drop();
  reader.join();
  EXPECT_TRUE(reader_in.load());
}

// checks: guards under contention keep data consistent. 4 threads increment per-page
// counters through write guards on a 2-frame pool, forcing constant eviction and reuse.
// Catches lost updates, missing dirty flags, pin leaks, and latch/unpin ordering bugs
// (the latch must be released before the unpin, or a frame can be reused while latched).
// Timing-dependent: a pass is strong evidence, not proof.
TEST_F(PageGuardTest, WriteGuardsUnderContentionLoseNoUpdates) {
  constexpr int kPages = 8;
  constexpr int kThreads = 4;
  constexpr int kIncrementsPerThread = 2000;
  auto bpm = MakeBpm(2);
  CreatePages(*bpm, kPages);

  std::vector<std::atomic<std::uint64_t>> expected(kPages);
  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t] {
      std::mt19937 rng(100 + t);
      for (int i = 0; i < kIncrementsPerThread; i++) {
        page_id_t id = static_cast<page_id_t>(rng() % kPages);
        WritePageGuard w = bpm->FetchPageWrite(id);
        while (!w.IsValid()) {  // both frames pinned by other threads: retry
          std::this_thread::yield();
          w = bpm->FetchPageWrite(id);
        }
        std::byte *data = w.GetDataMut();
        PutU64(data, kCounterOffset, GetU64(data, kCounterOffset) + 1);
        expected[id]++;
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }

  std::uint64_t total = 0;
  for (int id = 0; id < kPages; id++) {
    ReadPageGuard r = bpm->FetchPageRead(id);
    ASSERT_TRUE(r.IsValid());
    EXPECT_EQ(GetU64(r.GetData(), kCounterOffset), expected[id].load()) << "page " << id;
    total += GetU64(r.GetData(), kCounterOffset);
  }
  EXPECT_EQ(total, static_cast<std::uint64_t>(kThreads * kIncrementsPerThread));
}

}  // namespace minitub
