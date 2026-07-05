#include "field_mouse.hpp"

#include "fl/monsters/apply_monster_stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::primitives::EntityBuilder;

void FieldMouse::apply(EntityBuilder &b) {
  apply_monster_stats(b, MonsterKind::FieldMouse);

  // Optional tags
  // auto &t = reg.get_or_emplace<Tags>(b.entity());
  // t.values = {"mouse", "precious"};
}

void register_field_mouse() {
  register_monster(fl::monster::MonsterKind::FieldMouse,
                   [](EntityBuilder &b) { FieldMouse::apply(b); });
}

} // namespace fl::monster
