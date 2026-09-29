#include "storage/disk/disk_manager.h"

#include <fcntl.h>     // open, O_* flags
#include <sys/stat.h>  // fstat
#include <unistd.h>    // pread, pwrite, ftruncate, close

#include <cerrno>
#include <limits>
#include <string>
#include <system_error>

#include "common/exception.h"

namespace minitub {

namespace {

// Byte offset of a page in the file. Widen to 64-bit off_t BEFORE multiplying:
// page_id_t is 32-bit, and 32-bit math overflows past page 524287 (2 GiB).
auto PageOffset(page_id_t page_id) -> off_t { return static_cast<off_t>(page_id) * static_cast<off_t>(PAGE_SIZE); }

// Throws Exception(Io) describing the current errno. std::system_category().message()
// is used instead of strerror() because strerror() is not thread-safe.
[[noreturn]] void ThrowIo(const std::string &what, const std::filesystem::path &path) {
  int err = errno;
  throw Exception(ExceptionType::Io, what + " '" + path.string() + "': " + std::system_category().message(err));
}

void CheckPageId(page_id_t page_id, page_id_t num_pages) {
  if (page_id < 0 || page_id >= num_pages) {
    throw Exception(ExceptionType::OutOfRange,
                    "page " + std::to_string(page_id) + " not allocated (file has " + std::to_string(num_pages) + ")");
  }
}

}  // namespace

DiskManager::DiskManager(const std::filesystem::path &file, IoMode mode) : path_(file) {
  // O_CLOEXEC: do not leak the descriptor into child processes (good habit, no cost).
  int flags = O_RDWR | O_CREAT | O_CLOEXEC;
  if (mode == IoMode::Direct) {
    flags |= O_DIRECT;  // bypass the OS page cache; buffers/offsets/sizes must be 4096-aligned
  }
  fd_ = ::open(path_.c_str(), flags, 0644);
  if (fd_ < 0) {
    ThrowIo("cannot open", path_);  // e.g. EINVAL: filesystem without O_DIRECT (tmpfs)
  }

  struct stat st{};
  if (::fstat(fd_, &st) != 0) {
    // The destructor does not run when a constructor throws, so close here ourselves.
    int err = errno;
    ::close(fd_);
    errno = err;
    ThrowIo("cannot stat", path_);
  }
  // A torn last page (size not a multiple of PAGE_SIZE) is ignored: it was never fully allocated.
  num_pages_ = static_cast<page_id_t>(st.st_size / static_cast<off_t>(PAGE_SIZE));
}

DiskManager::~DiskManager() {
  if (fd_ >= 0) {
    ::close(fd_);  // errors ignored: destructors must not throw
  }
}

void DiskManager::ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> out) {
  CheckPageId(page_id, num_pages_.load());
  const off_t base = PageOffset(page_id);
  std::size_t done = 0;
  // pread may return fewer bytes than asked (a "short read"), so loop until the page is full.
  while (done < PAGE_SIZE) {
    ssize_t n = ::pread(fd_, out.data() + done, PAGE_SIZE - done, base + static_cast<off_t>(done));
    if (n < 0) {
      if (errno == EINTR) {
        continue;  // interrupted by a signal before reading anything: just retry
      }
      ThrowIo("cannot read page " + std::to_string(page_id) + " of", path_);
    }
    if (n == 0) {
      // End of file inside an allocated page: AllocatePage always grows the file, so the
      // file must have been truncated behind our back.
      throw Exception(ExceptionType::Io, "unexpected end of file in page " + std::to_string(page_id));
    }
    done += static_cast<std::size_t>(n);
  }
  num_reads_++;
}

void DiskManager::WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> data) {
  CheckPageId(page_id, num_pages_.load());
  const off_t base = PageOffset(page_id);
  std::size_t done = 0;
  while (done < PAGE_SIZE) {  // same short-write loop as ReadPage
    ssize_t n = ::pwrite(fd_, data.data() + done, PAGE_SIZE - done, base + static_cast<off_t>(done));
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      ThrowIo("cannot write page " + std::to_string(page_id) + " of", path_);  // e.g. ENOSPC: disk full
    }
    if (n == 0) {
      throw Exception(ExceptionType::Io, "write made no progress for page " + std::to_string(page_id));
    }
    done += static_cast<std::size_t>(n);
  }
  num_writes_++;
  // No fsync here: the data may sit in the OS page cache until the OS writes it out.
  // Durability (fsync / WAL) is M9's job.
}

auto DiskManager::AllocatePage() -> page_id_t {
  // Read-grow-publish must be one step, or two threads could get the same id.
  std::scoped_lock lock(latch_);
  const page_id_t page_id = num_pages_.load();
  if (page_id == std::numeric_limits<page_id_t>::max()) {
    throw Exception(ExceptionType::OutOfRange, "database has reached the maximum page count");
  }
  const page_id_t next_page_count = page_id + 1;
  // ftruncate extends the file with zeros, so a new page reads back as zeros.
  if (::ftruncate(fd_, PageOffset(next_page_count)) != 0) {
    ThrowIo("cannot grow", path_);
  }
  // Publish the new count only after the file really is longer, so a reader that sees
  // page_id as valid can also read its bytes.
  num_pages_.store(next_page_count);
  return page_id;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
  CheckPageId(page_id, num_pages_.load());
  std::scoped_lock lock(latch_);
  deallocated_.insert(page_id);  // M1: remembered only; a free-page list could reuse these later
}

auto DiskManager::NumPages() const -> page_id_t { return num_pages_.load(); }

}  // namespace minitub
