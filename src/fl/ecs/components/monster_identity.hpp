#pragma once

#include "fl/monsters/monster_kind.hpp"

namespace fl::ecs::components {

struct MonsterIdentity {
  fl::monster::MonsterKind kind;
};

} // namespace fl::ecs::components
