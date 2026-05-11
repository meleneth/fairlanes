#include "fmt/format.h"

#include "fl/context.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/lospec500.hpp"
#include "fl/loot/global_loot_table.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/party_data.hpp"
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
    auto party_member =
        ctx.reg().try_get<fl::ecs::components::PartyMember>(ctx.attacker());
    auto dead_party_member =
      ctx.reg().try_get<fl::ecs::components::PartyMember>(ctx.defender());

    ctx.log().append_markup(
        fmt::format("[name]({}) [error](killed) [name]({})!",
                    attacker_stats.name_, defender_stats.name_));

    if (dead_party_member) {
      auto &dead_party_data = dead_party_member->party().party_data();
      if (dead_party_data.has_encounter()) {
        dead_party_data.encounter_data().clear_pending_events_for(ctx.defender());
      }

      dead_party_data.party_bus().emit(fl::events::PartyEvent{
          fl::events::PlayerDied{ctx.defender(), ctx.attacker()}});
      if (dead_party_data.all_members_dead()) {
        dead_party_data.party_bus().emit(
            fl::events::PartyEvent{fl::events::PartyWiped{}});
      }
    }

    if (party_member) {
      fl::systems::GrantXPToParty::commit(
          ctx.entity_context(party_member->party_), 256);
      auto party_ctx = party_member->party().party_data().party_ctx();
      if (auto builder =
              fl::loot::global_loot_table().roll(party_ctx, "loot.global")) {
        auto item = builder->create(party_ctx.reg());

        const auto &equipment =
            party_ctx.reg().get<fl::ecs::components::Equipment>(item);

        ctx.log().append_markup(
            fmt::format("[ok](Loot found:) [ability]({})", equipment.name()));
        party_member->party().party_data().add_item(item);
      }
    }
  }
  // spdlog::info("{} hits {} for {} damage ({} HP left)",
  //            attacker.name(), name_, total, hp_);
}

} // namespace fl::ecs::systems
