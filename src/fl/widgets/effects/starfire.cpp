#include "fl/widgets/effects/starfire.hpp"

#include "fl/widgets/effects/decal_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace fl::widgets::effects {
namespace {
using detail::apply_cell;
using detail::clamp_progress;
using detail::make_frame;

constexpr int kFlamePix = 254;
constexpr int kMinStarHeat = 118;
constexpr int kMaxStarHeat = 218;
constexpr double kTau = 6.283185307179586476925286766559;
constexpr double kProjectionScale = 0.82;
constexpr int kSpinRevolutions = 1;

struct Star {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  int heat = kMaxStarHeat;
};

[[nodiscard]] std::pair<double, double> rotate2d(double a, double b,
                                                 double angle) {
  const double cos_a = std::cos(angle);
  const double sin_a = std::sin(angle);
  return {cos_a * a - sin_a * b, sin_a * a + cos_a * b};
}

[[nodiscard]] int sanitize_frame_count(int frame_count) {
  return std::max(2, frame_count);
}

[[nodiscard]] int frame_for_progress(float progress, int frame_count) {
  const int last_frame = sanitize_frame_count(frame_count) - 1;
  return std::clamp(
      static_cast<int>(std::round(clamp_progress(progress) *
                                  static_cast<float>(last_frame))),
      0, last_frame);
}

[[nodiscard]] double spin_angle_for_frame(int frame, int frame_count) {
  const int last_frame = sanitize_frame_count(frame_count) - 1;
  return (static_cast<double>(std::clamp(frame, 0, last_frame)) /
          static_cast<double>(last_frame)) *
         kTau * static_cast<double>(kSpinRevolutions);
}

[[nodiscard]] char glyph_for_heat(int value) {
  static constexpr std::string_view glyphs = " .,:;!*^#@";
  const float heat = std::clamp(
      static_cast<float>(value) / static_cast<float>(kFlamePix), 0.0F, 1.0F);
  const auto index =
      static_cast<std::size_t>(heat * static_cast<float>(glyphs.size() - 1));
  return glyphs[index];
}

[[nodiscard]] std::vector<ftxui::Color> make_flame_palette(std::uint32_t seed) {
  std::vector<ftxui::Color> colors;
  colors.reserve(static_cast<std::size_t>(kFlamePix + 1));

  std::mt19937 rng(seed ^ 0xA5C11F1Eu);
  std::uniform_int_distribution<int> blue_jitter(0, 79);
  for (int i = 0; i <= kFlamePix; ++i) {
    const int r = i;
    const int g = 255 - i;
    const int b = (i + blue_jitter(rng)) % 255;
    colors.push_back(ftxui::Color::RGB(r, g, b));
  }
  colors.front() = ftxui::Color::RGB(0, 0, 0);
  return colors;
}

[[nodiscard]] ftxui::Color
color_for_heat(int value, const std::vector<ftxui::Color> &flame_palette) {
  const auto index = static_cast<std::size_t>(
      std::clamp(value, 0, static_cast<int>(flame_palette.size() - 1)));
  return flame_palette[index];
}

[[nodiscard]] ftxui::Color
glow_for_heat(int value, const std::vector<ftxui::Color> &flame_palette) {
  const int glow_value = std::clamp(value / 2, 0, kFlamePix);
  return flame_palette[static_cast<std::size_t>(glow_value)];
}

void seed_stars(std::vector<int> &buffer, int width, int height,
                const std::vector<Star> &stars, double angle) {
  if (width <= 0 || height <= 0) {
    return;
  }

  const double cx = static_cast<double>(width) * 0.5;
  const double cy = static_cast<double>(height) * 0.5;

  for (const auto &star : stars) {
    double rx = star.x;
    double ry = star.y;
    double rz = star.z;

    std::tie(ry, rz) = rotate2d(ry, rz, angle);
    std::tie(rx, rz) = rotate2d(rx, rz, angle);
    std::tie(rx, ry) = rotate2d(rx, ry, angle);

    const double depth = std::max(0.35, 3.0 - rz);
    const int px = static_cast<int>(std::floor(
        (rx / depth) * static_cast<double>(width) * kProjectionScale + cx));
    const int py = static_cast<int>(std::floor(
        (ry / depth) * static_cast<double>(height) * kProjectionScale + cy));

    if (px >= 0 && py >= 0 && px < width && py < height) {
      auto &cell = buffer[static_cast<std::size_t>(py * width + px)];
      cell = std::max(cell, star.heat);
    }
  }
}

void diffuse_fire(std::vector<int> &buffer, int width, int height) {
  if (width < 3 || height < 3) {
    return;
  }

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      const auto index = static_cast<std::size_t>(y * width + x);
      const int value = (buffer[index - 1] + buffer[index + 1] +
                         buffer[index - static_cast<std::size_t>(width)] +
                         buffer[index + static_cast<std::size_t>(width)]) >>
                        2;
      buffer[index] = value > 0 ? value - 1 : 0;
    }
  }
}

} // namespace

