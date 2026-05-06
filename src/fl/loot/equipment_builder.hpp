#pragma once

#include <entt/entt.hpp>

#include "fl/ecs/components/equipment.hpp"

namespace fl::loot {

struct EquipmentBuilder {
  fl::loot::EquipmentSlot slot;
  fl::loot::ArmorKind armor_kind{fl::loot::ArmorKind::none};
  std::string name;

  entt::entity create(entt::registry &reg) const {
    auto item = reg.create();

    reg.emplace<fl::ecs::components::Equipment>(
        item,
        slot,
        name,
        armor_kind);

    return item;
  }
};

};
