#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/skills/skill_sequence.hpp"

namespace {
using fl::ecs::components::CombatStatusKind;
using fl::ecs::systems::CombatStatusSystem;
using fl::monster::MonsterKind;
using fl::skills::SkillId;

struct Combat {
  fl::GrandCentral game{1, 1, 2};
  fl::context::PartyCtx ctx{game.account_context(0).party_context(0)};
  fl::primitives::EncounterData &encounter{ctx.party_data().create_encounter()};
  Combat() {
    for (auto &member : ctx.party_data().members()) {
      encounter.defenders().members().push_back(member.member_id());
      encounter.add_party_combatant_bus(member.member_id());
      auto &stats =
          ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
      stats.hp_ = stats.max_hp_ = 100;
      stats.resistances_ = {};
    }
  }
  entt::entity monster(MonsterKind kind) {
    auto build = ctx.build_context();
    auto entity = fl::primitives::EntityBuilder{build}.monster(kind).build();
    fl::primitives::EncounterBuilder{ctx}.add_to_enemy_team(entity);
    return entity;
  }
  auto &scheduler() { return encounter.atb_engine().scheduler(); }
  void advance(int beats) {
    for (int i = 0; i < beats; ++i)
      scheduler().advance(seerin::UWU_PER_BEAT);
  }
};
} // namespace

TEST_CASE("Declared buff rules cover friendlies then choose other skills",
          "[monster-rules][combat]") {
  Combat combat;
  const auto actor = combat.monster(MonsterKind::GlassLizard);
  const auto ally = combat.monster(MonsterKind::FieldMouse);
  int finished = 0;
  fl::skills::SkillSequencer sequencer{combat.ctx, combat.scheduler(),
                                       [&](auto) { ++finished; }};
  for (const auto expected : {actor, ally}) {
    const auto action = combat.encounter.choose_action(actor);
    REQUIRE(action);
    REQUIRE(action->skill.base == SkillId::CinderVeil);
    REQUIRE(action->target == expected);
    sequencer.schedule(actor, action->target, action->skill);
    combat.advance(2);
    REQUIRE(CombatStatusSystem::has_status(combat.ctx.reg(), expected,
                                           CombatStatusKind::Shield));
  }
  REQUIRE(finished == 2);
  REQUIRE(combat.encounter.choose_action(actor)->skill.base !=
          SkillId::CinderVeil);
  REQUIRE(CombatStatusSystem::clear_status(combat.ctx, ally,
                                           CombatStatusKind::Shield));
  REQUIRE(combat.encounter.choose_action(actor)->target == ally);
}

TEST_CASE("Salamander combo requires and consumes the selected enemy's burn",
          "[monster-rules][combat]") {
  Combat combat;
  const auto actor = combat.monster(MonsterKind::Salamander);
  const auto clean = combat.encounter.defenders().members()[0];
  const auto target = combat.encounter.defenders().members()[1];
  REQUIRE(combat.encounter.choose_action(actor)->skill.base ==
          SkillId::KindleWound);
  REQUIRE(CombatStatusSystem::apply_status(combat.ctx, combat.scheduler(),
                                           {.kind = CombatStatusKind::Burn,
                                            .source = actor,
                                            .target = target,
                                            .duration_seconds = 18,
                                            .tick_damage = 3,
                                            .tick_count = 3}));
  const auto action = combat.encounter.choose_action(actor);
  REQUIRE(action->skill.base == SkillId::Cinderburst);
  REQUIRE(action->target == target);
  int finished = 0;
  fl::skills::SkillSequencer sequencer{combat.ctx, combat.scheduler(),
                                       [&](auto) { ++finished; }};
  sequencer.schedule(actor, action->target, action->skill);
  SECTION("burn is consumed and its later ticks cannot fire") {
    combat.advance(2);
    REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(target).hp_ == 92);
    REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(clean).hp_ == 100);
    REQUIRE_FALSE(CombatStatusSystem::has_status(combat.ctx.reg(), target,
                                                 CombatStatusKind::Burn));
    combat.advance(fl::primitives::WorldClock::beats_from_seconds(40));
    REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(target).hp_ == 92);
    REQUIRE(combat.encounter.choose_action(actor)->skill.base ==
            SkillId::KindleWound);
  }
  SECTION("a cleansed prerequisite prevents the queued explosion") {
    REQUIRE(CombatStatusSystem::clear_status(combat.ctx, target,
                                             CombatStatusKind::Burn));
    combat.advance(2);
    REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(target).hp_ ==
            100);
  }
  SECTION("a dead actor cannot detonate") {
    combat.ctx.reg().get<fl::ecs::components::Stats>(actor).hp_ = 0;
    combat.advance(2);
    REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(target).hp_ ==
            100);
  }
  REQUIRE(finished == 1);
}

TEST_CASE("Invalid detonation finishes without witnessing or dealing damage",
          "[monster-rules][combat]") {
  Combat combat;
  const auto actor = combat.monster(MonsterKind::Salamander);
  const auto target = combat.encounter.defenders().members()[0];
  int finished = 0;
  fl::skills::SkillSequencer sequencer{combat.ctx, combat.scheduler(),
                                       [&](auto) { ++finished; }};
  sequencer.schedule(actor, target, SkillId::Cinderburst);
  REQUIRE(finished == 1);
  REQUIRE_FALSE(combat.game.discoveries().knows(SkillId::Cinderburst));
  REQUIRE(combat.ctx.reg().get<fl::ecs::components::Stats>(target).hp_ == 100);
}

TEST_CASE("Self skills keep the caster as their target",
          "[monster-rules][combat]") {
  Combat combat;
  const auto actor = combat.monster(MonsterKind::DrillBeetle);
  combat.monster(MonsterKind::FieldMouse);
  REQUIRE(combat.encounter.target_for_skill(actor, SkillId::ArmorPlate) ==
          actor);
}
