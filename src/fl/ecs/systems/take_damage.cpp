#include "fmt/format.h"

#include "fl/context.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "grant_xp_to_party.hpp"
#include "take_damage.hpp"

namespace fl::ecs::systems {
using fl::context::EntityCtx;

void TakeDamage::commit(fl::context::AttackCtx &ctx) {
  using fl::ecs::components::Stats;
  auto &defender_stats = ctx.reg().get<Stats>(ctx.defender());

  auto adjusted = fl::primitives::apply_resistance(ctx.damage(),
                                                   defender_stats.resistances_);
  int total = adjusted.physical + adjusted.magical + adjusted.fire +
              adjusted.ice + adjusted.lightning;
  auto &attacker_stats = ctx.reg().get<Stats>(ctx.attacker());

  auto was_alive = defender_stats.is_alive();
  defender_stats.hp_ = std::max(0, defender_stats.hp_ - total);
  if (!defender_stats.is_alive() && was_alive) {
    auto party =
        ctx.reg().try_get<fl::ecs::components::PartyMember>(ctx.attacker());

    ctx.log().append_markup(
        fmt::format("[name]({}) [error](killed) [name]({})!",
                    attacker_stats.name_, defender_stats.name_));
    if (party) {
      fl::systems::GrantXPToParty::commit(ctx.entity_context(party->party_),
                                          256);
    }
  }
  // spdlog::info("{} hits {} for {} damage ({} HP left)",
  //            attacker.name(), name_, total, hp_);
}

} // namespace fl::ecs::systems
