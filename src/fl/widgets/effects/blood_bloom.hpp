#pragma once

#include <memory>
#include <string_view>

#include "fl/widgets/effects/decal.hpp"

namespace fl::widgets::effects {

class BloodBloom final : public DecalAnimation {
public:
  BloodBloom(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override;

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

} // namespace fl::widgets::effects
