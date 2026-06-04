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

struct TempoHarness {
  fl::GrandCentral gc{1, 1, 2};
  fl::context::AccountCtx account_ctx{gc.account_context(0)};
  fl::context::PartyCtx party_ctx{account_ctx.party_context(0)};
  fl::primitives::EncounterData &encounter{party_ctx.party_data().create_encounter()};
  entt::entity actor{party_ctx.party_data().members().front().member_id()};
  entt::entity caster{party_ctx.party_data().members().back().member_id()};

  TempoHarness() {
    encounter.defenders().members().push_back(actor);
    encounter.defenders().members().push_back(caster);
    encounter.add_party_combatant_bus(actor);
    encounter.add_party_combatant_bus(caster);
  }

  CombatStatusSystem::Scheduler &scheduler() {
    return encounter.atb_engine().scheduler();
  }

  void add_actor_to_atb() {
    encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{actor}});
  }

  int beats_until_active(int limit = 200) {
    for (int beat = 1; beat <= limit; ++beat) {
      encounter.atb_in().emit(seerin::AtbInEvent{seerin::Beat{}});
      if (encounter.atb_engine().active_combatant() == actor) {
        return beat;
      }
    }
    return -1;
  }

};

} // namespace

TEST_CASE("Slow reduces ATB charge rate through shared tempo modifiers",
          "[combat-status][tempo][atb]") {
  TempoHarness h;
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25}));

  h.add_actor_to_atb();

  REQUIRE(h.beats_until_active() == 80);
  REQUIRE(h.party_ctx.reg()
              .get<fl::ecs::components::AtbCharge>(h.actor)
              .charge_per_beat == 60);
}

TEST_CASE("Haste increases ATB charge rate through shared tempo modifiers",
          "[combat-status][tempo][atb]") {
  TempoHarness h;
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Test Haste",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25,
                                   .negative = false}));

  h.add_actor_to_atb();

  REQUIRE(h.beats_until_active() == 48);
  REQUIRE(h.party_ctx.reg()
              .get<fl::ecs::components::AtbCharge>(h.actor)
              .charge_per_beat == 100);
}

TEST_CASE("Applied Slow affects ATB readiness timing",
          "[combat-status][tempo][atb]") {
  TempoHarness h;

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Applied Slow",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25}));
  h.add_actor_to_atb();

  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), h.actor,
                                         CombatStatusKind::Slow));
  REQUIRE(h.beats_until_active() == 80);
}

TEST_CASE("Applied Haste affects ATB readiness timing",
          "[combat-status][tempo][atb]") {
  TempoHarness h;

  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Applied Haste",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25,
                                   .negative = false}));
  h.add_actor_to_atb();

  REQUIRE(CombatStatusSystem::has_status(h.party_ctx.reg(), h.actor,
                                         CombatStatusKind::Haste));
  REQUIRE(h.beats_until_active() == 48);
}

TEST_CASE("Cleanse removes Slow timing impact",
          "[combat-status][tempo][atb][cleanse]") {
  TempoHarness h;
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25}));

  REQUIRE(CombatStatusSystem::cleanse(h.party_ctx, h.caster, h.actor) == 1);
  h.add_actor_to_atb();

  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), h.actor,
                                               CombatStatusKind::Slow));
  REQUIRE(h.beats_until_active() == 60);
}

TEST_CASE("Expired Slow stops affecting future ATB charge timing",
          "[combat-status][tempo][atb][expiry]") {
  TempoHarness h;
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Brief Slow",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .duration_seconds = 1,
                                   .value = 25}));

  tick_party(h.party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  h.add_actor_to_atb();

  REQUIRE_FALSE(CombatStatusSystem::has_status(h.party_ctx.reg(), h.actor,
                                               CombatStatusKind::Slow));
  REQUIRE(h.beats_until_active() == 60);
}

TEST_CASE("Slow and Haste combine deterministically by net tempo modifier",
          "[combat-status][tempo][atb]") {
  TempoHarness h;
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Slow,
                                   .name = "Test Slow",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25}));
  REQUIRE(CombatStatusSystem::apply_status(
      h.party_ctx, h.scheduler(), {.kind = CombatStatusKind::Haste,
                                   .name = "Test Haste",
                                   .source = h.caster,
                                   .target = h.actor,
                                   .value = 25,
                                   .negative = false}));

  h.add_actor_to_atb();

  REQUIRE(CombatStatusSystem::turn_tempo_modifier_percent(h.party_ctx.reg(),
                                                          h.actor) == 0);
  REQUIRE(h.beats_until_active() == 60);
}
