#include "fl/ecs/systems/dire_bleed_system.hpp"

#include <fmt/format.h>

#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
constexpr int kDireBleedTickSeconds = 3;
constexpr int kDireBleedTickBeats =
    fl::primitives::WorldClock::beats_from_seconds(kDireBleedTickSeconds);
} // namespace

void DireBleedSystem::bind_cleanup_and_schedule(
    fl::context::PartyCtx &party_ctx, Scheduler &scheduler, entt::entity target,
    ClearPendingFn clear_pending) {
  auto *bleed = party_ctx.reg().try_get<fl::ecs::components::DireBleed>(target);
  if (bleed == nullptr) {
    return;
  }

  bleed->player_died_sub = fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::PlayerDied>,
      [&party_ctx, target, clear_pending](const fl::events::PlayerDied &ev) {
        if (ev.player == target) {
          DireBleedSystem::clear(party_ctx, target, clear_pending);
        }
      }};

  bleed->left_combat_sub = fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::PartyLeftCombat>,
      [&party_ctx, target, clear_pending](const fl::events::PartyLeftCombat &) {
        DireBleedSystem::clear(party_ctx, target, clear_pending);
      }};

  schedule_tick(party_ctx, scheduler, target, std::move(clear_pending));
}

void DireBleedSystem::schedule_tick(fl::context::PartyCtx &party_ctx,
                                    Scheduler &scheduler, entt::entity target,
                                    ClearPendingFn clear_pending) {
  scheduler.schedule_smelly_in_beats_for(
      kDireBleedTickBeats, target, "dire bleed: tick",
      [&party_ctx, &scheduler, target, clear_pending] {
        auto &reg = party_ctx.reg();
        if (!reg.valid(target)) {
          DireBleedSystem::clear(party_ctx, target, clear_pending);
          return;
        }

        auto *bleed = reg.try_get<fl::ecs::components::DireBleed>(target);
        auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
        if (bleed == nullptr || target_stats == nullptr ||
            !target_stats->is_alive() || !reg.valid(bleed->source)) {
          DireBleedSystem::clear(party_ctx, target, clear_pending);
          return;
        }

        auto *source_stats =
            reg.try_get<fl::ecs::components::Stats>(bleed->source);
        if (source_stats == nullptr || !source_stats->is_alive()) {
          DireBleedSystem::clear(party_ctx, target, clear_pending);
          return;
        }

        auto attack_ctx = fl::context::AttackCtx::make_attack(
            party_ctx, bleed->source, target);
        attack_ctx.damage().physical = bleed->damage_per_tick;
        party_ctx.log().append_markup(fmt::format(
            "[error](Dire Bleed) tears at [player_name]({}) for [error]({}) "
            "damage.",
            target_stats->name_, bleed->damage_per_tick));
        fl::ecs::systems::TakeDamage::commit(attack_ctx);

        if (reg.valid(target) &&
            reg.any_of<fl::ecs::components::DireBleed>(target)) {
          DireBleedSystem::schedule_tick(party_ctx, scheduler, target,
                                         clear_pending);
        }
      });
}

void DireBleedSystem::clear(fl::context::PartyCtx &party_ctx,
                            entt::entity target,
                            const ClearPendingFn &clear_pending) {
  auto &reg = party_ctx.reg();
  clear_pending(target);

  // TODO: replace broad color clears with scoped visual ownership once color
  // overrides can be attributed to the effect that created them.
  fl::ecs::components::safe_clear_color(reg, target);
  fl::ecs::components::safe_clear_hp_bar_color(reg, target);
  if (reg.valid(target) && reg.any_of<fl::ecs::components::DireBleed>(target)) {
    // This may run from one of DireBleed's own party-bus callbacks. eventpp
    // CallbackList supports removal during dispatch by keeping the current node
    // alive and linked for iteration.
    reg.remove<fl::ecs::components::DireBleed>(target);
  }
}

} // namespace fl::ecs::systems
