#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"

TEST_CASE("TrackXP grants permanent max HP per level", "[ecs][components][xp]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  auto &reg = party_ctx.reg();
  const auto member = party_ctx.party_data().members().front().member_id();

  auto &track = reg.get<fl::ecs::components::TrackXP>(member);
  auto &stats = reg.get<fl::ecs::components::Stats>(member);

  const int base_max_hp = stats.max_hp_;
  const int xp_to_level_2 = track.next_level_at - track.xp_;

  track.add_xp(entt::handle{reg, member}, xp_to_level_2);

  REQUIRE(track.level_ == 2);
  REQUIRE(stats.max_hp_ == base_max_hp + 5);
}

TEST_CASE("TrackXP emits PartyGainedLevel for each gained level",
          "[ecs][components][xp][events]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  auto &reg = party_ctx.reg();
  auto &member_data = party_ctx.party_data().members().front();
  const auto member = member_data.member_id();

  auto &track = reg.get<fl::ecs::components::TrackXP>(member);
  auto &stats = reg.get<fl::ecs::components::Stats>(member);

  std::vector<int> observed_levels;
  int observed_member_events = 0;

  fl::events::ScopedPartyListener gained_level_sub{
      member_data.bus(), std::in_place_type<fl::events::PartyGainedLevel>,
      [&](const fl::events::PartyGainedLevel &ev) {
        if (ev.member == member) {
          ++observed_member_events;
          observed_levels.push_back(ev.level);
        }
      }};

  const int base_max_hp = stats.max_hp_;
  const int xp_to_level_4 = track.xp_for_level(4) - track.xp_;
  track.add_xp(entt::handle{reg, member}, xp_to_level_4);

  REQUIRE(track.level_ == 4);
  REQUIRE(stats.max_hp_ == base_max_hp + 15);
  REQUIRE(observed_member_events == 3);
  REQUIRE(observed_levels == std::vector<int>{2, 3, 4});
}
