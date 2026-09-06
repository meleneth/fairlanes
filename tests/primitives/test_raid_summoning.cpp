#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/dire_bleed_system.hpp"
#include "fl/ecs/systems/freeze_system.hpp"
#include "fl/ecs/systems/poison_system.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/targeting/status_filters.hpp"

TEST_CASE("Raid summoning retains observed skills without claiming victory",
          "[raid][summoning][events][learning]") {
  fl::GrandCentral gc{1, 1, 2};
  auto account = gc.account_context(0);
  auto ctx = account.party_context(0);
  auto &party = ctx.party_data();
  auto &encounter = fl::primitives::EncounterBuilder{ctx}.thump_it_out();
  auto enemy = encounter.attackers().members().front();
  auto observer = party.members().back().member_id();
  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      ctx, observer, enemy, fl::skills::SkillId::Thump, 1));
  std::vector<std::string> events;
  fl::events::ScopedPartyListener summoned{
      ctx.bus(), std::in_place_type<fl::events::PartySummonedToRaid>,
      [&](const auto &) {
        events.push_back("summoned");
        party.summon_to_raid(); // Recursive listeners must not extract twice.
        party.leave_combat();
      }};
  fl::events::ScopedPartyListener left{
      ctx.bus(), std::in_place_type<fl::events::PartyLeftCombat>,
      [&](const auto &) { events.push_back("left"); }};
  fl::events::ScopedPartyListener victory{
      ctx.bus(), std::in_place_type<fl::events::PartyVictory>,
      [&](const auto &) { events.push_back("victory"); }};
  party.summon_to_raid();
  party.summon_to_raid();
  REQUIRE(events == std::vector<std::string>{"summoned", "left"});
  REQUIRE_FALSE(party.has_encounter());
  REQUIRE_FALSE(ctx.reg().valid(enemy));
  REQUIRE(party.members().back().grimoire().knows(fl::skills::SkillId::Thump));
  REQUIRE(ctx.reg()
              .get<fl::ecs::components::PartyMember>(observer)
              .closet()
              .has_equipped_skill(fl::skills::SkillId::Thump));

  (void)fl::primitives::EncounterBuilder{ctx}.thump_it_out();
  for (const auto &member : party.members())
    ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
  ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});
  REQUIRE(party.members().back().grimoire().knows(fl::skills::SkillId::Thump));
}

TEST_CASE("Raid extraction clears statuses and old work without healing or "
          "resurrection",
          "[raid][summoning][status]") {
  using namespace fl::ecs::components;
  using namespace fl::ecs::systems;
  fl::GrandCentral gc{1, 1, 3};
  auto account = gc.account_context(0);
  auto ctx = account.party_context(0);
  auto &party = ctx.party_data();
  auto &encounter = fl::primitives::EncounterBuilder{ctx}.thump_it_out();
  auto source = encounter.attackers().members().front();
  auto first = party.members().front().member_id();
  auto second = party.members()[1].member_id();
  auto dead = party.members().back().member_id();
  auto &scheduler = encounter.atb_engine().scheduler();
  PoisonSystem::apply(ctx, scheduler, source, first, 1, 30);
  FreezeSystem::apply(ctx, scheduler, source, first, 30);
  DireBleedSystem::apply(ctx, scheduler, source, second);
  REQUIRE(CombatStatusSystem::apply_status(ctx, scheduler,
                                           {.kind = CombatStatusKind::Haste,
                                            .source = first,
                                            .target = second,
                                            .duration_seconds = 30,
                                            .value = 10,
                                            .negative = false}));
  ctx.reg().get<Stats>(first).hp_ = 3;
  ctx.reg().get<Stats>(second).hp_ = 4;
  ctx.reg().get<Stats>(dead).hp_ = 0;
  ctx.reg().emplace_or_replace<ActiveGlow>(first);
  ctx.reg().emplace_or_replace<CombatantDecals>(first);
  int old_actions = 0;
  scheduler.schedule_smelly_in_beats(1, "test: old enemy action",
                                     [&] { ++old_actions; });
  party.summon_to_raid();
  REQUIRE_FALSE(party.has_encounter());
  REQUIRE(ctx.reg().get<Stats>(first).hp_ == 3);
  REQUIRE(ctx.reg().get<Stats>(second).hp_ == 4);
  REQUIRE(ctx.reg().get<Stats>(dead).hp_ == 0);
  REQUIRE_FALSE(
      ctx.reg().any_of<Poison, Freeze, ActiveGlow, CombatantDecals>(first));
  REQUIRE_FALSE(
      ctx.reg().any_of<DireBleed, CombatStatuses, StatusTint>(second));
  for (int i = 0; i < 100; ++i)
    ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyTick{}});
  REQUIRE(old_actions == 0);
  REQUIRE(ctx.reg().get<Stats>(first).hp_ == 3);
  REQUIRE(ctx.reg().get<Stats>(second).hp_ == 4);
  REQUIRE(ctx.reg().get<Stats>(dead).hp_ == 0);
}
