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
      "id0 id1 id2",
      "`some_macro endmodule",
  };
  SimpleTokenizer tk;
  for (auto &line : lines) {
    tk.ProcessLine(line);
  }
  EXPECT_EQ(tk.Comments(0).size(), 0);
  EXPECT_EQ(tk.Comments(1).size(), 0);
  EXPECT_EQ(tk.Comments(2).size(), 0);
  EXPECT_EQ(tk.Comments(3).size(), 0);
  EXPECT_EQ(tk.Comments(4).size(), 1);
  EXPECT_EQ(tk.Comments(4)[0].first, 0);
  EXPECT_EQ(tk.Comments(4)[0].second, 19);
  EXPECT_EQ(tk.Comments(5).size(), 1);
  EXPECT_EQ(tk.Comments(5)[0].first, 16);
  EXPECT_EQ(tk.Comments(5)[0].second, 41);
  EXPECT_EQ(tk.Comments(6).size(), 2);
  EXPECT_EQ(tk.Comments(6)[0].first, 1);
  EXPECT_EQ(tk.Comments(6)[0].second, 11);
  EXPECT_EQ(tk.Comments(6)[1].first, 35);
  EXPECT_EQ(tk.Comments(6)[1].second, 55);
  EXPECT_EQ(tk.Comments(7).size(), 0);
  EXPECT_EQ(tk.Comments(8).size(), 1);
  EXPECT_EQ(tk.Comments(9).size(), 1);
  EXPECT_EQ(tk.Comments(10).size(), 1);
  EXPECT_EQ(tk.Comments(11).size(), 0);
  EXPECT_EQ(tk.Identifiers(7).size(), 1);
  EXPECT_EQ(tk.Identifiers(7)[0].first, 2);
  EXPECT_EQ(tk.Identifiers(7)[0].second, "escaped^#ident.f(er");
  EXPECT_EQ(tk.Keywords(7).size(), 0);
  EXPECT_EQ(tk.Identifiers(11).size(), 3);
  EXPECT_EQ(tk.Keywords(12).size(), 1);
  EXPECT_EQ(tk.Identifiers(12).size(), 0);
  EXPECT_EQ(tk.Keywords(0).size(), 1);
  EXPECT_EQ(tk.Keywords(0)[0].first, 0);
  EXPECT_EQ(tk.Keywords(0)[0].second, 5);
}

} // namespace
} // namespace sv
