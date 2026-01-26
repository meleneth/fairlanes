#pragma once
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::widgets {

class LogWall : public ftxui::ComponentBase {
public:
  LogWall(FancyLog &master, std::deque<fl::primitives::AccountData> &accounts)
      : master_(master), accounts_(&accounts) {}
  ftxui::Element Render() override {
    using namespace ftxui;

    auto blank_panel_content = []() -> Element {
      return vbox({
                 text("This space intentionally left blank") | dim,
                 filler(),
             }) |
             frame | flex;
    };

    auto render_log_or_blank = [&](fl::widgets::FancyLog *log) -> Element {
      if (log == nullptr) {
        return blank_panel_content();
      }
      // frame is the key: it allows the log to shrink instead of owning the
      // world.
      return log->Render() | frame | vscroll_indicator | flex;
    };

    auto master_col = window(text(" Master "), master_.Render() | frame |
                                                   vscroll_indicator | flex) |
                      flex;

    auto account_stack = [&](size_t ai) -> Element {
      if (ai >= accounts_->size()) {
        return window(text(" "), blank_panel_content()) | flex;
      }

      auto &acct = (*accounts_)[ai];

      Elements stack;

      // Account log pane
      stack.push_back(window(text(" Account " + std::to_string(ai + 1) + " "),
                             render_log_or_blank(acct.log_.get())) |
                      flex);

      // Party panes
      for (size_t pi = 0; pi < acct.parties_.size(); ++pi) {
        auto &party = acct.parties_[pi];
        stack.push_back(window(text(" Party " + std::to_string(pi + 1) + " "),
                               render_log_or_blank(party.log_.get())) |
                        flex);
      }

      return vbox(std::move(stack)) | flex;
    };

    Elements cols;
    cols.push_back(master_col);

    // Keep your existing "pairs in a column" behavior.
    for (size_t ai = 0; ai < accounts_->size(); ai += 2) {
      Element top = account_stack(ai);
      Element bot = (ai + 1 < accounts_->size())
                        ? account_stack(ai + 1)
                        : window(text(" "), blank_panel_content()) | flex;
      cols.push_back(vbox({top | flex, bot | flex}) | flex);
    }

    return hbox(std::move(cols)) | flex;
  }

private:
  FancyLog &master_;
  std::deque<fl::primitives::AccountData> *accounts_;
};

} // namespace fl::widgets
