#include "source_panel.h"
#include "color.h"
#include "radix.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/ParameterSymbols.h"
#include "slang/parsing/LexerFacts.h"
#include "slang/parsing/Token.h"
#include "slang/syntax/SyntaxNode.h"
#include "slang/syntax/SyntaxVisitor.h"
#include "slang/text/SourceLocation.h"
#include "slang/text/SourceManager.h"
#include "slang_utils.h"
#include "utils.h"
#include "workspace.h"

#include <algorithm>
#include <concepts>
#include <curses.h>

namespace sv {
namespace {

constexpr int kMaxStateStackSize = 500;
constexpr int kTabSize = 4;

} // namespace

void SourcePanel::FindSymbols() {
  const slang::ast::Symbol &top = scope_->asSymbol();
  const slang::SourceManager *sm = Workspace::Get().SourceManager();
  int depth = 0;
  top.visit(slang::ast::makeVisitor(
      [&](auto &visitor, const std::derived_from<slang::ast::Symbol> auto &sym) {
        if (!(sym.kind == slang::ast::SymbolKind::Instance ||
              sym.kind == slang::ast::SymbolKind::GenerateBlock ||
              sym.kind == slang::ast::SymbolKind::GenerateBlockArray ||
              sym.kind == slang::ast::SymbolKind::InstanceArray ||
              sym.kind == slang::ast::SymbolKind::InstanceBody ||
              sym.kind == slang::ast::SymbolKind::Parameter ||
              sym.kind == slang::ast::SymbolKind::TypeAlias ||
              sym.kind == slang::ast::SymbolKind::Net ||
              sym.kind == slang::ast::SymbolKind::Variable ||
              sym.kind == slang::ast::SymbolKind::Subroutine)) {
          return;
        }
        // Skip anything without syntax, such as inferred variables for function return values.
        if (!sym.getSyntax()) return;
        const size_t start_col = sm->getColumnNumber(sym.location) - 1;
        src_info_[sm->getLineNumber(sym.location) - 1].insert(
            {.start_col = start_col, .end_col = start_col + sym.name.size() - 1, .sym = &sym});

        // Don't recurse into sub-instances, just find things within this scope.
        // For generate array constructs, only recurse into the first one when the generate
        // construct isn't the top of the traversal. That way the body of a generate block inside
        // the scope is still navigable. Users can explicitly set the scope to a particular instance
        // if index 0 isn't the desired one.
        const bool skip = sym.kind == slang::ast::SymbolKind::Instance ||
                          (sym.kind == slang::ast::SymbolKind::GenerateBlock && depth > 0 &&
                           sym.getHierarchicalParent()->asSymbol().kind ==
                               slang::ast::SymbolKind::GenerateBlockArray &&
                           sym.template as<slang::ast::GenerateBlockSymbol>().constructIndex > 0);

        if (!skip) {
          depth++;
          visitor.visitDefault(sym);
          depth--;
        }
      },
      [&](auto &visitor, const slang::ast::NamedValueExpression &nve) {
        // const int line = sm->getLineNumber(nve.sourceRange.start()) - 1;
        // const size_t start_col = sm->getColumnNumber(nve.sourceRange.start()) - 1;
        // const size_t end_col = sm->getColumnNumber(nve.sourceRange.start()) - 1;
        // src_info_[line].insert({.start_col = start_col, .end_col = end_col, .sym = &nve.symbol});
      }));
}

void SourcePanel::FindKeywordsAndComments() {
  struct TokenVisitor : public slang::syntax::SyntaxVisitor<TokenVisitor> {
    SourcePanel *panel_;
    const slang::SourceManager *sm_;
    TokenVisitor(SourcePanel *p, const slang::SourceManager *sm) : panel_(p), sm_(sm) {}
    void visitToken(slang::parsing::Token tok) {
      if (slang::parsing::LexerFacts::isKeyword(tok.kind)) {
        const int line = sm_->getLineNumber(tok.location()) - 1;
        panel_->src_info_[line].insert({.start_col = sm_->getColumnNumber(tok.range().start()) - 1,
                                        .end_col = sm_->getColumnNumber(tok.range().end()) - 1,
                                        .keyword = true});
      }
      slang::SourceLocation loc = tok.location();
      // Iterate from the last trivia, since they all come before the token. This enables
      // determining the characeter locations of the trivia blocks.
      for (const slang::parsing::Trivia &t : std::views::reverse(tok.trivia())) {
        slang::SourceLocation end_loc = loc - 1;
        loc -= t.getRawText().size();
        if (t.kind != slang::parsing::TriviaKind::BlockComment &&
            t.kind != slang::parsing::TriviaKind::LineComment) {
          continue;
        }
        const size_t start_line = sm_->getLineNumber(loc) - 1;
        const size_t end_line = sm_->getLineNumber(end_loc) - 1;
        const size_t start_col = sm_->getColumnNumber(loc) - 1;
        const size_t end_col = sm_->getColumnNumber(end_loc) - 1;
        if (start_line == end_line) {
          panel_->src_info_[start_line].insert(
              {.start_col = start_col, .end_col = end_col, .comment = true});
        } else {
          for (int line = start_line; line <= end_line; ++line) {
            const size_t a = line == start_line ? start_col : 0;
            const size_t b = line == end_line ? end_col : std::numeric_limits<int>::max();
            panel_->src_info_[line].insert({.start_col = a, .end_col = b, .comment = true});
          }
        }
      }
    }
  };
  const slang::SourceManager *sm = Workspace::Get().SourceManager();
  scope_->asSymbol().getSyntax()->visit(TokenVisitor(this, sm));
}

std::optional<std::pair<int, int>> SourcePanel::CursorLocation() const {
  if (scope_ == nullptr) return std::nullopt;
  // Compute width of the line numbers. Minus 1 to account for the header, but
  // plus one since line numbers start at 1. Add one to the final width to
  // account for the line number margin.
  const int ruler_size = 1 + NumDecimalDigits(scroll_row_ + getmaxy(w_) - 1);
  return std::pair(line_idx_ - scroll_row_ + 1, col_idx_ - scroll_col_ + ruler_size);
}

void SourcePanel::BuildHeader() {
  if (scope_ == nullptr) return;
  std::string scope_name = scope_->asSymbol().getHierarchicalPath();
  const std::string separator = " | ";
  header_ = current_file_ + separator + scope_name;
  // Attempt to strip as many leading directories from the front of the header
  // until it fits. If it still doesn't fit then oh well, it will get cut off
  // in the Draw funtion.
  const int max_w = getmaxx(w_);
  if (max_w != 0 && header_.size() > max_w) {
    const char sep = std::filesystem::path::preferred_separator;
    // No need to include the first character. It's either the leading
    // separator which is shorter than the replacement ellipsis, or it's the
    // leading ellipsis from previous iterations.
    int search_pos = 1;
    int pos = -1;
    do {
      int result = current_file_.find(sep, search_pos);
      if (result == std::string::npos) break;
      pos = result;
      search_pos = pos + 1;
      // Keep going while the new string length still doesn't fit.
      // Account for the ellipsis, 3 dots: ...
    } while (header_.size() - pos + 3 > max_w);
    if (pos > 0) {
      header_.replace(0, pos, "...");
    }
  }
}

void SourcePanel::Draw() {
  werase(w_);
  wattrset(w_, A_NORMAL);
  if (scope_ == nullptr) {
    mvwprintw(w_, 0, 0, "Module instance source code appears here.");
    return;
  }
  const int win_h = getmaxy(w_);
  const int win_w = getmaxx(w_);
  const int max_digits = NumDecimalDigits(scroll_row_ + win_h - 1);
  SetColor(w_, kSourceHeaderPair);
  mvwaddnstr(w_, 0, 0, header_.c_str(), win_w);

  if (src_.Empty()) {
    SetColor(w_, kSourceTextPair);
    mvwprintw(w_, 1, 0, "Unable to open file");
    return;
  }
  const auto highlight_attr = (!search_preview_ && has_focus_) ? A_REVERSE : A_UNDERLINE;
  int sel_pos = 0; // Save selection start position.
  for (int y = 1; y < win_h; ++y) {
    const int line_idx = y - 1 + scroll_row_;
    if (line_idx >= src_.NumLines()) break;
    const int line_num = line_idx + 1;
    const int line_num_size = NumDecimalDigits(line_num);
    const bool active = line_num >= start_line_ && line_num <= end_line_;
    const int text_color = active ? kSourceTextPair : kSourceInactivePair;
    SetColor(w_, line_idx == line_idx_ ? kSourceCurrentLineNrPair : kSourceLineNrPair);
    // Fill the space before the line number with blanks so that it has the
    // right background color.
    for (int x = 0; x < max_digits - line_num_size; ++x) {
      mvwaddch(w_, y, x, ' ');
    }
    mvwprintw(w_, y, max_digits - line_num_size, "%d", line_num);
    SetColor(w_, text_color);
    waddch(w_, ' ');

    // Go charachter by character, up to the window width.
    // Keep track of the current identifier, keyword and comment in the line.
    std::string_view s = src_[line_idx];
    // Start rendering the line at the start, so that tab calculation and identifier highlighting
    // work no matter the horizontal scrolling.
    int screen_x = -scroll_col_ + max_digits + 1;
    int pos = 0;
    auto line_it = src_info_.find(line_idx);
    std::optional<absl::btree_set<SourceInfo>::iterator> info_it_or_null;
    if (line_it != src_info_.end()) info_it_or_null = line_it->second.begin();

    while (screen_x < win_w) {
      bool end_of_color = false;
      // On active lines that have source info, and the source info hasn't been exhausted yet...
      if (active && info_it_or_null && *info_it_or_null != line_it->second.end()) {
        const SourceInfo &info = **info_it_or_null;
        //  At the start of a keyword, comment or symbol, potentially switch color.
        if (pos == info.start_col) {
          if (info.keyword) {
            SetColor(w_, kSourceKeywordPair);
          } else if (info.comment) {
            SetColor(w_, kSourceCommentPair);
          } else if (info.sym->kind == slang::ast::SymbolKind::Instance ||
                     info.sym->kind == slang::ast::SymbolKind::InstanceArray ||
                     info.sym->kind == slang::ast::SymbolKind::Subroutine) {
            SetColor(w_, kSourceInstancePair);
          } else if (info.sym->kind == slang::ast::SymbolKind::Parameter) {
            SetColor(w_, kSourceParamPair);
          } else if (IsTraceable(info.sym)) {
            SetColor(w_, kSourceIdentifierPair);
          }
          // Start highlighting the selected symbol, if the cursor is in it. But if currently
          // showing the search preview (typing the search text), don't highlight if nothing is
          // found because that would make it look like the selected item matches the search input.
          const bool cursor_in_sel =
              line_idx_ == line_idx && col_idx_ >= info.start_col && col_idx_ <= info.end_col;
          if (sel_ != nullptr && info.sym == sel_ && cursor_in_sel &&
              !(search_preview_ && search_start_col_ < 0)) {
            wattron(w_, highlight_attr);
            // Save the position where the selected item starts. This is where the value label
            // will be drawn.
            sel_pos = pos;
          }
        }
        if (pos == info.end_col) {
          end_of_color = true;
          // Advance to the next info item in the list for this line.
          (*info_it_or_null)++;
        }
      }

      // Highight partial search results.
      if (search_preview_ && !search_text_.empty() && line_idx == line_idx_ &&
          pos == search_start_col_) {
        wattron(w_, A_REVERSE);
      }
      // Don't add anything until X position has passed the line number ruler.
      if (screen_x > max_digits) waddch(w_, s[pos] == '\t' ? ' ' : s[pos]);
      if (search_preview_ && line_idx == line_idx_ &&
          pos == (search_start_col_ + search_text_.size() - 1)) {
        wattroff(w_, A_REVERSE);
      }

      // If the current colorized thing is now complete, turn off the color and advance.
      if (end_of_color) {
        SetColor(w_, text_color);
        wattroff(w_, highlight_attr);
      }

      //  Always advance the screenn coordinate.
      screen_x++;
      // Only advance to the next character if spaces aren't being inserted for tab expansion.
      if (s[pos] != '\t' || (screen_x + scroll_col_ - max_digits - 1) % kTabSize == 0) {
        pos++;
        if (pos >= s.size()) break;
      }
    }
    wattroff(w_, highlight_attr);
  }

  // Draw the current value of the selected item.
  std::string val;
  if (show_vals_ && sel_ != nullptr &&
      (sel_->kind == slang::ast::SymbolKind::Parameter || Workspace::Get().Waves() != nullptr)) {
    if (const auto *param = sel_->as_if<slang::ast::ParameterSymbol>()) {
      val = param->getValue().toString();
    } else {
      std::vector<const WaveData::Signal *> signals;
      //  TODO: Reinstate when workspace waves are slang-ified.
      // std::vector<const WaveData::Signal *> signals =
      //    Workspace::Get().DesignToSignals(sel_);
      // Don't bother with large arrays.
      // TODO: Is it useful to try to do something here?
      if (signals.size() == 1) {
        const auto &wave = Workspace::Get().Waves()->Wave(signals[0]);
        if (wave.empty()) {
          val = "No data";
        } else {
          const uint64_t idx = Workspace::Get().Waves()->FindSampleIndex(
              Workspace::Get().WaveCursorTime(), signals[0]);
          // TODO: How to allow for other radix values?
          val = FormatValue(wave[idx].value, Radix::kHex,
                            /* leading_zeroes*/ false);
        }
      }
    }
  }
  if (!val.empty()) {
    const int id_size = sel_->name.size();
    // Draw a nice box, with an empty value all around it, including the
    // connecting line:
    //
    //           +------+
    //           | 1234 |
    //           +------+
    //           |
    // blah blah identifier blah blah
    //
    // The tag moves to the right and/or below the identifier depending on
    // available room.
    const std::string box = " +" + std::string(val.size() + 2, '-') + "+ ";
    val = " | " + val + " | ";
    // Always try to draw above, unless the selected line is too close to the
    // top, then draw below.
    const int val_line = line_idx_ - scroll_row_ + 1;
    const bool above = line_idx_ - scroll_row_ > 3;
    // Always try to draw to the right, unless it doesn't fit. In that case pick
    // whichever side has more room.
    const int start_col = max_digits + 1 + sel_pos - scroll_col_ - 1;
    const bool right =
        start_col + val.size() <= win_w || (((win_w - start_col) > (start_col + id_size - 1)));
    SetColor(w_, kSourceValuePair);
    wattron(w_, A_BOLD);
    int connector_col = start_col + (right ? 0 : id_size - 3 + 2);
    int max_chars_connector = win_w - connector_col;
    mvwaddnstr(w_, val_line + (above ? -1 : 1), connector_col, " | ", max_chars_connector);
    const int col = start_col + (right ? 0 : id_size - val.size() + 2);
    int max_chars = win_w - col;
    mvwaddnstr(w_, val_line + (above ? -4 : 4), col, box.c_str(), max_chars);
    mvwaddnstr(w_, val_line + (above ? -3 : 3), col, val.c_str(), max_chars);
    mvwaddnstr(w_, val_line + (above ? -2 : 2), col, box.c_str(), max_chars);
  }
}

void SourcePanel::UIChar(int ch) {
  if (scope_ == nullptr) return;
  int prev_line_idx = line_idx_;
  int prev_col_idx = col_idx_;
  bool send_to_waves = false;
  switch (ch) {
  case 'u': {
    if (scope_->asSymbol().getHierarchicalParent()->asSymbol().kind ==
        slang::ast::SymbolKind::Root) {
      error_message_ = "This is a top level module.";
    } else if (const auto *gen = scope_->asSymbol().as_if<slang::ast::GenerateBlockSymbol>()) {
      SetItem(&gen->getHierarchicalParent()->asSymbol());
      item_for_design_tree_ = &scope_->asSymbol();
    } else if (const auto *body = scope_->asSymbol().as_if<slang::ast::InstanceBodySymbol>()) {
      SetItem(body->parentInstance);
      item_for_design_tree_ = &scope_->asSymbol();
    }
  } break;
  case 'h':
  case 0x104: // left
    if (scroll_col_ > 0 && scroll_col_ - col_idx_ == 0) {
      scroll_col_--;
    } else if (col_idx_ > 0) {
      col_idx_--;
      max_col_idx_ = col_idx_;
    } else if (col_idx_ == 0 && line_idx_ != 0) {
      if (line_idx_ == scroll_row_) scroll_row_--;
      line_idx_--;
      col_idx_ = std::max(0, src_.LineLength(line_idx_) - 1);
      max_col_idx_ = col_idx_;
    }
    break;
  case 'l':
  case 0x105: // right
    if (col_idx_ < src_.LineLength(line_idx_) - 1) {
      col_idx_++;
      max_col_idx_ = col_idx_;
      const int win_w = getmaxx(w_);
      const int win_h = getmaxy(w_);
      const int ruler_size = NumDecimalDigits(win_h + scroll_row_ - 1) + 1;
      if (col_idx_ >= win_w - ruler_size) scroll_col_++;
    } else if (line_idx_ < src_.NumLines() - 1) {
      line_idx_++;
      col_idx_ = 0;
      scroll_col_ = 0;
      max_col_idx_ = 0;
      const int win_h = getmaxy(w_);
      if (line_idx_ - scroll_row_ >= win_h - 1) scroll_row_++;
    }
    break;
  case 0x106: // Home
  case '^':
    // Start of line, scoll left if needed.
    scroll_col_ = 0;
    col_idx_ = 0;
    max_col_idx_ = col_idx_;
    break;
  case 0x168: // End
  case '$':
    // End of line
    col_idx_ = src_.LineLength(line_idx_) - 1;
    max_col_idx_ = col_idx_;
    break;
  case 'd':
    // Go to definition of a module instance.
    if (sel_ != nullptr) {
      // For modules, load the definition.
      if (const auto *inst = sel_->as_if<slang::ast::InstanceSymbol>()) {
        SetItem(&inst->body);
      } else if (const auto *arr = sel_->as_if<slang::ast::InstanceArraySymbol>()) {
        // For array instances, just use the first one. The design tree should be used if a specific
        // index is desired.
        if (const auto *inst = arr->elements[0]->as_if<slang::ast::InstanceSymbol>()) {
          SetItem(&inst->body);
        }
      } else {
        SetItem(sel_);
      }
      item_for_design_tree_ = &scope_->asSymbol();
    }
    break;
  case 'D':
  case 'L':
    if (sel_ != nullptr) {
      bool trace_drivers = ch == 'D';
      (void)trace_drivers;
      // TODO: reinstate
      // GetDriversOrLoads(sel_, trace_drivers, &drivers_or_loads_);
      trace_idx_ = 0;
      if (!drivers_or_loads_.empty()) SetLocation(drivers_or_loads_[0]);
    }
    break;
  case 'c':
    if (!drivers_or_loads_.empty()) {
      trace_idx_++;
      if (trace_idx_ == drivers_or_loads_.size()) trace_idx_ = 0;
      SetLocation(drivers_or_loads_[trace_idx_]);
    }
    break;
  case 'v':
    show_vals_ = !show_vals_;
    // The flag state is reflected in the tooltips.
    tooltips_changed_ = true;
    break;
  case 'b':
  case 'f': {
    // Helper function to restore a state.
    auto load_state = [&](State s) {
      if (s.scope != scope_) {
        SetItem(&s.scope->asSymbol(), /*save_state*/ false);
      }
      line_idx_ = s.line_idx;
      col_idx_ = s.col_idx;
      max_col_idx_ = s.col_idx;
      scroll_row_ = s.scroll_row;
      scroll_col_ = s.scroll_col;
      item_for_design_tree_ = &s.scope->asSymbol();
    };
    if (ch == 'f') {
      // Can't go forward past the end.
      if (stack_idx_ >= state_stack_.size() - 1) break;
      auto &s = state_stack_[++stack_idx_];
      load_state(s);
    } else {
      // Go back in the stack if possible.
      if (stack_idx_ == 0) break;
      if (stack_idx_ == state_stack_.size()) {
        // If not currently doing any kind of stack navigation, get the top of
        // the stack as the new item, but save the current one.
        auto s = state_stack_[stack_idx_ - 1];
        SaveState();
        // Saving the current one has grown the stack, but since we're now going
        // down the stack, go back and point *before*.
        stack_idx_ -= 2;
        load_state(s);
      } else {
        auto &s = state_stack_[--stack_idx_];
        load_state(s);
      }
    }
    break;
  }
  case 'p':
    // Go to the next highlightable thing.
    if (auto it = src_info_.find(line_idx_); it != src_info_.end()) {
      for (const SourceInfo &info : it->second) {
        if (info.start_col > col_idx_) {
          col_idx_ = info.start_col;
          max_col_idx_ = col_idx_;
          break;
        }
      }
    }
    break;
  case 'w':
    if (Workspace::Get().Waves() != nullptr) {
      send_to_waves = true;
    }
    break;
  default: Panel::UIChar(ch);
  }
  bool line_moved = line_idx_ != prev_line_idx;
  bool col_moved = col_idx_ != prev_col_idx;
  // If the current line changed, move the cursor to the end if this new line is
  // shorter. If the new line is longer, move it back to as far as it used to
  // be.
  if (line_moved) {
    const int new_line_size = src_.LineLength(line_idx_);
    if (col_idx_ >= new_line_size) {
      col_idx_ = std::max(0, new_line_size - 1);
    } else {
      col_idx_ = std::min(max_col_idx_, std::max(0, new_line_size - 1));
    }
  }
  if (line_moved || col_moved) {
    // Figure out if anything should be highlighted.
    SelectItem();
  }
  if (send_to_waves) {
    item_for_waves_ = sel_;
  }
}

void SourcePanel::SelectItem() {
  sel_ = nullptr;
  const auto it = src_info_.find(line_idx_);
  if (it == src_info_.end()) return;
  for (const SourceInfo &info : it->second) {
    if (info.sym != nullptr && col_idx_ >= info.start_col && col_idx_ <= info.end_col) {
      sel_ = info.sym;
      break;
    }
  }
}

std::pair<int, int> SourcePanel::ScrollArea() const {
  // Account for the header.
  int h, w;
  getmaxyx(w_, h, w);
  return {h - 1, w};
}

std::optional<const slang::ast::Symbol *> SourcePanel::ItemForDesignTree() {
  if (item_for_design_tree_ == nullptr) return std::nullopt;
  const slang::ast::Symbol *item = item_for_design_tree_;
  item_for_design_tree_ = nullptr;
  return item;
}

std::optional<const slang::ast::Symbol *> SourcePanel::ItemForWaves() {
  if (item_for_waves_ == nullptr) return std::nullopt;
  const slang::ast::Symbol *item = item_for_waves_;
  item_for_waves_ = nullptr;
  return item;
}

void SourcePanel::SetLocation(const slang::ast::Symbol *item) {
  const slang::SourceManager *sm = Workspace::Get().SourceManager();
  // 0-based counting internally.
  const int line = sm->getLineNumber(item->location) - 1;
  // Avoid history entries on the same line.
  if (line_idx_ != line) SaveState();
  col_idx_ = src_.DisplayCol(line, sm->getColumnNumber(item->location) - 1);
  max_col_idx_ = col_idx_;
  SetLineAndScroll(line);
}

void SourcePanel::SaveState() {
  if (stack_idx_ < state_stack_.size()) {
    state_stack_.erase(state_stack_.begin() + stack_idx_, state_stack_.end());
  }
  state_stack_.push_back({
      .scope = scope_,
      .line_idx = line_idx_,
      .col_idx = col_idx_,
      .scroll_row = scroll_row_,
      .scroll_col = scroll_col_,
  });
  stack_idx_++;
  if (state_stack_.size() > kMaxStateStackSize) {
    state_stack_.pop_front();
    stack_idx_--;
  }
}

void SourcePanel::SetItem(const slang::ast::Symbol *item) { SetItem(item, /*save_state*/ true); }

void SourcePanel::SetItem(const slang::ast::Symbol *item, bool save_state) {
  if (item == nullptr) return;
  if (save_state && scope_ != nullptr) SaveState();

  scope_ = GetScopeForUI(item);
  // Clear out old info.
  src_info_.clear();
  sel_ = nullptr;
  // tokenizer_ = SimpleTokenizer();

  FindSymbols();
  FindKeywordsAndComments();
  // Extract the lines as individial string_views.
  const slang::ast::Symbol *sym = &scope_->asSymbol();
  const slang::SourceManager *sm = Workspace::Get().SourceManager();
  src_.ProcessBuffer(sm->getSourceText(sym->location.buffer()));
  const int line_idx = sm->getLineNumber(item->location) - 1;
  start_line_ = sm->getLineNumber(sym->getSyntax()->sourceRange().start());
  end_line_ = sm->getLineNumber(sym->getSyntax()->sourceRange().end());
  col_idx_ = src_.DisplayCol(line_idx, sm->getColumnNumber(item->location) - 1);
  max_col_idx_ = col_idx_;
  current_file_ = sm->getFileName(sym->location);

  // TODO - find keywords somehow
  // scope_->asSymbol().getSyntax(); // and then something....
  // int n = 0;
  // for (std::string_view s : src_) {
  //  tokenizer_.ProcessLine(s);
  //  // Add all useful identifiers in this line to the appropriate list.
  //  for (auto &[start_col, end_col] : tokenizer_.Keywords(n)) {
  //    nav_[n].push_back({.start_col = start_col, .end_col = end_col, .keyword = true});
  //  }
  //  n++;
  //}
  SetLineAndScroll(line_idx);
  BuildHeader();
  UpdateWaveData();
}

void SourcePanel::UpdateWaveData() {
  WaveData *waves = Workspace::Get().Waves();
  if (waves == nullptr) return;
  std::vector<const WaveData::Signal *> signals;
  // TODO: reinstante after waves are slang-ified.
  // for (auto &[id, item] : nav_) {
  //   auto signals_for_item = Workspace::Get().DesignToSignals(item);
  //   signals.insert(signals.end(), signals_for_item.begin(), signals_for_item.end());
  // }
  // // TODO: Loading for the full range here. Is there a way to bound it?
  // waves->LoadSignalSamples(signals, waves->TimeRange().first, waves->TimeRange().second);
}

bool SourcePanel::Search(bool search_down) {
  if (src_.Empty()) return false;
  if (search_text_.empty()) return false;
  int row = line_idx_;
  int col = col_idx_;
  const auto line_step = [&] {
    row += search_down ? 1 : -1;
    if (row >= static_cast<int>(src_.NumLines())) {
      row = 0;
    } else if (row < 0) {
      row = src_.NumLines() - 1;
    }
    col = search_down ? 0 : (src_.LineLength(row) - 1);
  };
  if (!search_preview_) {
    // Go past the current location so that next/prev doesn't just find what's
    // under the cursor. Can't do this in preview mode otherwise the result
    // would keep jumping with every new keypress.
    if (search_down) {
      if (col == src_.LineLength(row) - 1) {
        line_step();
      } else {
        col++;
      }
    } else {
      if (col == 0) {
        line_step();
      } else {
        col--;
      }
    }
  }
  const int start_row = row;
  while (1) {
    const auto pos =
        search_down ? src_[row].find(search_text_, col) : src_[row].rfind(search_text_, col);
    if (pos != std::string::npos) {
      search_start_col_ = pos;
      col_idx_ = src_.DisplayCol(row, pos);
      SetLineAndScroll(row);
      return true;
    }
    line_step();
    if (row == start_row) {
      search_start_col_ = -1;
      return false;
    }
  }
}

void SourcePanel::Resized() {
  const int win_w = getmaxx(w_);
  const int win_h = getmaxy(w_);
  const int ruler_size = NumDecimalDigits(win_h + scroll_row_ - 1) + 1;
  if (col_idx_ + ruler_size >= win_w) col_idx_ = win_w - 1 - ruler_size;
  Panel::Resized();
  BuildHeader();
}

void SourcePanel::SetLineAndScroll(int l) {
  Panel::SetLineAndScroll(l);
  max_col_idx_ = col_idx_;
  SelectItem();
}

std::vector<Tooltip> SourcePanel::Tooltips() const {
  std::vector<Tooltip> tt;
  if (Workspace::Get().Waves() != nullptr) {
    tt.push_back({"w", "add to waves"});
  }
  tt.push_back({"u", "up scope"});
  tt.push_back({"d", "goto def"});
  tt.push_back({"D", "drivers"});
  tt.push_back({"L", "loads"});
  tt.push_back({"c", "cycle DL"});
  tt.push_back({"b", "back"});
  tt.push_back({"f", "forward"});
  tt.push_back(
      {"v", "" + (show_vals_ ? std::string("SHOW/hide") : std::string("show/HIDE")) + " vals"});
  return tt;
}

} // namespace sv
