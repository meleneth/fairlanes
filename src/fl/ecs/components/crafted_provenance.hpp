#pragma once

#include <string>
#include <vector>

#include <entt/entity/entity.hpp>

#include "fl/economy/material.hpp"
#include "fl/economy/profession.hpp"

namespace fl::ecs::components {

struct MaterialAmount {
  fl::economy::Material material{fl::economy::Material::WoodlandHerb};
  int quantity{0};
};

struct CraftedProvenance {
  entt::entity crafter{entt::null};
  std::string crafter_name;
  fl::economy::Profession profession{fl::economy::Profession::Alchemy};
  int proficiency{0};
  std::vector<MaterialAmount> materials;
};

} // namespace fl::ecs::components
