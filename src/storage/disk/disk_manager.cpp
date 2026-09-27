#include "storage/disk/disk_manager.h"

#include "common/exception.h"

namespace minitub {

DiskManager::DiskManager(const std::filesystem::path & /*file*/, IoMode /*mode*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::DiskManager");
}

DiskManager::~DiskManager() {
  // TODO(M1): close the file. (Destructors must not throw.)
}

void DiskManager::ReadPage(page_id_t /*page_id*/, std::span<std::byte, PAGE_SIZE> /*out*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::ReadPage");
}

void DiskManager::WritePage(page_id_t /*page_id*/, std::span<const std::byte, PAGE_SIZE> /*data*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::WritePage");
}

auto DiskManager::AllocatePage() -> page_id_t {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::AllocatePage");
}

void DiskManager::DeallocatePage(page_id_t /*page_id*/) {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::DeallocatePage");
}

auto DiskManager::NumPages() const -> page_id_t {
  throw Exception(ExceptionType::NotImplemented, "DiskManager::NumPages");
}

}  // namespace minitub
