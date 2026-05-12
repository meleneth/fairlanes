#pragma once

// AccountBattleView

#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::widgets {
using fl::context::AccountCtx;

class AccountBattleView : public ftxui::ComponentBase {
public:
  AccountBattleView(AccountCtx context);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  ~AccountBattleView() override = default;

private:
  ftxui::Component body_{nullptr};
  AccountCtx ctx_;
  int focused_log_{0};
};

} // namespace fl::widgets
