#pragma once

#include <cstddef>

#include "common/config.h"
#include "common/macros.h"

namespace minitub {

class BufferPoolManager;
class Page;

/*
 * RAII holders of a pinned page (design.md section 7.6).
 * - Move-only. A moved-from guard is empty and its destructor does nothing.
 * - Drop() releases early and is idempotent; the destructor calls Drop().
 * - Move-assigning into a guard first drops what it held.
 * - A guard from a failed fetch (no free frame) is empty: IsValid() == false.
 */

/** Holds a pin only. */
class BasicPageGuard {
 public:
  BasicPageGuard() = default;
  /** Takes over one pin of `page`, which `bpm` has already pinned. */
  BasicPageGuard(BufferPoolManager *bpm, Page *page);
  BasicPageGuard(BasicPageGuard &&other) noexcept;
  auto operator=(BasicPageGuard &&other) noexcept -> BasicPageGuard &;
  ~BasicPageGuard();
  DISALLOW_COPY(BasicPageGuard);

  /** Unpins the page (dirty if GetDataMut() was called). Safe to call twice. */
  void Drop();

  auto IsValid() const -> bool;
  auto PageId() const -> page_id_t;
  auto GetData() const -> const std::byte *;
  /** Mutable access; marks the page dirty. */
  auto GetDataMut() -> std::byte *;

 private:
  // TODO(M1): your members.
};

/** Holds a pin and the shared (read) latch of the page. */
class ReadPageGuard {
 public:
  ReadPageGuard() = default;
  /** `page` is already pinned and read-latched by `bpm`. */
  ReadPageGuard(BufferPoolManager *bpm, Page *page);
  ReadPageGuard(ReadPageGuard &&other) noexcept;
  auto operator=(ReadPageGuard &&other) noexcept -> ReadPageGuard &;
  ~ReadPageGuard();
  DISALLOW_COPY(ReadPageGuard);

  /** Releases the read latch first, then unpins. Safe to call twice. */
  void Drop();

  auto IsValid() const -> bool;
  auto PageId() const -> page_id_t;
  auto GetData() const -> const std::byte *;

 private:
  // TODO(M1): your members.
};

/** Holds a pin and the exclusive (write) latch of the page. */
class WritePageGuard {
 public:
  WritePageGuard() = default;
  /** `page` is already pinned and write-latched by `bpm`. */
  WritePageGuard(BufferPoolManager *bpm, Page *page);
  WritePageGuard(WritePageGuard &&other) noexcept;
  auto operator=(WritePageGuard &&other) noexcept -> WritePageGuard &;
  ~WritePageGuard();
  DISALLOW_COPY(WritePageGuard);

  /** Releases the write latch first, then unpins (dirty if GetDataMut() was called). Safe to call twice. */
  void Drop();

  auto IsValid() const -> bool;
  auto PageId() const -> page_id_t;
  auto GetData() const -> const std::byte *;
  /** Mutable access; marks the page dirty. */
  auto GetDataMut() -> std::byte *;

 private:
  // TODO(M1): your members.
};

}  // namespace minitub
