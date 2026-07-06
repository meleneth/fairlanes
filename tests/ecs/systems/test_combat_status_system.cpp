#include <catch2/catch_test_macros.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/field_debuff.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/poison_system.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill.hpp"
#include "fl/skills/skill_selection.hpp"
#include "fl/skills/skill_sequence.hpp"
#include "sr/atb_fsm.hpp"

namespace {

using fl::ecs::components::CombatStatusKind;
using fl::ecs::components::FieldDebuffKind;
using fl::ecs::components::FieldTeam;
using fl::ecs::systems::CombatStatusSystem;

void tick_party(fl::context::PartyCtx &party_ctx, int beats) {
  for (int i = 0; i < beats; ++i) {
    party_ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyTick{}});
  }
}

struct BuiltEncounter {
  fl::GrandCentral gc{1, 1, 2};
  fl::context::AccountCtx account_ctx{gc.account_context(0)};
  fl::context::PartyCtx party_ctx{account_ctx.party_context(0)};
  fl::primitives::EncounterBuilder builder{party_ctx};
  fl::primitives::EncounterData &encounter{builder.thump_it_out()};

  entt::entity attacker() const { return encounter.attackers().members().front(); }
  entt::entity defender() const { return encounter.defenders().members().front(); }

  CombatStatusSystem::Scheduler &scheduler() {
    return encounter.atb_engine().scheduler();
  }
};

void set_hp(fl::context::PartyCtx &party_ctx, entt::entity entity, int hp,
            int max_hp) {
  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(entity);
  stats.hp_ = hp;
  stats.max_hp_ = max_hp;
  stats.resistances_ = {};
}

int hp(fl::context::PartyCtx &party_ctx, entt::entity entity) {
  return party_ctx.reg().get<fl::ecs::components::Stats>(entity).hp_;
}

int deal_physical(fl::context::PartyCtx &party_ctx, entt::entity attacker,
                  entt::entity defender, int damage) {
  auto attack_ctx = fl::context::AttackCtx::make_attack(party_ctx, attacker,
                                                        defender);
  attack_ctx.damage().physical = damage;
  return fl::ecs::systems::TakeDamage::commit(attack_ctx);
}

} // namespace

TEST_CASE("Shield reduces incoming damage through the shared damage path",
          "[combat-status][shield]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Shield,
                                   .name = "Test Shield",
                                   .source = h.defender(),
                                   .target = h.defender(),
                                   .value = 50,
                                   .negative = false}));

  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 10);
  REQUIRE(hp(h.party_ctx, h.defender()) == 90);
}

TEST_CASE("Blind can force attacks to miss through shared accuracy checks",
          "[combat-status][blind]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Blind,
                                   .name = "Test Blind",
                                   .source = h.defender(),
                                   .target = h.attacker(),
                                   .value = 100}));

  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 0);
  REQUIRE(hp(h.party_ctx, h.defender()) == 100);
}

TEST_CASE("Silence blocks spell-tagged skills but leaves other skills usable",
          "[combat-status][silence]") {
  BuiltEncounter h;
  const auto actor = h.attacker();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Silence,
                                   .name = "Test Silence",
                                   .source = h.defender(),
                                   .target = actor}));

  REQUIRE_FALSE(CombatStatusSystem::can_use_skill(
      h.party_ctx.reg(), actor,
      fl::skills::SkillKey{fl::skills::SkillId::FlameStrike}));
  REQUIRE(CombatStatusSystem::can_use_skill(
      h.party_ctx.reg(), actor,
      fl::skills::SkillKey{fl::skills::SkillId::Thump}));

  h.party_ctx.reg().emplace_or_replace<fl::ecs::components::SkillSlots>(
      actor, fl::ecs::components::SkillSlots::with_known(
                 fl::skills::SkillKey{fl::skills::SkillId::FlameStrike}));
  REQUIRE(fl::skills::choose_skill(h.party_ctx.reg(), h.party_ctx.rng(), actor) ==
          fl::skills::SkillKey{fl::skills::SkillId::Observe});
}

TEST_CASE("Slow and Haste expose shared turn tempo modifiers",
          "[combat-status][tempo]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 20}));
  REQUIRE(CombatStatusSystem::turn_tempo_modifier_percent(h.party_ctx.reg(),
                                                          actor) == -20);

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Test Haste",
                                   .source = actor,
                                   .target = actor,
                                   .value = 35,
                                   .negative = false}));
  REQUIRE(CombatStatusSystem::turn_tempo_modifier_percent(h.party_ctx.reg(),
                                                          actor) == 15);
}

TEST_CASE("Stun consumes one action through shared turn resolution",
          "[combat-status][stun]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Stun,
                                   .name = "Test Stun",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .stacks = 1}));

  REQUIRE(CombatStatusSystem::consume_stun_turn(h.party_ctx, actor));
  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), actor,
                                               CombatStatusKind::Stun));
  REQUIRE_FALSE(CombatStatusSystem::consume_stun_turn(h.party_ctx, actor));
}

