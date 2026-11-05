#pragma once

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/equipment.hpp"

namespace fl::loot {

struct EquipmentBuilder {
  fl::loot::EquipmentSlot slot;
  fl::loot::ArmorKind armor_kind{fl::loot::ArmorKind::none};
  fl::loot::Tier tier{fl::loot::Tier::worn};
  std::string name;
  std::string unique_id{};
  fl::primitives::Resistances resistances{};
  int attack{0};
  int armor{0};
  int max_hp{0};

  entt::entity create(entt::registry& reg) const {
    auto item = reg.create();

    reg.emplace<fl::ecs::components::Equipment>(item, slot, name, armor_kind, tier, unique_id,
                                                resistances, attack, armor, max_hp);

    return item;
  }
};

}; // namespace fl::loot
