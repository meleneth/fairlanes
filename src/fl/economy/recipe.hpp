#pragma once

#include <array>
#include <string_view>

namespace fl::economy {

enum class RecipeId {
  HealingPotion,
  AdrenalinePotion,
  ReflexPotion,
  AttackFlask,
  ArmorFlask,
  ClothChest,
  LeatherChest,
  PlateChest,
  OreHammer,
  Count,
};

inline constexpr std::array<RecipeId, 9> kRecipes{
    RecipeId::HealingPotion, RecipeId::AdrenalinePotion, RecipeId::ReflexPotion,
    RecipeId::AttackFlask, RecipeId::ArmorFlask, RecipeId::ClothChest,
    RecipeId::LeatherChest, RecipeId::PlateChest, RecipeId::OreHammer};

inline constexpr std::string_view stable_id(RecipeId recipe) noexcept {
  switch (recipe) {
  case RecipeId::HealingPotion: return "healing_potion";
  case RecipeId::AdrenalinePotion: return "adrenaline_potion";
  case RecipeId::ReflexPotion: return "reflex_potion";
  case RecipeId::AttackFlask: return "attack_flask";
  case RecipeId::ArmorFlask: return "armor_flask";
  case RecipeId::ClothChest: return "cloth_chest";
  case RecipeId::LeatherChest: return "leather_chest";
  case RecipeId::PlateChest: return "plate_chest";
  case RecipeId::OreHammer: return "ore_hammer";
  case RecipeId::Count: return "unknown";
  }
  return "unknown";
}

} // namespace fl::economy
