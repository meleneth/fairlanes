#pragma once

#include <ftxui/screen/color.hpp>

#include "fl/widgets/effects/decal.hpp"

namespace fl::widgets::effects {

class HitpointNumber final : public DecalAnimation {
public:
  HitpointNumber(int width, int height, ftxui::Color color, int hitpoints,
                 DecalConfig config = {});

  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override;

private:
  int width_ = 0;
  int height_ = 0;
  ftxui::Color color_;
  int hitpoints_ = 0;
  DecalConfig config_;
};

} // namespace fl::widgets::effects
