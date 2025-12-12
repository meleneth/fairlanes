#include "field_mouse.hpp"

#include <algorithm>

#include "fl/ecs/components/stats.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/component_builder.hpp"
#include "fl/primitives/entity_builder.hpp"

namespace fl::monster {

using fl::ecs::components::Stats;
void FieldMouse::apply(EntityBuilder &b) {
  // Ensure Stats exists (either defaulted elsewhere or create fresh)
  auto &s = b.ctx().reg_.get<Stats>(b.entity());
  s.name_ = "Field Mouse";
  s.hp_ = 5;
  s.max_hp_ = 5;
  s.mp_ = 0;

  // Optional tags
  // auto &t = reg.get_or_emplace<Tags>(b.entity());
  // t.values = {"mouse", "precious"};
}

void register_field_mouse() {
  register_monster(fl::monster::MonsterKind::FieldMouse,
                   [](EntityBuilder &b) { FieldMouse::apply(b); });
}

} // namespace fl::monster
