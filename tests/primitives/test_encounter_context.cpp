#include <catch2/catch_test_macros.hpp>

#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"

TEST_CASE("Party combat conversion borrows its encounter and preserves scoped authority",
          "[context][encounter]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account = gc.account_context(0);
  auto party = account.party_context(0);
  auto &encounter = fl::primitives::EncounterBuilder{party}.thump_it_out();
  fl::context::EncounterCtx &ctx = party;
  REQUIRE(&ctx == &encounter.context());
  REQUIRE(&ctx.reg() == &party.reg());
  REQUIRE(&ctx.rng() == &party.rng());
  REQUIRE(&ctx.log() == &party.log());
  REQUIRE(&ctx.bus() == &party.bus());
  REQUIRE(ctx.self() == party.self());
  REQUIRE(&ctx.entity_context(ctx.self()).log() == &party.log());
}

TEST_CASE("An independently owned encounter has its own authority and tick source",
          "[context][encounter]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account = gc.account_context(0);
  auto id = gc.reg().create();
  fl::events::PartyBus bus;
  fl::primitives::EncounterData encounter{gc.reg(), gc.rng(), account.log(), bus, id, false};
  auto &ctx = encounter.context();
  REQUIRE(&ctx.encounter() == &encounter);
  REQUIRE(&ctx.log() == &account.log());
  REQUIRE(&ctx.bus() == &bus);
  REQUIRE(ctx.self() == id);
  bus.emit(fl::events::PartyEvent{fl::events::PartyTick{}});
  REQUIRE(encounter.visual_time().v == 0);
  encounter.atb_in().emit(seerin::Beat{});
  REQUIRE(encounter.visual_time() == seerin::UWU_PER_BEAT);
}
