#include "fl/ecs/systems/poison_system.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/status_effect_lifetime.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
constexpr int kPoisonTickSeconds = 3;
constexpr int kPoisonTickBeats =
    fl::primitives::WorldClock::beats_from_seconds(kPoisonTickSeconds);
constexpr int kPoisonPulseBeats =
    fl::primitives::WorldClock::beats_from_seconds(1);
constexpr std::size_t kPoisonDimGreen = 22;
constexpr std::size_t kPoisonBrightGreen = 20;

int ticks_for_duration(int duration_seconds) {
  if (duration_seconds <= 0) {
    return 0;
  }

  return std::max(1, duration_seconds / kPoisonTickSeconds);
}
} // namespace

fl::events::ScopedCombatantListener
PoisonSystem::bind_apply_listener(fl::context::PartyCtx &party_ctx,
                                  fl::events::CombatantBus &combatant_bus,
                                  Scheduler &scheduler) {
  return fl::events::ScopedCombatantListener{
      combatant_bus, std::in_place_type<fl::events::PoisonApplied>,
      [&party_ctx, &scheduler](const fl::events::PoisonApplied &ev) {
        PoisonSystem::apply(party_ctx, scheduler, ev.source, ev.target,
                            ev.damage_per_tick, ev.duration_seconds);
      }};
}

void PoisonSystem::apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                         entt::entity source, entt::entity target,
                         int damage_per_tick, int duration_seconds) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target) || damage_per_tick <= 0) {
    return;
  }

  const auto ticks_remaining = ticks_for_duration(duration_seconds);
  if (ticks_remaining <= 0) {
    return;
  }

  auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
  if (target_stats == nullptr || !target_stats->is_alive()) {
    return;
  }

  if (reg.any_of<fl::ecs::components::Poison>(target)) {
    PoisonSystem::clear(party_ctx, target);
  }

  auto &poison = reg.emplace<fl::ecs::components::Poison>(target);
  poison.source = source;
  poison.damage_per_tick = damage_per_tick;
  poison.ticks_remaining = ticks_remaining;
  poison.effect = StatusEffectLifetime::create_instance(party_ctx, target);

  StatusEffectLifetime lifetime{party_ctx, scheduler, poison.effect};
  lifetime.on_owner_died([&party_ctx, target](const fl::events::PlayerDied &) {
    PoisonSystem::clear(party_ctx, target);
  });
  lifetime.on_combat_removed([&party_ctx, target](const auto &) {
    PoisonSystem::clear(party_ctx, target);
  });

  fl::ecs::components::safe_add_hp_bar_color(
      reg, target, fl::lospec500::color_at(kPoisonDimGreen));
  party_ctx.log().append_markup(
      fmt::format("{} used [ability](Poison) on {}; poison seeps in.",
                  party_ctx.log().name_tag_for(entt::handle{reg, source}),
                  party_ctx.log().name_tag_for(entt::handle{reg, target})));

  schedule_visual_pulse(party_ctx, scheduler, target, true);
  schedule_tick(party_ctx, scheduler, target);
}

void PoisonSystem::schedule_tick(fl::context::PartyCtx &party_ctx,
                                 Scheduler &scheduler, entt::entity target) {
  auto *scheduled_poison =
      party_ctx.reg().try_get<fl::ecs::components::Poison>(target);
  if (scheduled_poison == nullptr) {
    return;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, scheduled_poison->effect};
  lifetime.schedule_in_beats(
      kPoisonTickBeats, "poison: tick", [&party_ctx, &scheduler, target] {
        auto &reg = party_ctx.reg();
        if (!reg.valid(target)) {
          PoisonSystem::clear(party_ctx, target);
          return;
        }

        auto *poison = reg.try_get<fl::ecs::components::Poison>(target);
        auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
        if (poison == nullptr || target_stats == nullptr ||
            !target_stats->is_alive() || poison->ticks_remaining <= 0 ||
            !reg.valid(poison->source)) {
          PoisonSystem::clear(party_ctx, target);
          return;
        }

        auto *source_stats =
            reg.try_get<fl::ecs::components::Stats>(poison->source);
        if (source_stats == nullptr || !source_stats->is_alive()) {
          PoisonSystem::clear(party_ctx, target);
          return;
        }

        auto attack_ctx = fl::context::AttackCtx::make_attack(
            party_ctx, poison->source, target);
        attack_ctx.damage().magical = poison->damage_per_tick;
        party_ctx.log().append_markup(fmt::format(
            "[ability](Poison) from {} hurts {} for "
            "[error]({}) damage.",
            party_ctx.log().name_tag_for(entt::handle{reg, poison->source}),
            party_ctx.log().name_tag_for(entt::handle{reg, target}),
            poison->damage_per_tick));
        fl::ecs::systems::TakeDamage::commit(attack_ctx);

        if (reg.valid(target)) {
          if (auto *remaining =
                  reg.try_get<fl::ecs::components::Poison>(target)) {
            --remaining->ticks_remaining;
            if (remaining->ticks_remaining > 0) {
              PoisonSystem::schedule_tick(party_ctx, scheduler, target);
              return;
            }
          }
        }

        PoisonSystem::clear(party_ctx, target);
      });
}

void PoisonSystem::schedule_visual_pulse(fl::context::PartyCtx &party_ctx,
                                         Scheduler &scheduler,
                                         entt::entity target, bool bright) {
  auto *scheduled_poison =
      party_ctx.reg().try_get<fl::ecs::components::Poison>(target);
  if (scheduled_poison == nullptr) {
    return;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, scheduled_poison->effect};
  lifetime.schedule_in_beats(
      kPoisonPulseBeats, "poison: visual pulse",
      [&party_ctx, &scheduler, target, bright] {
        auto &reg = party_ctx.reg();
        if (!reg.valid(target) ||
            !reg.any_of<fl::ecs::components::Poison>(target)) {
          return;
        }

        fl::ecs::components::safe_add_hp_bar_color(
            reg, target,
            fl::lospec500::color_at(bright ? kPoisonBrightGreen
                                           : kPoisonDimGreen));
        PoisonSystem::schedule_visual_pulse(party_ctx, scheduler, target,
                                            !bright);
      });
}

void PoisonSystem::clear(fl::context::PartyCtx &party_ctx,
                         entt::entity target) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(target)) {
    return;
  }

  auto *poison = reg.try_get<fl::ecs::components::Poison>(target);
  if (poison == nullptr) {
    return;
  }

  auto &scheduler =
      party_ctx.party_data().encounter_data().atb_engine().scheduler();
  StatusEffectLifetime lifetime{party_ctx, scheduler, poison->effect};
  lifetime.clear_scheduled();
  lifetime.destroy_instance_entity();

  fl::ecs::components::safe_clear_hp_bar_color(reg, target);
  reg.remove<fl::ecs::components::Poison>(target);
}

} // namespace fl::ecs::systems
