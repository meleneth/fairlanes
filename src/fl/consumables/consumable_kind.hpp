#pragma once

#include <string_view>

#include "fl/resources/combat_resource.hpp"

namespace fl::consumables {

enum class ConsumableKind {
  HealingPotion,
  AdrenalinePotion,
  ReflexPotion,
  AttackFlask,
  ArmorFlask,
  TempoFlask,
};

enum class ConsumableCategory { Potion, Flask };
enum class TimedBuffKind { Attack, Armor, AtbChargeRate };

inline constexpr ConsumableCategory category(ConsumableKind kind) noexcept {
  return kind == ConsumableKind::HealingPotion || kind == ConsumableKind::AdrenalinePotion ||
                 kind == ConsumableKind::ReflexPotion
             ? ConsumableCategory::Potion
             : ConsumableCategory::Flask;
}

inline constexpr std::string_view stable_id(ConsumableKind kind) noexcept {
  switch (kind) {
  case ConsumableKind::HealingPotion: return "healing_potion";
  case ConsumableKind::AdrenalinePotion: return "adrenaline_potion";
  case ConsumableKind::ReflexPotion: return "reflex_potion";
  case ConsumableKind::AttackFlask: return "attack_flask";
  case ConsumableKind::ArmorFlask: return "armor_flask";
  case ConsumableKind::TempoFlask: return "tempo_flask";
  }
  return "unknown";
}

inline constexpr std::string_view display_name(ConsumableKind kind) noexcept {
  switch (kind) {
  case ConsumableKind::HealingPotion: return "Healing Potion";
  case ConsumableKind::AdrenalinePotion: return "Adrenaline Potion";
  case ConsumableKind::ReflexPotion: return "Reflex Potion";
  case ConsumableKind::AttackFlask: return "Attack Flask";
  case ConsumableKind::ArmorFlask: return "Armor Flask";
  case ConsumableKind::TempoFlask: return "Tempo Flask";
  }
  return "Unknown Consumable";
}

inline constexpr fl::resources::CombatResource potion_resource(ConsumableKind kind) noexcept {
  return kind == ConsumableKind::AdrenalinePotion ? fl::resources::CombatResource::Adrenaline
                                                   : fl::resources::CombatResource::Reflexes;
}

inline constexpr std::string_view display_name(TimedBuffKind kind) noexcept {
  switch (kind) {
  case TimedBuffKind::Attack: return "Attack Flask";
  case TimedBuffKind::Armor: return "Armor Flask";
  case TimedBuffKind::AtbChargeRate: return "Tempo Flask";
  }
  return "Unknown Flask";
}

} // namespace fl::consumables
