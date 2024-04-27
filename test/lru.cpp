#include "gtest/gtest.h"
#include "storage/default/lru_replacer.h"

TEST(lru, 1) {
  LRUReplacer lru_replacer(100);
  EXPECT_EQ(100, lru_replacer.Size());
}