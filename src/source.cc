#include "source.h"
#include "BaseClass.h"
#include "color.h"
#include "utils.h"
#include <curses.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <uhdm/headers/constant.h>
#include <uhdm/headers/gen_scope.h>
#include <uhdm/headers/gen_scope_array.h>
#include <uhdm/headers/module.h>
#include <uhdm/headers/net.h>
#include <uhdm/headers/param_assign.h>
#include <uhdm/headers/parameter.h>
#include <uhdm/headers/variables.h>

namespace sv {
namespace {

constexpr int kMaxStateStackSize = 500;

// Remove newline characters from the start or end of the string.
void trim_string(std::string &s) {
  if (s.empty()) return;
  if (s.size() > 1 && s[s.size() - 2] == '\r' && s[s.size() - 1] == '\n') {
    s.erase(s.size() - 2, 1);
  }
  if (s[s.size() - 1] == '\n') s.erase(s.size() - 1, 1);
}

int num_decimal_digits(int n) {
  int ret = 0;
  do {
    ret++;
    n /= 10;
  } while (n != 0);
  return ret;
}

} // namespace

std::optional<std::pair<int, int>> Source::CursorLocation() const {
  if (item_ == nullptr) return std::nullopt;
  // Compute width of the line numbers. Minus 1 to account for the header. Add
  // one to the final width to account for the line number margin.
  int linenum_width = 1 + num_decimal_digits(scroll_row_ + getmaxy(w_) - 1);
  return std::pair(line_idx_ - scroll_row_ + 1,
                   col_idx_ - scroll_col_ + linenum_width);
}

std::string Source::GetHeader(int max_w = 0) {
  std::string type;
  switch (item_->VpiType()) {
  case vpiModule: {
    auto m = dynamic_cast<const UHDM::module *>(item_);
    type = m->VpiFullName();
    break;
  }
  case vpiGenScopeArray: {
    auto ga = dynamic_cast<const UHDM::gen_scope_array *>(item_);
    type = ga->VpiFullName();
    break;
  }
  default: type = "Unknown type: " + std::to_string(item_->VpiType()); break;
  }
  const std::string separator = " | ";
  auto s = current_file_ + separator + StripWorklib(type);
  // Attempt to strip as many leading directories from the front of the header
  // until it fits. If it still doesn't fit then oh well, it will get cut off
  // in the Draw funtion.
  if (max_w != 0 && s.size() > max_w) {
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
    } while (s.size() - pos + 3 > max_w);
    if (pos > 0) {
      s.replace(0, pos, "...");
    }
  }
  return s;
}

