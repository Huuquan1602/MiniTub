#include "buffer/buffer_pool_manager.h"

#include <memory>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "storage/disk/disk_manager.h"
#include "test_util.h"

namespace minitub {

using test::GetU64;
using test::kStampOffset;
using test::PageBuf;
using test::PutU64;
using test::TempFile;

class BufferPoolManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { dm_ = std::make_unique<DiskManager>(file_.Path()); }

  auto MakeBpm(std::size_t pool_size) -> std::unique_ptr<BufferPoolManager> {
    return std::make_unique<BufferPoolManager>(pool_size, dm_.get(), 2);
  }

  /** Reads a page straight from the file, bypassing the buffer pool. */
  auto ReadFromDisk(page_id_t id) -> PageBuf {
    PageBuf buf;
    dm_->ReadPage(id, buf.Span());
    return buf;
  }

  TempFile file_;
  std::unique_ptr<DiskManager> dm_;
};

// checks: NewPage returns a zeroed, clean page pinned exactly once, with ids 0, 1, ...
TEST_F(BufferPoolManagerTest, NewPageIsZeroedPinnedAndClean) {
  auto bpm = MakeBpm(3);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->GetPageId(), 0);
  EXPECT_EQ(p->GetPinCount(), 1);
  EXPECT_FALSE(p->IsDirty());
  EXPECT_TRUE(test::IsZero(p->GetData()));

  Page *q = bpm->NewPage();
  ASSERT_NE(q, nullptr);
  EXPECT_EQ(q->GetPageId(), 1);
}

// checks: when every frame holds a pinned page there is no frame: NewPage/FetchPage return
// nullptr, and a failed NewPage does not allocate a page id. Unpinning one page frees a frame.
TEST_F(BufferPoolManagerTest, PoolFullOfPinnedPagesHasNoFrame) {
  auto bpm = MakeBpm(3);
  for (int i = 0; i < 3; i++) {
    ASSERT_NE(bpm->NewPage(), nullptr);
  }
  EXPECT_EQ(bpm->FreeFrameCount(), 0U);
  EXPECT_EQ(bpm->NewPage(), nullptr);
  EXPECT_EQ(dm_->NumPages(), 3) << "a failed NewPage must not allocate a page id";

  ASSERT_TRUE(bpm->UnpinPage(0, false));
  Page *p = bpm->NewPage();  // evicts page 0
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->GetPageId(), 3);

  EXPECT_EQ(bpm->FetchPage(0), nullptr) << "pages 1, 2, 3 are pinned; page 0 cannot be loaded";
}

// checks: fetching a resident page returns the same frame, adds a pin, and counts a hit.
TEST_F(BufferPoolManagerTest, FetchResidentPageIsHitAndAddsPin) {
  auto bpm = MakeBpm(3);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  Page *again = bpm->FetchPage(0);
  EXPECT_EQ(again, p);
  EXPECT_EQ(p->GetPinCount(), 2);
  EXPECT_EQ(bpm->GetStats().hits, 1U);
  EXPECT_EQ(bpm->GetStats().misses, 0U);
}

// checks: a page pinned twice needs two unpins before its frame can be reused.
TEST_F(BufferPoolManagerTest, PinnedTwiceNeedsTwoUnpins) {
  auto bpm = MakeBpm(1);
  ASSERT_NE(bpm->NewPage(), nullptr);     // pin 1
  ASSERT_NE(bpm->FetchPage(0), nullptr);  // pin 2
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  EXPECT_EQ(bpm->NewPage(), nullptr) << "page 0 still has one pin";
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  EXPECT_NE(bpm->NewPage(), nullptr);
}

// checks: UnpinPage fails for a page that is not resident and for a pin count already at 0.
TEST_F(BufferPoolManagerTest, UnpinRules) {
  auto bpm = MakeBpm(3);
  EXPECT_FALSE(bpm->UnpinPage(42, false)) << "page 42 is not resident";
  ASSERT_NE(bpm->NewPage(), nullptr);
  EXPECT_TRUE(bpm->UnpinPage(0, false));
  EXPECT_FALSE(bpm->UnpinPage(0, false)) << "pin count is already 0";
}

// checks: the dirty flag is sticky: a later UnpinPage(is_dirty=false) does not clear it.
TEST_F(BufferPoolManagerTest, DirtyFlagIsSticky) {
  auto bpm = MakeBpm(3);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  ASSERT_TRUE(bpm->UnpinPage(0, true));
  ASSERT_EQ(bpm->FetchPage(0), p);
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  EXPECT_TRUE(p->IsDirty());
}

