#include "member_data.hpp"

#include "fl/ecs/components/stats.hpp"

namespace fl::primitives {

void MemberData::hook_level_progression(entt::registry &reg) {
  level_gained_sub_ = fl::events::ScopedPartyListener{
      bus_, std::in_place_type<fl::events::PartyGainedLevel>,
      [this, &reg](const fl::events::PartyGainedLevel &ev) {
        if (ev.member != member_id_) {
          return;
        }

        if (auto *stats = reg.try_get<fl::ecs::components::Stats>(member_id_)) {
          stats->max_hp_ += 5;
        }
      }};
}

} // namespace fl::primitives
