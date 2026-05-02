#include <catch2/catch_test_macros.hpp>

#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"

TEST_CASE("EncounterBuilder::thump_it_out returns the created EncounterData",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(&encounter == &party_ctx.party_data().encounter_data());
}

TEST_CASE("EncounterBuilder::thump_it_out enrolls party members as defenders",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.defenders().members().size() == 3);
}

TEST_CASE("EncounterBuilder::thump_it_out adds one field mouse attacker",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.attackers().members().size() == 1);
  REQUIRE(encounter.entities_to_cleanup().size() == 1);

  auto attacker = encounter.attackers().members().front();
  REQUIRE(encounter.entities_to_cleanup().front() == attacker);
}