// checks: evicting a dirty page writes it to disk first; fetching it again reads it back (a miss).
TEST_F(BufferPoolManagerTest, DirtyPageIsWrittenOnEviction) {
  auto bpm = MakeBpm(1);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  PutU64(p->GetData(), kStampOffset, 0xABCD);
  ASSERT_TRUE(bpm->UnpinPage(0, true));

  auto writes_before = dm_->GetStats().writes;
  ASSERT_NE(bpm->NewPage(), nullptr);  // page 1 evicts page 0
  EXPECT_EQ(dm_->GetStats().writes, writes_before + 1);
  EXPECT_EQ(bpm->GetStats().evictions, 1U);
  EXPECT_EQ(GetU64(ReadFromDisk(0).Data(), kStampOffset), 0xABCDU);

  ASSERT_TRUE(bpm->UnpinPage(1, false));
  Page *again = bpm->FetchPage(0);
  ASSERT_NE(again, nullptr);
  EXPECT_EQ(GetU64(again->GetData(), kStampOffset), 0xABCDU);
  EXPECT_EQ(bpm->GetStats().misses, 1U);
}

// checks: evicting a clean page does not write anything (no wasted I/O).
TEST_F(BufferPoolManagerTest, CleanPageIsNotWrittenOnEviction) {
  auto bpm = MakeBpm(1);
  ASSERT_NE(bpm->NewPage(), nullptr);
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  auto writes_before = dm_->GetStats().writes;
  ASSERT_NE(bpm->NewPage(), nullptr);
  EXPECT_EQ(dm_->GetStats().writes, writes_before);
  EXPECT_EQ(bpm->GetStats().evictions, 1U);
}

// checks: FlushPage writes the page (dirty or not) and clears the dirty flag;
// it fails for a page that is not resident.
TEST_F(BufferPoolManagerTest, FlushPageWritesAndClearsDirty) {
  auto bpm = MakeBpm(3);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  PutU64(p->GetData(), kStampOffset, 77);
  ASSERT_TRUE(bpm->UnpinPage(0, true));

  auto writes_before = dm_->GetStats().writes;
  EXPECT_TRUE(bpm->FlushPage(0));
  EXPECT_EQ(dm_->GetStats().writes, writes_before + 1);
  EXPECT_FALSE(p->IsDirty());
  EXPECT_EQ(GetU64(ReadFromDisk(0).Data(), kStampOffset), 77U);

  EXPECT_TRUE(bpm->FlushPage(0)) << "flushing a clean resident page still succeeds";
  EXPECT_EQ(dm_->GetStats().writes, writes_before + 2);
  EXPECT_FALSE(bpm->FlushPage(99));
}

// checks: FlushAllPages writes every resident page to disk.
TEST_F(BufferPoolManagerTest, FlushAllPagesWritesEveryPage) {
  auto bpm = MakeBpm(5);
  for (int i = 0; i < 5; i++) {
    Page *p = bpm->NewPage();
    ASSERT_NE(p, nullptr);
    PutU64(p->GetData(), kStampOffset, 100 + i);
    ASSERT_TRUE(bpm->UnpinPage(i, true));
  }
  bpm->FlushAllPages();
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(GetU64(ReadFromDisk(i).Data(), kStampOffset), static_cast<std::uint64_t>(100 + i));
  }
}

// checks: a pinned page cannot be deleted; once unpinned it can, and its frame returns to
// the free list. Deleting a page that is not resident succeeds.
TEST_F(BufferPoolManagerTest, DeletePinnedPageFails) {
  auto bpm = MakeBpm(3);
  ASSERT_NE(bpm->NewPage(), nullptr);
  EXPECT_EQ(bpm->FreeFrameCount(), 2U);
  EXPECT_FALSE(bpm->DeletePage(0));
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  EXPECT_TRUE(bpm->DeletePage(0));
  EXPECT_EQ(bpm->FreeFrameCount(), 3U);
  EXPECT_TRUE(bpm->DeletePage(0)) << "not resident any more: nothing to do";
}

// checks: deleting a dirty page does not write it back (the data is being thrown away).
TEST_F(BufferPoolManagerTest, DeletedDirtyPageIsNotWritten) {
  auto bpm = MakeBpm(3);
  Page *p = bpm->NewPage();
  ASSERT_NE(p, nullptr);
  PutU64(p->GetData(), kStampOffset, 5);
  ASSERT_TRUE(bpm->UnpinPage(0, true));
  auto writes_before = dm_->GetStats().writes;
  EXPECT_TRUE(bpm->DeletePage(0));
  EXPECT_EQ(dm_->GetStats().writes, writes_before);
}

