#include "fl/widgets/effects/hitpoint_number.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace fl::widgets::effects {

HitpointNumber::HitpointNumber(int width, int height, ftxui::Color color,
                               int hitpoints, DecalConfig config)
    : width_(std::max(1, width)), height_(std::max(1, height)), color_(color),
      hitpoints_(std::max(0, hitpoints)), config_(config) {}

Frame HitpointNumber::render(float progress) const {
  const float t = std::clamp(progress, 0.0F, 1.0F);
  Frame frame{width_, height_, {}};
  frame.cells.resize(static_cast<std::size_t>(width_ * height_));

  const auto label = std::to_string(hitpoints_);
  const int bottom = std::max(0, height_ - 2);

  float y_float = 0.0F;
  if (t < 0.62F) {
    const float drop = t / 0.62F;
    const float eased = drop * drop * (3.0F - 2.0F * drop);
    y_float = eased * static_cast<float>(bottom);
  } else {
    const float bounce_t = (t - 0.62F) / 0.38F;
    const float lift =
        std::sin(bounce_t * 6.2831853F) * (1.0F - bounce_t) * 2.0F;
    y_float = static_cast<float>(bottom) - std::max(0.0F, lift);
  }

  const int y = std::clamp(static_cast<int>(std::round(y_float)), 0, bottom);
  const int x0 = std::max(0, (width_ - static_cast<int>(label.size())) / 2);
  const float fade_in = std::clamp(t / 0.12F, 0.0F, 1.0F);

  for (std::size_t i = 0; i < label.size(); ++i) {
    const int x = x0 + static_cast<int>(i);
    if (x >= width_) {
      break;
    }

    auto &cell = frame.at(x, y);
    cell.glyph = label[i];
    cell.fg = color_;
    cell.alpha = fade_in;
  }

  return frame;
}

float HitpointNumber::duration_seconds() const {
  return config_.duration_seconds;
}

std::string_view HitpointNumber::name() const { return "HitpointNumber"; }

DecalAnimationKind HitpointNumber::kind() const {
  return DecalAnimationKind::HitpointNumber;
}

} // namespace fl::widgets::effects
