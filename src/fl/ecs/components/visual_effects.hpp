#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

#include "fl/widgets/effects/decal.hpp"
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

struct DecalEffect {
  using Clock = std::chrono::steady_clock;

  seerin::uWu expires_at{};
  Clock::time_point started_at = Clock::now();
  std::chrono::milliseconds duration{1000};
  fl::widgets::effects::DecalAnimationKind animation_kind{
      fl::widgets::effects::DecalAnimationKind::FlameWave};
  fl::widgets::effects::DecalConfig config{};
  int extra_height = 0;

  DecalEffect() = default;

  DecalEffect(seerin::uWu expires, Clock::time_point started,
              std::chrono::milliseconds effect_duration,
              fl::widgets::effects::DecalAnimationKind kind =
                  fl::widgets::effects::DecalAnimationKind::FlameWave,
              fl::widgets::effects::DecalConfig effect_config = {},
              int overscan_lines = 0)
      : expires_at(expires), started_at(started), duration(effect_duration),
        animation_kind(kind), config(std::move(effect_config)),
        extra_height(overscan_lines) {}

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

  [[nodiscard]] float loop_progress_at(Clock::time_point now) const noexcept {
    if (duration.count() <= 0) {
      return 0.0F;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - started_at);
    const auto duration_count = duration.count();
    const auto elapsed_count = elapsed.count();
    const auto looped =
        ((elapsed_count % duration_count) + duration_count) % duration_count;
    return static_cast<float>(looped) / static_cast<float>(duration_count);
  }
};

struct CombatantUnderlayDecals {
  std::vector<DecalEffect> effects;

  CombatantUnderlayDecals() = default;
  explicit CombatantUnderlayDecals(DecalEffect effect) {
    effects.push_back(std::move(effect));
  }
};

struct CombatantDecals {
  using Clock = DecalEffect::Clock;

  std::vector<DecalEffect> effects;

  CombatantDecals() = default;
  explicit CombatantDecals(DecalEffect effect) {
    effects.push_back(std::move(effect));
  }
};

inline void add_combatant_decal(entt::registry &reg, entt::entity entity,
                                DecalEffect effect) {
  if (!reg.valid(entity)) {
    return;
  }

  auto &decals = reg.get_or_emplace<CombatantDecals>(entity);
  decals.effects.push_back(std::move(effect));
}

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
