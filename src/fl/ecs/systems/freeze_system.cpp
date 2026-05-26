#include "fl/ecs/systems/freeze_system.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/status_effect_lifetime.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/fancy_log.hpp"
#include <ftxui/screen/color.hpp>

namespace fl::ecs::systems {
namespace {
constexpr std::size_t kFreezeBlue = 16;
constexpr std::size_t kFreezeFadeStartBlue = 28;
constexpr int kFreezeFadeInBeats =
    fl::primitives::WorldClock::kBeatsPerSecond / 4;

void set_background(entt::registry &reg, entt::entity target,
                    ftxui::Color color) {
  auto &status = reg.get_or_emplace<fl::ecs::components::StatusTint>(target);
  status.background_color = color;
}

void clear_background(entt::registry &reg, entt::entity target) {
  if (!reg.valid(target)) {
    return;
  }

  if (auto *status = reg.try_get<fl::ecs::components::StatusTint>(target)) {
    status->background_color.reset();
    if (!status->body_color && !status->hp_bar_color &&
        !status->background_color) {
      reg.remove<fl::ecs::components::StatusTint>(target);
    }
  }
}
} // namespace

fl::events::ScopedCombatantListener
FreezeSystem::bind_apply_listener(fl::context::PartyCtx &party_ctx,
                                  fl::events::CombatantBus &combatant_bus,
                                  Scheduler &scheduler) {
  return fl::events::ScopedCombatantListener{
      combatant_bus, std::in_place_type<fl::events::FreezeApplied>,
      [&party_ctx, &scheduler](const fl::events::FreezeApplied &ev) {
        FreezeSystem::apply(party_ctx, scheduler, ev.source, ev.target,
                            ev.duration_seconds);
      }};
}

void FreezeSystem::apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                         entt::entity source, entt::entity target,
                         int duration_seconds) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target)) {
    return;
  }

  const int clear_after_beats = fl::primitives::WorldClock::beats_from_seconds(
      std::max(1, duration_seconds));

  auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
  if (target_stats == nullptr || !target_stats->is_alive()) {
    return;
  }

  if (reg.any_of<fl::ecs::components::Freeze>(target)) {
    FreezeSystem::shatter(party_ctx, source, target);
    return;
  }

  auto &freeze = reg.emplace<fl::ecs::components::Freeze>(target);
  freeze.source = source;
  freeze.clear_after_beats = clear_after_beats;
  freeze.effect = StatusEffectLifetime::create_instance(party_ctx, target);

  StatusEffectLifetime lifetime{party_ctx, scheduler, freeze.effect};
  lifetime.on_owner_died([&party_ctx, target](const fl::events::PlayerDied &) {
    FreezeSystem::clear(party_ctx, target);
  });
  lifetime.on_combat_removed([&party_ctx, target](const auto &) {
    FreezeSystem::clear(party_ctx, target);
  });

  set_background(reg, target, fl::lospec500::color_at(kFreezeFadeStartBlue));
  party_ctx.log().append_markup(
      fmt::format("{} used [ability](Cold Snap) on {}; frost takes hold.",
                  party_ctx.log().name_tag_for(entt::handle{reg, source}),
                  party_ctx.log().name_tag_for(entt::handle{reg, target})));

  party_ctx.party_data().encounter_data().combatant_bus(target).emit(
      fl::events::CombatantEvent{fl::events::FreezeStarted{target}});

  const auto from = fl::lospec500::color_at(kFreezeFadeStartBlue);
  const auto to = fl::lospec500::color_at(kFreezeBlue);
  for (int beat = 1; beat <= kFreezeFadeInBeats; ++beat) {
    const auto t = static_cast<float>(beat) /
                   static_cast<float>(std::max(1, kFreezeFadeInBeats));
    const auto color = beat == kFreezeFadeInBeats
                           ? to
                           : ftxui::Color::Interpolate(t, from, to);
    lifetime.schedule_in_beats(
        beat, "freeze: background fade in", [&party_ctx, target, color] {
          auto &reg = party_ctx.reg();
          if (!reg.valid(target) ||
              !reg.any_of<fl::ecs::components::Freeze>(target)) {
            return;
          }
          set_background(reg, target, color);
        });
  }

  schedule_clear(party_ctx, scheduler, target, clear_after_beats);
}

void FreezeSystem::shatter(fl::context::PartyCtx &party_ctx,
                           entt::entity source, entt::entity target) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target)) {
    return;
  }

  auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
  if (target_stats == nullptr || !target_stats->is_alive()) {
    return;
  }

  const auto damage = std::max(1, target_stats->max_hp_ / 2);
  const auto target_name = target_stats->name_;
  FreezeSystem::clear(party_ctx, target);

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, source, target);
  attack_ctx.damage().ice = damage;
  party_ctx.log().append_markup(fmt::format(
      "{} used [ability](Shatter) on [player_name]({}) for [error]({}) "
      "damage.",
      party_ctx.log().name_tag_for(entt::handle{reg, source}), target_name,
      damage));
  fl::ecs::systems::TakeDamage::commit(attack_ctx);
}

void FreezeSystem::schedule_clear(fl::context::PartyCtx &party_ctx,
                                  Scheduler &scheduler, entt::entity target,
                                  int clear_after_beats) {
  auto *scheduled_freeze =
      party_ctx.reg().try_get<fl::ecs::components::Freeze>(target);
  if (scheduled_freeze == nullptr) {
    return;
  }

  StatusEffectLifetime lifetime{party_ctx, scheduler, scheduled_freeze->effect};
  lifetime.schedule_in_beats(
      clear_after_beats, "freeze: auto clear",
      [&party_ctx, target, clear_after_beats] {
        auto &reg = party_ctx.reg();
        auto *freeze = reg.try_get<fl::ecs::components::Freeze>(target);
        if (freeze == nullptr ||
            freeze->clear_after_beats != clear_after_beats) {
          return;
        }

        FreezeSystem::clear(party_ctx, target);
      });
}

void FreezeSystem::clear(fl::context::PartyCtx &party_ctx,
                         entt::entity target) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(target)) {
    return;
  }

  auto *freeze = reg.try_get<fl::ecs::components::Freeze>(target);
  if (freeze == nullptr) {
    return;
  }

  auto &scheduler =
      party_ctx.party_data().encounter_data().atb_engine().scheduler();
  StatusEffectLifetime lifetime{party_ctx, scheduler, freeze->effect};
  lifetime.clear_scheduled();
  lifetime.destroy_instance_entity();

  clear_background(reg, target);
  reg.remove<fl::ecs::components::Freeze>(target);
  party_ctx.party_data().encounter_data().combatant_bus(target).emit(
      fl::events::CombatantEvent{fl::events::FreezeEnded{target}});
}

} // namespace fl::ecs::systems