TEST_CASE("Burn deals scheduled damage over time and then clears",
          "[combat-status][burn]") {
  BuiltEncounter h;
  const auto target = h.defender();
  set_hp(h.party_ctx, target, 100, 100);

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Burn,
                                   .name = "Test Burn",
                                   .source = h.attacker(),
                                   .target = target,
                                   .tick_damage = 3,
                                   .tick_count = 2}));

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(hp(h.party_ctx, target) == 97);
  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), target,
                                         CombatStatusKind::Burn));

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(hp(h.party_ctx, target) == 94);
  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), target,
                                               CombatStatusKind::Burn));
}

TEST_CASE("Drain damages and heals through shared damage and heal paths",
          "[combat-status][drain]") {
  BuiltEncounter h;
  const auto source = h.attacker();
  const auto target = h.defender();
  set_hp(h.party_ctx, source, 5, 20);
  set_hp(h.party_ctx, target, 100, 100);

  auto damage = fl::primitives::Damage{};
  damage.physical = 12;

  REQUIRE(CombatStatusSystem::drain(h.party_ctx, source, target, damage, 50,
                                    "Test Drain") == 6);
  REQUIRE(hp(h.party_ctx, target) == 88);
  REQUIRE(hp(h.party_ctx, source) == 11);
}

TEST_CASE("Cleanse removes removable negative combat statuses and old poison",
          "[combat-status][cleanse]") {
  BuiltEncounter h;
  const auto target = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Blind,
                                   .name = "Test Blind",
                                   .source = h.attacker(),
                                   .target = target,
                                   .value = 20}));
  fl::ecs::systems::PoisonSystem::apply(h.party_ctx, h.scheduler(), h.attacker(),
                                        target, 2, 9);

  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), target,
                                         CombatStatusKind::Blind));
  REQUIRE(h.party_ctx.reg().all_of<fl::ecs::components::Poison>(target));

  REQUIRE(CombatStatusSystem::cleanse(h.party_ctx, target, target) == 2);
  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), target,
                                               CombatStatusKind::Blind));
  REQUIRE_FALSE(h.party_ctx.reg().any_of<fl::ecs::components::Poison>(target));
}

TEST_CASE("Field debuffs are encounter-scoped and feed shared damage modifiers",
          "[combat-status][field]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, {.team = FieldTeam::Attackers,
                    .kind = FieldDebuffKind::DamageDown,
                    .name = "Test Field Drag",
                    .source = h.defender(),
                    .value = 50});

  REQUIRE(CombatStatusSystem::has_field_debuff(h.party_ctx, FieldTeam::Attackers,
                                               FieldDebuffKind::DamageDown));
  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 10);
  REQUIRE(hp(h.party_ctx, h.defender()) == 90);
}

TEST_CASE("Field AccuracyDown expires and stops forcing misses",
          "[combat-status][field][expiry]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Attackers,
                                   .kind = FieldDebuffKind::AccuracyDown,
                                   .name = "Brief Fog",
                                   .source = h.defender(),
                                   .value = 100,
                                   .duration_seconds = 1});

  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 0);
  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(CombatStatusSystem::has_field_debuff(
      h.party_ctx, FieldTeam::Attackers, FieldDebuffKind::AccuracyDown));
  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 20);
}

TEST_CASE("Field DamageDown expires and stops reducing outgoing damage",
          "[combat-status][field][expiry]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Attackers,
                                   .kind = FieldDebuffKind::DamageDown,
                                   .name = "Brief Drag",
                                   .source = h.defender(),
                                   .value = 50,
                                   .duration_seconds = 1});

  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 10);
  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(CombatStatusSystem::has_field_debuff(
      h.party_ctx, FieldTeam::Attackers, FieldDebuffKind::DamageDown));
  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 20);
}

TEST_CASE("Field Vulnerable expires and stops increasing incoming damage",
          "[combat-status][field][expiry]") {
  BuiltEncounter h;
  set_hp(h.party_ctx, h.defender(), 100, 100);

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Defenders,
                                   .kind = FieldDebuffKind::Vulnerable,
                                   .name = "Brief Opening",
                                   .source = h.attacker(),
                                   .value = 50,
                                   .duration_seconds = 1});

  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 30);
  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(CombatStatusSystem::has_field_debuff(
      h.party_ctx, FieldTeam::Defenders, FieldDebuffKind::Vulnerable));
  REQUIRE(deal_physical(h.party_ctx, h.attacker(), h.defender(), 20) == 20);
}

