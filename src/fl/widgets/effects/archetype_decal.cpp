#include "fl/widgets/effects/archetype_decal.hpp"

#include "fl/widgets/effects/decal_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

namespace fl::widgets::effects {
namespace {
using detail::apply_bg_glow;
using detail::apply_cell;
using detail::clamp01;
using detail::clamp_progress;
using detail::in_bounds;
using detail::make_frame;
using detail::palette;

int iround(float value) { return static_cast<int>(std::round(value)); }

float smoothstep(float edge0, float edge1, float value) {
  const float t = clamp01((value - edge0) / std::max(0.001f, edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

void draw_line(Frame &frame, int x0, int y0, int x1, int y1, char glyph,
               ftxui::Color fg, std::optional<ftxui::Color> bg, float alpha) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    apply_cell(frame, x0, y0, glyph, fg, bg, alpha);
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int e2 = err * 2;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void draw_ring(Frame &frame, float cx, float cy, float radius, float thickness,
               char glyph, ftxui::Color fg, std::optional<ftxui::Color> bg,
               float alpha) {
  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      const float dx = static_cast<float>(x) - cx;
      const float dy = static_cast<float>(y) - cy;
      const float d = std::sqrt(dx * dx + dy * dy);
      const float edge = 1.0f - std::abs(d - radius) / thickness;
      if (edge > 0.0f) {
        apply_cell(frame, x, y, glyph, fg, bg, alpha * clamp01(edge));
      }
    }
  }
}

std::uint32_t mix(std::uint32_t value) {
  value ^= value >> 16U;
  value *= 0x7feb352dU;
  value ^= value >> 15U;
  value *= 0x846ca68bU;
  value ^= value >> 16U;
  return value;
}

bool is_archetype_kind(DecalAnimationKind kind) {
  switch (kind) {
  case DecalAnimationKind::Impact:
  case DecalAnimationKind::Slash:
  case DecalAnimationKind::Bite:
  case DecalAnimationKind::Projectile:
  case DecalAnimationKind::Sweep:
  case DecalAnimationKind::Burst:
  case DecalAnimationKind::Beam:
  case DecalAnimationKind::Heal:
  case DecalAnimationKind::Cleanse:
  case DecalAnimationKind::Glitch:
  case DecalAnimationKind::Aura:
  case DecalAnimationKind::Field:
  case DecalAnimationKind::Observe:
    return true;
  case DecalAnimationKind::FlameWave:
  case DecalAnimationKind::Shock:
  case DecalAnimationKind::RocksFall:
  case DecalAnimationKind::PoisonCloud:
  case DecalAnimationKind::HolyNova:
  case DecalAnimationKind::BloodBloom:
  case DecalAnimationKind::FrostCrack:
  case DecalAnimationKind::VoidRipple:
  case DecalAnimationKind::Starfire:
  case DecalAnimationKind::HitpointNumber:
    return false;
  }
  return false;
}

void render_impact(Frame &frame, float t) {
  const int cx = std::clamp(frame.width / 2 + frame.width / 10, 0,
                            std::max(0, frame.width - 1));
  const int cy = frame.height / 2;
  const auto white = palette(41);
  const auto yellow = palette(14);
  const auto bruise = palette(4);
  const auto scuff = palette(10);

  if (t < 0.22f) {
    const float alpha = 1.0f - smoothstep(0.12f, 0.22f, t);
    apply_cell(frame, cx, cy, 'X', white, bruise, 0.95f * alpha + 0.2f);
    apply_cell(frame, cx - 1, cy, '>', yellow, bruise, 0.82f * alpha);
    apply_cell(frame, cx + 1, cy, '<', yellow, bruise, 0.82f * alpha);
    apply_cell(frame, cx, cy - 1, '#', white, std::nullopt, 0.45f * alpha);
    apply_cell(frame, cx, cy + 1, '#', white, std::nullopt, 0.38f * alpha);
    return;
  }

  if (t < 0.58f) {
    const float phase = smoothstep(0.22f, 0.58f, t);
    const int reach = std::max(
        2, iround(static_cast<float>(frame.width) * (0.07f + 0.08f * phase)));
    const float alpha = 1.0f - smoothstep(0.46f, 0.58f, t);
    apply_cell(frame, cx, cy, 'X', yellow, bruise, 0.82f * alpha);
    apply_cell(frame, cx - reach, cy, '<', yellow, std::nullopt, 0.72f * alpha);
    apply_cell(frame, cx + reach, cy, '>', yellow, std::nullopt, 0.72f * alpha);
    apply_cell(frame, cx - std::max(1, reach / 2), cy - 1, '/', scuff,
               std::nullopt, 0.65f * alpha);
    apply_cell(frame, cx + std::max(1, reach / 2), cy + 1, '\\', scuff,
               std::nullopt, 0.65f * alpha);
    apply_cell(frame, cx - std::max(1, reach / 2), cy + 1, '\\', scuff,
               std::nullopt, 0.42f * alpha);
    apply_cell(frame, cx + std::max(1, reach / 2), cy - 1, '/', scuff,
               std::nullopt, 0.42f * alpha);
    return;
  }

  const float fade = 1.0f - smoothstep(0.58f, 1.0f, t);
  apply_cell(frame, cx, cy, '#', bruise, std::nullopt, 0.62f * fade);
  apply_cell(frame, cx - 1, cy, '/', scuff, std::nullopt, 0.46f * fade);
  apply_cell(frame, cx + 1, cy, '\\', scuff, std::nullopt, 0.46f * fade);
  apply_cell(frame, cx, cy + 1, '*', bruise, std::nullopt, 0.28f * fade);
}

int slash_y_for(Frame const &frame, float u) {
  const float mid = static_cast<float>(frame.height - 1) * 0.56f;
  const float arc = std::sin(u * 3.14159265f) *
                    static_cast<float>(std::max(1, frame.height)) * 0.33f;
  const float tilt =
      (0.5f - u) * static_cast<float>(std::max(1, frame.height)) * 0.28f;
  return std::clamp(iround(mid - arc + tilt), 0, std::max(0, frame.height - 1));
}

void render_slash(Frame &frame, float t) {
  const auto cut = palette(14);
  const auto hot = palette(41);
  const auto blood = palette(4);
  const float front = smoothstep(0.05f, 0.62f, t);
  const float fade = 1.0f - smoothstep(0.78f, 1.0f, t);
  const int samples = std::max(6, frame.width / 2);

  int previous_x = 0;
  int previous_y = slash_y_for(frame, 0.0f);
  for (int i = 1; i <= samples; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(samples);
    if (u > front) {
      break;
    }
    const int x = std::clamp(
        iround(static_cast<float>(frame.width - 1) * (0.18f + 0.64f * u)), 0,
        std::max(0, frame.width - 1));
    const int y = slash_y_for(frame, u);
    draw_line(frame, previous_x, previous_y, x, y, '/', blood, std::nullopt,
              0.72f * fade);
    previous_x = x;
    previous_y = y;
  }

  const int fx = std::clamp(
      iround(static_cast<float>(frame.width - 1) * (0.18f + 0.64f * front)), 0,
      std::max(0, frame.width - 1));
  const int fy = slash_y_for(frame, front);
  apply_cell(frame, fx, fy, '/', hot, cut, 0.95f * fade);
  apply_cell(frame, fx + 1, fy - 1, '/', cut, std::nullopt, 0.55f * fade);
}

void render_bite(Frame &frame, float t) {
  const int cx = frame.width / 2;
  const int cy = frame.height / 2;
  const auto bone = palette(41);
  const auto dark = palette(1);
  const float close =
      smoothstep(0.0f, 0.52f, t) * (1.0f - smoothstep(0.72f, 1.0f, t));
  const int max_open = std::max(2, frame.height / 3);
  const int gap =
      std::max(0, iround(static_cast<float>(max_open) * (1.0f - close)));
  const int half_width = std::max(3, frame.width / 7);

  for (int i = -half_width; i <= half_width; i += 2) {
    const int skew = (i % 4 == 0) ? 1 : 0;
    apply_cell(frame, cx + i, cy - gap - 1 + skew, 'V', bone, dark, 0.88f);
    apply_cell(frame, cx + i + 1, cy + gap + 1 - skew, '^', bone, dark, 0.82f);
  }

  if (close > 0.82f) {
    apply_cell(frame, cx - 1, cy, 'x', palette(4), std::nullopt, 0.75f);
    apply_cell(frame, cx + 1, cy, 'x', palette(4), std::nullopt, 0.55f);
  }
}

void render_projectile(Frame &frame, float t) {
  const int y = frame.height / 2;
  const int head = projectile_head_x_for_progress(frame.width, t);
  const bool holding = projectile_is_holding(t);
  const auto shaft = palette(13);
  const auto head_color = palette(41);
  const auto hit = palette(10);
  constexpr int kShaftLength = 4;

  for (int x = head - kShaftLength; x <= head; ++x) {
    if (!in_bounds(x, y, frame.width, frame.height)) {
      continue;
    }
    const bool is_head = x == head;
    apply_cell(frame, x, y, is_head ? '>' : '-', is_head ? head_color : shaft,
               std::nullopt, is_head ? 0.95f : 0.8f);
  }
  if (holding) {
    apply_cell(frame, std::max(0, head - 1), y - 1, '*', hit, std::nullopt,
               0.65f);
    apply_cell(frame, std::max(0, head - 1), y + 1, '*', hit, std::nullopt,
               0.45f);
  }
}

void render_sweep(Frame &frame, float t) {
  const auto cyan = palette(27);
  const auto dark = palette(1);
  const float fade_in = smoothstep(0.0f, 0.16f, t);
  const float fade_out = 1.0f - smoothstep(0.82f, 1.0f, t);
  const float move = smoothstep(0.12f, 0.72f, t);
  const int y =
      std::clamp(iround(-1.0f + move * static_cast<float>(frame.height + 1)), 0,
                 std::max(0, frame.height - 1));
  const float alpha = 0.85f * fade_in * fade_out;

  for (int x = 0; x < frame.width; ++x) {
    const char glyph = (x + y) % 4 == 0 ? '~' : '=';
    apply_cell(frame, x, y, glyph, cyan, dark, alpha);
    if (y > 0 && x % 3 == 0) {
      apply_cell(frame, x, y - 1, '-', cyan, std::nullopt, alpha * 0.35f);
    }
    if (y + 1 < frame.height && x % 3 == 1) {
      apply_cell(frame, x, y + 1, '-', cyan, std::nullopt, alpha * 0.25f);
    }
  }
}

void render_burst(Frame &frame, float t) {
  const float cx = static_cast<float>(frame.width - 1) * 0.5f;
  const float cy = static_cast<float>(frame.height - 1) * 0.5f;
  const float max_r =
      static_cast<float>(std::min(frame.width, frame.height)) * 0.55f;
  const auto orange = palette(10);
  const auto yellow = palette(14);
  draw_ring(frame, cx, cy, std::max(1.0f, max_r * t), 1.4f,
            t < 0.6f ? '*' : 'o', yellow, orange, 0.9f * (1.0f - t * 0.35f));
}

void render_beam(Frame &frame, float t, DecalConfig const &config) {
  const int y = frame.height / 2;
  const auto span = beam_span_for_progress(frame.width, t);
  const auto core = config.color.value_or(palette(41));
  const auto glow = config.color.value_or(palette(30));
  const char glyph = config.glyph.value_or('=');

  for (int x = span.start; x <= span.end; ++x) {
    if (!in_bounds(x, y, frame.width, frame.height)) {
      continue;
    }
    apply_cell(frame, x, y, (x % 2 == 0) ? glyph : '#', core, glow, 0.9f);
    if (y > 0 && x % 3 == 0) {
      apply_cell(frame, x, y - 1, '.', glow, std::nullopt, 0.3f);
    }
    if (y + 1 < frame.height && x % 3 == 1) {
      apply_cell(frame, x, y + 1, '.', glow, std::nullopt, 0.3f);
    }
  }
}

void render_heal(Frame &frame, float t) {
  const float cx = static_cast<float>(frame.width - 1) * 0.5f;
  const float cy = static_cast<float>(frame.height - 1) * 0.5f;
  const auto green = palette(22);
  const auto gold = palette(13);
  draw_ring(frame, cx, cy,
            1.0f + static_cast<float>(std::min(frame.width, frame.height)) *
                       0.35f * t,
            1.2f, '+', gold, green, 0.8f);
  apply_cell(frame, iround(cx), iround(cy), '+', gold, green, 0.95f);
}

void render_cleanse(Frame &frame, float t, DecalConfig const &config) {
  const auto blue = config.color.value_or(palette(27));
  const auto white = palette(41);
  const char beam_glyph = config.glyph.value_or('|');
  const auto span = beam_span_for_progress(frame.width, t);

  for (int y = 1; y < frame.height; y += 2) {
    for (int x = 1; x < frame.width; x += 4) {
      const float distance_from_beam = static_cast<float>(span.start - x);
      if (distance_from_beam > 2.0f) {
        apply_cell(frame, x, y, 'o', white, std::nullopt, 0.62f);
        continue;
      }
      if (distance_from_beam > -1.0f) {
        apply_cell(frame, x, y, 'o', blue, std::nullopt,
                   0.62f * clamp01((distance_from_beam + 1.0f) / 3.0f));
      }
    }
  }

  for (int x = span.start; x <= span.end; ++x) {
    if (!in_bounds(x, frame.height / 2, frame.width, frame.height)) {
      continue;
    }
    for (int y = 0; y < frame.height; ++y) {
      apply_cell(frame, x, y, beam_glyph, white, blue, 0.42f);
    }
  }
}

void render_glitch(Frame &frame, float t, std::uint32_t seed) {
  static constexpr char glyphs[] = {'#', '%', '@', '?', '/', '\\', '|', '!'};
  const auto magenta = palette(31);
  const auto cyan = palette(27);
  const int count = std::max(12, frame.width * frame.height / 8);
  const auto phase = static_cast<std::uint32_t>(t * 17.0f);
  for (int i = 0; i < count; ++i) {
    const auto r =
        mix(seed + phase * 0x9e3779b9U + static_cast<std::uint32_t>(i * 97));
    const int x = static_cast<int>(
        r % static_cast<std::uint32_t>(std::max(1, frame.width)));
    const int y = static_cast<int>(
        (r >> 12U) % static_cast<std::uint32_t>(std::max(1, frame.height)));
    const char glyph = glyphs[(r >> 24U) % 8U];
    apply_cell(frame, x, y, glyph, (i % 2 == 0) ? magenta : cyan, std::nullopt,
               0.55f + 0.35f * static_cast<float>(i % 3) / 2.0f);
  }
}

void render_aura(Frame &frame, float t) {
  const float cx = static_cast<float>(frame.width - 1) * 0.5f;
  const float cy = static_cast<float>(frame.height - 1) * 0.58f;
  const auto teal = palette(24);
  const auto blue = palette(27);
  draw_ring(
      frame, cx, cy,
      std::max(1.0f, static_cast<float>(std::min(frame.width, frame.height)) *
                         (0.25f + 0.08f * std::sin(t * 6.2831853f))),
      1.1f, 'o', teal, blue, 0.75f);
  apply_cell(frame, iround(cx), iround(cy), '^', teal, std::nullopt, 0.7f);
}

void render_field(Frame &frame, float t, std::uint32_t seed) {
  const auto green = palette(20);
  const auto dark = palette(1);
  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      const auto r = mix(seed + static_cast<std::uint32_t>(x * 31 + y * 131) +
                         static_cast<std::uint32_t>(t * 11.0f) * 0x45d9f3bU);
      if ((r % 100U) < 30U) {
        const char glyph = (r & 1U) == 0U ? '~' : ':';
        apply_cell(frame, x, y, glyph, green, dark,
                   0.25f +
                       0.35f * static_cast<float>((r >> 8U) % 100U) / 100.0f);
      }
    }
  }
}

void render_observe(Frame &frame, float t, std::uint32_t seed) {
  const float cx = static_cast<float>(frame.width - 1) * 0.5f;
  const float cy = static_cast<float>(frame.height - 1) * 0.52f;
  const float half_width =
      std::max(4.0f, static_cast<float>(frame.width) * 0.34f);
  const float half_height =
      std::max(2.0f, static_cast<float>(frame.height) * 0.32f);
  const float open = smoothstep(0.02f, 0.24f, t) *
                     (1.0f - 0.35f * smoothstep(0.72f, 1.0f, t));
  const auto sclera = palette(41);
  const auto iris = palette(27);
  const auto pupil = palette(1);
  const auto vein = palette(5);
  const auto glow = palette(31);
  const auto sick = palette(22);

  apply_bg_glow(frame, iround(cx), iround(cy),
                std::max(3, std::min(frame.width, frame.height) / 2), glow,
                0.16f + 0.18f * (1.0f - std::abs(0.5f - t) * 2.0f));

  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      const float dx = (static_cast<float>(x) - cx) / half_width;
      const float dy = (static_cast<float>(y) - cy) /
                       std::max(0.1f, half_height * open);
      const float eye = dx * dx + dy * dy;
      if (eye > 1.08f) {
        continue;
      }

      const float rim = 1.0f - std::abs(eye - 1.0f) / 0.22f;
      if (rim > 0.0f) {
        const char glyph = dy < -0.35f ? '^' : dy > 0.35f ? 'v' : '=';
        apply_cell(frame, x, y, glyph, sclera, glow, 0.76f * clamp01(rim));
        continue;
      }

      if (eye < 0.92f) {
        const auto r = mix(seed + static_cast<std::uint32_t>(x * 67 + y * 131));
        const bool bloodshot = (r % 100U) < 12U || std::abs(dy) < 0.08f;
        apply_cell(frame, x, y, bloodshot ? '~' : '.', bloodshot ? vein : sclera,
                   std::nullopt, bloodshot ? 0.58f : 0.28f);
      }
    }
  }

  const float wobble =
      std::sin((t * 6.2831853f * 1.5f) + static_cast<float>(seed & 7U));
  const float pupil_cx = cx + wobble * std::max(1.0f, half_width * 0.08f);
  const float iris_radius = std::max(1.6f, std::min(half_width, half_height) *
                                               (0.34f + 0.08f * open));
  const float pupil_radius = std::max(1.0f, iris_radius * 0.48f);
  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      const float dx = static_cast<float>(x) - pupil_cx;
      const float dy = (static_cast<float>(y) - cy) * 1.8f;
      const float d = std::sqrt(dx * dx + dy * dy);
      if (d <= pupil_radius) {
        apply_cell(frame, x, y, '@', pupil, iris, 0.98f);
      } else if (d <= iris_radius) {
        const char glyph = ((x + y) % 2 == 0) ? '*' : '+';
        apply_cell(frame, x, y, glyph, iris, sick,
                   0.84f * (1.0f - d / std::max(1.0f, iris_radius) * 0.25f));
      }
    }
  }

  const int lash_count = std::max(6, frame.width / 8);
  for (int i = 0; i < lash_count; ++i) {
    const float u = lash_count == 1
                        ? 0.0f
                        : static_cast<float>(i) /
                              static_cast<float>(lash_count - 1);
    const float px = cx - half_width * 0.86f + u * half_width * 1.72f;
    const int top_y = std::clamp(iround(cy - half_height * open *
                                                (0.58f + 0.36f *
                                                            std::sin(u *
                                                                     3.14159f))),
                                 0, std::max(0, frame.height - 1));
    const int bottom_y = std::clamp(iround(cy + half_height * open *
                                                   (0.56f + 0.24f *
                                                               std::sin(u *
                                                                        3.14159f))),
                                    0, std::max(0, frame.height - 1));
    const int x = std::clamp(iround(px), 0, std::max(0, frame.width - 1));
    apply_cell(frame, x, top_y - 1, '/', vein, std::nullopt, 0.62f * open);
    apply_cell(frame, x, bottom_y + 1, '\\', vein, std::nullopt, 0.48f * open);
  }
}

