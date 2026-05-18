#include "fl/ecs/systems/visual_resolver.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/lospec500.hpp"

#include <vector>

namespace fl::ecs::systems {
namespace {

template <typename Component>
void clear_expired(entt::registry &reg, seerin::uWu now) {
  std::vector<entt::entity> expired;
  auto view = reg.view<Component>();
  for (auto entity : view) {
    const auto &effect = view.template get<Component>(entity);
    if (effect.expires_at.v <= now.v) {
      expired.push_back(entity);
    }
  }

  for (auto entity : expired) {
    reg.remove<Component>(entity);
  }
}

void clear_finished_flame_waves(
    entt::registry &reg,
    fl::ecs::components::FlameWaveDecal::Clock::time_point now) {
  std::vector<entt::entity> finished;
  auto view = reg.view<fl::ecs::components::FlameWaveDecal>();
  for (auto entity : view) {
    const auto &effect = view.get<fl::ecs::components::FlameWaveDecal>(entity);
    if (effect.progress_at(now) >= 1.0F) {
      finished.push_back(entity);
    }
  }

  for (auto entity : finished) {
    reg.remove<fl::ecs::components::FlameWaveDecal>(entity);
  }
}

bool is_dead(entt::registry &reg, entt::entity entity) {
  auto *stats = reg.try_get<fl::ecs::components::Stats>(entity);
  return stats != nullptr && !stats->is_alive();
}

} // namespace

void VisualResolver::resolve(entt::registry &reg, seerin::uWu now) {
  clear_expired<fl::ecs::components::DamageFlash>(reg, now);
  clear_expired<fl::ecs::components::FlameWaveDecal>(reg, now);
  clear_finished_flame_waves(reg,
                             fl::ecs::components::FlameWaveDecal::Clock::now());
  clear_expired<fl::ecs::components::ActiveGlow>(reg, now);

  reg.clear<fl::ecs::components::ResolvedColorOverride>();
  reg.clear<fl::ecs::components::ResolvedHPBarColorOverride>();
  reg.clear<fl::ecs::components::ResolvedBackgroundColorOverride>();

  for (auto entity : reg.view<fl::ecs::components::Stats>()) {
    resolve_entity(reg, entity, now);
  }
  for (auto entity : reg.view<fl::ecs::components::DamageFlash>()) {
    resolve_entity(reg, entity, now);
  }
  for (auto entity : reg.view<fl::ecs::components::ActiveGlow>()) {
    resolve_entity(reg, entity, now);
  }
  for (auto entity : reg.view<fl::ecs::components::StatusTint>()) {
    resolve_entity(reg, entity, now);
  }
  for (auto entity : reg.view<fl::ecs::components::BaseVisual>()) {
    resolve_entity(reg, entity, now);
  }
  for (auto entity : reg.view<fl::ecs::components::DeadVisual>()) {
    resolve_entity(reg, entity, now);
  }
}

void VisualResolver::resolve_entity(entt::registry &reg, entt::entity entity,
                                    seerin::uWu now) {
  using namespace fl::ecs::components;

  if (!reg.valid(entity)) {
    return;
  }

  if (auto *damage = reg.try_get<DamageFlash>(entity);
      damage != nullptr && damage->expires_at.v <= now.v) {
    reg.remove<DamageFlash>(entity);
  }

  if (auto *active = reg.try_get<ActiveGlow>(entity);
      active != nullptr && active->expires_at.v <= now.v) {
    reg.remove<ActiveGlow>(entity);
  }

  if (auto *flame = reg.try_get<FlameWaveDecal>(entity);
      flame != nullptr &&
      flame->progress_at(FlameWaveDecal::Clock::now()) >= 1.0F) {
    reg.remove<FlameWaveDecal>(entity);
  }

  reg.remove<ResolvedColorOverride>(entity);
  reg.remove<ResolvedHPBarColorOverride>(entity);
  reg.remove<ResolvedBackgroundColorOverride>(entity);

  if (auto color = resolve_body_color(reg, entity, now)) {
    reg.emplace_or_replace<ResolvedColorOverride>(
        entity, ResolvedColorOverride{*color});
  }

  if (auto hp_color = resolve_hp_bar_color(reg, entity, now)) {
    reg.emplace_or_replace<ResolvedHPBarColorOverride>(
        entity, ResolvedHPBarColorOverride{*hp_color});
  }

  if (auto bg_color = resolve_background_color(reg, entity, now)) {
    reg.emplace_or_replace<ResolvedBackgroundColorOverride>(
        entity, ResolvedBackgroundColorOverride{*bg_color});
  }
}

std::optional<ftxui::Color>
VisualResolver::resolve_body_color(entt::registry &reg, entt::entity entity,
                                   seerin::uWu now) {
  using namespace fl::ecs::components;

  if (!reg.valid(entity)) {
    return std::nullopt;
  }

  if (is_dead(reg, entity)) {
    if (auto *dead = reg.try_get<DeadVisual>(entity)) {
      return dead->color;
    }
    return fl::lospec500::color_at(6);
  }

  if (auto *damage = reg.try_get<DamageFlash>(entity);
      damage != nullptr && damage->expires_at.v > now.v) {
    return damage->color;
  }

  if (auto *active = reg.try_get<ActiveGlow>(entity);
      active != nullptr && active->expires_at.v > now.v) {
    return active->color;
  }

  if (auto *status = reg.try_get<StatusTint>(entity);
      status != nullptr && status->body_color) {
    return status->body_color;
  }

  if (auto *base = reg.try_get<BaseVisual>(entity);
      base != nullptr && base->body_color) {
    return base->body_color;
  }

  return std::nullopt;
}

std::optional<ftxui::Color>
VisualResolver::resolve_hp_bar_color(entt::registry &reg, entt::entity entity,
                                     seerin::uWu) {
  using namespace fl::ecs::components;

  if (!reg.valid(entity) || is_dead(reg, entity)) {
    return std::nullopt;
  }

  if (auto *status = reg.try_get<StatusTint>(entity);
      status != nullptr && status->hp_bar_color) {
    return status->hp_bar_color;
  }

  return std::nullopt;
}

std::optional<ftxui::Color>
VisualResolver::resolve_background_color(entt::registry &reg,
                                         entt::entity entity, seerin::uWu) {
  using namespace fl::ecs::components;

  if (!reg.valid(entity) || is_dead(reg, entity)) {
    return std::nullopt;
  }

  if (auto *status = reg.try_get<StatusTint>(entity);
      status != nullptr && status->background_color) {
    return status->background_color;
  }

  return std::nullopt;
}

} // namespace fl::ecs::systems
