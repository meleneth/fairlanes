#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "fl/generated/monster_content.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/skills/skill.hpp"
namespace fl::primitives {
class EntityBuilder;
}
namespace fl::monster {
using fl::primitives::EntityBuilder;
using MonsterArchetypeFn = std::function<void(EntityBuilder &)>;

struct MonsterDefinition {
  MonsterArchetypeFn archetype;
  std::vector<fl::skills::SkillId> known_skills;
};

// Accessor for a global registry.
// (Static inside a function so ODR stays sane.)
inline std::unordered_map<fl::monster::MonsterKind, MonsterDefinition> &
monster_registry() {
  static std::unordered_map<fl::monster::MonsterKind, MonsterDefinition>
      registry;
  return registry;
}

// Helper to register one monster.
inline void register_monster(fl::monster::MonsterKind kind,
                             MonsterArchetypeFn fn) {
  const auto known_skills = generated_content::known_skills(kind);
  monster_registry()[kind] =
      MonsterDefinition{std::move(fn), std::vector<fl::skills::SkillId>(
                                           known_skills.begin(),
                                           known_skills.end())};
}

} // namespace fl::monster
