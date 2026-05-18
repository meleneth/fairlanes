#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace fl::widgets::effects {

struct FlameWaveConfig {
  // Duration is metadata for callers that map wall-clock time to progress.
  // The renderer itself consumes normalized progress in [0, 1].
  float duration_seconds = 1.0f;

  // Fractional controls.
  float ignition_width = 0.12f;
  float flame_height = 0.70f;
  float turbulence = 0.38f;
  float cooling_rate = 1.35f;

  // Timing phase knobs. Small blue lip, red blast, then orange/yellow cooling.
  float blue_phase = 0.10f;
  float red_phase = 0.30f;
  float yellow_phase = 0.75f;

  bool use_background_glow = true;
  bool use_foreground_sparks = true;

  // Deterministic seed lets tests and recordings stay stable.
  std::uint32_t seed = 0xC001F1A5u;
};

struct RenderCell {
  char glyph = ' ';
  std::optional<ftxui::Color> fg;
  std::optional<ftxui::Color> bg;
  float alpha = 0.0f;

  [[nodiscard]] bool active() const {
    return alpha > 0.0f || fg || bg || glyph != ' ';
  }
};

struct Frame {
  int width = 0;
  int height = 0;
  std::vector<RenderCell> cells;

  [[nodiscard]] RenderCell &at(int x, int y);
  [[nodiscard]] const RenderCell &at(int x, int y) const;
};

class FlameWave {
public:
  explicit FlameWave(FlameWaveConfig config = {});

  [[nodiscard]] const FlameWaveConfig &config() const { return config_; }
  [[nodiscard]] Frame render(float progress, int width, int height) const;

private:
  FlameWaveConfig config_;

  [[nodiscard]] float noise01(int x, int y, int octave = 0) const;
};

// Decal helpers. Render a full rectangle as FTXUI Elements, or apply active
// cells over an existing text buffer while preserving legibility.
[[nodiscard]] ftxui::Element to_element(const Frame &frame);
[[nodiscard]] ftxui::Element overlay_text(const std::vector<std::string> &lines,
                                          const Frame &decal);

} // namespace fl::widgets::effects
