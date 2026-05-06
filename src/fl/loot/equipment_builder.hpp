#pragma once

#include <entt/entt.hpp>

#include "fl/ecs/components/equipment.hpp"

namespace fl::loot {

struct EquipmentBuilder {
  fl::ecs::components::EquipmentSlot slot;
  fl::ecs::components::ArmorKind armor_kind{fl::ecs::components::ArmorKind::none};
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
