#pragma once
#include <span>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/widgets/fancy_log.hpp"

namespace fl::widgets {

class LogWall : public ftxui::ComponentBase {
public:
  LogWall(FancyLog &master, std::span<FancyLog *const> accounts)
      : master_(master), accounts_(accounts.begin(), accounts.end()) {}

  ftxui::Element Render() override {
    using namespace ftxui;

    auto master_col = window(text(" Master "), master_.Render()) | flex;

    auto blank_panel = []() -> Element {
      return window(text(" "),
                    vbox({
                        text("This space intentionally left blank") | dim,
                        filler(),
                    })) |
             flex;
    };

    auto acct_panel = [&](size_t idx) -> Element {
      auto title = text(" Account " + std::to_string(idx + 1) + " ");
      FancyLog *l = (idx < accounts_.size()) ? accounts_[idx] : nullptr;
      auto body = l ? l->Render() : blank_panel() | flex; // fallback if nullptr
      // Note: FancyLog already has borders; we still window() per-account for
      // title. If that double-borders, swap window(...) for vbox({title, body})
      // etc.
      return window(title, body) | flex;
    };

    auto col_pair = [&](size_t top_idx) -> Element {
      const size_t bot_idx = top_idx + 1;

      Element top = acct_panel(top_idx);
      Element bot =
          (bot_idx < accounts_.size()) ? acct_panel(bot_idx) : blank_panel();

      // No separator() between them; FancyLog panels already have borders.
      return vbox({top | flex, bot | flex}) | flex;
    };

    Elements cols;
    cols.push_back(master_col);

    // Accounts in 2-row columns: (0,1), (2,3), (4,5), ...
    for (size_t i = 0; i < accounts_.size(); i += 2) {
      // cols.push_back(separator());
      cols.push_back(col_pair(i));
    }

    // If there are zero accounts, still show master only.
    return hbox(std::move(cols)) | flex;
  }

private:
  FancyLog &master_;
  std::vector<FancyLog *> accounts_;
};

} // namespace fl::widgets
