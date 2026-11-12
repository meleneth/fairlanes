#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/change_combat_affinity.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_scaling.hpp"

TEST_CASE("Origin content has explicit affinity and scaling policies", "[affinity][content]") {
  for (const auto* skill : fl::skills::all_definitions()) {
    CAPTURE(skill->display_name);
    REQUIRE(skill->scaling.base_effect >= 0);
    REQUIRE(skill->scaling.denominator > 0);
  }
  const auto mixed =
      fl::monster::generated_content::stats(fl::monster::MonsterKind::WeblingSpider).affinities;
  REQUIRE(mixed.size() == 3);
  REQUIRE(mixed.contains(fl::primitives::CombatAffinity::Cloth));
  REQUIRE(mixed.contains(fl::primitives::CombatAffinity::Leather));
  REQUIRE(mixed.contains(fl::primitives::CombatAffinity::Plate));
}

TEST_CASE("Mixed monster affinities apply one matching contribution", "[affinity][scaling]") {
  entt::registry reg;
  const auto actor = reg.create();
  auto& actor_stats = reg.emplace<fl::ecs::components::Stats>(actor, "Mixed");
  actor_stats.hp_ = actor_stats.max_hp_ = 100;
  actor_stats.attack_ = 10;
  reg.emplace<fl::ecs::components::CombatAffinities>(
      actor, fl::ecs::components::CombatAffinities{fl::primitives::CombatAffinitySet{
                 fl::primitives::CombatAffinity::Leather, fl::primitives::CombatAffinity::Plate}});
  const fl::skills::SkillScalingRule rule{fl::primitives::CombatAffinity::Plate,
                                          fl::skills::ScalingStat::Attack,
                                          fl::skills::ScalingDimension::Damage,
                                          10,
                                          1,
                                          2,
                                          30,
                                          20};
  const auto result = fl::skills::evaluate_scaling(reg, actor, rule, 10);
  REQUIRE(result.stat_bonus == 5);
  REQUIRE(result.affinity_bonus == 3);
  REQUIRE(result.total == 18);
}

TEST_CASE("Player affinity changes only at an encounter boundary", "[affinity][town]") {
  fl::GrandCentral game{1, 1, 1};
  auto ctx = game.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  const auto member = party.members().front().member_id();
  REQUIRE(fl::ecs::systems::ChangeCombatAffinity::commit(ctx, member,
                                                         fl::primitives::CombatAffinity::Leather) ==
          fl::ecs::systems::AffinityChangeResult::Changed);
  REQUIRE(ctx.reg().get<fl::ecs::components::ActiveCombatAffinity>(member).value ==
          fl::primitives::CombatAffinity::Leather);
  REQUIRE_FALSE(ctx.reg().all_of<fl::ecs::components::CombatAffinities>(member));

  party.select_tower("origin_woodland_tower");
  party.begin_tower_encounter();
  fl::primitives::EncounterBuilder{
      ctx, fl::primitives::EncounterMode::ChaosAttractor,
      fl::primitives::EncounterBuilder::TowerFloor{"origin_woodland_tower", 1}}
      .thump_it_out();
  REQUIRE(fl::ecs::systems::ChangeCombatAffinity::commit(ctx, member,
                                                         fl::primitives::CombatAffinity::Plate) ==
          fl::ecs::systems::AffinityChangeResult::NotInTown);
  REQUIRE(ctx.reg().get<fl::ecs::components::ActiveCombatAffinity>(member).value ==
          fl::primitives::CombatAffinity::Leather);
}

TEST_CASE("Tower progression stays near floors while preserving identity HP", "[tower][affinity]") {
  const fl::primitives::TowerBalance balance{};
  const auto fragile = balance.resolve(50, "origin_woodland_tower", 2, 7, 50);
  const auto durable = balance.resolve(50, "origin_woodland_tower", 6, 120, 50);
  REQUIRE(fragile.level >= 45);
  REQUIRE(fragile.level <= 55);
  REQUIRE(durable.level >= 45);
  REQUIRE(durable.level <= 55);
  REQUIRE(durable.level != fragile.level);
  REQUIRE(durable.max_hp > fragile.max_hp);
  REQUIRE(durable.attack >= fragile.attack);
}
