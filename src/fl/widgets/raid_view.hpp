#pragma once
#include "fl/context.hpp"
#include <ftxui/component/component_base.hpp>

namespace fl::widgets {
class RaidView : public ftxui::ComponentBase {
public:
  explicit RaidView(fl::context::AccountCtx ctx) : ctx_(ctx) {}
  ftxui::Element Render() override;

private:
  fl::context::AccountCtx ctx_;
};
} // namespace fl::widgets
