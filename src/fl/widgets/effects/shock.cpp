#include "fl/widgets/effects/shock.hpp"

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

struct Shock::Impl {
  struct BoltPoint {
    int x = 0;
    int y = 0;
    float start = 0.0f;
    float end = 1.0f;
    char glyph = '*';
    bool core = true;
  };
  struct Spark {
    int x = 0;
    int y = 0;
    float start = 0.0f;
    float end = 1.0f;
    char glyph = '.';
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<BoltPoint> bolts;
  std::vector<Spark> sparks;
};

Shock::Shock(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0x50C4A9D1u);
  std::uniform_int_distribution<int> start_x(1, std::max(1, width - 2));
  std::uniform_real_distribution<float> jitter(-1.2f, 1.2f);
  std::uniform_real_distribution<float> window(0.0f, 0.7f);
  std::uniform_real_distribution<float> duration(0.15f, 0.35f);
  static constexpr std::array<char, 7> bolt_glyphs{'*',  '+', '.', '/',
                                                   '\\', '|', '-'};

  const int bolt_count = std::max(3, width / 18);
  for (int i = 0; i < bolt_count; ++i) {
    float x = static_cast<float>(start_x(rng));
    const int top =
        std::uniform_int_distribution<int>(0, std::max(0, height / 3))(rng);
    const int len = std::uniform_int_distribution<int>(
        std::max(3, height / 2), std::max(4, height - 1))(rng);
    const float t0 = window(rng);
    const float t1 = std::min(1.0f, t0 + duration(rng));

    for (int step = 0; step < len; ++step) {
      const int y = std::clamp(top + step, 0, std::max(0, height - 1));
      x += jitter(rng);
      const int xi = std::clamp(static_cast<int>(std::round(x)), 0,
                                std::max(0, width - 1));
      const char glyph = bolt_glyphs[static_cast<std::size_t>(
          std::uniform_int_distribution<int>(0, 6)(rng))];
      impl->bolts.push_back({xi, y, t0, t1, glyph, true});

      if (step > 2 && std::uniform_int_distribution<int>(0, 9)(rng) < 3) {
        const int bx =
            std::clamp(xi + std::uniform_int_distribution<int>(-3, 3)(rng), 0,
                       std::max(0, width - 1));
        const int by =
            std::clamp(y + std::uniform_int_distribution<int>(-2, 2)(rng), 0,
                       std::max(0, height - 1));
        const float bt0 = std::min(1.0f, t0 + 0.05f * static_cast<float>(step));
        impl->bolts.push_back(
            {bx, by, bt0, std::min(1.0f, bt0 + 0.18f), '+', false});
      }
    }
  }

  const int spark_count = std::max(24, width * height / 10);
  for (int i = 0; i < spark_count; ++i) {
    const int x =
        std::uniform_int_distribution<int>(0, std::max(0, width - 1))(rng);
    const int y =
        std::uniform_int_distribution<int>(0, std::max(0, height - 1))(rng);
    const float t0 = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    const float t1 = std::min(
        1.0f, t0 + std::uniform_real_distribution<float>(0.03f, 0.14f)(rng));
    const char glyph = bolt_glyphs[static_cast<std::size_t>(
        std::uniform_int_distribution<int>(0, 2)(rng))];
    impl->sparks.push_back({x, y, t0, t1, glyph});
  }

  impl_ = std::move(impl);
}

Frame Shock::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto corona = palette(26);
  const auto electric_blue = palette(27);
  const auto hot_yellow = palette(8);
  const auto white_hot = palette(41);

  for (const auto &p : impl_->bolts) {
    if (t < p.start || t > p.end) {
      continue;
    }
    const float life = 1.0f - (t - p.start) / std::max(0.001f, p.end - p.start);
    const int halo = p.core ? 2 : 1;
    if (impl_->config.use_background_glow) {
      apply_bg_glow(frame, p.x, p.y, halo, corona, 0.6f * life);
    }
    const ftxui::Color fg = (p.core && life > 0.5f) ? white_hot : hot_yellow;
    apply_cell(frame, p.x, p.y, p.glyph, fg, electric_blue, 1.0f * life);
  }

  for (const auto &s : impl_->sparks) {
    if (t < s.start || t > s.end) {
      continue;
    }
    const float life = 1.0f - (t - s.start) / std::max(0.001f, s.end - s.start);
    apply_cell(frame, s.x, s.y, s.glyph, electric_blue, std::nullopt,
               0.4f * life);
  }

  return frame;
}

float Shock::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view Shock::name() const { return "Shock"; }

DecalAnimationKind Shock::kind() const { return DecalAnimationKind::Shock; }

} // namespace fl::widgets::effects
