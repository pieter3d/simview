#include "simple_tokenizer.h"
#include "gtest/gtest.h"

namespace sv {
namespace {

TEST(SimpleTokenizer, BasicIdentifiers) {
  std::vector<std::string> lines = {
      "module test (",
      "input a,",
      "output b",
      ");",
      "// this is a comment",
      "always_ff clock // this is another comment",
      " /* block */ identifier split^ident/* another comment */ ;",
      " \\escaped^#ident.f(er \"string \\\"literal\"",
      " _plus$args /* block comment start",
      " and it keeps going...",
      "boop beep noop neep */ d34545_id;",
      "`some_macro endmodule",
  };
  SimpleTokenizer tk;
  for (auto &line :lines) {
    tk.ProcessLine(line);
  }
  EXPECT_EQ(tk.CommentRanges(0).size(), 0);
  EXPECT_EQ(tk.CommentRanges(1).size(), 0);
  EXPECT_EQ(tk.CommentRanges(2).size(), 0);
  EXPECT_EQ(tk.CommentRanges(3).size(), 0);
  EXPECT_EQ(tk.CommentRanges(4).size(), 1);
  EXPECT_EQ(tk.CommentRanges(4)[0].first, 0);
  EXPECT_EQ(tk.CommentRanges(4)[0].second, 19);
  EXPECT_EQ(tk.CommentRanges(5).size(), 1);
  EXPECT_EQ(tk.CommentRanges(5)[0].first, 16);
  EXPECT_EQ(tk.CommentRanges(5)[0].second, 41);
  EXPECT_EQ(tk.CommentRanges(6).size(), 2);
  EXPECT_EQ(tk.CommentRanges(6)[0].first, 1);
  EXPECT_EQ(tk.CommentRanges(6)[0].second, 11);
  EXPECT_EQ(tk.CommentRanges(6)[1].first, 35);
  EXPECT_EQ(tk.CommentRanges(6)[1].second, 55);
  EXPECT_EQ(tk.CommentRanges(7).size(), 0);
  EXPECT_EQ(tk.CommentRanges(8).size(), 1);
  EXPECT_EQ(tk.CommentRanges(9).size(), 1);
  EXPECT_EQ(tk.CommentRanges(10).size(), 1);
  EXPECT_EQ(tk.CommentRanges(11).size(), 0);
  EXPECT_EQ(tk.Identifiers(7).size(), 1);
  EXPECT_EQ(tk.Identifiers(7)[0].first, 2);
  EXPECT_EQ(tk.Identifiers(7)[0].second, "escaped^#ident.f(er");
  EXPECT_EQ(tk.Keywords(7).size(), 0);
  EXPECT_EQ(tk.Keywords(11).size(), 1);
  EXPECT_EQ(tk.Identifiers(11).size(), 0);
}

} // namespace
} // namespace sv
