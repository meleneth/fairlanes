#include "fl/widgets/effects/rocks_fall.hpp"

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

struct RocksFall::Impl {
  struct Column {
    float delay = 0.0f;
    float speed = 1.0f;
    int extra_rocks = 0;
    float phase = 0.0f;
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Column> columns;
};

RocksFall::RocksFall(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0xA332F19Bu);
  std::uniform_real_distribution<float> delay(0.0f, 0.58f);
  std::uniform_real_distribution<float> speed(0.75f, 1.55f);
  std::uniform_real_distribution<float> phase(0.0f, 1.0f);
  std::uniform_int_distribution<int> extras(0, 2);

  impl->columns.resize(static_cast<std::size_t>(std::max(0, width)));
  for (int x = 0; x < width; ++x) {
    impl->columns[static_cast<std::size_t>(x)] = {delay(rng), speed(rng),
                                                  extras(rng), phase(rng)};
  }

  impl_ = std::move(impl);
}

Frame RocksFall::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto rock = palette(13);
  const auto motion = palette(14);
  const auto bg = palette(1);

  for (int x = 0; x < impl_->width; ++x) {
    const auto &col = impl_->columns[static_cast<std::size_t>(x)];
    if (t < col.delay) {
      continue;
    }
    const float local = (t - col.delay) / std::max(0.001f, 1.0f - col.delay);
    const float head =
        local * col.speed * static_cast<float>(impl_->height + 6) +
        col.phase * 3.0f - 4.0f;
    const int rocks = 1 + col.extra_rocks;
    for (int i = 0; i < rocks; ++i) {
      const int y =
          static_cast<int>(std::round(head - static_cast<float>(i * 3)));
      if (!in_bounds(x, y, impl_->width, impl_->height)) {
        continue;
      }
      apply_cell(frame, x, y, 'o', rock, bg, 0.95f);
      const int trail_y = y - 1;
      if (in_bounds(x, trail_y, impl_->width, impl_->height)) {
        apply_cell(frame, x, trail_y, '|', motion, std::nullopt, 0.75f);
      }
    }
  }

  return frame;
}

float RocksFall::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view RocksFall::name() const { return "RocksFall"; }

DecalAnimationKind RocksFall::kind() const {
  return DecalAnimationKind::RocksFall;
}

} // namespace fl::widgets::effects
