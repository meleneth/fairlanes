// tests/account_data.test.cpp
#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"

TEST_CASE("AccountData creates a valid entity and log",
          "[account][account_data]") {
  entt::registry reg;
  entt::entity ent = reg.create();

  fl::primitives::AccountData acc(ent);

  REQUIRE(acc.account_id() != static_cast<entt::entity>(entt::null));
  REQUIRE(reg.valid(acc.account_id()));
}

TEST_CASE("AccountData creates unique entity ids per instance",
          "[account][account_data]") {
  entt::registry reg;
  entt::entity ent_a = reg.create();
  entt::entity ent_b = reg.create();

  fl::primitives::AccountData a(ent_a);
  fl::primitives::AccountData b(ent_b);
  REQUIRE(a.account_id() != static_cast<entt::entity>(entt::null));
  REQUIRE(b.account_id() != static_cast<entt::entity>(entt::null));
  REQUIRE(a.account_id() != b.account_id());

  REQUIRE(reg.valid(a.account_id()));
  REQUIRE(reg.valid(b.account_id()));
}

TEST_CASE(
    "PartyData starts a one minute town penalty when all members are dead",
    "[party][town][penalty]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();

  fl::primitives::EncounterBuilder builder(party_ctx);
  (void)builder.thump_it_out();
  REQUIRE(party.in_combat());

  bool saw_party_wiped = false;
  (void)party.party_bus().on<fl::events::PartyWiped>(
      [&](const fl::events::PartyWiped &) { saw_party_wiped = true; });

  for (const auto &member : party.members()) {
    auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    stats.hp_ = 0;
  }

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});

  REQUIRE(saw_party_wiped);
  REQUIRE_FALSE(party.in_combat());
  REQUIRE(party.town_penalty_beats_remaining() ==
          fl::primitives::PartyData::kTownPenaltyBeats);
  REQUIRE(party.town_penalty_active());

  for (int i = 0; i < fl::primitives::PartyData::kTownPenaltyBeats; ++i) {
    party.tick_town_penalty();
  }

  REQUIRE_FALSE(party.town_penalty_active());
}

TEST_CASE("Returning to town revitalizes party members",
          "[party][town][revitalize]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();

  for (const auto &member : party.members()) {
    auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    stats.hp_ = 0;
    stats.mp_ = 0;
  }

  fl::fsm::PartyLoop::Ops::enter_dead(party_ctx);

  bool saw_revitalize_request = false;
  int healed_members = 0;
  (void)party.party_bus().on<fl::events::PartyRevitalizeRequested>(
      [&](const fl::events::PartyRevitalizeRequested &) {
        saw_revitalize_request = true;
      });
  (void)party.party_bus().on<fl::events::PartyHealed>(
      [&](const fl::events::PartyHealed &ev) {
        if (ev.amount > 0) {
          ++healed_members;
        }
      });

  for (const auto &member : party.members()) {
    const auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    REQUIRE(stats.hp_ == 0);
    REQUIRE(stats.mp_ == 0);
  }

  for (int i = 0; i < fl::primitives::PartyData::kTownPenaltyBeats; ++i) {
    fl::fsm::PartyLoop::Ops::fixing_tick(party_ctx);
  }

  REQUIRE(saw_revitalize_request);
  REQUIRE(healed_members == static_cast<int>(party.members().size()));

  for (const auto &member : party.members()) {
    const auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    REQUIRE(stats.hp_ > 0);
    REQUIRE(stats.hp_ == stats.max_hp_);
    REQUIRE(stats.mp_ == stats.max_mp_);
  }
}

TEST_CASE("Party loop revitalizes members after a wipe town stay",
          "[party][town][revitalize][fsm]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();

  int healed_members = 0;
  (void)party.party_bus().on<fl::events::PartyHealed>(
      [&](const fl::events::PartyHealed &ev) {
        if (ev.amount > 0) {
          ++healed_members;
        }
      });

  for (const auto &member : party.members()) {
    auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    stats.hp_ = 0;
    stats.mp_ = 0;
  }

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});

  REQUIRE_FALSE(party.in_combat());
  REQUIRE(party.town_penalty_active());

  party.loop_machine().beat_event();

  for (int i = 0; i < fl::primitives::PartyData::kTownPenaltyBeats; ++i) {
    party.loop_machine().beat_event();
  }

  REQUIRE_FALSE(party.town_penalty_active());
  REQUIRE(healed_members == static_cast<int>(party.members().size()));

  for (const auto &member : party.members()) {
    const auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id());
    REQUIRE(stats.hp_ == stats.max_hp_);
    REQUIRE(stats.mp_ == stats.max_mp_);
  }
}
