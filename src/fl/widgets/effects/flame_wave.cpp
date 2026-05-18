#include "fl/widgets/effects/flame_wave.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace fl::widgets::effects {
namespace {

[[nodiscard]] float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float smoothstep(float edge0, float edge1, float x) {
  if (edge0 == edge1) {
    return x < edge0 ? 0.0f : 1.0f;
  }
  const float t = clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] ftxui::Color rgb(int r, int g, int b) {
  return ftxui::Color::RGB(r, g, b);
}

[[nodiscard]] ftxui::Color lerp_color(ftxui::Color a, ftxui::Color b, float t) {
  t = clamp01(t);
  return ftxui::Color::Interpolate(t, a, b);
}

[[nodiscard]] char glyph_for_heat(float heat, float noise) {
  static constexpr std::string_view glyphs = " .,:;!*^#";
  const float warped = clamp01(heat * 0.88f + noise * 0.18f);
  const auto index =
      static_cast<std::size_t>(warped * static_cast<float>(glyphs.size() - 1));
  return glyphs[index];
}

[[nodiscard]] ftxui::Color foreground_for(float age01, float heat,
                                          const FlameWaveConfig &cfg) {
  const auto blue = rgb(25, 30, 120);
  const auto violet = rgb(80, 36, 160);
  const auto red = rgb(225, 42, 24);
  const auto orange = rgb(255, 122, 28);
  const auto yellow = rgb(255, 212, 80);
  const auto ember = rgb(145, 70, 22);

  if (age01 < cfg.blue_phase) {
    return lerp_color(blue, violet, age01 / std::max(cfg.blue_phase, 0.001f));
  }
  if (age01 < cfg.red_phase) {
    const float phase = (age01 - cfg.blue_phase) /
                        std::max(cfg.red_phase - cfg.blue_phase, 0.001f);
    return lerp_color(violet, red, phase);
  }
  if (age01 < cfg.yellow_phase) {
    const float phase = (age01 - cfg.red_phase) /
                        std::max(cfg.yellow_phase - cfg.red_phase, 0.001f);
    return lerp_color(orange, yellow, std::max(phase, heat * 0.30f));
  }
  return lerp_color(yellow, ember,
                    (age01 - cfg.yellow_phase) /
                        std::max(1.0f - cfg.yellow_phase, 0.001f));
}

[[nodiscard]] ftxui::Color background_for(float age01, float heat) {
  const auto floor_blue = rgb(7, 12, 42);
  const auto coal = rgb(40, 10, 6);
  const auto ember = rgb(90, 35, 10);
  const auto glow = rgb(155, 80, 22);

  if (age01 < 0.12f) {
    return lerp_color(floor_blue, coal, age01 / 0.12f);
  }
  return lerp_color(ember, glow, heat * (1.0f - age01 * 0.45f));
}

} // namespace

RenderCell &Frame::at(int x, int y) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    throw std::out_of_range("Frame::at coordinate out of range");
  }
  return cells[static_cast<std::size_t>(y * width + x)];
}

const RenderCell &Frame::at(int x, int y) const {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    throw std::out_of_range("Frame::at coordinate out of range");
  }
  return cells[static_cast<std::size_t>(y * width + x)];
}

FlameWave::FlameWave(FlameWaveConfig config) : config_(config) {}

