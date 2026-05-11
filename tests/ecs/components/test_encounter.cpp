#include <catch2/catch_test_macros.hpp>

#include "fl/context.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
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

TEST_CASE("TakeDamage emits PlayerDied when a party member dies",
          "[encounter][events][death]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.attackers().members().front();
  const auto defender = encounter.defenders().members().front();

  bool saw_player_died = false;
  entt::entity seen_player = entt::null;
  entt::entity seen_killer = entt::null;

  (void)party_ctx.party_data().party_bus().on<fl::events::PlayerDied>(
      [&](const fl::events::PlayerDied &ev) {
        saw_player_died = true;
        seen_player = ev.player;
        seen_killer = ev.killer;
      });

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(saw_player_died);
  REQUIRE(seen_player == defender);
  REQUIRE(seen_killer == attacker);
}

TEST_CASE("TakeDamage emits PartyWiped when the final party member dies",
          "[encounter][events][death]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.attackers().members().front();
  const auto defender = encounter.defenders().members().front();

  bool saw_party_wiped = false;

  (void)party_ctx.party_data().party_bus().on<fl::events::PartyWiped>(
      [&](const fl::events::PartyWiped &) { saw_party_wiped = true; });

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(saw_party_wiped);
}
