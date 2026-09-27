#pragma once

#include <cstddef>
#include <cstdint>

namespace minitub {

/** Size of a disk page and of a buffer pool frame, in bytes. */
inline constexpr std::size_t PAGE_SIZE = 4096;

using page_id_t = std::int32_t;   // page number inside the database file
using frame_id_t = std::int32_t;  // slot index inside the buffer pool
using txn_id_t = std::int64_t;    // transaction id
using lsn_t = std::int64_t;       // log sequence number (WAL position)

inline constexpr page_id_t INVALID_PAGE_ID = -1;

}  // namespace minitub
