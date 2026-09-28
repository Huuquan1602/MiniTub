#include "storage/disk/disk_manager.h"

#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "common/exception.h"
#include "test_util.h"

namespace minitub {

using test::PageBuf;
using test::TempFile;

// checks: a brand-new database file contains no pages.
TEST(DiskManagerTest, NewFileHasNoPages) {
  TempFile file;
  DiskManager dm(file.Path());
  EXPECT_EQ(dm.NumPages(), 0);
}

// checks: page ids are handed out as 0, 1, 2, ... and NumPages() follows them.
TEST(DiskManagerTest, AllocateGivesSequentialIds) {
  TempFile file;
  DiskManager dm(file.Path());
  EXPECT_EQ(dm.AllocatePage(), 0);
  EXPECT_EQ(dm.AllocatePage(), 1);
  EXPECT_EQ(dm.AllocatePage(), 2);
  EXPECT_EQ(dm.NumPages(), 3);
}

// checks: bytes written to a page are read back unchanged, and neighbours are untouched.
TEST(DiskManagerTest, WriteThenReadRoundTrip) {
  TempFile file;
  DiskManager dm(file.Path());
  for (int i = 0; i < 3; i++) {
    dm.AllocatePage();
  }
  PageBuf in;
  test::Fill(in.Span(), 7);
  dm.WritePage(1, in.Span());

  PageBuf out;
  dm.ReadPage(1, out.Span());
  EXPECT_EQ(in.bytes, out.bytes);

  dm.ReadPage(0, out.Span());
  EXPECT_TRUE(test::IsZero(out.Data())) << "page 0 changed by a write to page 1";
  dm.ReadPage(2, out.Span());
  EXPECT_TRUE(test::IsZero(out.Data())) << "page 2 changed by a write to page 1";
}

// checks: an allocated page that was never written reads as all zeros.
TEST(DiskManagerTest, NeverWrittenPageReadsAsZeros) {
  TempFile file;
  DiskManager dm(file.Path());
  dm.AllocatePage();
  PageBuf out;
  test::Fill(out.Span(), 99);  // garbage that must be overwritten
  dm.ReadPage(0, out.Span());
  EXPECT_TRUE(test::IsZero(out.Data()));
}

// checks: a second write to the same page fully replaces the first.
TEST(DiskManagerTest, OverwriteReplacesContent) {
  TempFile file;
  DiskManager dm(file.Path());
  dm.AllocatePage();
  PageBuf first;
  PageBuf second;
  test::Fill(first.Span(), 1);
  test::Fill(second.Span(), 2);
  dm.WritePage(0, first.Span());
  dm.WritePage(0, second.Span());

  PageBuf out;
  dm.ReadPage(0, out.Span());
  EXPECT_EQ(out.bytes, second.bytes);
}

// checks: page count and contents survive closing and reopening the file,
// and allocation continues after the existing pages.
TEST(DiskManagerTest, ReopenKeepsPagesAndData) {
  TempFile file;
  PageBuf p1;
  PageBuf p2;
  test::Fill(p1.Span(), 11);
  test::Fill(p2.Span(), 22);
  {
    DiskManager dm(file.Path());
    for (int i = 0; i < 3; i++) {
      dm.AllocatePage();
    }
    dm.WritePage(1, p1.Span());
    dm.WritePage(2, p2.Span());
  }  // closes the file

  DiskManager dm(file.Path());
  EXPECT_EQ(dm.NumPages(), 3);
  PageBuf out;
  dm.ReadPage(1, out.Span());
  EXPECT_EQ(out.bytes, p1.bytes);
  dm.ReadPage(2, out.Span());
  EXPECT_EQ(out.bytes, p2.bytes);
  dm.ReadPage(0, out.Span());
  EXPECT_TRUE(test::IsZero(out.Data()));
  EXPECT_EQ(dm.AllocatePage(), 3);
}

// checks: ids outside [0, NumPages()) are rejected with OutOfRange instead of
// silently reading/writing past the end of the file.
TEST(DiskManagerTest, OutOfRangePageIdsThrow) {
  TempFile file;
  DiskManager dm(file.Path());
  dm.AllocatePage();
  dm.AllocatePage();
  PageBuf buf;
  test::ExpectThrowsType([&] { dm.ReadPage(2, buf.Span()); }, ExceptionType::OutOfRange);
  test::ExpectThrowsType([&] { dm.WritePage(5, buf.Span()); }, ExceptionType::OutOfRange);
  test::ExpectThrowsType([&] { dm.ReadPage(INVALID_PAGE_ID, buf.Span()); }, ExceptionType::OutOfRange);
}

// checks: the reads/writes counters count every ReadPage/WritePage call (used by E2).
TEST(DiskManagerTest, StatsCountReadsAndWrites) {
  TempFile file;
  DiskManager dm(file.Path());
  dm.AllocatePage();
  PageBuf buf;
  dm.WritePage(0, buf.Span());
  dm.WritePage(0, buf.Span());
  dm.ReadPage(0, buf.Span());
  EXPECT_EQ(dm.GetStats().writes, 2U);
  EXPECT_EQ(dm.GetStats().reads, 1U);
}

// checks: DiskManager is thread-safe: 8 threads writing and reading their own
// pages at the same time never see each other's data (run under TSan too).
TEST(DiskManagerTest, ConcurrentAccessToDifferentPages) {
  constexpr int kThreads = 8;
  constexpr int kPagesPerThread = 16;
  TempFile file;
  DiskManager dm(file.Path());
  for (int i = 0; i < kThreads * kPagesPerThread; i++) {
    dm.AllocatePage();
  }

  std::atomic<int> mismatches{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t] {
      for (int round = 0; round < 5; round++) {
        for (int i = 0; i < kPagesPerThread; i++) {
          page_id_t id = t * kPagesPerThread + i;
          PageBuf in;
          test::Fill(in.Span(), static_cast<std::uint32_t>(id * 10 + round));
          dm.WritePage(id, in.Span());
          PageBuf out;
          dm.ReadPage(id, out.Span());
          if (in.bytes != out.bytes) {
            mismatches++;
          }
        }
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }
  EXPECT_EQ(mismatches.load(), 0);
  EXPECT_EQ(dm.NumPages(), kThreads * kPagesPerThread);
}

// checks: IoMode::Direct (O_DIRECT) round-trips an aligned page.
// Skipped when the filesystem does not support O_DIRECT (DiskManager throws Io).
TEST(DiskManagerTest, DirectIoRoundTrip) {
  TempFile file;
  std::unique_ptr<DiskManager> dm;
  try {
    dm = std::make_unique<DiskManager>(file.Path(), IoMode::Direct);
  } catch (const Exception &e) {
    if (e.GetType() == ExceptionType::Io) {
      GTEST_SKIP() << "O_DIRECT not supported here: " << e.what();
    }
    throw;
  }
  dm->AllocatePage();
  PageBuf in;
  test::Fill(in.Span(), 5);
  dm->WritePage(0, in.Span());
  PageBuf out;
  dm->ReadPage(0, out.Span());
  EXPECT_EQ(in.bytes, out.bytes);
}

}  // namespace minitub
