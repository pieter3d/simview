#include "source_buffer.h"

#include "external/googletest/googletest/include/gtest/gtest.h"

namespace sv {
namespace {

TEST(SourceBuffer, Basic) {
  constexpr std::string_view src = R"(line 1
line 2
line 3)";
  SourceBuffer sb;
  sb.ProcessBuffer(src);
  ASSERT_EQ(sb.NumLines(), 3);
  ASSERT_EQ(sb[0], "line 1");
  ASSERT_EQ(sb.LineLength(1), 6);
}

TEST(SourceBuffer, TrailingNewline) {
  constexpr std::string_view src = R"(line 1
line 2
line 3
)";
  SourceBuffer sb;
  sb.ProcessBuffer(src);
  ASSERT_EQ(sb.NumLines(), 3);
  ASSERT_EQ(sb[0], "line 1");
  ASSERT_EQ(sb.LineLength(2), 6);
}

TEST(SourceBuffer, Tabs) {
  constexpr std::string_view src = "line 1\n"
                                   "word\tword\t\n"
                                   "word\tword\tword\n"
                                   "one\tone\tone\n"
                                   "xx\tyy\tzz\n";
  SourceBuffer sb;
  sb.ProcessBuffer(src);
  ASSERT_EQ(sb.LineLength(1), 16);
  ASSERT_EQ(sb.LineLength(2), 20);
  ASSERT_EQ(sb.LineLength(3), 11);
  ASSERT_EQ(sb.LineLength(4), 10);
  ASSERT_EQ(sb.DisplayCol(0, 0), 0);
  ASSERT_EQ(sb.DisplayCol(0, 5), 5);
  ASSERT_EQ(sb.DisplayCol(0, 10), 10);

  std::vector<int> line1cols = {0, 1, 2, 3, 4, 8, 9, 10, 11, 12, 16, 17, 18};
  for (int i = 0; i < line1cols.size(); ++i) {
    ASSERT_EQ(sb.DisplayCol(1, i), line1cols[i]) << "col " << i;
  }
  std::vector<int> line2cols = {0, 1, 2, 3, 4, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 21};
  for (int i = 0; i < line2cols.size(); ++i) {
    ASSERT_EQ(sb.DisplayCol(2, i), line2cols[i]) << "col " << i;
  }
  std::vector<int> line3cols = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  for (int i = 0; i < line3cols.size(); ++i) {
    ASSERT_EQ(sb.DisplayCol(3, i), line3cols[i]) << "col " << i;
  }
  std::vector<int> line4cols = {0, 1, 2, 4, 5, 6, 8, 9, 10, 11};
  for (int i = 0; i < line4cols.size(); ++i) {
    ASSERT_EQ(sb.DisplayCol(4, i), line4cols[i]) << "col " << i;
  }
}

} // namespace
} // namespace sv
