// Fundamental identifiers and constants shared by every layer of Basic_DB.
// See DESIGN.md §3 (Core Design Decisions).
#pragma once

#include <cstddef>
#include <cstdint>

namespace basicdb {

using page_id_t  = int32_t;    // unique id of a page within the database file
using frame_id_t = int32_t;    // index of a frame inside the buffer pool
using slot_id_t  = uint16_t;   // slot number within a slotted page
using txn_id_t   = uint64_t;   // transaction id
using lsn_t      = uint64_t;   // log sequence number (byte offset into the WAL)

// Every page in the system is exactly this many bytes (DESIGN.md §3).
inline constexpr std::size_t PAGE_SIZE = 4096;

// Reserved page ids.
inline constexpr page_id_t INVALID_PAGE_ID = -1;
inline constexpr page_id_t HEADER_PAGE_ID  = 0;    // file header lives in page 0

}  // namespace basicdb
