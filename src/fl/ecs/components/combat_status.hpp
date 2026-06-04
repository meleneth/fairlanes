#pragma once

#include <entt/entt.hpp>

#include <string>
#include <vector>

#include "fl/ecs/components/status_effect.hpp"

namespace fl::ecs::components {

enum class CombatStatusKind {
  Shield,
  Blind,
  Silence,
  Slow,
  Stun,
  Haste,
  Burn,
};

struct CombatStatusEffect {
  CombatStatusEffect() = default;
  CombatStatusEffect(CombatStatusEffect &&) noexcept = default;
  CombatStatusEffect &operator=(CombatStatusEffect &&) noexcept = default;
  CombatStatusEffect(const CombatStatusEffect &) = delete;
  CombatStatusEffect &operator=(const CombatStatusEffect &) = delete;

  int id{0};
  CombatStatusKind kind{CombatStatusKind::Blind};
  std::string name;
  entt::entity source{entt::null};
  int value{0};
  int stacks{1};
  int turns_remaining{0};
  int ticks_remaining{0};
  int tick_damage{0};
  bool negative{true};
  bool removable{true};
  StatusEffectInstance effect;
};

struct CombatStatuses {
  CombatStatuses() = default;
  CombatStatuses(CombatStatuses &&) noexcept = default;
  CombatStatuses &operator=(CombatStatuses &&) noexcept = default;
  CombatStatuses(const CombatStatuses &) = delete;
  CombatStatuses &operator=(const CombatStatuses &) = delete;

  int next_id{1};
  std::vector<CombatStatusEffect> effects;
};

} // namespace fl::ecs::components
