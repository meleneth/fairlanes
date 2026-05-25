#include <fmt/core.h>

#include "fl/context.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "track_xp.hpp"

namespace fl::ecs::components {
/// Closed-form XP curve: sum_{i=1}^{n} i * BASE_XP_VALUE
int TrackXP::xp_for_level(int level_calc) {
  if (level_calc <= 0)
    return 0;
  if (level_calc > 100)
    return 0;
  return BASE_XP_VALUE * (level_calc * (level_calc + 1)) / 2;
}

TrackXP::TrackXP(fl::context::EntityCtx ctx, int starting_xp)
    : xp_(starting_xp), next_level_at(xp_for_level(level_ + 1)), ctx_(ctx) {}

void TrackXP::add_xp(entt::handle self, int amount) {
  xp_ += amount;
  while (next_level_at && xp_ >= next_level_at) {
    ++level_;
    next_level_at = xp_for_level(level_ + 1);
    if (auto info = self.try_get<fl::ecs::components::Stats>()) {
      ctx_.log().append_markup(fmt::format(
          "[name]({}) Level up! level=[level]({}) in next=[xp]({}) XP",
          info->name_, level_, next_level_at));
    }

    if (auto *member = self.try_get<fl::ecs::components::PartyMember>()) {
      member->member_data().bus().emit(fl::events::PartyEvent{
          fl::events::PartyGainedLevel{.member = self.entity(),
                                       .level = level_}});
    }
  }
}
} // namespace fl::ecs::components
