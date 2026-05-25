#include "fl/ecs/systems/dire_bleed_system.hpp"

#include <fmt/format.h>

#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/status_effect_lifetime.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
constexpr int kDireBleedTickSeconds = 3;
constexpr int kDireBleedTickBeats =
    fl::primitives::WorldClock::beats_from_seconds(kDireBleedTickSeconds);
} // namespace

void DireBleedSystem::bind_cleanup_and_schedule(
    fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
    entt::entity target) {
  auto *bleed = party_ctx.reg().try_get<fl::ecs::components::DireBleed>(target);
  if (bleed == nullptr) {
    return;
  }

  if (!party_ctx.reg().valid(bleed->effect.effect_id)) {
    bleed->effect = StatusEffectLifetime::create_instance(party_ctx, target);
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, bleed->effect};
  lifetime.on_owner_died([&party_ctx, target](const fl::events::PlayerDied &) {
    DireBleedSystem::clear(party_ctx, target);
  });
  lifetime.on_combat_removed([&party_ctx, target](const auto &) {
    DireBleedSystem::clear(party_ctx, target);
  });

  schedule_tick(party_ctx, scheduler, target);
}

void DireBleedSystem::schedule_tick(fl::context::PartyCtx &party_ctx,
                                    Scheduler &scheduler, entt::entity target) {
  auto *scheduled_bleed =
      party_ctx.reg().try_get<fl::ecs::components::DireBleed>(target);
  if (scheduled_bleed == nullptr) {
    return;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, scheduled_bleed->effect};
  lifetime.schedule_in_beats(
      kDireBleedTickBeats, "dire bleed: tick",
      [&party_ctx, &scheduler, target] {
        auto &reg = party_ctx.reg();
        if (!reg.valid(target)) {
          DireBleedSystem::clear(party_ctx, target);
          return;
        }

        auto *bleed = reg.try_get<fl::ecs::components::DireBleed>(target);
        auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
        if (bleed == nullptr || target_stats == nullptr ||
            !target_stats->is_alive() || !reg.valid(bleed->source)) {
          DireBleedSystem::clear(party_ctx, target);
          return;
        }

        auto *source_stats =
            reg.try_get<fl::ecs::components::Stats>(bleed->source);
        if (source_stats == nullptr || !source_stats->is_alive()) {
          DireBleedSystem::clear(party_ctx, target);
          return;
        }

        auto attack_ctx = fl::context::AttackCtx::make_attack(
            party_ctx, bleed->source, target);
        attack_ctx.damage().physical = bleed->damage_per_tick;
        party_ctx.log().append_markup(
            fmt::format("[error](Dire Bleed) tears at "
                        "[player_name]({}) for [error]({}) "
                        "damage.",
                        target_stats->name_, bleed->damage_per_tick));
        fl::ecs::systems::TakeDamage::commit(attack_ctx);

        if (reg.valid(target) &&
            reg.any_of<fl::ecs::components::DireBleed>(target)) {
          DireBleedSystem::schedule_tick(party_ctx, scheduler, target);
        }
      });
}

void DireBleedSystem::clear(fl::context::PartyCtx &party_ctx,
                            entt::entity target) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(target)) {
    return;
  }

  auto *bleed = reg.try_get<fl::ecs::components::DireBleed>(target);
  if (bleed == nullptr) {
    return;
  }

  auto &scheduler =
      party_ctx.party_data().encounter_data().atb_engine().scheduler();
  StatusEffectLifetime lifetime{party_ctx, scheduler, bleed->effect};
  lifetime.clear_scheduled();
  lifetime.destroy_instance_entity();

  fl::ecs::components::safe_clear_hp_bar_color(reg, target);
  reg.remove<fl::ecs::components::DireBleed>(target);
}

} // namespace fl::ecs::systems
