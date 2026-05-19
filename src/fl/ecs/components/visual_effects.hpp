#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include <ftxui/screen/color.hpp>

#include "fl/widgets/effects/decal_animation.hpp"
#include "sr/uWu.hpp"

namespace fl::ecs::components {

struct ResolvedColorOverride {
  ftxui::Color color;
};

struct ResolvedHPBarColorOverride {
  ftxui::Color color;
};

struct ResolvedBackgroundColorOverride {
  ftxui::Color color;
};

struct DamageFlash {
  ftxui::Color color;
  seerin::uWu expires_at{};
};

struct FlameWaveDecal {
  using Clock = std::chrono::steady_clock;

  seerin::uWu expires_at{};
  Clock::time_point started_at = Clock::now();
  std::chrono::milliseconds duration{1000};
  fl::widgets::effects::DecalAnimationKind animation_kind{
      fl::widgets::effects::DecalAnimationKind::FlameWave};
  std::shared_ptr<const fl::widgets::effects::DecalAnimation> animation{};

  FlameWaveDecal() = default;

  FlameWaveDecal(seerin::uWu expires, Clock::time_point started,
                 std::chrono::milliseconds effect_duration,
                 std::shared_ptr<const fl::widgets::effects::DecalAnimation>
                     decal_animation = {})
      : expires_at(expires), started_at(started), duration(effect_duration),
        animation(std::move(decal_animation)) {
    if (animation) {
      animation_kind = animation->kind();
    } else {
      animation =
          fl::widgets::effects::make_decal_animation(animation_kind, 1, 1);
    }
  }

  [[nodiscard]] float progress_at(Clock::time_point now) const noexcept {
    if (duration.count() <= 0) {
      return 1.0F;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - started_at);
    const auto progress = static_cast<float>(elapsed.count()) /
                          static_cast<float>(duration.count());
    return std::clamp(progress, 0.0F, 1.0F);
  }
};

struct ActiveGlow {
  ftxui::Color color;
  seerin::uWu expires_at{};
};

struct StatusTint {
  std::optional<ftxui::Color> body_color;
  std::optional<ftxui::Color> hp_bar_color;
  std::optional<ftxui::Color> background_color;
};

struct BaseVisual {
  std::optional<ftxui::Color> body_color;
};

struct DeadVisual {
  ftxui::Color color;
};

} // namespace fl::ecs::components
