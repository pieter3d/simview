#include "tree_data.h"
#include <functional>

namespace sv {

namespace {
// Limit number of instances dumped into the UI at once.
constexpr int kMaxExpandInstances = 100;
} // namespace

void TreeData::ToggleExpand(int idx) {
  // Helper function to find the parent.
  auto get_parent_idx = [&](int idx) {
    const int depth = list_[idx]->Depth();
    while (idx > 0) {
      if (list_[idx]->Depth() < depth) {
        return idx;
      }
      idx--;
    }
    return -1;
  };

  if (list_.empty()) return;
  auto item = list_[idx];
  if (item->MoreIdx() != 0) {
    // Check if the "... more ..." entry needs to be further expanded.
    int stopped_pos = item->MoreIdx();
    item->SetMoreIdx(0); // Indicate it isn't a more-expander anymore.
    std::vector<TreeItem *> new_entries;
    auto *parent = list_[get_parent_idx(idx)];
    for (int i = stopped_pos + 1; i < parent->NumChildren(); ++i) {
      const int more_idx =
          (more_enabled_ && (i - stopped_pos) >= kMaxExpandInstances) ? i : 0;
      new_entries.push_back(parent->Child(i));
      new_entries.back()->SetDepth(item->Depth());
      new_entries.back()->SetMoreIdx(more_idx);
      if (more_idx != 0) break;
    }
    // Insert new entries after the more-expander.
    list_.insert(list_.begin() + idx + 1, new_entries.cbegin(),
                 new_entries.cend());
    return;
  } else if (!item->Expandable()) {
    return;
  } else if (item->Expanded()) {
    // Delete everything under the current index that has greater depth.
    int last = idx + 1;
    while (last < list_.size() && list_[last]->Depth() > item->Depth()) {
      last++;
    }
    list_.erase(list_.begin() + idx + 1, list_.begin() + last);
    item->SetExpanded(false);
  } else {
    std::vector<TreeItem *> new_entries;
    std::function<void(TreeItem *, std::vector<TreeItem *> &)>
        recurse_add_subs = [&](TreeItem *item, std::vector<TreeItem *> &list) {
          for (int i = 0; i < item->NumChildren(); ++i) {
            auto child = item->Child(i);
            list.push_back(child);
            child->SetDepth(item->Depth() + 1);
            // Expand anyway if there are only limited items to go.
            if (more_enabled_ && i >= kMaxExpandInstances &&
                (item->NumChildren() - 1) < kMaxExpandInstances) {
              child->SetMoreIdx(i);
              break;
            } else if (child->Expanded()) {
              recurse_add_subs(child, list);
            }
          }
        };
    recurse_add_subs(item, new_entries);
    // Insert new entries after opened item
    list_.insert(list_.begin() + idx + 1, new_entries.cbegin(),
                 new_entries.cend());
    item->SetExpanded(true);
  }
}

} // namespace sv
