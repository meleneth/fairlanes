#include "member_data.hpp"

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::primitives {

void MemberData::hook_level_progression(entt::registry& reg) {
  level_gained_sub_ = fl::events::ScopedPartyListener{
      bus_, std::in_place_type<fl::events::PartyGainedLevel>,
      [this, &reg](const fl::events::PartyGainedLevel& ev) {
        if (ev.member != member_id_) {
          return;
        }

        if (auto* stats = reg.try_get<fl::ecs::components::Stats>(member_id_)) {
          stats->max_hp_ += 5;
        }
        if (auto* member = reg.try_get<fl::ecs::components::PartyMember>(member_id_)) {
          fl::ecs::systems::PartyGearing::commit(member->party().party_data().party_ctx());
        }
      }};
}

} // namespace fl::primitives