void Source::Draw() {
  werase(w_);
  wattrset(w_, A_NORMAL);
  if (item_ == nullptr) {
    mvwprintw(w_, 0, 0, "Module instance source code appears here.");
    return;
  }
  const int win_h = getmaxy(w_);
  const int win_w = getmaxx(w_);
  const int max_digits = num_decimal_digits(scroll_row_ + win_h);
  SetColor(w_, kSourceHeaderPair);
  mvwaddnstr(w_, 0, 0, GetHeader(win_w).c_str(), win_w);

  if (lines_.empty()) {
    SetColor(w_, kSourceTextPair);
    mvwprintw(w_, 1, 0, "Unable to open file");
    return;
  }
  // See if the horizontal scroll position needs to be fixed.
  // TODO: Kinda ugly to do this here, but this is where the line number size is
  // computed and that's needed to see where the cursor actually lands in the
  // window.
  if (col_idx_ - scroll_col_ + max_digits + 2 >= win_w) {
    scroll_col_ = col_idx_ + max_digits + 2 - win_w;
  }
  const auto highlight_attr = has_focus_ ? A_REVERSE : A_UNDERLINE;
  int sel_pos = 0; // Save selection start position.
  for (int y = 1; y < win_h; ++y) {
    int line_idx = y - 1 + scroll_row_;
    if (line_idx >= lines_.size()) break;
    const int line_num = line_idx + 1;
    const int line_num_size = num_decimal_digits(line_num);
    const bool active = line_num >= start_line_ && line_num <= end_line_;
    const int text_color = active ? kSourceTextPair : kSourceInactivePair;
    SetColor(w_, line_idx == line_idx_ ? kSourceCurrentLineNrPair
                                       : kSourceLineNrPair);
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
    const auto &s = lines_[line_idx];
    const auto &keywords = tokenizer_.Keywords(line_idx);
    const auto &identifiers = tokenizer_.Identifiers(line_idx);
    const auto &comments = tokenizer_.Comments(line_idx);
    bool in_keyword = false;
    bool in_identifier = false;
    bool in_comment = false;
    int k_idx = 0;
    int id_idx = 0;
    int c_idx = 0;
    // Subtract the horizontal scroll postion so that the logic iterates over
    // all text characters. Otherwise the text that is cut off doesn't get
    // recognized as keywords, identifiers etc.
    for (int x = max_digits + 1 - scroll_col_; x < win_w; ++x) {
      const int pos = x - max_digits - 1 + scroll_col_;
      if (pos >= s.size()) {
        // Put a space after an empty line so there is a place to show the
        // cursor.
        if (s.size() == 0) {
          waddch(w_, ' ');
          wattroff(w_, highlight_attr);
        }
        break;
      }
      // Figure out if we have to switch to a new color
      if (active && !in_identifier && identifiers.size() > id_idx &&
          identifiers[id_idx].first == pos) {
        auto id = identifiers[id_idx].second;
        bool cursor_in_id = line_idx == line_idx_ &&
                            col_idx_ >= identifiers[id_idx].first &&
                            col_idx_ < (identifiers[id_idx].first + id.size());
        if (nav_.find(id) != nav_.end()) {
          if (nav_[id]->VpiType() == vpiModule) {
            SetColor(w_, kSourceInstancePair);
          } else if (nav_[id]->VpiType() == vpiNet ||
                     nav_[id]->VpiType() == vpiBitVar) {
            SetColor(w_, kSourceIdentifierPair);
          }
          if (nav_[id] == sel_ && cursor_in_id) {
            wattron(w_, highlight_attr);
            sel_pos = pos;
          }
        } else if (params_.find(id) != params_.end()) {
          SetColor(w_, kSourceParamPair);
          if (sel_param_ == id && cursor_in_id) {
            wattron(w_, highlight_attr);
            sel_pos = pos;
          }
        }
        in_identifier = true;
      } else if (active && !in_keyword && keywords.size() > k_idx &&
                 keywords[k_idx].first == pos) {
        in_keyword = true;
        SetColor(w_, kSourceKeywordPair);
      } else if (!in_comment && comments.size() > c_idx &&
                 comments[c_idx].first == pos) {
        // Highlight inactive comments still.
        in_comment = true;
        SetColor(w_, kSourceCommentPair);
      }

      // Skip any characters that are before the line numbers
      if (x > max_digits) waddch(w_, s[pos]);
      // See if the color should be turned off.
      if (in_keyword && keywords[k_idx].second == pos) {
        in_keyword = false;
        k_idx++;
        SetColor(w_, text_color);
      } else if (in_identifier &&
                 (identifiers[id_idx].first +
                  identifiers[id_idx].second.size() - 1) == pos) {
        in_identifier = false;
        id_idx++;
        SetColor(w_, text_color);
        wattroff(w_, highlight_attr);
      } else if (in_comment && comments[c_idx].second == pos) {
        in_comment = false;
        c_idx++;
        SetColor(w_, text_color);
      }
    }
    wattroff(w_, highlight_attr);
  }
  // Draw the current value of the selected item.
  // TODO: Also wave values, when sel_ is not null and a net/var.
  if (show_vals_ && !sel_param_.empty()) {
    std::string val;
    if (!sel_param_.empty()) {
      val = params_[sel_param_];
    }
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
        start_col + val.size() <= win_w ||
        (((win_w - start_col) > (start_col + sel_param_.size() - 1)));
    SetColor(w_, kSourceValuePair);
    wattron(w_, A_BOLD);
    int connector_col = start_col + (right ? 0 : sel_param_.size() - 3 + 2);
    int max_chars_connector = win_w - connector_col;
    mvwaddnstr(w_, val_line + (above ? -1 : 1), connector_col, " | ",
               max_chars_connector);
    const int col =
        start_col + (right ? 0 : sel_param_.size() - val.size() + 2);
    int max_chars = win_w - col;
    mvwaddnstr(w_, val_line + (above ? -4 : 4), col, box.c_str(), max_chars);
    mvwaddnstr(w_, val_line + (above ? -3 : 3), col, val.c_str(), max_chars);
    mvwaddnstr(w_, val_line + (above ? -2 : 2), col, box.c_str(), max_chars);
  }
}

