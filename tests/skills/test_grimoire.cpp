#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/grimoire.hpp"
#include "fl/skills/skill_learning.hpp"

TEST_CASE("Grimoire learns exact ranked skills", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE_FALSE(grimoire.knows(thump_viii));
  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.knows(thump_viii));
}

TEST_CASE("Grimoire learning is idempotent", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};

  REQUIRE(grimoire.learn(thump_i));
  REQUIRE_FALSE(grimoire.learn(thump_i));
  REQUIRE(grimoire.known_skills().size() == 1);
}

TEST_CASE("Grimoire does not infer lower ranks from higher ranks", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.knows(thump_viii));
  REQUIRE_FALSE(grimoire.knows(thump_i));
}

TEST_CASE("Grimoire unlearns exact ranked skills", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE(grimoire.learn(thump_i));
  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.unlearn(thump_viii));

  REQUIRE(grimoire.knows(thump_i));
  REQUIRE_FALSE(grimoire.knows(thump_viii));
  REQUIRE_FALSE(grimoire.unlearn(thump_viii));
  REQUIRE(grimoire.forgotten(thump_viii));
  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE_FALSE(grimoire.forgotten(thump_viii));
}

namespace {

void clear_frontier(fl::context::PartyCtx ctx) {
  auto& party = ctx.party_data();
  const auto route = party.tower_route();
  REQUIRE(route.has_value());
  party.begin_tower_encounter();
  auto& fight = fl::primitives::EncounterBuilder{ctx, fl::primitives::EncounterMode::ChaosAttractor,
                                                 fl::primitives::EncounterBuilder::TowerFloor{
                                                     std::string(route->biome_id), route->floor}}
                    .thump_it_out();
  for (const auto enemy : fight.attackers().members())
    ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  party.leave_combat();
}

} // namespace

TEST_CASE("Grimoire teacher lookup uses the highest accessible resolved floor",
          "[skills][grimoire][teacher][tower]") {
  fl::GrandCentral game{1, 1, 1};
  auto ctx = game.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  REQUIRE(party.select_tower("origin_woodland_tower"));
  for (int floor = 1; floor <= 7; ++floor) clear_frontier(ctx);

  const auto member = party.members().front().member_id();
  auto& data = party.members().front();
  auto& assignment = ctx.reg().get<fl::ecs::components::PartyMember>(member);
  REQUIRE(data.grimoire().learn(fl::skills::SkillId::Brace));
  REQUIRE(assignment.equip_known_skill(fl::skills::SkillId::Brace));
  REQUIRE(party.drop_skill(member, 1));
  REQUIRE(data.grimoire().forgotten(fl::skills::SkillId::Brace));

  const auto teacher = party.find_accessible_teacher(member, fl::skills::SkillId::Brace);
  REQUIRE(teacher.has_value());
  REQUIRE(teacher->biome_id == "origin_woodland_tower");
  REQUIRE(teacher->floor == 7);
  REQUIRE_FALSE(party.find_accessible_teacher(member, fl::skills::SkillId::FlameWave).has_value());

  auto& lesson =
      fl::primitives::EncounterBuilder{
          ctx, fl::primitives::EncounterMode::ChaosAttractor,
          fl::primitives::EncounterBuilder::TowerFloor{"origin_woodland_tower", 7}}
          .thump_it_out();
  const auto stag = lesson.attackers().members().front();
  REQUIRE(
      fl::skills::learn_observed_skill_with_roll(ctx, member, stag, fl::skills::SkillId::Brace, 1));
  for (const auto enemy : lesson.attackers().members())
    ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  party.leave_combat();
  REQUIRE(data.grimoire().knows(fl::skills::SkillId::Brace));
}

TEST_CASE("Forgotten-skill farming waits for combat boundary and repeats its floor",
          "[skills][grimoire][teacher][boundary]") {
  fl::GrandCentral game{1, 1, 1};
  auto ctx = game.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  REQUIRE(party.select_tower("origin_woodland_tower"));
  for (int floor = 1; floor <= 7; ++floor) clear_frontier(ctx);
  const auto member = party.members().front().member_id();
  auto& data = party.members().front();
  auto& assignment = ctx.reg().get<fl::ecs::components::PartyMember>(member);
  REQUIRE(data.grimoire().learn(fl::skills::SkillId::Brace));
  REQUIRE(assignment.equip_known_skill(fl::skills::SkillId::Brace));
  REQUIRE(party.drop_skill(member, 1));

  const auto route = party.tower_route();
  REQUIRE(route->floor == 8);
  party.begin_tower_encounter();
  auto& fight = fl::primitives::EncounterBuilder{ctx, fl::primitives::EncounterMode::ChaosAttractor,
                                                 fl::primitives::EncounterBuilder::TowerFloor{
                                                     std::string(route->biome_id), 8}}
                    .thump_it_out();
  REQUIRE(party.farm_for_forgotten_skill(member, fl::skills::SkillId::Brace));
  REQUIRE(party.tower_route()->floor == 8);
  for (const auto enemy : fight.attackers().members())
    ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  party.leave_combat();
  REQUIRE(party.tower_route()->floor == 7);
  clear_frontier(ctx);
  REQUIRE(party.tower_route()->floor == 7);
  REQUIRE(data.grimoire().forgotten(fl::skills::SkillId::Brace));
}

TEST_CASE("Teacher lookup follows the assigned party's progression after transfer",
          "[skills][grimoire][teacher][transfer]") {
  fl::GrandCentral game{1, 2, 1};
  auto account = game.account_context(0);
  auto first_ctx = account.party_context(0);
  auto& first = first_ctx.party_data();
  auto& second = account.account_data().party(1);
  REQUIRE(first.select_tower("origin_woodland_tower"));
  for (int floor = 1; floor <= 7; ++floor) clear_frontier(first_ctx);
  const auto member = first.members().front().member_id();
  auto& assignment = first_ctx.reg().get<fl::ecs::components::PartyMember>(member);
  REQUIRE(first.members().front().grimoire().learn(fl::skills::SkillId::Brace));
  REQUIRE(assignment.equip_known_skill(fl::skills::SkillId::Brace));
  REQUIRE(first.drop_skill(member, 1));
  REQUIRE(first.find_accessible_teacher(member, fl::skills::SkillId::Brace).has_value());

  REQUIRE(account.account_data().request_roster_transfer(account, member, 1));
  REQUIRE(second.members().back().grimoire().forgotten(fl::skills::SkillId::Brace));
  REQUIRE_FALSE(second.find_accessible_teacher(member, fl::skills::SkillId::Brace).has_value());
  REQUIRE(second.highest_tower_floor_cleared("origin_woodland_tower") == 0);
}
