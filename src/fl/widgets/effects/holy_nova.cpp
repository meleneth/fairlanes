#include "fl/widgets/effects/holy_nova.hpp"

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

struct HolyNova::Impl {
  struct Spark {
    float angle = 0.0f;
    float radius = 0.0f;
    float start = 0.0f;
    char glyph = '.';
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Spark> sparks;
};

HolyNova::HolyNova(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0x7711AA0Bu);
  std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
  std::uniform_real_distribution<float> radius(
      1.0f, static_cast<float>(std::max(2, std::min(width, height) / 2)));
  std::uniform_real_distribution<float> start(0.0f, 0.7f);
  static constexpr std::array<char, 3> glyphs{'.', '+', '*'};

  const int spark_count = std::max(30, width * height / 20);
  for (int i = 0; i < spark_count; ++i) {
    impl->sparks.push_back(
        {angle(rng), radius(rng), start(rng),
         glyphs[static_cast<std::size_t>(i % glyphs.size())]});
  }

  impl_ = std::move(impl);
}

Frame HolyNova::render(float progress) const {
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
  const float max_r = std::sqrt(cx * cx + cy * cy);
  const float ring_r = max_r * t;
  const float thickness = std::max(1.2f, max_r * 0.07f);
  const auto gold = palette(13);
  const auto white = palette(41);
  const auto warm_bg = palette(10);

  for (int y = 0; y < impl_->height; ++y) {
    for (int x = 0; x < impl_->width; ++x) {
      const float dx = static_cast<float>(x) - cx;
      const float dy = static_cast<float>(y) - cy;
      const float d = std::sqrt(dx * dx + dy * dy);
      const float ring = 1.0f - std::abs(d - ring_r) / thickness;
      if (ring <= 0.0f) {
        continue;
      }
      const float fade = 1.0f - 0.55f * t;
      const auto fg = lerp_color(gold, white, ring);
      apply_cell(frame, x, y, ring > 0.7f ? '*' : '.', fg, warm_bg,
                 ring * fade);
    }
  }

  for (const auto &spark : impl_->sparks) {
    if (t < spark.start) {
      continue;
    }
    const float local =
        (t - spark.start) / std::max(0.001f, 1.0f - spark.start);
    const float r = spark.radius * local;
    const int x = static_cast<int>(std::round(cx + std::cos(spark.angle) * r));
    const int y = static_cast<int>(std::round(cy + std::sin(spark.angle) * r));
    apply_cell(frame, x, y, spark.glyph, white, std::nullopt,
               0.9f * (1.0f - local));
  }

  return frame;
}

float HolyNova::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view HolyNova::name() const { return "HolyNova"; }

DecalAnimationKind HolyNova::kind() const {
  return DecalAnimationKind::HolyNova;
}

} // namespace fl::widgets::effects
