// Smoke tests for the foundational types (src/common/types.h).
#include "common/types.h"
#include "test_framework.h"

using namespace basicdb;

TEST(types, page_size_is_4_kib) {
  ASSERT_TRUE(PAGE_SIZE == 4096);
  ASSERT_TRUE(PAGE_SIZE % 512 == 0);  // stays sector-aligned for direct I/O later
}

TEST(types, reserved_page_ids) {
  ASSERT_EQ(HEADER_PAGE_ID, page_id_t{0});
  ASSERT_TRUE(INVALID_PAGE_ID < 0);
  ASSERT_NE(HEADER_PAGE_ID, INVALID_PAGE_ID);
}

TEST(types, identifier_widths_are_pinned) {
  // Pin the wire/disk widths so a careless refactor breaks loudly.
  ASSERT_TRUE(sizeof(page_id_t) == 4);
  ASSERT_TRUE(sizeof(frame_id_t) == 4);
  ASSERT_TRUE(sizeof(slot_id_t) == 2);
  ASSERT_TRUE(sizeof(txn_id_t) == 8);
  ASSERT_TRUE(sizeof(lsn_t) == 8);
}

TEST(smoke, framework_itself) {
  ASSERT_TRUE(true);
  ASSERT_FALSE(false);
}
