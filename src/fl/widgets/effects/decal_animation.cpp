#include "fl/widgets/effects/decal_animation.hpp"
#include "fl/lospec500.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float clamp_progress(float progress) {
  if (std::isnan(progress)) {
    return 0.0f;
  }
  return clamp01(progress);
}

[[nodiscard]] ftxui::Color palette(std::size_t index) {
  return fl::lospec500::color_at(index);
}

[[nodiscard]] ftxui::Color lerp_color(ftxui::Color a, ftxui::Color b, float t) {
  return clamp01(t) < 0.5F ? a : b;
}

[[nodiscard]] bool in_bounds(int x, int y, int width, int height) {
  return x >= 0 && y >= 0 && x < width && y < height;
}

void apply_cell(fl::widgets::effects::Frame &frame, int x, int y, char glyph,
                std::optional<ftxui::Color> fg, std::optional<ftxui::Color> bg,
                float alpha) {
  if (!in_bounds(x, y, frame.width, frame.height)) {
    return;
  }

  auto &cell = frame.at(x, y);
  cell.alpha = std::max(cell.alpha, clamp01(alpha));
  if (glyph != ' ') {
    cell.glyph = glyph;
  }
  if (fg) {
    cell.fg = *fg;
  }
  if (bg) {
    cell.bg = *bg;
  }
}

void apply_bg_glow(fl::widgets::effects::Frame &frame, int cx, int cy,
                   int radius, ftxui::Color color, float alpha) {
  for (int y = cy - radius; y <= cy + radius; ++y) {
    for (int x = cx - radius; x <= cx + radius; ++x) {
      if (!in_bounds(x, y, frame.width, frame.height)) {
        continue;
      }
      const int dx = x - cx;
      const int dy = y - cy;
      const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
      if (dist > static_cast<float>(radius)) {
        continue;
      }
      const float fade =
          1.0f - dist / std::max(1.0f, static_cast<float>(radius));
      apply_cell(frame, x, y, ' ', std::nullopt, color, alpha * fade);
    }
  }
}

[[nodiscard]] fl::widgets::effects::Frame make_frame(int width, int height) {
  if (width <= 0 || height <= 0) {
    return {};
  }
  return {width, height,
          std::vector<fl::widgets::effects::RenderCell>(
              static_cast<std::size_t>(width * height))};
}

} // namespace

namespace fl::widgets::effects {

class FlameWaveAnimation final : public DecalAnimation {
public:
  FlameWaveAnimation(int width, int height, DecalConfig config)
      : width_(width), height_(height) {
    FlameWaveConfig flame_config;
    flame_config.duration_seconds = config.duration_seconds;
    flame_config.use_background_glow = config.use_background_glow;
    flame_config.use_foreground_sparks = config.use_foreground_sparks;
    flame_config.seed = config.seed;
    flame_ = FlameWave{flame_config};
  }

  [[nodiscard]] Frame render(float progress) const override {
    return flame_.render(progress, width_, height_);
  }
  [[nodiscard]] float duration_seconds() const override {
    return flame_.config().duration_seconds;
  }
  [[nodiscard]] std::string_view name() const override { return "FlameWave"; }
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::FlameWave;
  }

private:
  int width_ = 0;
  int height_ = 0;
  FlameWave flame_;
};