void Source::UIChar(int ch) {
  if (item_ == nullptr) return;
  int prev_line_idx = line_idx_;
  int prev_col_idx = col_idx_;
  switch (ch) {
  case 'u':
    if (showing_def_) {
      // Go back up to the instance location if showing a definition
      SetItem(item_, false);
      item_for_hier_ = item_;
    } else {
      auto p = item_->VpiParent();
      while (p != nullptr && p->VpiType() != vpiModule &&
             p->VpiType() != vpiGenScopeArray) {
        p = p->VpiParent();
      }
      if (p != nullptr) {
        SetItem(p, false);
        item_for_hier_ = p;
      }
    }
    break;
  case 'h':
  case 0x104: // left
    if (scroll_col_ > 0 && scroll_col_ - col_idx_ == 0) {
      scroll_col_--;
    } else if (col_idx_ > 0) {
      col_idx_--;
      max_col_idx_ = col_idx_;
    }
    break;
  case 'l':
  case 0x105: // right
    if (col_idx_ >= lines_[line_idx_].size() - 1) break;
    col_idx_++;
    max_col_idx_ = col_idx_;
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
    col_idx_ = lines_[line_idx_].size() - 1;
    max_col_idx_ = col_idx_;
    break;
  case 'd':
    // Go to definition of a module instance.
    if (sel_ != nullptr) {
      if (sel_->VpiType() == vpiModule) {
        item_for_hier_ = sel_;
        SetItem(sel_, true);
        // Don't do anything more.
        return;
      } else {
        SetLineAndScroll(sel_->VpiLineNo() - 1);
        col_idx_ = sel_->VpiColumnNo() - 1;
        max_col_idx_ = col_idx_;
      }
    }
    break;
  case 'v': show_vals_ = !show_vals_; break;
  case 'f': {
    // Can't go forward past the end.
    if (stack_idx_ >= state_stack_.size() - 1) break;
    auto &s = state_stack_[++stack_idx_];
    SetItem(s.item, s.show_def, /* save_state */ false);
    line_idx_ = s.line_idx;
    scroll_col_ = s.scroll_row;
    item_for_hier_ = s.item;
    break;
  }
  case 'b': {
    // Go back in the stack if possible.
    if (stack_idx_ == 0) break;
    if (stack_idx_ == state_stack_.size()) {
      // If not currently doing any kind of stack navigation, get the top of the
      // stack as the new item, but save the current one.
      auto s = state_stack_[stack_idx_ - 1];
      SetItem(s.item, s.show_def, true);
      // Saving the current one has grown the stack, but since we're now going
      // down the stack, go back and point *before*.
      stack_idx_ -= 2;
      line_idx_ = s.line_idx;
      scroll_col_ = s.scroll_row;
      item_for_hier_ = s.item;
    } else {
      auto &s = state_stack_[--stack_idx_];
      SetItem(s.item, s.show_def, false);
      line_idx_ = s.line_idx;
      scroll_col_ = s.scroll_row;
      item_for_hier_ = s.item;
    }
    break;
  }
  case 'w': {
    // Go to the next highlightable thing.
    int param_pos = -1;
    int identifier_pos = -1;
    // Look for naviable items
    if (nav_by_line_.find(line_idx_) != nav_by_line_.end()) {
      for (auto &item : nav_by_line_[line_idx_]) {
        if (item.first > col_idx_) {
          identifier_pos = item.first;
          break;
        }
      }
    }
    // Look for parameters
    if (params_by_line_.find(line_idx_) != params_by_line_.end()) {
      for (auto &p : params_by_line_[line_idx_]) {
        if (p.first > col_idx_) {
          param_pos = p.first;
          break;
        }
      }
    }
    // Pick the closest one.
    if (param_pos > 0 && identifier_pos > 0) {
      col_idx_ = std::min(param_pos, identifier_pos);
    } else if (param_pos > 0 || identifier_pos > 0) {
      col_idx_ = std::max(param_pos, identifier_pos);
    }
  }
  }
  Panel::UIChar(ch);
  bool line_moved = line_idx_ != prev_line_idx;
  bool col_moved = col_idx_ != prev_col_idx;
  // If the current line changed, move the cursor to the end if this new line is
  // shorter. If the new line is longer, move it back to as far as it used to
  // be.
  if (line_moved) {
    int new_line_size = lines_[line_idx_].size();
    if (col_idx_ >= new_line_size) {
      col_idx_ = std::max(0, new_line_size - 1);
    } else {
      col_idx_ = std::min(max_col_idx_, std::max(0, new_line_size - 1));
    }
  }
  if (line_moved || col_moved) {
    // Figure out if anything should be highlighted.
    sel_ = nullptr;
    sel_param_.clear();
    if (nav_by_line_.find(line_idx_) != nav_by_line_.end()) {
      for (auto &item : nav_by_line_[line_idx_]) {
        if (col_idx_ >= item.first &&
            col_idx_ < (item.first + item.second->VpiName().size())) {
          sel_ = item.second;
          break;
        }
      }
    }
    if (params_by_line_.find(line_idx_) != params_by_line_.end()) {
      for (auto &p : params_by_line_[line_idx_]) {
        if (col_idx_ >= p.first && col_idx_ < (p.first + p.second.size())) {
          sel_param_ = p.second;
        }
      }
    }
  }
}

