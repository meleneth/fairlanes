#include "fl/widgets/effects/blood_bloom.hpp"

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

struct BloodBloom::Impl {
  struct Droplet {
    float angle = 0.0f;
    float distance = 0.0f;
    float start = 0.0f;
    float end = 1.0f;
    char glyph = '.';
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Droplet> droplets;
};

BloodBloom::BloodBloom(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0xB100DB1Eu);
  std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
  std::uniform_real_distribution<float> dist(
      1.0f, static_cast<float>(std::max(2, std::min(width, height) / 2)));
  std::uniform_real_distribution<float> start(0.0f, 0.55f);
  std::uniform_real_distribution<float> span(0.3f, 0.65f);
  static constexpr std::array<char, 4> glyphs{'.', '*', '+', ':'};

  const int count = std::max(28, width * height / 18);
  for (int i = 0; i < count; ++i) {
    const float t0 = start(rng);
    impl->droplets.push_back(
        {angle(rng), dist(rng), t0, std::min(1.0f, t0 + span(rng)),
         glyphs[static_cast<std::size_t>(i % glyphs.size())]});
  }

  impl_ = std::move(impl);
}

Frame BloodBloom::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const float cx = static_cast<float>(impl_->width - 1) * 0.5f;
  const float cy = static_cast<float>(impl_->height - 1) * 0.5f;
  const auto bright = palette(4);
  const auto dark = palette(1);
  const auto dried = palette(1);

  for (const auto &d : impl_->droplets) {
    if (t < d.start || t > d.end) {
      continue;
    }
    const float local = (t - d.start) / std::max(0.001f, d.end - d.start);
    const float r = d.distance * local;
    const int x = static_cast<int>(std::round(cx + std::cos(d.angle) * r));
    const int y = static_cast<int>(std::round(cy + std::sin(d.angle) * r));
    const auto wet = lerp_color(bright, dark, t);
    const auto stain = lerp_color(dark, dried, t);
    apply_cell(frame, x, y, d.glyph, wet, stain, 0.95f * (1.0f - local * 0.3f));
  }

  return frame;
}

float BloodBloom::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view BloodBloom::name() const { return "BloodBloom"; }

DecalAnimationKind BloodBloom::kind() const {
  return DecalAnimationKind::BloodBloom;
}

} // namespace fl::widgets::effects
