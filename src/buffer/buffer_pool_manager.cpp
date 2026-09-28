#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"

namespace minitub {

BufferPoolManager::BufferPoolManager(std::size_t /*pool_size*/, DiskManager * /*disk_manager*/,
                                     std::size_t /*replacer_k*/, int /*disk_workers*/) {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::BufferPoolManager");
}

BufferPoolManager::~BufferPoolManager() {
  // TODO(M1): decide what happens to dirty pages here. (Destructors must not throw.)
}

// Getting a frame (used by NewPage and FetchPage): free list first, then the replacer.
// If the victim is dirty, write it back before reusing the frame:
//   M9: flush log up to page_lsn
// (WAL rule: the log records for a page must reach disk before the page itself.)

auto BufferPoolManager::NewPage() -> Page * {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::NewPage");
}

auto BufferPoolManager::FetchPage(page_id_t /*page_id*/) -> Page * {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FetchPage");
}

auto BufferPoolManager::UnpinPage(page_id_t /*page_id*/, bool /*is_dirty*/) -> bool {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::UnpinPage");
}

auto BufferPoolManager::FlushPage(page_id_t /*page_id*/) -> bool {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FlushPage");
}

void BufferPoolManager::FlushAllPages() {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FlushAllPages");
}

auto BufferPoolManager::DeletePage(page_id_t /*page_id*/) -> bool {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::DeletePage");
}

auto BufferPoolManager::NewPageGuarded() -> BasicPageGuard {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::NewPageGuarded");
}

auto BufferPoolManager::FetchPageBasic(page_id_t /*page_id*/) -> BasicPageGuard {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FetchPageBasic");
}

auto BufferPoolManager::FetchPageRead(page_id_t /*page_id*/) -> ReadPageGuard {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FetchPageRead");
}

auto BufferPoolManager::FetchPageWrite(page_id_t /*page_id*/) -> WritePageGuard {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FetchPageWrite");
}

auto BufferPoolManager::FreeFrameCount() const -> std::size_t {
  throw Exception(ExceptionType::NotImplemented, "BufferPoolManager::FreeFrameCount");
}

}  // namespace minitub
