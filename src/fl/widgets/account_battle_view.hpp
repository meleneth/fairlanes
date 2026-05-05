#pragma once

// AccountBattleView

#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::widgets {
using fl::context::AccountCtx;

class AccountBattleView : public ftxui::ComponentBase {
public:
  AccountBattleView(AccountCtx context);
  ftxui::Element Render() override;
  ~AccountBattleView() override = default;

private:
  ftxui::Component body_{nullptr};
  AccountCtx ctx_;
};

} // namespace fl::widgets
