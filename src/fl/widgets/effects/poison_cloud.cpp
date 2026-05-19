#include "fl/widgets/effects/poison_cloud.hpp"

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

struct PoisonCloud::Impl {
  struct Blob {
    float cx = 0.0f;
    float cy = 0.0f;
    float radius = 1.0f;
    float start = 0.0f;
    float end = 1.0f;
    float drift_x = 0.0f;
    float rise = 0.0f;
    char glyph = '.';
    float mix = 0.5f;
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Blob> blobs;
};

PoisonCloud::PoisonCloud(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0x08FF11A1u);
  std::uniform_real_distribution<float> cx(
      0.0f, static_cast<float>(std::max(1, width - 1)));
  std::uniform_real_distribution<float> cy(
      static_cast<float>(height) * 0.4f,
      static_cast<float>(std::max(1, height - 1)));
  std::uniform_real_distribution<float> radius(
      std::max(1.6f, static_cast<float>(width) * 0.05f),
      std::max(2.2f, static_cast<float>(width) * 0.18f));
  std::uniform_real_distribution<float> timing(0.0f, 0.65f);
  std::uniform_real_distribution<float> duration(0.25f, 0.55f);
  std::uniform_real_distribution<float> drift(-3.5f, 3.5f);
  std::uniform_real_distribution<float> rise(0.6f, 2.8f);
  std::uniform_real_distribution<float> mix(0.15f, 0.85f);
  static constexpr std::array<char, 4> glyphs{'.', '*', '+', ':'};

  const int count = std::max(8, width * height / 28);
  for (int i = 0; i < count; ++i) {
    const float t0 = timing(rng);
    impl->blobs.push_back(
        {cx(rng), cy(rng), radius(rng), t0, std::min(1.0f, t0 + duration(rng)),
         drift(rng), rise(rng),
         glyphs[static_cast<std::size_t>(i % glyphs.size())], mix(rng)});
  }

  impl_ = std::move(impl);
}

Frame PoisonCloud::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto green = palette(19);
  const auto purple = palette(30);
  const auto deep = palette(16);

  for (const auto &blob : impl_->blobs) {
    if (t < blob.start || t > blob.end) {
      continue;
    }
    const float life =
        (t - blob.start) / std::max(0.001f, blob.end - blob.start);
    const float fade = 1.0f - life;
    const float cx = blob.cx + blob.drift_x * life;
    const float cy = blob.cy - blob.rise * life;

    const int min_x =
        std::max(0, static_cast<int>(std::floor(cx - blob.radius - 1.0f)));
    const int max_x = std::min(
        impl_->width - 1, static_cast<int>(std::ceil(cx + blob.radius + 1.0f)));
    const int min_y =
        std::max(0, static_cast<int>(std::floor(cy - blob.radius - 1.0f)));
    const int max_y =
        std::min(impl_->height - 1,
                 static_cast<int>(std::ceil(cy + blob.radius + 1.0f)));

    for (int y = min_y; y <= max_y; ++y) {
      for (int x = min_x; x <= max_x; ++x) {
        const float dx = static_cast<float>(x) - cx;
        const float dy = static_cast<float>(y) - cy;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > blob.radius) {
          continue;
        }
        const float influence =
            clamp01(1.0f - dist / std::max(0.001f, blob.radius));
        const auto fog = lerp_color(green, purple, blob.mix);
        apply_cell(frame, x, y, ' ', std::nullopt,
                   lerp_color(deep, fog, influence), influence * 0.6f * fade);

        const int hash =
            (x * 131 + y * 17 + static_cast<int>(blob.mix * 1000.0f)) & 7;
        if (influence > 0.62f && hash == 0) {
          apply_cell(frame, x, y, blob.glyph, fog, std::nullopt,
                     influence * fade);
        }
      }
    }
  }

  return frame;
}

float PoisonCloud::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view PoisonCloud::name() const { return "PoisonCloud"; }

DecalAnimationKind PoisonCloud::kind() const {
  return DecalAnimationKind::PoisonCloud;
}

} // namespace fl::widgets::effects
