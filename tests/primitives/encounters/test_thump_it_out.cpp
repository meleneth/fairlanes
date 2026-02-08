// tests/encounter_builder_tests.cpp
#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/register_monsters.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/random_hub.hpp"

namespace {

TEST_CASE("EncounterBuilder::thump_it_out wires Encounter teams and members",
          "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  // Ensure monster registry is populated for
  // EntityBuilder(...).monster(FieldMouse). If register_monsters takes args in
  // your project, update this call.
  //  fl::monster::register_monsters();

  fl::primitives::RandomHub rng;
  // ---- build encounter ----
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  builder.thump_it_out();

  // ---- asserts ----
  REQUIRE(gc.reg().all_of<fl::ecs::components::Encounter>(party_ctx.self()));
  auto &enc = gc.reg().get<fl::ecs::components::Encounter>(party_ctx.self());

  REQUIRE(enc.attackers_ != nullptr);
  REQUIRE(enc.defenders_ != nullptr);

  // defenders should contain the party members (order matters if IsParty
  // enumerates in order)

  REQUIRE(enc.defenders_->members_.size() == 3);

  // attackers should contain exactly one enemy: the field mouse
  REQUIRE(enc.attackers_->members_.size() == 1);
  const entt::entity enemy = enc.attackers_->members_.front();
  CHECK(gc.reg().valid(enemy));

  // cleanup list should include the spawned enemy
  REQUIRE(enc.e_to_cleanup_.size() == 1);
  CHECK(enc.e_to_cleanup_.front() == enemy);
}

TEST_CASE("thump_it_out attaches Encounter to the party entity",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  builder.thump_it_out();

  auto &reg = party_ctx.reg(); // adjust accessor
  entt::entity party_e = party_ctx.self();

  REQUIRE(reg.valid(party_e));
  REQUIRE(reg.any_of<fl::ecs::components::Encounter>(party_e));
}

} // namespace
