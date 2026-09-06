#pragma once
#include <ftxui/component/component_base.hpp>
#include "fl/context.hpp"

namespace fl::widgets {
class RaidView : public ftxui::ComponentBase {
public:
  explicit RaidView(fl::context::AccountCtx ctx) : ctx_(ctx) {}
  ftxui::Element Render() override;
private:
  fl::context::AccountCtx ctx_;
};
} // namespace fl::widgets