float duration_multiplier_for(DecalAnimationKind kind) {
  switch (kind) {
  case DecalAnimationKind::Sweep:
  case DecalAnimationKind::Cleanse:
    return 2.0f;
  default:
    return 1.0f;
  }
}

} // namespace

int beam_visual_length(int width) noexcept { return std::max(1, width / 2); }

BeamSpan beam_span_for_progress(int width, float progress) noexcept {
  const int safe_width = std::max(1, width);
  const int length = beam_visual_length(safe_width);
  const float t = clamp_progress(progress);
  const int start = iround(static_cast<float>(-length + 1) +
                           t * static_cast<float>(safe_width + length - 1));
  return BeamSpan{start, start + length - 1};
}

int projectile_embed_x(int width) noexcept { return std::max(0, width - 1); }

bool projectile_is_holding(float progress) noexcept {
  return clamp_progress(progress) >= 0.78f;
}

int projectile_head_x_for_progress(int width, float progress) noexcept {
  const int safe_width = std::max(1, width);
  const float travel = std::min(clamp_progress(progress) / 0.78f, 1.0f);
  constexpr int kShaftLength = 4;
  return std::clamp(
      iround(static_cast<float>(-kShaftLength) +
             travel * static_cast<float>(safe_width - 1 + kShaftLength)),
      -kShaftLength, projectile_embed_x(safe_width));
}

