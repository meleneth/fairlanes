#pragma once

#include <entt/entt.hpp>

#include <string_view>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/field_debuff.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/skills/skill.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::context {
struct AttackCtx;
struct PartyCtx;
} // namespace fl::context

namespace fl::ecs::systems {

class CombatStatusSystem {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;

  struct ApplyStatusRequest {
    fl::ecs::components::CombatStatusKind kind{
        fl::ecs::components::CombatStatusKind::Blind};
    std::string_view name{"Status"};
    entt::entity source{entt::null};
    entt::entity target{entt::null};
    int duration_seconds{0};
    int value{0};
    int stacks{1};
    int turns{0};
    int tick_damage{0};
    int tick_count{0};
    bool negative{true};
    bool removable{true};
  };

  struct FieldDebuffRequest {
    fl::ecs::components::FieldTeam team{
        fl::ecs::components::FieldTeam::Defenders};
    fl::ecs::components::FieldDebuffKind kind{
        fl::ecs::components::FieldDebuffKind::AccuracyDown};
    std::string_view name{"Field Debuff"};
    entt::entity source{entt::null};
    int value{0};
    int duration_seconds{0};
    bool removable{true};
  };

  static bool apply_status(fl::context::PartyCtx &party_ctx,
                           Scheduler &scheduler,
                           const ApplyStatusRequest &request);
  static bool clear_status(fl::context::PartyCtx &party_ctx,
                           entt::entity target,
                           fl::ecs::components::CombatStatusKind kind);
  static int cleanse(fl::context::PartyCtx &party_ctx, entt::entity source,
                     entt::entity target);

  static bool has_status(entt::registry &reg, entt::entity target,
                         fl::ecs::components::CombatStatusKind kind);
  static int status_value(entt::registry &reg, entt::entity target,
                          fl::ecs::components::CombatStatusKind kind);

  static bool can_use_skill(entt::registry &reg, entt::entity actor,
                            fl::skills::SkillKey skill);
  static bool consume_stun_turn(fl::context::PartyCtx &party_ctx,
                                entt::entity actor);
  static bool attack_misses(fl::context::AttackCtx &ctx);
  static void apply_damage_modifiers(fl::context::AttackCtx &ctx,
                                     fl::primitives::Damage &damage);

  static int heal(fl::context::PartyCtx &party_ctx, entt::entity source,
                  entt::entity target, int amount, std::string_view label);
  static int drain(fl::context::PartyCtx &party_ctx, entt::entity source,
                   entt::entity target, fl::primitives::Damage damage,
                   int heal_percent, std::string_view label);

  static void apply_field_debuff(fl::context::PartyCtx &party_ctx,
                                 const FieldDebuffRequest &request);
  static bool has_field_debuff(fl::context::PartyCtx &party_ctx,
                               fl::ecs::components::FieldTeam team,
                               fl::ecs::components::FieldDebuffKind kind);
  static int field_debuff_value(fl::context::PartyCtx &party_ctx,
                                fl::ecs::components::FieldTeam team,
                                fl::ecs::components::FieldDebuffKind kind);
  static int turn_tempo_modifier_percent(entt::registry &reg,
                                         entt::entity actor);

private:
  static void clear_status_by_id(fl::context::PartyCtx &party_ctx,
                                 entt::entity target, int status_id);
  static void schedule_burn_tick(fl::context::PartyCtx &party_ctx,
                                 Scheduler &scheduler, entt::entity target,
                                 int status_id);
};

} // namespace fl::ecs::systems
