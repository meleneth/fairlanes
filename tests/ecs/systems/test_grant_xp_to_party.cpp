#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"

TEST_CASE("Victory XP is individual and limited to living encounter participants",
          "[xp][victory][eligibility]") {
  fl::GrandCentral game{1, 2, 3};
  auto account = game.account_context(0);
  auto party_ctx = account.party_context(0);
  auto& party = party_ctx.party_data();
  auto& other_party = account.account_data().party(1);
  const auto living = party.members()[0].member_id();
  const auto recruit = party.members()[1].member_id();
  const auto dead = party.members()[2].member_id();
  const auto outsider = other_party.members()[0].member_id();
  auto& living_xp = party_ctx.reg().get<fl::ecs::components::TrackXP>(living);
  auto& dead_xp = party_ctx.reg().get<fl::ecs::components::TrackXP>(dead);
  auto& recruit_xp = party_ctx.reg().get<fl::ecs::components::TrackXP>(recruit);
  auto& outsider_xp = party_ctx.reg().get<fl::ecs::components::TrackXP>(outsider);
  living_xp.level_ = 40;
  dead_xp.level_ = 1;
  recruit_xp.level_ = 1;
  const int living_before = living_xp.xp_;
  const int dead_before = dead_xp.xp_;
  const int recruit_before = recruit_xp.xp_;
  const int outsider_before = outsider_xp.xp_;
  party_ctx.reg().get<fl::ecs::components::Stats>(dead).hp_ = 0;
  int awards = 0;
  fl::events::ScopedPartyListener xp_sub{
      party.party_bus(), std::in_place_type<fl::events::PartyGainedXP>, [&](const auto& event) {
        REQUIRE((event.member == living || event.member == recruit));
        ++awards;
      }};

  auto& fight = fl::primitives::EncounterBuilder{party_ctx}.thump_it_out();
  const int expected = static_cast<int>(fight.attackers().members().size()) * 256;
  for (const auto enemy : fight.attackers().members())
    party_ctx.reg().get<fl::ecs::components::Stats>(enemy).hp_ = 0;
  party.leave_combat();

  REQUIRE(living_xp.xp_ - living_before == expected);
  REQUIRE(recruit_xp.xp_ - recruit_before == expected);
  REQUIRE(dead_xp.xp_ == dead_before);
  REQUIRE(outsider_xp.xp_ == outsider_before);
  REQUIRE(awards == 2);
}

TEST_CASE("A wipe awards no encounter XP", "[xp][wipe][eligibility]") {
  fl::GrandCentral game{1, 1, 2};
  auto ctx = game.account_context(0).party_context(0);
  auto& party = ctx.party_data();
  (void)fl::primitives::EncounterBuilder{ctx}.thump_it_out();
  for (const auto& member : party.members())
    ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
  const auto first = party.members().front().member_id();
  const int before = ctx.reg().get<fl::ecs::components::TrackXP>(first).xp_;
  int awards = 0;
  fl::events::ScopedPartyListener xp_sub{party.party_bus(),
                                         std::in_place_type<fl::events::PartyGainedXP>,
                                         [&](const auto&) { ++awards; }};

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});
  REQUIRE(ctx.reg().get<fl::ecs::components::TrackXP>(first).xp_ == before);
  REQUIRE(awards == 0);
}
