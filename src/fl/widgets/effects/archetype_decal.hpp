#pragma once

#include "fl/widgets/effects/decal.hpp"

namespace fl::widgets::effects {

struct BeamSpan {
  int start{0};
  int end{0};
};

[[nodiscard]] int beam_visual_length(int width) noexcept;
[[nodiscard]] BeamSpan beam_span_for_progress(int width,
                                              float progress) noexcept;
[[nodiscard]] int projectile_embed_x(int width) noexcept;
[[nodiscard]] int projectile_head_x_for_progress(int width,
                                                 float progress) noexcept;
[[nodiscard]] bool projectile_is_holding(float progress) noexcept;

class ArchetypeDecal final : public DecalAnimation {
public:
  ArchetypeDecal(DecalAnimationKind kind, int width, int height,
                 DecalConfig config = {});

  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override;

private:
  DecalAnimationKind kind_;
  int width_ = 0;
  int height_ = 0;
  DecalConfig config_;
};

} // namespace fl::widgets::effects