TEST_CASE("Multiple field debuffs coexist and expire independently",
          "[combat-status][field][expiry]") {
  BuiltEncounter h;

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Attackers,
                                   .kind = FieldDebuffKind::AccuracyDown,
                                   .name = "Short Fog",
                                   .source = h.defender(),
                                   .value = 20,
                                   .duration_seconds = 1});
  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Attackers,
                                   .kind = FieldDebuffKind::DamageDown,
                                   .name = "Long Drag",
                                   .source = h.defender(),
                                   .value = 30,
                                   .duration_seconds = 2});

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(CombatStatusSystem::has_field_debuff(
      h.party_ctx, FieldTeam::Attackers, FieldDebuffKind::AccuracyDown));
  REQUIRE(CombatStatusSystem::field_debuff_value(
              h.party_ctx, FieldTeam::Attackers,
              FieldDebuffKind::DamageDown) == 30);

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(CombatStatusSystem::has_field_debuff(
      h.party_ctx, FieldTeam::Attackers, FieldDebuffKind::DamageDown));
}

TEST_CASE("Permanent field debuffs remain when no finite duration is set",
          "[combat-status][field][expiry]") {
  BuiltEncounter h;

  CombatStatusSystem::apply_field_debuff(
      h.party_ctx, h.scheduler(), {.team = FieldTeam::Attackers,
                                   .kind = FieldDebuffKind::DamageDown,
                                   .name = "Standing Drag",
                                   .source = h.defender(),
                                   .value = 25,
                                   .duration_seconds = 0});

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(5));
  REQUIRE(CombatStatusSystem::field_debuff_value(
              h.party_ctx, FieldTeam::Attackers,
              FieldDebuffKind::DamageDown) == 25);
}

namespace {

void add_actor_to_charge_model(fl::context::PartyCtx &party_ctx,
                               entt::entity actor) {
  auto &charge =
      party_ctx.reg().get_or_emplace<fl::ecs::components::AtbCharge>(actor);
  const int rate_percent =
      100 +
      CombatStatusSystem::turn_tempo_modifier_percent(party_ctx.reg(), actor);
  charge.charge_per_beat = std::max<int64_t>(
      1, (seerin::AtbMachine::kChargePerBeat *
          static_cast<int64_t>(rate_percent)) /
             100);
}

int beats_until_active(fl::context::PartyCtx &party_ctx, entt::entity actor) {
  const auto &charge =
      party_ctx.reg().get<fl::ecs::components::AtbCharge>(actor);
  return static_cast<int>((charge.max_charge + charge.charge_per_beat - 1) /
                          charge.charge_per_beat);
}

} // namespace

TEST_CASE("Slow reduces ATB charge rate through shared tempo modifiers",
          "[combat-status][tempo][atb]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25}));

  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE(beats_until_active(h.party_ctx, actor) == 80);
  REQUIRE(h.party_ctx.reg()
              .get<fl::ecs::components::AtbCharge>(actor)
              .charge_per_beat == 60);
}

TEST_CASE("Haste increases ATB charge rate through shared tempo modifiers",
          "[combat-status][tempo][atb]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Test Haste",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25,
                                   .negative = false}));

  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE(beats_until_active(h.party_ctx, actor) == 48);
  REQUIRE(h.party_ctx.reg()
              .get<fl::ecs::components::AtbCharge>(actor)
              .charge_per_beat == 100);
}

TEST_CASE("Applied Slow affects ATB readiness timing",
          "[combat-status][tempo][atb]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Applied Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25}));
  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), actor,
                                         CombatStatusKind::Slow));
  REQUIRE(beats_until_active(h.party_ctx, actor) == 80);
}

TEST_CASE("Applied Haste affects ATB readiness timing",
          "[combat-status][tempo][atb]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Applied Haste",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25,
                                   .negative = false}));
  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), actor,
                                         CombatStatusKind::Haste));
  REQUIRE(beats_until_active(h.party_ctx, actor) == 48);
}

TEST_CASE("Cleanse removes Slow timing impact",
          "[combat-status][tempo][atb][cleanse]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25}));

  REQUIRE(CombatStatusSystem::cleanse(h.party_ctx, h.attacker(), actor) == 1);
  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), actor,
                                               CombatStatusKind::Slow));
  REQUIRE(beats_until_active(h.party_ctx, actor) == 60);
}

TEST_CASE("Expired Slow stops affecting future ATB charge timing",
          "[combat-status][tempo][atb][expiry]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Brief Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .duration_seconds = 1,
                                   .value = 25}));

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), actor,
                                               CombatStatusKind::Slow));
  REQUIRE(beats_until_active(h.party_ctx, actor) == 60);
}

TEST_CASE("Slow and Haste combine deterministically by net tempo modifier",
          "[combat-status][tempo][atb]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25}));
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Test Haste",
                                   .source = h.attacker(),
                                   .target = actor,
                                   .value = 25,
                                   .negative = false}));

  add_actor_to_charge_model(h.party_ctx, actor);

  REQUIRE(CombatStatusSystem::turn_tempo_modifier_percent(h.party_ctx.reg(),
                                                          actor) == 0);
  REQUIRE(beats_until_active(h.party_ctx, actor) == 60);
}
