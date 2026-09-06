#include <array>
#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/raid_data.hpp"
#include "fl/skills/skill_learning.hpp"

TEST_CASE(
    "One account raid owns all five parties and a party wipe cannot end it",
    "[raid]") {
  fl::GrandCentral gc{1, 5, 5};
  auto ctx = gc.account_context(0);
  auto &account = ctx.account_data();
  const std::array enemies{fl::monster::MonsterKind::HoneyBadger};
  REQUIRE(account.start_raid(ctx, enemies));
  auto &raid = *account.raid();
  auto &encounter = raid.encounter();
  REQUIRE(encounter.defenders().members().size() == 25);
  auto enemy = encounter.attackers().members().front();
  for (auto &party : account.parties()) {
    REQUIRE(&party.encounter_data() == &encounter);
    REQUIRE(party.in_raid());
  }
  for (const auto &member : account.party(0).members()) {
    auto attack = fl::context::AttackCtx::make_attack(
        encounter.context(), enemy, member.member_id());
    attack.damage().physical = 99999;
    fl::ecs::systems::TakeDamage::commit(attack);
  }
  REQUIRE(account.party(0).all_members_dead());
  account.party(0).loop_machine().beat_event();
  raid.tick();
  REQUIRE(raid.active());
  REQUIRE_FALSE(account.party(0).town_penalty_active());
  REQUIRE(gc.reg().valid(enemy));
  int victories = 0;
  std::vector<fl::events::ScopedPartyListener> listeners;
  for (auto &party : account.parties()) {
    listeners.emplace_back(party.party_bus(),
                           std::in_place_type<fl::events::PartyVictory>,
                           [&](const auto &) { ++victories; });
  }
  gc.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  raid.tick();
  REQUIRE(raid.result() == fl::events::RaidResult::Victory);
  REQUIRE(victories == 5);
  REQUIRE_FALSE(account.in_raid());
  REQUIRE(account.party(0).town_penalty_active());
  raid.tick();
  REQUIRE(victories == 5);
}

TEST_CASE(
    "Raid mutual extinction resolves once as defeat and preserves dead HP",
    "[raid][events]") {
  fl::GrandCentral gc{1, 2, 2};
  auto ctx = gc.account_context(0);
  auto &account = ctx.account_data();
  int starts = 0, outcomes = 0;
  fl::events::ScopedRaidListener start{
      account.raid_bus(), std::in_place_type<fl::events::RaidStarted>,
      [&](const auto &) {
        ++starts;
        REQUIRE(account.in_raid());
      }};
  fl::events::ScopedRaidListener end{
      account.raid_bus(), std::in_place_type<fl::events::RaidResolved>,
      [&](const auto &event) {
        ++outcomes;
        REQUIRE(event.result == fl::events::RaidResult::MutualDestruction);
      }};
  const std::array enemies{fl::monster::MonsterKind::FieldMouse};
  REQUIRE(account.start_raid(ctx, enemies));
  auto &raid = *account.raid();
  REQUIRE_FALSE(account.start_raid(ctx, enemies));
  for (auto e : raid.encounter().defenders())
    gc.reg().get<fl::ecs::components::Stats>(e).hp_ = 0;
  for (auto e : raid.encounter().attackers())
    gc.reg().get<fl::ecs::components::Stats>(e).hp_ = 0;
  raid.tick();
  raid.tick();
  REQUIRE(starts == 1);
  REQUIRE(outcomes == 1);
  REQUIRE_FALSE(account.in_raid());
  for (auto &party : account.parties()) {
    REQUIRE_FALSE(party.has_encounter());
    REQUIRE(party.all_members_dead());
  }
}

TEST_CASE("A wiped raid party keeps newly observed skills if the account wins",
          "[raid][learning]") {
  fl::GrandCentral gc{1, 2, 1};
  auto ctx = gc.account_context(0);
  auto &account = ctx.account_data();
  const std::array enemies{fl::monster::MonsterKind::HoneyBadger};
  REQUIRE(account.start_raid(ctx, enemies));
  auto &raid = *account.raid();
  auto enemy = raid.encounter().attackers().members().front();
  auto observer = account.party(0).members().front().member_id();
  auto party_ctx = ctx.party_context(0);
  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, enemy, fl::skills::SkillId::Thump, 1));
  auto attack = fl::context::AttackCtx::make_attack(raid.encounter().context(),
                                                    enemy, observer);
  attack.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack);
  REQUIRE(account.party(0).members().front().grimoire().knows(
      fl::skills::SkillId::Thump));
  gc.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  raid.tick();
  REQUIRE(account.party(0).members().front().grimoire().knows(
      fl::skills::SkillId::Thump));
}
