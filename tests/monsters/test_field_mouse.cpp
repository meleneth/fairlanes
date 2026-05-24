// tests/monsters/test_field_mouse.cpp
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>

#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/skills/skill_visuals.hpp"
#include "fl/widgets/effects/decal.hpp"

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
        const auto scaredy_cat = fl::primitives::EntityBuilder(context)
                                                                                                                         .monster(fl::monster::MonsterKind::ScaredyCat)
                                                                                                                         .build();
  const auto smack = fl::primitives::EntityBuilder(context)
                         .monster(fl::monster::MonsterKind::BarkSmack)
                         .build();
  const auto poison = fl::primitives::EntityBuilder(context)
                          .monster(fl::monster::MonsterKind::PoisonToad)
                          .build();
  const auto yeti = fl::primitives::EntityBuilder(context)
                        .monster(fl::monster::MonsterKind::Yeti)
                        .build();
  const auto salamander = fl::primitives::EntityBuilder(context)
                              .monster(fl::monster::MonsterKind::Salamander)
                              .build();
  const auto fire_drake = fl::primitives::EntityBuilder(context)
                              .monster(fl::monster::MonsterKind::FireDrake)
                              .build();

  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(bump).name_ ==
          "Bumpkin Hare");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(bump).level_ == 2);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(squish).name_ ==
          "Mire Squish");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(squish).level_ ==
          3);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(scaredy_cat).name_ ==
          "Scaredy Cat");
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::TrackXP>(scaredy_cat).level_ ==
      2);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(smack).name_ ==
          "Bark Smack");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(smack).level_ == 4);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(poison).name_ ==
          "Poison Toad");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::TrackXP>(poison).level_ ==
          5);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(yeti).name_ ==
          "Yeti");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(salamander).name_ ==
          "Salamander");
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::TrackXP>(salamander).level_ ==
      6);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(fire_drake).name_ ==
          "Fire Drake");
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(fire_drake).hp_ ==
          500);
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::TrackXP>(fire_drake).level_ ==
      8);
}

TEST_CASE("Monster builder assigns each monster its registered skills",
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
        expect_known_skill(fl::monster::MonsterKind::ScaredyCat,
                                                                                 fl::skills::SkillId::Flee);
        expect_known_skill(fl::monster::MonsterKind::ScaredyCat,
                                                                                 fl::skills::SkillId::Thump);
  expect_known_skill(fl::monster::MonsterKind::MireSquish,
                     fl::skills::SkillId::Squish);
  expect_known_skill(fl::monster::MonsterKind::BarkSmack,
                     fl::skills::SkillId::Smack);
  expect_known_skill(fl::monster::MonsterKind::PoisonToad,
                     fl::skills::SkillId::Poison);
  expect_known_skill(fl::monster::MonsterKind::Yeti,
                     fl::skills::SkillId::ColdSnap);
  expect_known_skill(fl::monster::MonsterKind::Salamander,
                     fl::skills::SkillId::FlameStrike);
  expect_known_skill(fl::monster::MonsterKind::FireDrake,
                     fl::skills::SkillId::FlameWave);
}

TEST_CASE("Decal attack monsters construct with expected skills and visuals",
          "[monsters][skills][decal]") {
  struct ExpectedMonster {
    fl::monster::MonsterKind kind;
    fl::skills::SkillId skill;
    fl::widgets::effects::DecalAnimationKind animation;
    std::string_view name;
  };

  constexpr std::array<ExpectedMonster, 7> expected{{
      {fl::monster::MonsterKind::StormtickImp, fl::skills::SkillId::Joltspasm,
       fl::widgets::effects::DecalAnimationKind::Shock, "Stormtick Imp"},
      {fl::monster::MonsterKind::CeilingGrudge, fl::skills::SkillId::RocksFall,
       fl::widgets::effects::DecalAnimationKind::RocksFall, "Ceiling Grudge"},
      {fl::monster::MonsterKind::MiasmaToad, fl::skills::SkillId::SourBreath,
       fl::widgets::effects::DecalAnimationKind::PoisonCloud, "Miasma Toad"},
      {fl::monster::MonsterKind::ChoirWisp, fl::skills::SkillId::Mercyburst,
       fl::widgets::effects::DecalAnimationKind::HolyNova, "Choir Wisp"},
      {fl::monster::MonsterKind::GorecapSprout, fl::skills::SkillId::BloodBloom,
       fl::widgets::effects::DecalAnimationKind::BloodBloom, "Gorecap Sprout"},
      {fl::monster::MonsterKind::RimefangHare, fl::skills::SkillId::IceSplitter,
       fl::widgets::effects::DecalAnimationKind::FrostCrack, "Rimefang Hare"},
      {fl::monster::MonsterKind::NullMote, fl::skills::SkillId::GravitySigh,
       fl::widgets::effects::DecalAnimationKind::VoidRipple, "Null Mote"},
  }};

  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto context = party_ctx.build_context();

  for (const auto &monster : expected) {
    const auto entity =
        fl::primitives::EntityBuilder(context).monster(monster.kind).build();
    const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(entity);
    const auto &skills =
        party_ctx.reg().get<fl::ecs::components::SkillSlots>(entity);

    CAPTURE(monster.name);
    REQUIRE(stats.name_ == monster.name);
    REQUIRE(skills.knows(monster.skill));
    REQUIRE(fl::skills::decal_animation_for(monster.skill) ==
            monster.animation);
  }
}

TEST_CASE("Field Mouse Thump remains a non-decal skill",
          "[monsters][skills][decal]") {
  REQUIRE_FALSE(
      fl::skills::decal_animation_for(fl::skills::SkillId::Thump).has_value());
}
