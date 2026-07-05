#include "register_monsters.hpp"

#include "fl/generated/monster_registration.hpp"

namespace fl::monster {

void register_all_monsters() {
  generated_content::register_monster_archetypes();
}
} // namespace fl::monster
