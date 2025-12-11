// tests/test_encounter.cpp
#include <chrono>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

// #include "fl/encounter/forest_encounter.hpp"
#include "fl/events/combat_events.hpp"
#include "fl/events/typed_bus.hpp"
#include "fl/primitives/random_hub.hpp"

using namespace std::chrono_literals;

TEST_CASE(
    "Forest encounter: single hero vs field mouse yields at least one attack",
    "[encounter][combat]") {
  entt::registry reg;
  fl::primitives::RandomHub rng; // whatever ctor you already have
  fl::events::RawBus bus;        // type-indexed eventpp bus

  // System under test: this is what you'll implement.
  fl::encounter::ForestEncounter encounter{reg, rng, bus};

  // Let the encounter set up one hero party and one mouse.
  const auto party_ent = encounter.create_player_party();
  const auto mouse_ent = encounter.spawn_field_mouse();

  REQUIRE(party_ent != entt::null);
  REQUIRE(mouse_ent != entt::null);

  // We'll watch the bus for AttackResolved events.
  std::vector<fl::events::AttackResolved> resolved;

  auto handle = fl::events::subscribe<fl::events::AttackResolved>(
      bus,
      [&](const fl::events::AttackResolved &ev) { resolved.push_back(ev); });

  // Drive the encounter with ticks until we see at least one attack resolved
  // or we give up.
  for (int i = 0; i < 32 && resolved.empty(); ++i) {
    encounter.tick(250ms);
  }

  REQUIRE_FALSE(resolved.empty());

  // Basic sanity on the first resolved hit.
  const auto &hit = resolved.front();
  REQUIRE(hit.attacker != entt::null);
  REQUIRE(hit.target != entt::null);
  REQUIRE(hit.damage > 0);
}
