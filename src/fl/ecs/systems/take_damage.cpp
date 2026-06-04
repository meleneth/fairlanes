#include "fmt/format.h"

#include "fl/context.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "grant_xp_to_party.hpp"
#include "take_damage.hpp"

#include <chrono>
#include <limits>

namespace fl::ecs::systems {
int TakeDamage::commit(fl::context::AttackCtx &ctx) {
  return commit(ctx, fl::lospec500::color_at(4));
}

int TakeDamage::commit(fl::context::AttackCtx &ctx,
                       ftxui::Color damage_number_color) {
  using fl::ecs::components::Stats;
  auto &defender_stats = ctx.reg().get<Stats>(ctx.defender());

  if (fl::ecs::systems::CombatStatusSystem::attack_misses(ctx)) {
    return 0;
  }

  auto adjusted = fl::primitives::apply_resistance(ctx.damage(),
                                                   defender_stats.resistances_);
  fl::ecs::systems::CombatStatusSystem::apply_damage_modifiers(ctx, adjusted);
  int total = adjusted.physical + adjusted.magical + adjusted.fire +
              adjusted.ice + adjusted.lightning;
  auto &attacker_stats = ctx.reg().get<Stats>(ctx.attacker());

  auto was_alive = defender_stats.is_alive();
  defender_stats.hp_ = std::max(0, defender_stats.hp_ - total);

  if (total > 0) {
    fl::widgets::effects::DecalConfig config;
    config.color = damage_number_color;
    config.hitpoints = total;
    fl::ecs::components::add_combatant_decal(
        ctx.reg(), ctx.defender(),
        fl::ecs::components::DecalEffect{
            seerin::uWu{std::numeric_limits<int64_t>::max()},
            fl::ecs::components::DecalEffect::Clock::now(),
            std::chrono::seconds{1},
            fl::widgets::effects::DecalAnimationKind::HitpointNumber, config,
            2});
  }
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
        dead_party_data.encounter_data().clear_active_turn_for(ctx.defender());
        dead_party_data.encounter_data()
            .combatant_bus(ctx.defender())
            .emit(fl::events::CombatantEvent{
                fl::events::PlayerDied{ctx.defender(), ctx.attacker()}});
      }

      if (dead_party_data.all_members_dead()) {
        dead_party_data.party_bus().emit(
            fl::events::PartyEvent{fl::events::PartyWiped{}});
      }
    }

    if (party_member) {
      fl::systems::GrantXPToParty::commit(
          ctx.entity_context(party_member->party_), 256);
      auto &party_data = party_member->party().party_data();
      party_data.party_bus().emit(fl::events::PartyEvent{
          fl::events::LootDropRequested{ctx.defender(), party_member->party_}});
    }
  }
  // spdlog::info("{} hits {} for {} damage ({} HP left)",
  //            attacker.name(), name_, total, hp_);
  return total;
}

} // namespace fl::ecs::systems