std::pair<int, int> Source::ScrollArea() {
  // Account for the header.
  int h, w;
  getmaxyx(w_, h, w);
  return {h - 1, w};
}

std::optional<const UHDM::BaseClass *> Source::ItemForHierarchy() {
  if (item_for_hier_ == nullptr) return std::nullopt;
  auto item = item_for_hier_;
  item_for_hier_ = nullptr;
  return item;
}

void Source::SetItem(const UHDM::BaseClass *item, bool show_def) {
  SetItem(item, show_def, /*save_state*/ true);
}

void Source::SetItem(const UHDM::BaseClass *item, bool show_def,
                     bool save_state) {
  if (save_state && item_ != nullptr) {
    if (stack_idx_ < state_stack_.size()) {
      state_stack_.erase(state_stack_.begin() + stack_idx_, state_stack_.end());
    }
    state_stack_.push_back({
        .item = item_,
        .line_idx = line_idx_,
        .show_def = showing_def_,
    });
    stack_idx_++;
    if (state_stack_.size() > kMaxStateStackSize) {
      state_stack_.pop_front();
      stack_idx_--;
    }
  }
  item_ = item;
  showing_def_ = show_def;
  // Clear out old info.
  lines_.clear();
  nav_.clear();
  nav_by_line_.clear();
  params_.clear();
  params_by_line_.clear();
  sel_ = nullptr;
  sel_param_.clear();
  tokenizer_ = SimpleTokenizer();
  line_idx_ = 0;
  col_idx_ = 0;
  max_col_idx_ = 0;

  // This lamda recurses through all generate blocks in the item, adding any
  // navigable things found to the hashes.
  std::function<void(const UHDM::BaseClass *)> find_navigable_items =
      [&](const UHDM::BaseClass *item) {
        switch (item->VpiType()) {
        case vpiModule: {
          auto m = dynamic_cast<const UHDM::module *>(item);
          if (m->Nets() != nullptr) {
            for (auto &n : *m->Nets()) {
              nav_[n->VpiName()] = n;
            }
          }
          if (m->Variables() != nullptr) {
            for (auto &v : *m->Variables()) {
              nav_[v->VpiName()] = v;
            }
          }
          if (m->Modules() != nullptr) {
            for (auto &sub : *m->Modules()) {
              nav_[sub->VpiName()] = sub;
            }
            // No recursion into modules since that source code is not in scope.
          }
          if (m->Param_assigns() != nullptr) {
            for (auto &pa : *m->Param_assigns()) {
              // Elaborated designs should only have this type of assignment.
              if (pa->Lhs()->VpiType() != vpiParameter) continue;
              if (pa->Rhs()->VpiType() != vpiConstant) continue;
              auto p = dynamic_cast<const UHDM::parameter *>(pa->Lhs());
              auto c = dynamic_cast<const UHDM::constant *>(pa->Rhs());
              params_[p->VpiName()] = c->VpiDecompile();
            }
          }
          if (m->Gen_scope_arrays() != nullptr) {
            for (auto &sub_ga : *m->Gen_scope_arrays()) {
              find_navigable_items(sub_ga);
            }
          }
          break;
        }
        case vpiGenScopeArray: {
          auto ga = dynamic_cast<const UHDM::gen_scope_array *>(item);
          // Surelog always uses a gen_scope_array to wrap a single gen_scope
          // for any generate block, wether it's a single if statement or one
          // iteration of an unrolled for loop.
          if (ga->Gen_scopes() != nullptr) {
            auto &g = (*ga->Gen_scopes())[0];
            if (g->Nets() != nullptr) {
              for (auto &n : *g->Nets()) {
                nav_[n->VpiName()] = n;
              }
            }
            if (g->Variables() != nullptr) {
              for (auto &v : *g->Variables()) {
                nav_[v->VpiName()] = v;
              }
            }
            if (g->Modules() != nullptr) {
              for (auto &sub : *g->Modules()) {
                nav_[sub->VpiName()] = sub;
              }
              // No recursion into modules since that source code is not in
              // scope.
            }
            if (g->Gen_scope_arrays() != nullptr) {
              for (auto &sub_ga : *g->Gen_scope_arrays()) {
                find_navigable_items(sub_ga);
              }
            }
          }
          break;
        }
        }
      };
  int line_num = 1;
  switch (item->VpiType()) {
  case vpiModule: {
    auto m = dynamic_cast<const UHDM::module *>(item);
    // Treat top modules as an instance open.
    if (show_def && m->VpiParent() != nullptr) {
      // VpiDefFile isn't super useful here, still need the definition to get
      // the start and end line number.
      auto def = Workspace::Get().GetDefinition(m);
      current_file_ = def->VpiFile();
      line_num = def->VpiLineNo();
      start_line_ = def->VpiLineNo();
      end_line_ = def->VpiEndLineNo();
      // Add all instances in this module as navigable instances.
      find_navigable_items(m);
    } else {
      // If showing the instance, collect stuff in the owning instance.
      // This will allow nets in the port connections to be navigable.
      current_file_ = m->VpiFile();
      line_num = m->VpiLineNo();
      col_idx_ = m->VpiColumnNo() - 1;
      max_col_idx_ = col_idx_;
      auto p = item->VpiParent();
      while (p != nullptr && p->VpiType() != vpiModule) {
        find_navigable_items(p);
        p = p->VpiParent();
      }
      if (p != nullptr) {
        // Find the nets and stuff in the containing module, but use the line
        // number of that module's definition. Otherwise the line numbers are of
        // the containing module's instance in its parent!
        find_navigable_items(p);
        auto def = Workspace::Get().GetDefinition(
            dynamic_cast<const UHDM::module *>(p));
        start_line_ = def->VpiLineNo();
        end_line_ = def->VpiEndLineNo();
      } else {
        // For top modules:
        find_navigable_items(m);
        start_line_ = m->VpiLineNo();
        end_line_ = m->VpiEndLineNo();
      }
    }
    break;
  }
  case vpiGenScopeArray: {
    auto ga = dynamic_cast<const UHDM::gen_scope_array *>(item);
    find_navigable_items(ga);
    current_file_ = ga->VpiFile();
    line_num = ga->VpiLineNo();
    col_idx_ = ga->VpiColumnNo() - 1;
    max_col_idx_ = col_idx_;
    // Find the containing module, since that's all in the same file and in
    // scope.
    auto p = item->VpiParent();
    while (p != nullptr && p->VpiType() != vpiModule) {
      find_navigable_items(p);
      p = p->VpiParent();
    }
    if (p != nullptr) {
      find_navigable_items(p);
      // Ditto
      auto def =
          Workspace::Get().GetDefinition(dynamic_cast<const UHDM::module *>(p));
      start_line_ = def->VpiLineNo();
      end_line_ = def->VpiEndLineNo();
    }
    break;
  }
  default:
    // Do nothing for unknown types.
    return;
  }
  // Read all lines. TODO: Handle huge files.
  std::ifstream is(current_file_);
  if (is.fail()) return; // Draw function handles file open issues.
  std::string s;
  int n = 0;
  while (std::getline(is, s)) {
    trim_string(s);
    tokenizer_.ProcessLine(s);
    lines_.push_back(std::move(s));
    // Add all useful identifiers in this line to the appropriate list.
    for (auto &id : tokenizer_.Identifiers(n)) {
      if (params_.find(id.second) != params_.end()) {
        params_by_line_[n].push_back(id);
      } else if (nav_.find(id.second) != nav_.end()) {
        nav_by_line_[n].push_back({id.first, nav_[id.second]});
      }
    }
    n++;
  }
  SetLineAndScroll(line_num - 1);
}

bool Source::ReceiveText(const std::string &s, bool preview) { return false; }

std::string Source ::Tooltip() const {
  std::string tt = "u:up scope";
  tt += "  d:goto def";
  tt += "  b:back";
  tt += "  f:forward";
  tt += "  v:";
  tt += (show_vals_ ? "SHOW/hide" : "show/HIDE");
  tt += " vals";
  return tt;
}

} // namespace sv