struct Starfire::Impl {
  int width = 0;
  int height = 0;
  int frame_count = Starfire::kDefaultFrameCount;
  DecalConfig config;
  std::vector<ftxui::Color> flame_palette;
  std::vector<Star> stars;
};

Starfire::Starfire(int width, int height, DecalConfig config)
    : Starfire(width, height, kDefaultFrameCount, config) {}

Starfire::Starfire(int width, int height, int frame_count, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->frame_count = sanitize_frame_count(frame_count);
  impl->config = config;
  impl->flame_palette = make_flame_palette(config.seed);

  const int area = std::max(0, width) * std::max(0, height);
  const int star_count = std::clamp((area * 3) / 2, 36, 900);

  std::mt19937 rng(config.seed ^ 0x57A2F17Eu);
  std::uniform_real_distribution<double> xy(-1.0, 1.0);
  std::uniform_real_distribution<double> z(-1.0, 1.0);
  std::uniform_int_distribution<int> heat(kMinStarHeat, kMaxStarHeat);
  impl->stars.reserve(static_cast<std::size_t>(star_count));
  for (int i = 0; i < star_count; ++i) {
    impl->stars.push_back({xy(rng), xy(rng), z(rng), heat(rng)});
  }

  impl_ = std::move(impl);
}

Frame Starfire::render(float progress) const {
  if (!impl_) {
    return {};
  }

  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const int target_frame = frame_for_progress(progress, impl_->frame_count);
  const int buffer_height = frame.height + 3;
  std::vector<int> buffer(static_cast<std::size_t>(frame.width * buffer_height),
                          0);

  for (int step = 0; step <= target_frame; ++step) {
    seed_stars(buffer, frame.width, frame.height, impl_->stars,
               spin_angle_for_frame(step, impl_->frame_count));
    diffuse_fire(buffer, frame.width, frame.height);
  }

  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      const int value = buffer[static_cast<std::size_t>(y * frame.width + x)];
      if (value <= 0) {
        continue;
      }

      const float alpha =
          std::clamp(static_cast<float>(value) / static_cast<float>(kFlamePix),
                     0.0F, 1.0F);
      const auto bg = impl_->config.use_background_glow && value > 20
                          ? std::optional<ftxui::Color>{glow_for_heat(
                                value, impl_->flame_palette)}
                          : std::nullopt;
      const char glyph =
          impl_->config.use_foreground_sparks ? glyph_for_heat(value) : ' ';
      apply_cell(frame, x, y, glyph,
                 color_for_heat(value, impl_->flame_palette), bg, alpha);
    }
  }

  return frame;
}

float Starfire::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0F;
}

std::string_view Starfire::name() const { return "Starfire"; }

DecalAnimationKind Starfire::kind() const {
  return DecalAnimationKind::Starfire;
}

} // namespace fl::widgets::effects
