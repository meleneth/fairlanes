#pragma once

#include "fl/primitives/combat_affinity.hpp"

namespace fl::ecs::components {

struct ActiveCombatAffinity {
  fl::primitives::CombatAffinity value{fl::primitives::CombatAffinity::Cloth};
};

struct CombatAffinities {
  fl::primitives::CombatAffinitySet values{};
};

} // namespace fl::ecs::components
