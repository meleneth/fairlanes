#pragma once

#include <algorithm>

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/loot/equipment_slot.hpp"

namespace fl::ecs::systems {

// Stats stores the combatant's base numbers. Equipment is read from the current
// closet each time, so replacing an item cannot accumulate its old bonuses.
struct EffectiveCombatStats {
  int attack;
  int armor;
  int max_hp;
  fl::primitives::Resistances resistances;
};

inline EffectiveCombatStats effective_combat_stats(entt::registry &reg,
                                                   entt::entity actor) {
  const auto &base = reg.get<fl::ecs::components::Stats>(actor);
  EffectiveCombatStats result{base.attack_, base.armor_, base.max_hp_,
                              base.resistances_};
  const auto *member = reg.try_get<fl::ecs::components::PartyMember>(actor);
  if (!member) return result;
  for (const auto slot : fl::loot::kEquipmentSlots) {
    const auto item = member->closet().equipped_item(slot);
    if (!reg.valid(item)) continue;
    const auto *gear = reg.try_get<fl::ecs::components::Equipment>(item);
    if (!gear) continue;
    result.attack += gear->attack();
    result.armor += gear->armor();
    result.max_hp += gear->max_hp();
    result.resistances += gear->resistances();
  }
  result.max_hp = std::max(1, result.max_hp);
  return result;
}
} // namespace fl::ecs::systems
