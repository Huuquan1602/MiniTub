#include "storage/page/page_guard.h"

#include <cstdio>
#include <cstdlib>

#include "common/exception.h"

namespace minitub {

namespace {
// noexcept functions cannot throw, so their stubs abort with a message instead.
[[noreturn]] void NotImplementedNoexcept(const char *what) {
  std::fprintf(stderr, "NotImplemented: %s\n", what);
  std::abort();
}
}  // namespace

// ---- BasicPageGuard ----
BasicPageGuard::BasicPageGuard(BufferPoolManager * /*bpm*/, Page * /*page*/) {
  throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::BasicPageGuard");
}
BasicPageGuard::BasicPageGuard(BasicPageGuard && /*other*/) noexcept {
  NotImplementedNoexcept("BasicPageGuard move constructor");
}
auto BasicPageGuard::operator=(BasicPageGuard && /*other*/) noexcept -> BasicPageGuard & {
  NotImplementedNoexcept("BasicPageGuard move assignment");
}
BasicPageGuard::~BasicPageGuard() {
  // TODO(M1): Drop(). (Destructors must not throw.)
}
void BasicPageGuard::Drop() { throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::Drop"); }
auto BasicPageGuard::IsValid() const -> bool {
  throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::IsValid");
}
auto BasicPageGuard::PageId() const -> page_id_t {
  throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::PageId");
}
auto BasicPageGuard::GetData() const -> const std::byte * {
  throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::GetData");
}
auto BasicPageGuard::GetDataMut() -> std::byte * {
  throw Exception(ExceptionType::NotImplemented, "BasicPageGuard::GetDataMut");
}

// ---- ReadPageGuard ----
ReadPageGuard::ReadPageGuard(BufferPoolManager * /*bpm*/, Page * /*page*/) {
  throw Exception(ExceptionType::NotImplemented, "ReadPageGuard::ReadPageGuard");
}
ReadPageGuard::ReadPageGuard(ReadPageGuard && /*other*/) noexcept {
  NotImplementedNoexcept("ReadPageGuard move constructor");
}
auto ReadPageGuard::operator=(ReadPageGuard && /*other*/) noexcept -> ReadPageGuard & {
  NotImplementedNoexcept("ReadPageGuard move assignment");
}
ReadPageGuard::~ReadPageGuard() {
  // TODO(M1): Drop(). (Destructors must not throw.)
}
void ReadPageGuard::Drop() { throw Exception(ExceptionType::NotImplemented, "ReadPageGuard::Drop"); }
auto ReadPageGuard::IsValid() const -> bool {
  throw Exception(ExceptionType::NotImplemented, "ReadPageGuard::IsValid");
}
auto ReadPageGuard::PageId() const -> page_id_t {
  throw Exception(ExceptionType::NotImplemented, "ReadPageGuard::PageId");
}
auto ReadPageGuard::GetData() const -> const std::byte * {
  throw Exception(ExceptionType::NotImplemented, "ReadPageGuard::GetData");
}

// ---- WritePageGuard ----
WritePageGuard::WritePageGuard(BufferPoolManager * /*bpm*/, Page * /*page*/) {
  throw Exception(ExceptionType::NotImplemented, "WritePageGuard::WritePageGuard");
}
WritePageGuard::WritePageGuard(WritePageGuard && /*other*/) noexcept {
  NotImplementedNoexcept("WritePageGuard move constructor");
}
auto WritePageGuard::operator=(WritePageGuard && /*other*/) noexcept -> WritePageGuard & {
  NotImplementedNoexcept("WritePageGuard move assignment");
}
WritePageGuard::~WritePageGuard() {
  // TODO(M1): Drop(). (Destructors must not throw.)
}
void WritePageGuard::Drop() { throw Exception(ExceptionType::NotImplemented, "WritePageGuard::Drop"); }
auto WritePageGuard::IsValid() const -> bool {
  throw Exception(ExceptionType::NotImplemented, "WritePageGuard::IsValid");
}
auto WritePageGuard::PageId() const -> page_id_t {
  throw Exception(ExceptionType::NotImplemented, "WritePageGuard::PageId");
}
auto WritePageGuard::GetData() const -> const std::byte * {
  throw Exception(ExceptionType::NotImplemented, "WritePageGuard::GetData");
}
auto WritePageGuard::GetDataMut() -> std::byte * {
  throw Exception(ExceptionType::NotImplemented, "WritePageGuard::GetDataMut");
}

}  // namespace minitub
