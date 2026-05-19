#include "register_monsters.hpp"

namespace fl::monster {

void register_all_monsters() {
  register_field_mouse();
  register_woodland_critters();
  register_honey_badger();
  register_yeti();
  register_salamander();
  register_fire_drake();
  register_decal_monsters();
  // register_wolf();
  // register_slime();
  // ...
}
} // namespace fl::monster
