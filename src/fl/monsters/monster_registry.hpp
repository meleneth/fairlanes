#pragma once

#include <functional>
#include <unordered_map>

#include "fl/monsters/monster_kind.hpp"
namespace fl::primitives {
class EntityBuilder;
}
namespace fl::monster {
using fl::primitives::EntityBuilder;
using MonsterArchetypeFn = std::function<void(EntityBuilder &)>;

// Accessor for a global registry.
// (Static inside a function so ODR stays sane.)
inline std::unordered_map<fl::monster::MonsterKind, MonsterArchetypeFn> &
monster_registry() {
  static std::unordered_map<fl::monster::MonsterKind, MonsterArchetypeFn>
      registry;
  return registry;
}

// Helper to register one monster.
inline void register_monster(fl::monster::MonsterKind kind,
                             MonsterArchetypeFn fn) {
  monster_registry()[kind] = std::move(fn);
}

} // namespace fl::monster