// checks: hits, misses and evictions are counted correctly (used by E1/E2).
TEST_F(BufferPoolManagerTest, StatsCountHitsMissesAndEvictions) {
  auto bpm = MakeBpm(2);
  ASSERT_NE(bpm->NewPage(), nullptr);  // page 0
  ASSERT_NE(bpm->NewPage(), nullptr);  // page 1
  ASSERT_TRUE(bpm->UnpinPage(0, true));
  ASSERT_TRUE(bpm->UnpinPage(1, true));

  ASSERT_NE(bpm->FetchPage(0), nullptr);  // hit
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  ASSERT_NE(bpm->NewPage(), nullptr);  // page 2: evicts one page
  ASSERT_TRUE(bpm->UnpinPage(2, false));

  // Pages 0 and 1 cannot both be resident any more, so fetching both is at least one miss.
  ASSERT_NE(bpm->FetchPage(0), nullptr);
  ASSERT_TRUE(bpm->UnpinPage(0, false));
  ASSERT_NE(bpm->FetchPage(1), nullptr);
  ASSERT_TRUE(bpm->UnpinPage(1, false));

  auto stats = bpm->GetStats();
  EXPECT_GE(stats.hits, 1U);
  EXPECT_GE(stats.misses, 1U);
  EXPECT_EQ(stats.hits + stats.misses, 3U) << "every FetchPage is exactly one hit or one miss";
  EXPECT_EQ(stats.evictions, 1U + stats.misses) << "each miss and the NewPage needed one eviction";
}

// checks: a pool of 10 frames serves 1000 pages correctly: every page keeps its own
// content through many evictions (sequential, then random access).
TEST_F(BufferPoolManagerTest, TenFramesServeThousandPages) {
  constexpr int kPages = 1000;
  auto bpm = MakeBpm(10);
  for (int i = 0; i < kPages; i++) {
    Page *p = bpm->NewPage();
    ASSERT_NE(p, nullptr) << "at page " << i;
    ASSERT_EQ(p->GetPageId(), i);
    PutU64(p->GetData(), kStampOffset, static_cast<std::uint64_t>(i));
    ASSERT_TRUE(bpm->UnpinPage(i, true));
  }
  for (int i = 0; i < kPages; i++) {
    Page *p = bpm->FetchPage(i);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(GetU64(p->GetData(), kStampOffset), static_cast<std::uint64_t>(i));
    ASSERT_TRUE(bpm->UnpinPage(i, false));
  }
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> pick(0, kPages - 1);
  for (int n = 0; n < 2000; n++) {
    int id = pick(rng);
    Page *p = bpm->FetchPage(id);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(GetU64(p->GetData(), kStampOffset), static_cast<std::uint64_t>(id));
    ASSERT_TRUE(bpm->UnpinPage(id, false));
  }
  EXPECT_GE(bpm->GetStats().evictions, static_cast<std::uint64_t>(kPages - 10));
}

// checks: after many new/fetch/unpin/delete operations no frame is leaked: all 10 frames
// can be pinned at once, and after deleting everything all 10 are back on the free list.
TEST_F(BufferPoolManagerTest, NoFrameLeakAfterManyOperations) {
  constexpr std::size_t kPool = 10;
  auto bpm = MakeBpm(kPool);
  std::mt19937 rng(7);
  std::vector<page_id_t> live;
  for (int n = 0; n < 500; n++) {
    int op = static_cast<int>(rng() % 3);
    if (op == 0 || live.empty()) {
      Page *p = bpm->NewPage();
      ASSERT_NE(p, nullptr);
      live.push_back(p->GetPageId());
      ASSERT_TRUE(bpm->UnpinPage(p->GetPageId(), true));
    } else if (op == 1) {
      page_id_t id = live[rng() % live.size()];
      ASSERT_NE(bpm->FetchPage(id), nullptr);
      ASSERT_TRUE(bpm->UnpinPage(id, false));
    } else {
      std::size_t idx = rng() % live.size();
      ASSERT_TRUE(bpm->DeletePage(live[idx]));
      live.erase(live.begin() + static_cast<std::ptrdiff_t>(idx));
    }
  }

  std::vector<page_id_t> pinned;
  for (std::size_t i = 0; i < kPool; i++) {
    Page *p = bpm->NewPage();
    ASSERT_NE(p, nullptr) << "frame leaked: only " << i << " of " << kPool << " frames usable";
    pinned.push_back(p->GetPageId());
  }
  EXPECT_EQ(bpm->NewPage(), nullptr);

  for (page_id_t id : pinned) {
    ASSERT_TRUE(bpm->UnpinPage(id, false));
    ASSERT_TRUE(bpm->DeletePage(id));
  }
  for (page_id_t id : live) {
    ASSERT_TRUE(bpm->DeletePage(id));
  }
  EXPECT_EQ(bpm->FreeFrameCount(), kPool);
}

}  // namespace minitub
