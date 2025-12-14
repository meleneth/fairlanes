#include "fmt/format.h"

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/widgets/fancy_log.hpp"

#include "grant_xp_to_party.hpp"

namespace fl::systems {
using fl::context::EntityCtx;

void GrantXPToParty::commit(const EntityCtx &ctx, int amount) {
  using namespace fl::ecs::components;
  ctx.log_.append_markup(fmt::format("Party received [xp]({}) XP.", amount));

  auto view = ctx.reg().view<PartyMember, TrackXP>();
  for (auto entity : view) {
    auto &member = view.get<PartyMember>(entity);
    auto &track = view.get<TrackXP>(entity);

    if (member.party_ == ctx.self_) {
      entt::handle h{ctx.reg(), entity};
      track.add_xp(h, amount);
    }
  }
}

} // namespace fl::systems
