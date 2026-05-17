#include "fl/ecs/systems/freeze_system.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
constexpr int kFreezeDurationSeconds = 5;
constexpr int kFreezeDurationBeats =
    fl::primitives::WorldClock::beats_from_seconds(kFreezeDurationSeconds);
constexpr std::size_t kFreezeBlue = 16;

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

fl::events::ScopedPartyListener
FreezeSystem::bind_apply_listener(fl::context::PartyCtx &party_ctx,
                                  Scheduler &scheduler,
                                  ClearPendingFn clear_pending) {
  return fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::FreezeApplied>,
      [&party_ctx, &scheduler, clear_pending = std::move(clear_pending)](
          const fl::events::FreezeApplied &ev) {
        FreezeSystem::apply(party_ctx, scheduler, ev.source, ev.target,
                            ev.duration_seconds, clear_pending);
      }};
}

void FreezeSystem::apply(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                         entt::entity source, entt::entity target,
                         int duration_seconds, ClearPendingFn clear_pending) {
  auto &reg = party_ctx.reg();
  if (!reg.valid(source) || !reg.valid(target)) {
    return;
  }

  (void)duration_seconds;

  auto *target_stats = reg.try_get<fl::ecs::components::Stats>(target);
  if (target_stats == nullptr || !target_stats->is_alive()) {
    return;
  }

  if (reg.any_of<fl::ecs::components::Freeze>(target)) {
    FreezeSystem::shatter(party_ctx, source, target, clear_pending);
    return;
  }

  auto &freeze = reg.emplace_or_replace<fl::ecs::components::Freeze>(target);
  freeze.source = source;
  freeze.clear_after_beats = kFreezeDurationBeats;

  set_background(reg, target, fl::lospec500::color_at(kFreezeBlue));
  party_ctx.log().append_markup(fmt::format(
      "[ability](Cold Snap) freezes [player_name]({}).", target_stats->name_));

  freeze.player_died_sub = fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::PlayerDied>,
      [&party_ctx, target, clear_pending](const fl::events::PlayerDied &ev) {
        if (ev.player == target) {
          FreezeSystem::clear(party_ctx, target, clear_pending);
        }
      }};

  freeze.left_combat_sub = fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::PartyLeftCombat>,
      [&party_ctx, target, clear_pending](const fl::events::PartyLeftCombat &) {
        FreezeSystem::clear(party_ctx, target, clear_pending);
      }};

  party_ctx.bus().emit(
      fl::events::PartyEvent{fl::events::FreezeStarted{target}});
  schedule_clear(party_ctx, scheduler, target, kFreezeDurationBeats);
}

void FreezeSystem::shatter(fl::context::PartyCtx &party_ctx,
                           entt::entity source, entt::entity target,
                           const ClearPendingFn &clear_pending) {
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
  FreezeSystem::clear(party_ctx, target, clear_pending);

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, source, target);
  attack_ctx.damage().ice = damage;
  party_ctx.log().append_markup(fmt::format(
      "[ability](Shatter) breaks [player_name]({}) for [error]({}) damage.",
      target_name, damage));
  fl::ecs::systems::TakeDamage::commit(attack_ctx);
}

void FreezeSystem::schedule_clear(fl::context::PartyCtx &party_ctx,
                                  Scheduler &scheduler, entt::entity target,
                                  int clear_after_beats) {
  scheduler.schedule_smelly_in_beats_for(
      clear_after_beats, target, "freeze: auto clear",
      [&party_ctx, target, clear_after_beats] {
        auto &reg = party_ctx.reg();
        auto *freeze = reg.try_get<fl::ecs::components::Freeze>(target);
        if (freeze == nullptr ||
            freeze->clear_after_beats != clear_after_beats) {
          return;
        }

        FreezeSystem::clear(party_ctx, target, [](entt::entity) {});
      });
}

void FreezeSystem::clear(fl::context::PartyCtx &party_ctx, entt::entity target,
                         const ClearPendingFn &clear_pending) {
  auto &reg = party_ctx.reg();
  clear_pending(target);
  clear_background(reg, target);

  const bool had_freeze =
      reg.valid(target) && reg.any_of<fl::ecs::components::Freeze>(target);
  if (had_freeze) {
    reg.remove<fl::ecs::components::Freeze>(target);
    party_ctx.bus().emit(
        fl::events::PartyEvent{fl::events::FreezeEnded{target}});
  }
}

} // namespace fl::ecs::systems
