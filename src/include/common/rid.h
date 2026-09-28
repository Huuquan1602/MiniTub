#pragma once

#include <cstdint>
#include <string>

#include "common/config.h"

namespace minitub {

/** Record ID: where a tuple lives (page + slot inside that page). */
class RID {
 public:
  RID() = default;
  RID(page_id_t page_id, std::uint32_t slot_num) : page_id_(page_id), slot_num_(slot_num) {}

  auto GetPageId() const -> page_id_t { return page_id_; }
  auto GetSlotNum() const -> std::uint32_t { return slot_num_; }
  auto IsValid() const -> bool { return page_id_ != INVALID_PAGE_ID; }

  auto ToString() const -> std::string {
    return "RID(" + std::to_string(page_id_) + ", " + std::to_string(slot_num_) + ")";
  }

  friend auto operator==(const RID &, const RID &) -> bool = default;

 private:
  page_id_t page_id_{INVALID_PAGE_ID};
  std::uint32_t slot_num_{0};
};

}  // namespace minitub
