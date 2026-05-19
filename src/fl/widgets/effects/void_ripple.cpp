#include "fl/widgets/effects/void_ripple.hpp"

#include "fl/widgets/effects/decal_helpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

namespace fl::widgets::effects {
namespace {
using detail::apply_bg_glow;
using detail::apply_cell;
using detail::clamp01;
using detail::clamp_progress;
using detail::in_bounds;
using detail::lerp_color;
using detail::make_frame;
using detail::palette;
} // namespace

struct VoidRipple::Impl {
  struct Center {
    float x = 0.0f;
    float y = 0.0f;
    float start = 0.0f;
  };
  struct Star {
    int x = 0;
    int y = 0;
    float start = 0.0f;
    char glyph = '.';
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Center> centers;
  std::vector<Star> stars;
};

VoidRipple::VoidRipple(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0x01D5A99Fu);
  std::uniform_real_distribution<float> cx(
      0.0f, static_cast<float>(std::max(1, width - 1)));
  std::uniform_real_distribution<float> cy(
      0.0f, static_cast<float>(std::max(1, height - 1)));
  std::uniform_real_distribution<float> start(0.0f, 0.55f);

  const int center_count = std::max(1, std::min(3, width / 25 + 1));
  for (int i = 0; i < center_count; ++i) {
    impl->centers.push_back({cx(rng), cy(rng), start(rng)});
  }

  static constexpr std::array<char, 3> glyphs{'.', '*', '+'};
  const int stars = std::max(24, width * height / 12);
  for (int i = 0; i < stars; ++i) {
    impl->stars.push_back(
        {std::uniform_int_distribution<int>(0, std::max(0, width - 1))(rng),
         std::uniform_int_distribution<int>(0, std::max(0, height - 1))(rng),
         std::uniform_real_distribution<float>(0.0f, 1.0f)(rng),
         glyphs[static_cast<std::size_t>(i % glyphs.size())]});
  }

  impl_ = std::move(impl);
}

Frame VoidRipple::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto purple = palette(30);
  const auto violet = palette(31);
  const auto dark_void = palette(0);

  for (const auto &center : impl_->centers) {
    if (t < center.start) {
      continue;
    }
    const float local =
        (t - center.start) / std::max(0.001f, 1.0f - center.start);
    const float max_r = std::sqrt(static_cast<float>(
        impl_->width * impl_->width + impl_->height * impl_->height));
    const float r = local * max_r * 0.6f;
    const float thickness = std::max(1.1f, 2.4f - local);
    for (int y = 0; y < impl_->height; ++y) {
      for (int x = 0; x < impl_->width; ++x) {
        const float dx = static_cast<float>(x) - center.x;
        const float dy = static_cast<float>(y) - center.y;
        const float d = std::sqrt(dx * dx + dy * dy);
        const float ring = 1.0f - std::abs(d - r) / thickness;
        if (ring <= 0.0f) {
          continue;
        }
        apply_cell(frame, x, y, ring > 0.65f ? 'o' : '.',
                   lerp_color(purple, violet, ring), dark_void,
                   ring * (1.0f - local * 0.4f));
      }
    }
  }

  for (const auto &star : impl_->stars) {
    if (t < star.start) {
      continue;
    }
    const float local = (t - star.start) / std::max(0.001f, 1.0f - star.start);
    apply_cell(frame, star.x, star.y, star.glyph, violet, std::nullopt,
               0.55f * (1.0f - local));
  }

  return frame;
}

float VoidRipple::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view VoidRipple::name() const { return "VoidRipple"; }

DecalAnimationKind VoidRipple::kind() const {
  return DecalAnimationKind::VoidRipple;
}

} // namespace fl::widgets::effects
