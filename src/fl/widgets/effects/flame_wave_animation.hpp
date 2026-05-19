#pragma once

#include <string_view>

#include "fl/widgets/effects/decal.hpp"
#include "fl/widgets/effects/flame_wave.hpp"

namespace fl::widgets::effects {

class FlameWaveAnimation final : public DecalAnimation {
public:
  FlameWaveAnimation(int width, int height, DecalConfig config = {});

  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override;

private:
  int width_ = 0;
  int height_ = 0;
  FlameWave flame_;
};

} // namespace fl::widgets::effects
