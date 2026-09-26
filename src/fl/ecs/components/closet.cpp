#include "closet.hpp"

namespace fl::ecs::components {
namespace {

using EquipmentMember = entt::entity Closet::*;

EquipmentMember equipment_member(fl::loot::EquipmentSlot slot) noexcept {
  using fl::loot::EquipmentSlot;

  switch (slot) {
  case EquipmentSlot::chest:
    return &Closet::chest;
  case EquipmentSlot::helm:
    return &Closet::helm;
  case EquipmentSlot::pants:
    return &Closet::pants;
  case EquipmentSlot::belt:
    return &Closet::belt;
  case EquipmentSlot::boots:
    return &Closet::boots;
  case EquipmentSlot::gloves:
    return &Closet::gloves;
  case EquipmentSlot::sleeves:
    return &Closet::sleeves;
  case EquipmentSlot::cape:
    return &Closet::cape;
  case EquipmentSlot::necklace:
    return &Closet::necklace;
  case EquipmentSlot::ring_1:
    return &Closet::ring_1;
  case EquipmentSlot::ring_2:
    return &Closet::ring_2;
  case EquipmentSlot::mainhand:
    return &Closet::mainhand;
  case EquipmentSlot::offhand:
    return &Closet::offhand;
  case EquipmentSlot::knife:
    return &Closet::knife;
  }

  return &Closet::chest;
}

} // namespace

Closet::Closet()
    : chest(entt::null), helm(entt::null), pants(entt::null), belt(entt::null),
      boots(entt::null), gloves(entt::null), sleeves(entt::null),
      cape(entt::null), necklace(entt::null), ring_1(entt::null),
      ring_2(entt::null), mainhand(entt::null), offhand(entt::null),
      knife(entt::null),
      skill_slots{fl::skills::SkillKey{fl::skills::SkillId::Observe},
                  std::nullopt, std::nullopt, std::nullopt, std::nullopt} {}

bool Closet::empty() const {
  return chest == entt::null && helm == entt::null && pants == entt::null &&
         belt == entt::null && boots == entt::null && gloves == entt::null &&
         sleeves == entt::null && cape == entt::null &&
         necklace == entt::null && ring_1 == entt::null &&
         ring_2 == entt::null && mainhand == entt::null &&
         offhand == entt::null && knife == entt::null;
}

bool Closet::has_equipped_skill(fl::skills::SkillKey skill) const noexcept {
  for (const auto equipped : skill_slots) {
    if (equipped == skill) {
      return true;
    }
  }
  return false;
}

bool Closet::has_open_skill_slot() const noexcept {
  for (const auto equipped : skill_slots) {
    if (!equipped.has_value()) {
      return true;
    }
  }
  return false;
}

bool Closet::equip_skill(fl::skills::SkillKey skill) noexcept {
  if (has_equipped_skill(skill)) {
    return false;
  }

  for (auto &equipped : skill_slots) {
    if (!equipped.has_value()) {
      equipped = skill;
      return true;
    }
  }

  return false;
}

bool Closet::unequip_skill(fl::skills::SkillKey skill) noexcept {
  for (auto &equipped : skill_slots) {
    if (equipped == skill) {
      equipped.reset();
      return true;
    }
  }

  return false;
}

entt::entity
Closet::equipped_item(fl::loot::EquipmentSlot slot) const noexcept {
  return this->*equipment_member(slot);
}

entt::entity &Closet::equipped_item(fl::loot::EquipmentSlot slot) {
  return this->*equipment_member(slot);
}

} // namespace fl::ecs::components