ArchetypeDecal::ArchetypeDecal(DecalAnimationKind kind, int width, int height,
                               DecalConfig config)
    : kind_{is_archetype_kind(kind) ? kind : DecalAnimationKind::Impact},
      width_{width}, height_{height}, config_{config} {}

Frame ArchetypeDecal::render(float progress) const {
  Frame frame = make_frame(width_, height_);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  switch (kind_) {
  case DecalAnimationKind::Impact:
    render_impact(frame, t);
    break;
  case DecalAnimationKind::Slash:
    render_slash(frame, t);
    break;
  case DecalAnimationKind::Bite:
    render_bite(frame, t);
    break;
  case DecalAnimationKind::Projectile:
    render_projectile(frame, t);
    break;
  case DecalAnimationKind::Sweep:
    render_sweep(frame, t);
    break;
  case DecalAnimationKind::Burst:
    render_burst(frame, t);
    break;
  case DecalAnimationKind::Beam:
    render_beam(frame, t, config_);
    break;
  case DecalAnimationKind::Heal:
    render_heal(frame, t);
    break;
  case DecalAnimationKind::Cleanse:
    render_cleanse(frame, t, config_);
    break;
  case DecalAnimationKind::Glitch:
    render_glitch(frame, t, config_.seed);
    break;
  case DecalAnimationKind::Aura:
    render_aura(frame, t);
    break;
  case DecalAnimationKind::Field:
    render_field(frame, t, config_.seed);
    break;
  case DecalAnimationKind::Observe:
    render_observe(frame, t, config_.seed);
    break;
  case DecalAnimationKind::FlameWave:
  case DecalAnimationKind::Shock:
  case DecalAnimationKind::RocksFall:
  case DecalAnimationKind::PoisonCloud:
  case DecalAnimationKind::HolyNova:
  case DecalAnimationKind::BloodBloom:
  case DecalAnimationKind::FrostCrack:
  case DecalAnimationKind::VoidRipple:
  case DecalAnimationKind::Starfire:
  case DecalAnimationKind::HitpointNumber:
    render_impact(frame, t);
    break;
  }

  return frame;
}

float ArchetypeDecal::duration_seconds() const {
  return config_.duration_seconds * duration_multiplier_for(kind_);
}

std::string_view ArchetypeDecal::name() const { return effects::name(kind_); }

DecalAnimationKind ArchetypeDecal::kind() const { return kind_; }

} // namespace fl::widgets::effects