float FlameWave::noise01(int x, int y, int octave) const {
  std::uint32_t h = config_.seed;
  h ^= static_cast<std::uint32_t>(x + 0x9E3779B9u + (h << 6u) + (h >> 2u));
  h ^= static_cast<std::uint32_t>((y + 37 * octave) + 0x85EBCA6Bu + (h << 6u) +
                                  (h >> 2u));
  h ^= h >> 16u;
  h *= 0x7FEB352Du;
  h ^= h >> 15u;
  h *= 0x846CA68Bu;
  h ^= h >> 16u;
  return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

Frame FlameWave::render(float progress, int width, int height) const {
  if (width <= 0 || height <= 0) {
    return {};
  }

  const float t = clamp01(progress);
  Frame frame{
      width, height,
      std::vector<RenderCell>(static_cast<std::size_t>(width * height))};

  const float ignition_px =
      std::max(1.0f, config_.ignition_width * static_cast<float>(width));
  const float front_x =
      t * (static_cast<float>(width) + ignition_px) - ignition_px;
  const float max_flame_height =
      std::max(1.0f, config_.flame_height * static_cast<float>(height));

  for (int y = 0; y < height; ++y) {
    const float from_bottom = static_cast<float>(height - 1 - y);

    for (int x = 0; x < width; ++x) {
      const float passed_by = front_x - static_cast<float>(x);
      if (passed_by < -ignition_px * 0.25f) {
        continue;
      }

      const float age01 = clamp01(
          passed_by / std::max(1.0f, static_cast<float>(width) * 0.85f));
      const float n0 = noise01(x, y, 0);
      const float n1 = noise01(x / 2, y / 2, 1);
      const float turbulence = (n0 - 0.5f) * config_.turbulence;

      const float front_lip =
          1.0f -
          smoothstep(-ignition_px * 0.25f, ignition_px, std::abs(passed_by));
      const float age_decay = std::exp(-age01 * config_.cooling_rate);
      const float local_height = max_flame_height *
                                 (0.30f + 0.70f * age_decay) *
                                 (0.72f + n1 * 0.56f + turbulence);

      if (from_bottom > local_height) {
        continue;
      }

      const float vertical_heat =
          1.0f - (from_bottom / std::max(local_height, 0.001f));
      const float bottom_bias =
          1.0f - smoothstep(0.0f, max_flame_height, from_bottom);
      const float heat = clamp01(
          (vertical_heat * 0.68f + bottom_bias * 0.36f + front_lip * 0.38f) *
              age_decay +
          (n0 - 0.45f) * config_.turbulence);

      if (heat < 0.08f) {
        continue;
      }

      auto &cell = frame.at(x, y);
      cell.alpha = heat;
      cell.fg = foreground_for(age01, heat, config_);
      cell.glyph =
          config_.use_foreground_sparks ? glyph_for_heat(heat, n0) : ' ';

      if (config_.use_background_glow && heat > 0.16f) {
        cell.bg = background_for(age01, heat);
      }
    }
  }

  return frame;
}

ftxui::Element to_element(const Frame &frame) {
  std::vector<ftxui::Element> rows;
  rows.reserve(static_cast<std::size_t>(frame.height));

  for (int y = 0; y < frame.height; ++y) {
    std::vector<ftxui::Element> cols;
    cols.reserve(static_cast<std::size_t>(frame.width));

    for (int x = 0; x < frame.width; ++x) {
      const auto &cell = frame.at(x, y);
      auto elem = ftxui::text(std::string(1, cell.glyph));
      if (cell.fg) {
        elem = elem | ftxui::color(*cell.fg);
      }
      if (cell.bg) {
        elem = elem | ftxui::bgcolor(*cell.bg);
      }
      cols.push_back(elem);
    }

    rows.push_back(ftxui::hbox(std::move(cols)));
  }

  return ftxui::vbox(std::move(rows));
}

ftxui::Element overlay_text(const std::vector<std::string> &lines,
                            const Frame &decal) {
  std::vector<ftxui::Element> rows;
  rows.reserve(static_cast<std::size_t>(decal.height));

  for (int y = 0; y < decal.height; ++y) {
    std::vector<ftxui::Element> cols;
    cols.reserve(static_cast<std::size_t>(decal.width));

    for (int x = 0; x < decal.width; ++x) {
      char base = ' ';
      if (y < static_cast<int>(lines.size()) &&
          x < static_cast<int>(lines[static_cast<std::size_t>(y)].size())) {
        base = lines[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
      }

      const auto &cell = decal.at(x, y);
      const char glyph = cell.glyph == ' ' ? base : cell.glyph;
      auto elem = ftxui::text(std::string(1, glyph));

      if (cell.fg) {
        elem = elem | ftxui::color(*cell.fg);
      }
      if (cell.bg) {
        elem = elem | ftxui::bgcolor(*cell.bg);
      }

      // Text survives by becoming bright when the decal mostly contributes bg.
      if (base != ' ' && cell.bg && cell.glyph == ' ') {
        elem = elem | ftxui::color(ftxui::Color::White);
      }

      cols.push_back(elem);
    }

    rows.push_back(ftxui::hbox(std::move(cols)));
  }

  return ftxui::vbox(std::move(rows));
}

} // namespace fl::widgets::effects
