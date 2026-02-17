#pragma once

// AccountBattleView

#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::widgets {
using fl::context::EntityCtx;

class AccountBattleView : public ftxui::ComponentBase {
public:
  AccountBattleView(fl::context::EntityCtx context);
  ftxui::Element Render() override;
  ~AccountBattleView() override = default;

private:
  ftxui::Component body_{nullptr};
  fl::context::EntityCtx ctx_;
};

} // namespace fl::widgets
