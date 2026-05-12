// tests/monsters/test_field_mouse.cpp
#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"

TEST_CASE("Field mouse monster archetype has expected stats", "[monsters]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  auto context = party_ctx.build_context();
  const auto e = fl::primitives::EntityBuilder(context)
                     .monster(fl::monster::MonsterKind::FieldMouse)
                     .build();

  const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(e);
  REQUIRE(stats.name_ == "Field Mouse");
  REQUIRE(stats.hp_ == 5);
  REQUIRE(stats.max_hp_ == 5);
}

TEST_CASE("Honey badger monster archetype has expected stats", "[monsters]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  auto context = party_ctx.build_context();
  const auto e = fl::primitives::EntityBuilder(context)
                     .monster(fl::monster::MonsterKind::HoneyBadger)
                     .build();

  const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(e);
  REQUIRE(stats.name_ == "Honey Badger");
  REQUIRE(stats.hp_ == 500);
  REQUIRE(stats.max_hp_ == 500);
}

TEST_CASE("Woodland skill monsters have their own names and levels",
          "[monsters][skills]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto context = party_ctx.build_context();

  const auto bump = fl::primitives::EntityBuilder(context)
                        .monster(fl::monster::MonsterKind::BumpkinHare)
                        .build();
  const auto squish = fl::primitives::EntityBuilder(context)
                          .monster(fl::monster::MonsterKind::MireSquish)
                          .build();
  const auto smack = fl::primitives::EntityBuilder(context)
                         .monster(fl::monster::MonsterKind::BarkSmack)
                         .build();

  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(bump).name_ ==
          "Bumpkin Hare");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(bump).level_ == 2);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(squish).name_ ==
          "Mire Squish");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(squish).level_ ==
          3);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(smack).name_ ==
          "Bark Smack");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(smack).level_ == 4);
}

TEST_CASE("Monster builder assigns each monster its known skill",
          "[monsters][skills]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto context = party_ctx.build_context();

  auto expect_known_skill = [&](fl::monster::MonsterKind kind,
                                fl::skills::SkillId skill) {
    const auto entity =
        fl::primitives::EntityBuilder(context).monster(kind).build();
    const auto &skills =
        party_ctx.reg().get<fl::ecs::components::SkillSlots>(entity);
    REQUIRE(skills.knows(skill));
  };

  expect_known_skill(fl::monster::MonsterKind::FieldMouse,
                     fl::skills::SkillId::Thump);
  expect_known_skill(fl::monster::MonsterKind::HoneyBadger,
                     fl::skills::SkillId::Eviscerate);
  expect_known_skill(fl::monster::MonsterKind::BumpkinHare,
                     fl::skills::SkillId::Bump);
  expect_known_skill(fl::monster::MonsterKind::MireSquish,
                     fl::skills::SkillId::Squish);
  expect_known_skill(fl::monster::MonsterKind::BarkSmack,
                     fl::skills::SkillId::Smack);
}
