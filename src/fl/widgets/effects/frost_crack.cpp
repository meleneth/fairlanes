#include "fl/widgets/effects/frost_crack.hpp"

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

struct FrostCrack::Impl {
  struct CrackPoint {
    int x = 0;
    int y = 0;
    float appear = 0.0f;
    char glyph = '-';
  };
  struct Frost {
    int x = 0;
    int y = 0;
    float start = 0.0f;
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<CrackPoint> cracks;
  std::vector<Frost> frost;
};

FrostCrack::FrostCrack(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0xF20C5E01u);
  std::uniform_int_distribution<int> start_x(0, std::max(0, width - 1));
  std::uniform_int_distribution<int> start_y(0, std::max(0, height - 1));
  std::uniform_int_distribution<int> step_dir(-1, 1);
  std::uniform_real_distribution<float> step_t(0.0f, 0.9f);
  static constexpr std::array<char, 4> glyphs{'-', '|', '/', '\\'};

  const int branch_count = std::max(5, width / 12);
  for (int b = 0; b < branch_count; ++b) {
    int x = start_x(rng);
    int y = start_y(rng);
    const int steps = std::uniform_int_distribution<int>(
        std::max(6, width / 3), std::max(8, width))(rng);
    float t = step_t(rng) * 0.3f;
    for (int i = 0; i < steps; ++i) {
      const char glyph = glyphs[static_cast<std::size_t>(
          std::uniform_int_distribution<int>(0, 3)(rng))];
      impl->cracks.push_back({x, y, std::min(1.0f, t), glyph});
      x = std::clamp(x + step_dir(rng), 0, std::max(0, width - 1));
      y = std::clamp(y + step_dir(rng), 0, std::max(0, height - 1));
      t += 0.012f + std::uniform_real_distribution<float>(0.0f, 0.03f)(rng);

      if (std::uniform_int_distribution<int>(0, 10)(rng) < 3) {
        const int fx = std::clamp(x + step_dir(rng), 0, std::max(0, width - 1));
        const int fy =
            std::clamp(y + step_dir(rng), 0, std::max(0, height - 1));
        impl->frost.push_back({fx, fy, std::min(1.0f, t)});
      }
    }
  }

  impl_ = std::move(impl);
}

Frame FrostCrack::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto cyan = palette(29);
  const auto white = palette(41);
  const auto ice_bg = palette(25);

  for (const auto &c : impl_->cracks) {
    if (t < c.appear) {
      continue;
    }
    const float local = (t - c.appear) / std::max(0.001f, 1.0f - c.appear);
    const float fade = local < 0.7f ? 1.0f : (1.0f - (local - 0.7f) / 0.3f);
    apply_cell(frame, c.x, c.y, c.glyph, lerp_color(cyan, white, local), ice_bg,
               fade);
  }

  for (const auto &f : impl_->frost) {
    if (t < f.start) {
      continue;
    }
    const float local = (t - f.start) / std::max(0.001f, 1.0f - f.start);
    apply_cell(frame, f.x, f.y, '.', white, std::nullopt,
               0.45f * (1.0f - local));
  }

  return frame;
}

float FrostCrack::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view FrostCrack::name() const { return "FrostCrack"; }

DecalAnimationKind FrostCrack::kind() const {
  return DecalAnimationKind::FrostCrack;
}

} // namespace fl::widgets::effects
