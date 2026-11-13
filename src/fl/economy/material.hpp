#pragma once

#include <array>
#include <string_view>

namespace fl::economy {

enum class Material {
  WoodlandHerb,
  WoodlandFiber,
  WoodlandHide,
  PlainsBloom,
  PlainsLeather,
  SwampReagent,
  CaveOre,
  DesertSalt,
  Count,
};

inline constexpr std::array<Material, 8> kMaterials{
    Material::WoodlandHerb, Material::WoodlandFiber, Material::WoodlandHide,
    Material::PlainsBloom, Material::PlainsLeather, Material::SwampReagent,
    Material::CaveOre, Material::DesertSalt};

inline constexpr std::string_view stable_id(Material material) noexcept {
  switch (material) {
  case Material::WoodlandHerb: return "woodland_herb";
  case Material::WoodlandFiber: return "woodland_fiber";
  case Material::WoodlandHide: return "woodland_hide";
  case Material::PlainsBloom: return "plains_bloom";
  case Material::PlainsLeather: return "plains_leather";
  case Material::SwampReagent: return "swamp_reagent";
  case Material::CaveOre: return "cave_ore";
  case Material::DesertSalt: return "desert_salt";
  case Material::Count: return "unknown";
  }
  return "unknown";
}

} // namespace fl::economy