class Shock final : public DecalAnimation {
public:
  Shock(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::Shock;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class RocksFall final : public DecalAnimation {
public:
  RocksFall(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::RocksFall;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class PoisonCloud final : public DecalAnimation {
public:
  PoisonCloud(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::PoisonCloud;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class HolyNova final : public DecalAnimation {
public:
  HolyNova(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::HolyNova;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class BloodBloom final : public DecalAnimation {
public:
  BloodBloom(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::BloodBloom;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class FrostCrack final : public DecalAnimation {
public:
  FrostCrack(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::FrostCrack;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

class VoidRipple final : public DecalAnimation {
public:
  VoidRipple(int width, int height, DecalConfig config = {});
  [[nodiscard]] Frame render(float progress) const override;
  [[nodiscard]] float duration_seconds() const override;
  [[nodiscard]] std::string_view name() const override;
  [[nodiscard]] DecalAnimationKind kind() const override {
    return DecalAnimationKind::VoidRipple;
  }

private:
  struct Impl;
  std::shared_ptr<const Impl> impl_;
};

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

struct VoidRipple::Impl {
  struct Center {
    float x = 0.0f;
    float y = 0.0f;
    float start = 0.0f;
  };
  struct Star {
    int x = 0;
    int y = 0;
    float start = 0.0f;
    char glyph = '.';
  };

  int width = 0;
  int height = 0;
  DecalConfig config;
  std::vector<Center> centers;
  std::vector<Star> stars;
};

VoidRipple::VoidRipple(int width, int height, DecalConfig config) {
  auto impl = std::make_shared<Impl>();
  impl->width = width;
  impl->height = height;
  impl->config = config;

  std::mt19937 rng(config.seed ^ 0x01D5A99Fu);
  std::uniform_real_distribution<float> cx(
      0.0f, static_cast<float>(std::max(1, width - 1)));
  std::uniform_real_distribution<float> cy(
      0.0f, static_cast<float>(std::max(1, height - 1)));
  std::uniform_real_distribution<float> start(0.0f, 0.55f);

  const int center_count = std::max(1, std::min(3, width / 25 + 1));
  for (int i = 0; i < center_count; ++i) {
    impl->centers.push_back({cx(rng), cy(rng), start(rng)});
  }

  static constexpr std::array<char, 3> glyphs{'.', '*', '+'};
  const int stars = std::max(24, width * height / 12);
  for (int i = 0; i < stars; ++i) {
    impl->stars.push_back(
        {std::uniform_int_distribution<int>(0, std::max(0, width - 1))(rng),
         std::uniform_int_distribution<int>(0, std::max(0, height - 1))(rng),
         std::uniform_real_distribution<float>(0.0f, 1.0f)(rng),
         glyphs[static_cast<std::size_t>(i % glyphs.size())]});
  }

  impl_ = std::move(impl);
}

Frame VoidRipple::render(float progress) const {
  if (!impl_) {
    return {};
  }
  Frame frame = make_frame(impl_->width, impl_->height);
  if (frame.width <= 0 || frame.height <= 0) {
    return frame;
  }

  const float t = clamp_progress(progress);
  const auto purple = palette(30);
  const auto violet = palette(31);
  const auto dark_void = palette(0);

  for (const auto &center : impl_->centers) {
    if (t < center.start) {
      continue;
    }
    const float local =
        (t - center.start) / std::max(0.001f, 1.0f - center.start);
    const float max_r = std::sqrt(static_cast<float>(
        impl_->width * impl_->width + impl_->height * impl_->height));
    const float r = local * max_r * 0.6f;
    const float thickness = std::max(1.1f, 2.4f - local);
    for (int y = 0; y < impl_->height; ++y) {
      for (int x = 0; x < impl_->width; ++x) {
        const float dx = static_cast<float>(x) - center.x;
        const float dy = static_cast<float>(y) - center.y;
        const float d = std::sqrt(dx * dx + dy * dy);
        const float ring = 1.0f - std::abs(d - r) / thickness;
        if (ring <= 0.0f) {
          continue;
        }
        apply_cell(frame, x, y, ring > 0.65f ? 'o' : '.',
                   lerp_color(purple, violet, ring), dark_void,
                   ring * (1.0f - local * 0.4f));
      }
    }
  }

  for (const auto &star : impl_->stars) {
    if (t < star.start) {
      continue;
    }
    const float local = (t - star.start) / std::max(0.001f, 1.0f - star.start);
    apply_cell(frame, star.x, star.y, star.glyph, violet, std::nullopt,
               0.55f * (1.0f - local));
  }

  return frame;
}

float VoidRipple::duration_seconds() const {
  return impl_ ? impl_->config.duration_seconds : 1.0f;
}

std::string_view VoidRipple::name() const { return "VoidRipple"; }

std::shared_ptr<const DecalAnimation>
make_decal_animation(DecalAnimationKind kind, int width, int height,
                     DecalConfig config) {
  switch (kind) {
  case DecalAnimationKind::FlameWave:
    return std::make_shared<FlameWaveAnimation>(width, height, config);
  case DecalAnimationKind::Shock:
    return std::make_shared<Shock>(width, height, config);
  case DecalAnimationKind::RocksFall:
    return std::make_shared<RocksFall>(width, height, config);
  case DecalAnimationKind::PoisonCloud:
    return std::make_shared<PoisonCloud>(width, height, config);
  case DecalAnimationKind::HolyNova:
    return std::make_shared<HolyNova>(width, height, config);
  case DecalAnimationKind::BloodBloom:
    return std::make_shared<BloodBloom>(width, height, config);
  case DecalAnimationKind::FrostCrack:
    return std::make_shared<FrostCrack>(width, height, config);
  case DecalAnimationKind::VoidRipple:
    return std::make_shared<VoidRipple>(width, height, config);
  }

  return std::make_shared<FlameWaveAnimation>(width, height, config);
}

} // namespace fl::widgets::effects
