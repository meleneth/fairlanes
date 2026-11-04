#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/skills/skill_sequence.hpp"

namespace {

struct PlainsCoverFight {
  fl::GrandCentral game{1, 1, 1};
  fl::context::PartyCtx party{game.account_context(0).party_context(0)};
  fl::primitives::EncounterData &encounter{fl::primitives::EncounterBuilder{
      party, fl::primitives::EncounterMode::HeroesJourney,
      fl::primitives::EncounterBuilder::TowerFloor{"origin_plains_tower", 3}}
                                               .thump_it_out()};

  entt::entity hero() const { return encounter.defenders().members().front(); }
  entt::entity meerkat() const { return encounter.attackers().members()[0]; }
  entt::entity boar() const { return encounter.attackers().members()[1]; }
};

} // namespace

TEST_CASE("Plains Cover redirects the complete projectile hit path",
          "[cover][skills][tower]") {
  PlainsCoverFight fight;
  const auto meerkat_hp =
      fight.game.reg().get<fl::ecs::components::Stats>(fight.meerkat()).hp_;
  const auto boar_hp =
      fight.game.reg().get<fl::ecs::components::Stats>(fight.boar()).hp_;
  entt::entity landed_target{entt::null};
  fl::events::ScopedCombatantListener landed{
      fight.encounter.combatant_bus(fight.boar()),
      std::in_place_type<fl::events::SkillHitLanded>,
      [&landed_target](const auto &event) { landed_target = event.target; }};
  int finished = 0;
  fl::skills::SkillSequencer sequencer{fight.encounter.context(),
                                       fight.encounter.atb_engine().scheduler(),
                                       [&finished](auto) { ++finished; }};

  sequencer.schedule(fight.hero(), fight.meerkat(),
                     fl::skills::SkillId::PebbleSpit);
  for (int beat = 0; beat < seerin::BEATS_PER_SEC + 1; ++beat)
    fight.encounter.atb_engine().scheduler().advance(seerin::UWU_PER_BEAT);

  REQUIRE(finished == 1);
  REQUIRE(landed_target == fight.boar());
  REQUIRE(
      fight.game.reg().get<fl::ecs::components::Stats>(fight.meerkat()).hp_ ==
      meerkat_hp);
  REQUIRE(fight.game.reg().get<fl::ecs::components::Stats>(fight.boar()).hp_ <
          boar_hp);
}

TEST_CASE("Cover ignores melee and ends on charge or protector death",
          "[cover][skills][tower]") {
  SECTION("melee bypasses the screen") {
    PlainsCoverFight fight;
    REQUIRE(fight.encounter.resolve_skill_target(fight.hero(), fight.meerkat(),
                                                 fl::skills::SkillId::Bite) ==
            fight.meerkat());
  }
  SECTION("the boar charge opens the screen") {
    PlainsCoverFight fight;
    fight.encounter.release_projectile_cover(fight.boar(),
                                             fl::skills::SkillId::BumperRush);
    REQUIRE(fight.encounter.resolve_skill_target(
                fight.hero(), fight.meerkat(),
                fl::skills::SkillId::PebbleSpit) == fight.meerkat());
  }
  SECTION("a dead boar cannot screen") {
    PlainsCoverFight fight;
    fight.game.reg().get<fl::ecs::components::Stats>(fight.boar()).hp_ = 0;
    REQUIRE(fight.encounter.resolve_skill_target(
                fight.hero(), fight.meerkat(),
                fl::skills::SkillId::PebbleSpit) == fight.meerkat());
  }
}
